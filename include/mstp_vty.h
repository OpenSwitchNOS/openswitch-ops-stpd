/*
 * (c) Copyright 2016 Hewlett Packard Enterprise Development LP
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

/******************************************************************************
 *    File               : mstp_vty.h
 *    Description        : MSTP Protocol CLI MACROS
 ******************************************************************************/
#ifndef _MSTP_VTY_H
#define _MSTP_VTY_H

#include "vtysh/command.h"

#define MSTP_CISTID                 0
#define MIN_MSTP_MSTID              1
#define MAX_MSTP_INSTANCES          64
#define MAX_CONFIG_NAME_LEN         32

#define MSTP_VALID_MSTID(mstid) \
    (((mstid) >= MSTP_MSTID_MIN) && ((mstid) <= MSTP_MSTID_MAX))


#define REMOVE_VLAN_FROM_INSTANCE   1
#define REMOVE_INSTANCE             2
#define ADD_VLAN_TO_INSTANCE        3
#define INVALID_ID                  -1


/*
** depending on the outcome of the db transaction, returns
** the appropriate value for the cli command execution.
*/
inline static int
cli_command_result (enum ovsdb_idl_txn_status status)
{
    if ((status == TXN_SUCCESS) || (status == TXN_UNCHANGED)) {
        return CMD_SUCCESS;
    }
    return CMD_WARNING;
}
/********************** standard database txn operations ***********************/

#define START_DB_TXN(txn)                                       \
    do {                                                        \
        txn = cli_do_config_start();                            \
        if (txn == NULL) {                                      \
            vty_out(vty, "ovsdb_idl_txn_create failed: %s: %d\n",   \
                    __FILE__, __LINE__);                            \
            cli_do_config_abort(txn);                               \
            return CMD_OVSDB_FAILURE;                               \
        }                                                           \
    } while (0)

#define END_DB_TXN(txn)                                   \
    do {                                                  \
        enum ovsdb_idl_txn_status status;                 \
        status = cli_do_config_finish(txn);               \
        return cli_command_result(status);                \
    } while (0)

#define ERRONEOUS_DB_TXN(txn, error_message)                        \
    do {                                                            \
        cli_do_config_abort(txn);                                   \
        vty_out(vty, "database transaction failed: %s: %d -- %s\n", \
                __FILE__, __LINE__, error_message);                 \
        return CMD_WARNING;                                         \
    } while (0)

/* used when NO error is detected but still need to terminate */
#define ABORT_DB_TXN(txn, message)                             \
    do {                                                       \
        cli_do_config_abort(txn);                                   \
        vty_out(vty, "database transaction aborted: %s: %d, %s\n",  \
                __FILE__, __LINE__, message);                       \
        return CMD_SUCCESS;                                         \
    } while (0)

/********************** helper routines which find things ***********************/

void MSTP_vty_init (void);
void mstp_vty_init(void);
#define STATUS_ENABLE				"enable"
#define STATUS_DISABLE				"disable"

/*********** STP_CONFIG DEFAULT VALUES **************************/
#define DEF_ADMIN_STATUS			false
#define DEF_HELLO_TIME				2
#define DEF_FORWARD_DELAY			15
#define DEF_ADMIN_EDGE				false
#define DEF_BPDU_STATUS				false
#define DEF_BRIDGE_PRIORITY		    32768
#define DEF_MAX_AGE     			20
#define DEF_HOLD_COUNT     			6
#define DEF_MAX_HOPS     			20
#define DEF_CONFIG_REV     			"0"
#define DEF_MSTP_PORT_PRIORITY  	128
#define DEF_MSTP_COST     			20000
#define DEF_LINK_TYPE     			"point_to_point"

/*********** MSTP_CONFIG OF BRIDGE TABLE **************************/
#define MSTP_STATE_BLOCK           "Blocking"
#define MSTP_STATE_LEARN           "Learning"
#define MSTP_STATE_FORWARD         "Forwarding"
#define MSTP_STATE_DISABLE         "Disabled"


#define MSTP_ROLE_ROOT             "root_port",
#define MSTP_ROLE_DESIGNATE        "designated_port",
#define MSTP_ROLE_ALTERNATE        "alternate_port",
#define MSTP_ROLE_BACKUP           "backup_port",
#define MSTP_ROLE_DISABLE          "disabled_port"

/*********** MSTP_CONFIG OF BRIDGE TABLE **************************/
#define MSTP_ADMIN_STATUS	        "mstp_MSTP_ADMIN_STATUS"
#define MSTP_HELLO_TIME				"mstp_hello_time"
#define MSTP_FORWARD_DELAY			"mstp_forward_delay"
#define MSTP_MAX_AGE				"mstp_max_age"
#define MSTP_TX_HOLD_COUNT			"mstp_tx_hold_count"
#define MSTP_MAX_HOP_COUNT			"mstp_maximum_hop_count"
#define MSTP_BRIDGE_PRIORITY		"mstp_priority"
#define MSTP_PORT_PRIORITY			"mstp_port_priority"
#define MSTP_PORT_COST				"mstp_admin_path_cost"
#define MSTP_CONFIG_REV				"mstp_config_revision"
#define MSTP_CONFIG_NAME   			"mstp_config_name"
#define MSTP_INSTANCE_CONFIG		"mstp_instances_configured"

/************ MSTP_CONFIG OF PORT TABLE **************************/

#define MSTP_ADMIN_EDGE  			"admin_edge_port"
#define MSTP_BPDU_FILTER			"bpdu-filter"
#define MSTP_BPDU_GUARD				"bpdu-guard"
#define MSTP_LOOP_GUARD				"loop-guard"
#define MSTP_ROOT_GUARD				"root-guard"

/************ mstp_msti_config TABLE **************************/

#define MSTP_INSTANCE_ID			"mstp_instid"
#define MSTP_VLANS					"mstp_vlans"
#define STG_ID						"mstp_stg"
#define DESIGNATED_ROOT             "mstp_designated_root"
#define ROOT_PATH_COST              "mstp_root_path_cost"
#define ROOT_PORT                   "mstp_root_port"
#define ROOT_PRIORITY               "mstp_root_priority"
#define REMAINING_HOPS              "mstp_remaing_hops"
#define TIME_SINCE_TOP_CHANGE       "mstp_time_since_top_change"
#define TOP_CHANGE_CNT              "mstp_top_change_cnt"
#define OPER_HELLO_TIME             "mstp_oper_hello_time"
#define OPER_FORWARD_DELAY          "mstp_oper_forward_delay"
#define OPER_MAX_AGE                "mstp_oper_max_age"
#define OPER_TX_HOLD_COUNT          "mstp_oper_tx_hold_count"
#define HELLO_EXPIRY_TIME           "mstp_hello_expiry_time"
#define FORWARD_DELAY_EXP_TIME      "mstp_forward_delay_expiry_time"
#define MESSAGE_EXP_TIME            "mstp_messageage_expiry_time"
#define TOPO_CHANGE_EXP_TIME        "mstp_topology_change_expirty_time"
#define NOTIFICATION_EXP_TIME       "mstp_notification_expiry_time"
#define HOLD_TIMER_EXP_TIME         "mstp_hold_timer_expiry_time"

void cli_pre_init(void);
void cli_post_init(void);

#endif /* _MSTP_VTY_H */
