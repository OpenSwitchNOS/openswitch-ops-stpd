/*
 * (c) Copyright 2015 Hewlett Packard Enterprise Development LP
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */
/************************************************************************//**
 * @ingroup stpd
 *
 * @file
 * Source for intfd OVSDB access interface.
 *
 ***************************************************************************/

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <config.h>
#include <command-line.h>
#include <compiler.h>
#include <daemon.h>
#include <dirs.h>
#include <dynamic-string.h>
#include <fatal-signal.h>
#include <ovsdb-idl.h>
#include <poll-loop.h>
#include <unixctl.h>
#include <util.h>
#include <openvswitch/vconn.h>
#include <openvswitch/vlog.h>
#include <vswitch-idl.h>
#include <openswitch-idl.h>
#include <hash.h>
#include <shash.h>

#include "mstp_ovsdb_if.h"

VLOG_DEFINE_THIS_MODULE(mstpd_ovsdb_if);

/* To serialize updates to OVSDB.  Both MSTP and OVS
 * interface threads calls to update OVSDB states. */
pthread_mutex_t ovsdb_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Macros to lock and unlock mutexes in a verbose manner. */
#define MSTP_OVSDB_LOCK { \
                VLOG_DBG("%s(%d): MSTP_OVSDB_LOCK: taking lock...", __FUNCTION__, __LINE__); \
                pthread_mutex_lock(&ovsdb_mutex); \
}

#define MSTP_OVSDB_UNLOCK { \
                VLOG_DBG("%s(%d): MSTP_OVSDB_UNLOCK: releasing lock...", __FUNCTION__, __LINE__); \
                pthread_mutex_unlock(&ovsdb_mutex); \
}
struct ovsdb_idl *idl;           /*!< Session handle for OVSDB IDL session. */
static unsigned int idl_seqno;
static int system_configured = false;

void
mstpd_debug_dump(struct ds *ds, int argc, const char *argv[])
{

    //TBD
}

/* Create a connection to the OVSDB at db_path and create a dB cache
 * for this daemon. */
void
mstpd_ovsdb_init(const char *db_path)
{
    /* Initialize IDL through a new connection to the dB. */
    idl = ovsdb_idl_create(db_path, &ovsrec_idl_class, false, true);
    idl_seqno = ovsdb_idl_get_seqno(idl);
    ovsdb_idl_set_lock(idl, "ops_mstpd");

    /* Reject writes to columns which are not marked write-only using
     * ovsdb_idl_omit_alert(). */
    ovsdb_idl_verify_write_only(idl);

    /* Choose some OVSDB tables and columns to cache. */
    ovsdb_idl_add_table(idl, &ovsrec_table_system);
    ovsdb_idl_add_table(idl, &ovsrec_table_subsystem);

    /* Monitor the following columns, marking them read-only. */
    ovsdb_idl_add_column(idl, &ovsrec_system_col_cur_cfg);
    ovsdb_idl_add_column(idl, &ovsrec_subsystem_col_name);
    ovsdb_idl_add_column(idl, &ovsrec_subsystem_col_other_info);

    /* Mark the following columns write-only. */
    //TBD

} /* mstpd_ovsdb_init */

void
mstpd_ovsdb_exit(void)
{
    ovsdb_idl_destroy(idl);
} /* mstpd_ovsdb_exit */

static inline void
mstpd_chk_for_system_configured(void)
{
    const struct ovsrec_system *sys = NULL;

    if (system_configured) {
        /* Nothing to do if we're already configured. */
        return;
    }

    sys = ovsrec_system_first(idl);
    if (sys && sys->cur_cfg > (int64_t)0) {

        system_configured = true;
    }

} /* mstpd_chk_for_system_configured */

static int
mstpd_reconfigure(void)
{
    return 0;
} /* mstpd_reconfigure */

/***
 * @ingroup mstpd
 * @{
 */
void
mstpd_run(void)
{
    struct ovsdb_idl_txn *txn;

    MSTP_OVSDB_LOCK;

    /* Process a batch of messages from OVSDB. */
    ovsdb_idl_run(idl);

    if (ovsdb_idl_is_lock_contended(idl)) {
        static struct vlog_rate_limit rl = VLOG_RATE_LIMIT_INIT(1, 1);
        VLOG_ERR_RL(&rl, "Another mstpd process is running, "
                    "disabling this process until it goes away");
        MSTP_OVSDB_UNLOCK;
        return;
    } else if (!ovsdb_idl_has_lock(idl)) {
        MSTP_OVSDB_UNLOCK;
        return;
    }

    /* Update the local configuration and push any changes to the DB. */
    mstpd_chk_for_system_configured();

    if (system_configured) {
        txn = ovsdb_idl_txn_create(idl);
        if (mstpd_reconfigure()) {
            /* Some OVSDB write needs to happen. */
            ovsdb_idl_txn_commit_block(txn);
        }
        ovsdb_idl_txn_destroy(txn);
    }

    MSTP_OVSDB_UNLOCK;

    return;
} /* mstpd_run */

void
mstpd_wait(void)
{
    ovsdb_idl_wait(idl);
} /* mstpd_wait */

/**********************************************************************/
/*                        OVS Main Thread                             */
/**********************************************************************/
/**
 * Cleanup function at daemon shutdown time.
 */
static void
mstpd_exit(void)
{
    mstpd_ovsdb_exit();
    VLOG_INFO("mstpd OVSDB thread exiting...");
} /* mstpd_exit */

/**
 * @details
 * mstpd daemon's main OVS interface function.  Repeat loop that
 * calls run, wait, poll_block, etc. functions for mstpd.
 *
 * @param arg pointer to ovs-appctl server struct.
 */
void *
mstpd_ovs_main_thread(void *arg)
{
    struct unixctl_server *appctl;

    /* Detach thread to avoid memory leak upon exit. */
    pthread_detach(pthread_self());

    appctl = (struct unixctl_server *)arg;

    exiting = false;
    while (!exiting) {
        mstpd_run();
        unixctl_server_run(appctl);

        mstpd_wait();
        unixctl_server_wait(appctl);
        if (exiting) {
            poll_immediate_wake();
        } else {
            poll_block();
        }
    }

    mstpd_exit();
    unixctl_server_destroy(appctl);

    /* OPS_TODO -- need to tell main loop to exit... */

    return NULL;

} /* mstpd_ovs_main_thread */
