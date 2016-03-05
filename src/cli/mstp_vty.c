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
 *    File               : mstp_vty.c
 *    Description        : MSTP Protocol CLI Commands
 ******************************************************************************/
#include <sys/un.h>
#include <sys/wait.h>
#include <pwd.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "vtysh/lib/version.h"
#include "getopt.h"
#include "vtysh/memory.h"
#include "vtysh/vtysh.h"
#include "vtysh/vector.h"
#include "vtysh/vtysh_user.h"
#include "vtysh/vtysh_utils.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "mstp_vty.h"
#include "vtysh_ovsdb_mstp_context.h"

extern struct ovsdb_idl *idl;
bool init_required = false;

VLOG_DEFINE_THIS_MODULE(vtysh_mstp_cli);

static int
util_add_default_ports_to_cist() {

    struct ovsrec_mstp_common_instance_port *cist_port_row = NULL;
    struct ovsrec_mstp_common_instance_port **cist_port_info = NULL;
    struct ovsdb_idl_txn *txn = NULL;
    const struct ovsrec_bridge *bridge_row = NULL;
    const struct ovsrec_mstp_common_instance *cist_row = NULL;
    int64_t i = 0;

    int64_t cist_hello_time = DEF_HELLO_TIME;
    int64_t cist_port_priority = DEF_MSTP_PORT_PRIORITY;
    int64_t admin_path_cost = 0;

    START_DB_TXN(txn);

    bridge_row = ovsrec_bridge_first(idl);
    if (!bridge_row) {
        vty_out(vty, "no bridge record found\n");
        END_DB_TXN(txn);
    }

    cist_row = ovsrec_mstp_common_instance_first (idl);
    if (!cist_row) {
        vty_out(vty, "no MSTP common instance record found\n");
        END_DB_TXN(txn);
    }

    /* Add CIST port entry for all ports to the CIST table */
    cist_port_info = xmalloc(sizeof *cist_row->mstp_common_instance_ports * bridge_row->n_ports);

    for (i = 0; i < bridge_row->n_ports; i++) {
        /* create CIST_port entry */
        cist_port_row = ovsrec_mstp_common_instance_port_insert(txn);

        /* FILL the default values for CIST_port entry */
        ovsrec_mstp_common_instance_port_set_port( cist_port_row,
                                                      bridge_row->ports[i]);
        ovsrec_mstp_common_instance_port_set_port_state( cist_port_row,
                                                      MSTP_STATE_BLOCK);
        ovsrec_mstp_common_instance_port_set_port_role( cist_port_row,
                                                      MSTP_ROLE_DISABLE);
        ovsrec_mstp_common_instance_port_set_admin_path_cost( cist_port_row,
                                                      &admin_path_cost, 1);
        ovsrec_mstp_common_instance_port_set_port_priority( cist_port_row,
                                                      &cist_port_priority, 1);
        ovsrec_mstp_common_instance_port_set_link_type( cist_port_row,
                                                      DEF_LINK_TYPE);
        ovsrec_mstp_common_instance_port_set_port_hello_time( cist_port_row,
                                                      &cist_hello_time, 1);
        cist_port_info[i] = cist_port_row;
    }

    ovsrec_mstp_common_instance_set_mstp_common_instance_ports (cist_row,
                                    cist_port_info, bridge_row->n_ports);
    free(cist_port_info);
    END_DB_TXN(txn);
}

int
util_mstp_set_defaults(struct smap *smap) {

    const struct ovsrec_mstp_common_instance *cist_row = NULL;
    const struct ovsrec_system *system_row = NULL;
    const int64_t cist_top_change_count = 0;
    struct ovsdb_idl_txn *txn = NULL;
    time_t cist_time_since_top_change;
    const int64_t cist_priority = DEF_BRIDGE_PRIORITY;
    const int64_t hello_time = DEF_HELLO_TIME;
    const int64_t fwd_delay = DEF_FORWARD_DELAY;
    const int64_t max_age = DEF_MAX_AGE;
    const int64_t max_hops = DEF_MAX_HOPS;
    const int64_t tx_hold_cnt = DEF_HOLD_COUNT;
    const struct ovsrec_bridge *bridge_row = NULL;

    START_DB_TXN(txn);

    system_row = ovsrec_system_first(idl);
    if (!system_row) {
        vty_out(vty, "no system record found\n");
        END_DB_TXN(txn);
    }

    bridge_row = ovsrec_bridge_first(idl);
    if (!bridge_row) {
        vty_out(vty, "no bridge record found\n");
        END_DB_TXN(txn);
    }

    time(&cist_time_since_top_change);

    /* If config name is NULL, Set the system mac as config-name */
    if (!smap_get(&bridge_row->other_config, MSTP_CONFIG_NAME)) {
        smap_replace (smap, MSTP_CONFIG_NAME, system_row->system_mac);
    }

    /* If config revision number is NULL, Set the system mac as config-name */
    if (!smap_get(&bridge_row->other_config, MSTP_CONFIG_REV)) {
        smap_replace (smap, MSTP_CONFIG_REV, DEF_CONFIG_REV);
    }

    cist_row = ovsrec_mstp_common_instance_first (idl);
    if (!cist_row) {

        /* Crate a CIST instance */
        cist_row = ovsrec_mstp_common_instance_insert(txn);

        /* updating the default values to the CIST table */
        ovsrec_mstp_common_instance_set_hello_time(cist_row, &hello_time, 1);
        ovsrec_mstp_common_instance_set_priority(cist_row, &cist_priority, 1);
        ovsrec_mstp_common_instance_set_forward_delay(cist_row, &fwd_delay,1);
        ovsrec_mstp_common_instance_set_max_age(cist_row, &max_age, 1);
        ovsrec_mstp_common_instance_set_max_hop_count(cist_row, &max_hops, 1);
        ovsrec_mstp_common_instance_set_tx_hold_count(cist_row, &tx_hold_cnt,1);
        ovsrec_mstp_common_instance_set_regional_root(cist_row,
                                                      system_row->system_mac);
        ovsrec_mstp_common_instance_set_bridge_identifier(cist_row,
                                                      system_row->system_mac);
        ovsrec_mstp_common_instance_set_top_change_cnt(cist_row,
                                                     &cist_top_change_count, 1);
        ovsrec_mstp_common_instance_set_time_since_top_change(cist_row,
                                     (int64_t *)&cist_time_since_top_change, 1);

        /* Add the CIST instance to bridge table */
        ovsrec_bridge_set_mstp_common_instance(bridge_row, cist_row);
    }

    END_DB_TXN(txn);
}

int64_t
util_mstp_get_mstid_for_vlanID(int64_t vlan_id,
        const struct ovsrec_bridge *bridge_row) {

    size_t i = 0, j = 0;

    /* Loop for all instance in bridge table */
    for (i=0; i < bridge_row->n_mstp_instances; i++) {
        /* Loop for all vlans in one MST instance table */
        for (j=0; j<bridge_row->value_mstp_instances[i]->n_vlans; j++) {
            /* Return the instance ID if the VLAN exist in the instance*/
            if(vlan_id == bridge_row->value_mstp_instances[i]->vlans[j]->id) {
                return bridge_row->key_mstp_instances[i];
            }
        }
    }
    return INVALID_ID;
}

void
cli_show_spanning_tree_config() {
    const struct ovsrec_mstp_common_instance_port *cist_port;
    const struct ovsrec_mstp_common_instance *cist_row;
    const struct ovsrec_bridge *bridge_row = NULL;
    const struct ovsrec_system *system_row = NULL;

    /* Get the current time to calculate the last topology change */
    time_t cur_time;
    time(&cur_time);

    system_row = ovsrec_system_first(idl);
    if (!system_row) {
        vty_out(vty, "no system record found\n");
        return;
    }

    bridge_row = ovsrec_bridge_first(idl);
    if (!bridge_row) {
        vty_out(vty, "no bridge record found\n");
        return;
    }

    cist_row = ovsrec_mstp_common_instance_first (idl);
    if (!cist_row) {
        vty_out(vty, "no MSTP common instance record found\n");
        return;
    }

    if (*bridge_row->mstp_enable != DEF_ADMIN_STATUS) {
        vty_out(vty, "%s\n", "MST0");
        vty_out(vty, "  %s\n", "Spanning tree enabled protocol mstp");
        vty_out(vty, "  %-10s %-10s : %-20ld\n", "Root ID", "Priority",
                                                  *cist_row->priority);
        vty_out(vty, "  %21s : %-20s\n", "MACADDRESS", system_row->system_mac);
        if (VTYSH_STR_EQ(system_row->system_mac, cist_row->regional_root)) {
            vty_out(vty, "  %34s\n", "This bridge is the root");
        }
        vty_out(vty, "  %23s %ld \tMax Age : %ld\tForward Delay : %ld\n",
                "Hello time :", *cist_row->hello_time,
                                *cist_row->max_age, *cist_row->forward_delay);

        vty_out(vty, "\n\n  %-10s %-10s : %-20ld\n", "Bridge ID", "Priority",
                                                  *cist_row->priority);
        vty_out(vty, "  %21s : %-20s\n", "MACADDRESS", system_row->system_mac);
        vty_out(vty, "  %23s %ld \tMax Age : %ld\tForward Delay : %ld\n",
                "Hello time :", *cist_row->hello_time,
                                *cist_row->max_age, *cist_row->forward_delay);

        vty_out(vty, "\n\n%-14s %-14s %-10s %-7s %-10s %s\n",
                "Interface", "Role", "State", "Cost", "Priority", "Type");
        vty_out(vty, "%s %s\n",
                     "-------------- --------------",
                     "---------- ------- ---------- ----------");
        OVSREC_MSTP_COMMON_INSTANCE_PORT_FOR_EACH(cist_port, idl) {
            vty_out(vty, "%-14s %-14s %-10s %-7ld %-10ld %s\n",
                    cist_port->port->name, cist_port->port_role,
                    cist_port->port_state, *cist_port->admin_path_cost,
                    *cist_port->port_priority, cist_port->link_type);
        }
    }
    else {
        vty_out(vty, "spanning-tree is disabled\n");
    }
}

void
cli_show_mstp_config() {
    const struct ovsrec_bridge *bridge_row = NULL;
    size_t i = 0, j = 0;

    bridge_row = ovsrec_bridge_first(idl);
    if (!bridge_row) {
        vty_out(vty, "no bridge record found\n");
    }

    if (*bridge_row->mstp_enable != DEF_ADMIN_STATUS) {
        vty_out(vty, "%s\n", "MST Configuration Identifier Information");
        vty_out(vty, "   %-30s : %-15s \n", "MST Configuration Identifier",
                smap_get(&bridge_row->other_config, MSTP_CONFIG_NAME));
        vty_out(vty, "   %-30s : %-15d \n", "MST Configuration Revision",
                atoi(smap_get(&bridge_row->other_config, MSTP_CONFIG_REV)));
        /*vty_out(vty, "   %-30s : %-15s \n", "MST Configuration Digest",
          smap_get(&bridge_row->other_config, MSTP_CONFIG_DIGEST));*/
        vty_out(vty, "   %-30s : %-15ld \n", "Instances configured",
                bridge_row->n_mstp_instances);

        vty_out(vty, "\n%-15s %-18s\n", "Instance ID", "Mapped VLANs");
        vty_out(vty, "--------------- ----------------------------------\n");

        /* Loop for all instance in bridge table */
        for (i=0; i < bridge_row->n_mstp_instances; i++) {
            /* Loop for all vlans in one MST instance table */
            vty_out(vty,"%-15ld %ld", bridge_row->key_mstp_instances[i],
                        bridge_row->value_mstp_instances[i]->vlans[0]->id);
            for (j=1; j<bridge_row->value_mstp_instances[i]->n_vlans; j++) {
                        vty_out(vty, ",%ld",
                        bridge_row->value_mstp_instances[i]->vlans[j]->id );
            }
            vty_out(vty, "\n" );
        }
    }
    else {
        vty_out(vty, "spanning-tree is disabled\n");
    }
}

void
show_common_instance_info(const struct ovsrec_mstp_common_instance *cist_row) {

    const struct ovsrec_mstp_common_instance_port *cist_port = NULL;
    const struct ovsrec_system *system_row = NULL;
    size_t j = 0;

    system_row = ovsrec_system_first(idl);
    if (!system_row) {
        vty_out(vty, "no system record found\n");
        return;
    }

    /* common instance table details */
    vty_out(vty, "%-14s \n%-14s", "#### MST0", "vlans mapped:");
    if (cist_row->vlans) {
        vty_out(vty, "%ld", cist_row->vlans[0]->id);
        for (j=1; j<cist_row->n_vlans; j++) {
            vty_out(vty, ",%ld", cist_row->vlans[j]->id);
        }
        vty_out(vty, "\n" );
    }
    vty_out(vty, "%-14s %s:%-15s    %s:%ld\n", "Bridge", "address",
            system_row->system_mac, "priority", *cist_row->priority);
    if (VTYSH_STR_EQ(system_row->system_mac, cist_row->regional_root)) {
        vty_out(vty, "%-14s %s\n", "Root", "this switch for the CIST");
    }
    if (VTYSH_STR_EQ(system_row->system_mac, cist_row->designated_root)) {
        vty_out(vty, "%-14s %s\n", "Regional Root", "this switch");
    }
    vty_out(vty, "%-14s %s:%ld  %s:%ld  %s:%ld  %s:%ld\n", "Operational",
            "Hello time",(cist_row->oper_hello_time)?*cist_row->oper_hello_time:DEF_HELLO_TIME,
            "Forward delay",(cist_row->oper_forward_delay)?*cist_row->oper_forward_delay:DEF_FORWARD_DELAY,
            "Max-age",(cist_row->oper_max_age)?*cist_row->oper_max_age:DEF_MAX_AGE,
            "txHoldCount",(cist_row->oper_tx_hold_count)?*cist_row->oper_tx_hold_count:DEF_HOLD_COUNT);
    vty_out(vty, "%-14s %s:%ld  %s:%ld %s:%ld %s:%ld\n", "Configured",
            "Hello time",(cist_row->hello_time)?*cist_row->hello_time:DEF_HELLO_TIME,
            "Forward delay",(cist_row->forward_delay)?*cist_row->forward_delay:DEF_FORWARD_DELAY,
            "Max-age",(cist_row->max_age)?*cist_row->max_age:DEF_MAX_AGE,
            "txHoldCount",(cist_row->tx_hold_count)?*cist_row->tx_hold_count:DEF_HOLD_COUNT);

    vty_out(vty, "\n%-14s %-14s %-10s %-7s %-10s %s\n",
            "Interface", "Role", "State", "Cost", "Priority", "Type");
    vty_out(vty, "%s %s\n",
            "-------------- --------------",
            "---------- ------- ---------- ----------");
    OVSREC_MSTP_COMMON_INSTANCE_PORT_FOR_EACH(cist_port, idl) {
        vty_out(vty, "%-14s %-14s %-10s %-7ld %-10ld %s\n",
                cist_port->port->name, cist_port->port_role,
                cist_port->port_state, *cist_port->admin_path_cost,
                *cist_port->port_priority, cist_port->link_type);
    }
}

void
show_instance_info(const struct ovsrec_mstp_common_instance *cist_row,
                   const struct ovsrec_bridge *bridge_row) {
    const struct ovsrec_system *system_row = NULL;
    const struct ovsrec_mstp_instance *mstp_row = NULL;
    const struct ovsrec_mstp_instance_port *mstp_port = NULL;
    size_t j = 0, i = 0;

    system_row = ovsrec_system_first(idl);
    if (!system_row) {
        vty_out(vty, "no system record found\n");
        return;
    }

    /* Loop for all instance in bridge table */
    for (i=0; i < bridge_row->n_mstp_instances; i++) {
        mstp_row = bridge_row->value_mstp_instances[i];
        vty_out(vty, "\n\n%s%ld \n%-14s", "#### MST",
                bridge_row->key_mstp_instances[i], "vlans mapped:");
        if (cist_row->vlans) {
            vty_out(vty, "%ld", cist_row->vlans[0]->id);
            for (j=1; j<mstp_row->n_vlans; j++) {
                vty_out(vty, ",%ld", mstp_row->vlans[j]->id);
            }
            vty_out(vty, "\n" );
        }
        vty_out(vty, "%-14s %s:%-18s %s:%ld\n", "Bridge", "address",
                system_row->system_mac, "priority",
                (mstp_row->priority)?*mstp_row->priority:DEF_BRIDGE_PRIORITY);

        vty_out(vty, "%-14s address:%-18s priority:%ld\n", "Root",
                (mstp_row->designated_root)?:system_row->system_mac,
                (mstp_row->root_priority)?*mstp_row->root_priority:DEF_BRIDGE_PRIORITY);

        vty_out(vty, "%19s:%ld, Cost:%ld, Rem Hops:%ld\n", "Port",
                (mstp_row->root_port)?*mstp_row->root_port:(int64_t)0,
                (mstp_row->root_path_cost)?*mstp_row->root_path_cost:DEF_MSTP_COST,
                (cist_row->remaining_hops)?*cist_row->remaining_hops:(int64_t)0);

        vty_out(vty, "\n%-14s %-14s %-10s %-7s %-10s %s\n",
                "Interface", "Role", "State", "Cost", "Priority", "Type");
        vty_out(vty, "%s %s\n",
                "-------------- --------------",
                "---------- ------- ---------- ----------");
        for (j=0; j < mstp_row->n_mstp_instance_ports; j++) {
            mstp_port = mstp_row->mstp_instance_ports[j];
            vty_out(vty, "%-14s %-14s %-10s %-7ld %-10ld %s\n",
                    mstp_port->port->name, mstp_port->port_role, mstp_port->port_state,
                    (mstp_port->admin_path_cost)?*mstp_port->admin_path_cost:DEF_MSTP_COST,
                    *mstp_port->port_priority, "p2p");
        }
    }
}

void
cli_show_mst() {
    const struct ovsrec_bridge *bridge_row = NULL;
    const struct ovsrec_mstp_common_instance *cist_row = NULL;

    bridge_row = ovsrec_bridge_first(idl);
    if (!bridge_row) {
        vty_out(vty, "no bridge record found\n");
        return;
    }

    if (*bridge_row->mstp_enable != DEF_ADMIN_STATUS) {
        cist_row = ovsrec_mstp_common_instance_first (idl);
        if (!cist_row) {
            vty_out(vty, "no MSTP common instance record found\n");
            return;
        }
        show_common_instance_info(cist_row);
        show_instance_info(cist_row, bridge_row);
    }
    else {
        vty_out(vty, "spanning-tree is disabled\n");
    }
}

int
cli_show_mstp_global_config() {
    const struct ovsrec_bridge *bridge_row = NULL;
    const struct ovsrec_system *system_row = NULL;
    const struct ovsrec_mstp_common_instance *cist_row = NULL;
    const struct ovsrec_mstp_instance *mstp_row = NULL;
    const struct ovsrec_mstp_instance_port *mstp_port_row = NULL;
    const char *data = NULL;
    size_t i = 0, j = 0;

    system_row = ovsrec_system_first(idl);
    if (!system_row) {
        return e_vtysh_error;
    }

    /* Bridge configs */
    bridge_row = ovsrec_bridge_first(idl);
    if (bridge_row) {
        if (bridge_row->mstp_enable &&
                (*bridge_row->mstp_enable != DEF_ADMIN_STATUS)) {
            vty_out(vty, "spanning-tree enable\n");
        }

        data = smap_get(&bridge_row->other_config, MSTP_CONFIG_NAME);
        if (data && (!VTYSH_STR_EQ(data, system_row->system_mac))) {
            vty_out(vty, "spanning-tree config-name %s\n", data);
        }

        data = smap_get(&bridge_row->other_config, MSTP_CONFIG_REV);
        if (data && (atoi(DEF_CONFIG_REV) != atoi(data))) {
            vty_out(vty, "spanning-tree config-revision %d\n", atoi(data));
        }

        /* Loop for all instance in bridge table */
        for (i=0; i < bridge_row->n_mstp_instances; i++) {
            mstp_row = bridge_row->value_mstp_instances[i];
            vty_out(vty, "spanning-tree instance %ld vlan %ld",
                    bridge_row->key_mstp_instances[i],mstp_row->vlans[0]->id );

            /* Loop for all vlans in one MST instance table */
            for (j=1; j<mstp_row->n_vlans; j++) {
                vty_out(vty, ",%ld", mstp_row->vlans[j]->id);
            }
            vty_out(vty, "\n" );

            if (mstp_row->priority &&
                    (*mstp_row->priority != DEF_BRIDGE_PRIORITY)) {
                vty_out(vty, "spanning-tree instance %ld priority %ld\n",
                bridge_row->key_mstp_instances[i], *mstp_row->priority);
            }

            /* Loop for all ports in the instance table */
            for (j=0; j<mstp_row->n_mstp_instance_ports; j++) {
                mstp_port_row = mstp_row->mstp_instance_ports[j];
                if (mstp_port_row->port_priority &&
                   (*mstp_port_row->port_priority != DEF_MSTP_PORT_PRIORITY)) {
                    vty_out(vty, "spanning-tree instance %ld port-priority %ld\n",
                            bridge_row->key_mstp_instances[i],
                            *mstp_port_row->port_priority);
                }
                if (mstp_port_row->admin_path_cost &&
                        (*mstp_port_row->admin_path_cost != DEF_MSTP_COST)) {
                    vty_out(vty, "spanning-tree instance %ld cost %ld\n",
                            bridge_row->key_mstp_instances[i],
                            *mstp_port_row->admin_path_cost);
                }
            }
        }
    }

    /* CIST configs */
    cist_row = ovsrec_mstp_common_instance_first (idl);
    if (cist_row) {
        if (cist_row->priority &&
                *cist_row->priority != DEF_BRIDGE_PRIORITY) {
            vty_out(vty, "spanning-tree priority %ld\n", *cist_row->priority);
        }
        if (cist_row->forward_delay &&
                *cist_row->forward_delay != DEF_FORWARD_DELAY) {
            vty_out(vty, "spanning-tree forward-delay %ld\n",
                    *cist_row->forward_delay);
        }
        if (cist_row->max_age && *cist_row->max_age != DEF_MAX_AGE) {
            vty_out(vty, "spanning-tree max-age %ld\n", *cist_row->max_age);
        }
        if (cist_row->max_hop_count &&
                *cist_row->max_hop_count != DEF_MAX_HOPS) {
            vty_out(vty, "spanning-tree max-hops %ld\n",
                    *cist_row->max_hop_count);
        }

    }
    return e_vtysh_ok;
}

void
cli_show_mstp_intf_config() {
    const struct ovsrec_mstp_common_instance *cist_row = NULL;
    const struct ovsrec_mstp_common_instance_port *cist_port_row = NULL;
    size_t i = 0;

    cist_row = ovsrec_mstp_common_instance_first (idl);
    if (cist_row) {
        /* CIST port configs */
        for (i=0; i < cist_row->n_mstp_common_instance_ports; i++) {
            cist_port_row = cist_row->mstp_common_instance_ports[i];
            if (cist_port_row->loop_guard_disable &&
                    *cist_port_row->loop_guard_disable != DEF_BPDU_STATUS) {
                vty_out(vty, "spanning-tree loop-guard enable\n");
            }
            if (cist_port_row->root_guard_disable &&
                    *cist_port_row->root_guard_disable != DEF_BPDU_STATUS) {
                vty_out(vty, "spanning-tree root-guard enable\n");
            }
            if (cist_port_row->bpdu_guard_disable &&
                    *cist_port_row->bpdu_guard_disable != DEF_BPDU_STATUS) {
                vty_out(vty, "spanning-tree bpdu-guard enable\n");
            }
            if (cist_port_row->bpdu_filter_disable &&
                    *cist_port_row->bpdu_filter_disable != DEF_BPDU_STATUS) {
                vty_out(vty, "spanning-tree bpdu-filter enable\n");
            }
            if (cist_port_row->admin_edge_port_disable &&
                  *cist_port_row->admin_edge_port_disable != DEF_ADMIN_EDGE) {
                vty_out(vty, "spanning-tree port-type admin-edge\n");
            }
        }
    }
}

void
cli_show_mstp_running_config() {
    /* Global configuration of MSTP, in config context */
    cli_show_mstp_global_config();

    /* Inerface level configuration of MSTP */
    cli_show_mstp_intf_config();
}

static int
cli_set_mstp_cist_port_table (const char *if_name, const char *key,
                              const bool value) {

    struct ovsdb_idl_txn *txn = NULL;
    const struct ovsrec_mstp_common_instance_port *cist_port_row = NULL;

    START_DB_TXN(txn);

    OVSREC_MSTP_COMMON_INSTANCE_PORT_FOR_EACH(cist_port_row, idl) {
        if (VTYSH_STR_EQ(cist_port_row->port->name, if_name)) {
            break;
        }
    }
    if (cist_port_row) {
        if (VTYSH_STR_EQ(key, MSTP_ADMIN_EDGE)) {
            ovsrec_mstp_common_instance_port_set_admin_edge_port_disable(cist_port_row,
                    &value, 1);
        }
        else if (VTYSH_STR_EQ(key, MSTP_BPDU_GUARD)) {
            ovsrec_mstp_common_instance_port_set_bpdu_guard_disable(cist_port_row, &value, 1);
        }
        else if (VTYSH_STR_EQ(key, MSTP_BPDU_FILTER)) {
            ovsrec_mstp_common_instance_port_set_bpdu_filter_disable(cist_port_row, &value, 1);
        }
        else if (VTYSH_STR_EQ(key, MSTP_ROOT_GUARD)) {
            ovsrec_mstp_common_instance_port_set_root_guard_disable(cist_port_row, &value, 1);
        }
        else if (VTYSH_STR_EQ(key, MSTP_LOOP_GUARD)) {
            ovsrec_mstp_common_instance_port_set_loop_guard_disable(cist_port_row, &value, 1);
        }
    }
    else {
        vty_out(vty, "interface not found in CIST\n");
    }
    /* End of transaction. */
    END_DB_TXN(txn);
}

static int
cli_set_mstp_cist_table (const char *key, int64_t value) {
    struct ovsdb_idl_txn *txn = NULL;
    const struct ovsrec_mstp_common_instance *cist_row = NULL;

    START_DB_TXN(txn);

    cist_row = ovsrec_mstp_common_instance_first (idl);
    if (!cist_row) {
        vty_out(vty, "no MSTP common instance record found\n");
        END_DB_TXN(txn);
    }

    if (VTYSH_STR_EQ(key, MSTP_BRIDGE_PRIORITY)) {
        ovsrec_mstp_common_instance_set_priority(cist_row, &value, 1);
    }
    else if (VTYSH_STR_EQ(key, MSTP_HELLO_TIME)) {
        ovsrec_mstp_common_instance_set_hello_time(cist_row, &value, 1);
    }
    else if (VTYSH_STR_EQ(key, MSTP_FORWARD_DELAY)) {
        ovsrec_mstp_common_instance_set_forward_delay(cist_row, &value, 1);
    }
    else if (VTYSH_STR_EQ(key, MSTP_MAX_HOP_COUNT)) {
        ovsrec_mstp_common_instance_set_max_hop_count(cist_row, &value, 1);
    }
    else if (VTYSH_STR_EQ(key, MSTP_MAX_AGE)) {
        ovsrec_mstp_common_instance_set_max_age(cist_row, &value, 1);
    }
    else if (VTYSH_STR_EQ(key, MSTP_TX_HOLD_COUNT)) {
        ovsrec_mstp_common_instance_set_tx_hold_count(cist_row, &value, 1);
    }

    END_DB_TXN(txn);
}

static int
cli_set_mstp_configs_bridge_table (const char *key, const char *value) {
    const struct ovsrec_bridge *bridge_row = NULL;
    struct ovsdb_idl_txn *txn = NULL;
    struct smap smap = SMAP_INITIALIZER(&smap);
    bool mstp_enable = false;

    if (VTYSH_STR_EQ(key, MSTP_ADMIN_STATUS)) {
        mstp_enable = (VTYSH_STR_EQ(value, STATUS_ENABLE))?true:false;

        /* Set the default config-name at the time of enable spanning-tree */
        if((mstp_enable == true) && (init_required == true)) {
            util_mstp_set_defaults(&smap);
            util_add_default_ports_to_cist();
            init_required = false;
        }
    }

    START_DB_TXN(txn);

    bridge_row = ovsrec_bridge_first(idl);
    if (!bridge_row) {
        vty_out(vty, "no bridge record found\n");
    }

    if (VTYSH_STR_EQ(key, MSTP_ADMIN_STATUS)) {
        ovsrec_bridge_set_mstp_enable(bridge_row, &mstp_enable, 1);
    }
    else {
        smap_clone(&smap, &bridge_row->other_config);
        smap_replace(&smap, key , value);

        ovsrec_bridge_set_other_config(bridge_row, &smap);
        smap_destroy(&smap);
    }
    END_DB_TXN(txn);
}

static int
cli_set_mstp_inst(const char *if_name,const char *key,
                  const int64_t instid, const int64_t value) {
    struct ovsdb_idl_txn *txn = NULL;
    const struct ovsrec_bridge *bridge_row = NULL;
    const struct ovsrec_mstp_instance *mstp_row = NULL;
    const struct ovsrec_mstp_instance_port *mstp_port_row = NULL;
    int i = 0;

    START_DB_TXN(txn);

    bridge_row = ovsrec_bridge_first(idl);
    if (!bridge_row) {
        ERRONEOUS_DB_TXN(txn, "no bridge record not found");
    }

    /* Find the MSTP instance entry matching with the instid */
    for (i=0; i < bridge_row->n_mstp_instances; i++) {
        if (bridge_row->key_mstp_instances[i] == instid) {
            mstp_row = bridge_row->value_mstp_instances[i];
            break;
        }
    }

    /* Instance not created */
    if (!mstp_row) {
        vty_out(vty, "No MSTP instance found with with This instance ID\n");
        END_DB_TXN(txn);
    }

    /* Find the MSTP instance port entry matching with the port index */
    if( if_name != NULL) {
        for (i=0; i < mstp_row->n_mstp_instance_ports; i++) {
            mstp_port_row = mstp_row->mstp_instance_ports[i];
            if (VTYSH_STR_EQ(mstp_port_row->port->name, if_name)) {
                break;
            }
        }
        if (!mstp_port_row) {
            vty_out(vty, "No MSTP instance port found with this port index\n");
            END_DB_TXN(txn);
        }
    }

    if(VTYSH_STR_EQ(key, MSTP_BRIDGE_PRIORITY)) {
        ovsrec_mstp_instance_set_priority(mstp_row, &value, 1);
    }
    else if(VTYSH_STR_EQ(key, MSTP_PORT_COST)) {
        ovsrec_mstp_instance_port_set_admin_path_cost(mstp_port_row, &value, 1);
    }
    else if(VTYSH_STR_EQ(key, MSTP_PORT_PRIORITY)) {
        ovsrec_mstp_instance_port_set_port_priority(mstp_port_row, &value, 1);
    }

    /* End of transaction. */
    END_DB_TXN(txn);
}

static int
cli_mstp_inst_vlan_map(const int64_t instid, const char *vlanid,
        int64_t operation) {

    struct ovsdb_idl_txn *txn = NULL;
    const struct ovsrec_mstp_instance *mstp_inst_row;
    struct ovsrec_mstp_instance *row=NULL, **mstp_info=NULL;
    struct ovsrec_mstp_instance_port *mstp_inst_port_row=NULL, **mstp_inst_port_info=NULL;
    const struct ovsrec_bridge *bridge_row = NULL;
    const struct ovsrec_vlan *vlan_row = NULL;
    struct ovsrec_vlan **vlans = NULL;
    int64_t mstp_old_inst_id = 0, *instId_list = NULL;
    size_t i=0, j=0;
    int64_t port_priority = DEF_MSTP_PORT_PRIORITY;

    int vlan_id =(vlanid)? atoi(vlanid):INVALID_ID;

    START_DB_TXN(txn);

    bridge_row = ovsrec_bridge_first(idl);
    if (!bridge_row) {
        ERRONEOUS_DB_TXN(txn, "no bridge record not found");
    }

    if (vlanid) {
        OVSREC_VLAN_FOR_EACH(vlan_row, idl) {
            if (vlan_id == vlan_row->id) {
                break;
            }
        }
        if (!vlan_row) {
            vty_out(vty, "Invalid vlan ID\n");
            END_DB_TXN(txn);
        }

        /* Check if the vlan is already mapped to another instance */
        mstp_old_inst_id = util_mstp_get_mstid_for_vlanID(vlan_row->id, bridge_row);
        if ((mstp_old_inst_id != INVALID_ID) &&
                (mstp_old_inst_id != instid)) {
            if (operation == ADD_VLAN_TO_INSTANCE) {
                vty_out(vty,
                        "This vlan is already mapped to MSTP instance %ld\n",
                        mstp_old_inst_id );
            }
            else if (operation == REMOVE_VLAN_FROM_INSTANCE) {
                vty_out(vty,
                        "This vlan is not mapped mapped to This instance\n");
            }
            END_DB_TXN(txn);
        }
    }

    /* Check if any column with the same instid already exist */
    for (i=0; i < bridge_row->n_mstp_instances; i++) {

        mstp_inst_row = bridge_row->value_mstp_instances[i];
        /* Adding a VLAN to existing instance */
        if ((bridge_row->key_mstp_instances[i] == instid)&&
                (operation == ADD_VLAN_TO_INSTANCE)) {

            /* Push the complete vlan list to MSTP instance table
             * including the new vlan*/
            vlans = xmalloc(sizeof *mstp_inst_row->vlans * (mstp_inst_row->n_vlans + 1));
            for (i = 0; i < mstp_inst_row->n_vlans; i++) {
                vlans[i] = mstp_inst_row->vlans[i];
            }
            vlans[mstp_inst_row->n_vlans] = (struct ovsrec_vlan *)vlan_row;

            ovsrec_mstp_instance_set_vlans(mstp_inst_row, vlans,
                    mstp_inst_row->n_vlans + 1);
            free(vlans);
            END_DB_TXN(txn);
        }

        /* Removing a VLAN from existing instance */
        else if ((bridge_row->key_mstp_instances[i] == instid)&&
                (operation == REMOVE_VLAN_FROM_INSTANCE)) {

            /* Push the complete vlan list to MSTP instance table,
             * except the removed one */
            vlans = xmalloc(sizeof *mstp_inst_row->vlans * (mstp_inst_row->n_vlans - 1));
            for (j=0, i = 0; i < mstp_inst_row->n_vlans; i++) {
                if(vlan_id != mstp_inst_row->vlans[i]->id) {
                    vlans[j++] = mstp_inst_row->vlans[i];
                }
            }
            ovsrec_mstp_instance_set_vlans(mstp_inst_row, vlans,
                    mstp_inst_row->n_vlans - 1);
            free(vlans);
            END_DB_TXN(txn);
        }

        /* Removing a complete MSTP instance */
        else if ((bridge_row->key_mstp_instances[i] == instid) &&
                (operation == REMOVE_INSTANCE)) {

            instId_list = xmalloc(sizeof(int64_t) * (bridge_row->n_mstp_instances -1 ));
            mstp_info = xmalloc(sizeof *bridge_row->value_mstp_instances * (bridge_row->n_mstp_instances - 1));
            for (j=0, i = 0; i < bridge_row->n_mstp_instances; i++) {
                if (bridge_row->key_mstp_instances[i] != instid) {
                    instId_list[j++] = bridge_row->key_mstp_instances[i];
                    mstp_info[j++] = bridge_row->value_mstp_instances[i];
                }
            }

            /* Push the complete MSTP table into the bridge table */
            ovsrec_bridge_set_mstp_instances(bridge_row, instId_list,
                    mstp_info, bridge_row->n_mstp_instances - 1);
            free(mstp_info);
            free(instId_list);
            END_DB_TXN(txn);
        }
    }

    if ((i == bridge_row->n_mstp_instances) &&
            (operation != ADD_VLAN_TO_INSTANCE)) {
        vty_out(vty, "No MSTP instance found with with This instance ID\n");
        END_DB_TXN(txn);
    }

    /* Create s MSTP instance column with the incoming data */
    row = ovsrec_mstp_instance_insert(txn);
    ovsrec_mstp_instance_set_vlans(row, (struct ovsrec_vlan **)&vlan_row, 1);

    /* Add CSTI instance for all ports to the CIST table */
    mstp_inst_port_info =
        xmalloc(sizeof *row->mstp_instance_ports * bridge_row->n_ports);

    for (i = 0; i < bridge_row->n_ports; i++) {
        mstp_inst_port_row = ovsrec_mstp_instance_port_insert(txn);
        ovsrec_mstp_instance_port_set_port_state( mstp_inst_port_row,
                                                  MSTP_STATE_BLOCK);
        ovsrec_mstp_instance_port_set_port_priority( mstp_inst_port_row,
                                                  &port_priority, 1 );
        ovsrec_mstp_instance_port_set_port( mstp_inst_port_row,
                                                  bridge_row->ports[i]);
        mstp_inst_port_info[i] = mstp_inst_port_row;
    }

    ovsrec_mstp_instance_set_mstp_instance_ports(row, mstp_inst_port_info,
                                                 bridge_row->n_ports);

    /* Append the MSTP new instance to the existing list */
    mstp_info =
        xmalloc(sizeof *bridge_row->value_mstp_instances * (bridge_row->n_mstp_instances + 1));
    instId_list =
        xmalloc(sizeof *bridge_row->key_mstp_instances * (bridge_row->n_mstp_instances + 1));
    for (i = 0; i < bridge_row->n_mstp_instances; i++) {
        instId_list[j++] = bridge_row->key_mstp_instances[i];
        mstp_info[i] = bridge_row->value_mstp_instances[i];
    }
    instId_list[bridge_row->n_mstp_instances] = instid;
    mstp_info[bridge_row->n_mstp_instances] = row;

    /* Push the complete MSTP table into the bridge table */
    ovsrec_bridge_set_mstp_instances(bridge_row, instId_list,
            mstp_info, bridge_row->n_mstp_instances + 1);

    free(mstp_info);
    free(mstp_inst_port_info);
    free(instId_list);

    /* End of transaction. */
    END_DB_TXN(txn);
}

DEFUN(cli_mstp_func,
      cli_mstp_func_cmd,
      "spanning-tree",
      SPAN_TREE) {

    cli_set_mstp_configs_bridge_table (MSTP_ADMIN_STATUS,
            STATUS_ENABLE);
    return CMD_SUCCESS;
}

DEFUN(cli_mstp_func_enable,
      cli_mstp_func_enable_cmd,
      "spanning-tree (enable | disable)",
      SPAN_TREE
      "Enable spanning-tree(Default)\n"
      "Disable spanning-tree\n") {

    cli_set_mstp_configs_bridge_table(MSTP_ADMIN_STATUS, argv[0]);
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_func,
      cli_no_mstp_func_cmd,
      "no spanning-tree",
      NO_STR
      SPAN_TREE) {
    cli_set_mstp_configs_bridge_table(MSTP_ADMIN_STATUS, STATUS_DISABLE);
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_func_enable,
      cli_no_mstp_func_enable_cmd,
      "no spanning-tree (enable | disable)",
      NO_STR
      SPAN_TREE
      "Enable spanning-tree\n"
      "Disable spanning-tree(Default)\n") {
    cli_set_mstp_configs_bridge_table(MSTP_ADMIN_STATUS, STATUS_DISABLE);
    return CMD_SUCCESS;
}

DEFUN(cli_mstp_config_name,
      cli_mstp_config_name_cmd,
      "spanning-tree config-name WORD",
      SPAN_TREE
      "Set the MST region configuration name\n"
      "Specify the configuration name (maximum 32 characters)\n") {

    if (strlen(argv[0]) > MAX_CONFIG_NAME_LEN) {
        vty_out(vty, "Config-name string length exceeded\n");
        return CMD_WARNING;
    }
    cli_set_mstp_configs_bridge_table(MSTP_CONFIG_NAME, argv[0]);
    return CMD_SUCCESS;
}

DEFUN(cli_mstp_config_rev,
      cli_mstp_config_rev_cmd,
      "spanning-tree config-revision <1-40>",
      SPAN_TREE
      "Set the MST region configuration revision number(Default: 0)\n"
      "Enter an integer number\n") {

    cli_set_mstp_configs_bridge_table(MSTP_CONFIG_REV, argv[0]);
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_config_name,
      cli_no_mstp_config_name_cmd,
      "no spanning-tree config-name [WORD]",
      NO_STR
      SPAN_TREE
      "Set the MST region configuration name\n"
      "Specify the configuration name (maximum 32 characters)\n") {

    const struct ovsrec_system *system_row;
    system_row = ovsrec_system_first(idl);

    if(!system_row) {
        return CMD_OVSDB_FAILURE;
    }

    cli_set_mstp_configs_bridge_table(MSTP_CONFIG_NAME, system_row->system_mac);
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_config_rev,
      cli_no_mstp_config_rev_cmd,
      "no spanning-tree config-revision [<1-40>]",
      NO_STR
      SPAN_TREE
      "Set the MST region configuration revision number(Default: 0)\n"
      "Enter an integer number\n") {

    cli_set_mstp_configs_bridge_table(MSTP_CONFIG_REV, DEF_CONFIG_REV);
    return CMD_SUCCESS;
}

DEFUN(cli_mstp_inst_vlanid,
      cli_mstp_inst_vlanid_cmd,
      "spanning-tree instance <1-64> vlan VLANID",
      SPAN_TREE
      MST_INST
      "Enter an integer number\n"
      VLAN_STR
      "VLAN to add or to remove from the MST instance\n") {
    cli_mstp_inst_vlan_map (atoi(argv[0]), argv[1], ADD_VLAN_TO_INSTANCE);
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_inst_vlanid,
      cli_no_mstp_inst_vlanid_cmd,
      "no spanning-tree instance <1-64> vlan VLANID",
      NO_STR
      SPAN_TREE
      MST_INST
      "Enter an integer number\n"
      VLAN_STR
      "VLAN to add or to remove from the MST instance\n") {
    cli_mstp_inst_vlan_map (atoi(argv[0]), argv[1], REMOVE_VLAN_FROM_INSTANCE);
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_inst,
      cli_no_mstp_inst_cmd,
      "no spanning-tree instance <1-64>",
      NO_STR
      SPAN_TREE
      MST_INST
      "Enter an integer number\n") {
    cli_mstp_inst_vlan_map (atoi(argv[0]), NULL, REMOVE_INSTANCE);
    return CMD_SUCCESS;
}

DEFUN(cli_mstp_port_type,
      cli_mstp_port_type_cmd,
      "spanning-tree port-type (admin-edge | admin-network)",
      SPAN_TREE
      "Type of port\n"
      "Set as administrative edge port\n"
      "Set as administrative network port\n") {

    const bool value = (VTYSH_STR_EQ(argv[0], "admin-edge"))?true:false;
    cli_set_mstp_cist_port_table( vty->index, MSTP_ADMIN_EDGE, value);
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_port_type_admin,
      cli_no_mstp_port_type_admin_cmd,
      "no spanning-tree port-type (admin-edge | admin-network)",
      NO_STR
      SPAN_TREE
      "Type of port\n"
      "Set as administrative edge port\n"
      "Set as administrative network port\n") {

    cli_set_mstp_cist_port_table(vty->index, MSTP_ADMIN_EDGE, DEF_ADMIN_EDGE);
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_port_type,
      cli_no_mstp_port_type_cmd,
      "no spanning-tree port-type",
      NO_STR
      SPAN_TREE
      "Type of port\n") {

    cli_set_mstp_cist_port_table(vty->index, MSTP_ADMIN_EDGE, DEF_ADMIN_EDGE);
    return CMD_SUCCESS;
}

DEFUN(cli_mstp_bpdu,
      cli_mstp_bpdu_cmd,
      "spanning-tree (bpdu-guard | root-guard | loop-guard | bpdu-filter)",
      SPAN_TREE
      BPDU_GUARD
      ROOT_GUARD
      LOOP_GUARD
      BPDU_FILTER) {

    cli_set_mstp_cist_port_table(vty->index, argv[0], true);
    return CMD_SUCCESS;
}

DEFUN(cli_mstp_bpdu_enable,
      cli_mstp_bpdu_enable_cmd,
      "spanning-tree (bpdu-guard | root-guard | loop-guard | bpdu-filter) (enable | disable)",
      SPAN_TREE
      BPDU_GUARD
      ROOT_GUARD
      LOOP_GUARD
      BPDU_FILTER
      "Enable feature for this interface\n"
      "Disable feture for this interface\n") {

    const bool value = (VTYSH_STR_EQ(argv[1], "enable"))?true:false;
    cli_set_mstp_cist_port_table( vty->index, argv[0], value);
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_bpdu_enable,
      cli_no_mstp_bpdu_enable_cmd,
      "no spanning-tree (bpdu-guard | root-guard | loop-guard | bpdu-filter) (enable | disable)",
      NO_STR
      SPAN_TREE
      BPDU_GUARD
      ROOT_GUARD
      LOOP_GUARD
      BPDU_FILTER
      "Enable feature for this interface\n"
      "Disable feture for this interface\n") {

    cli_set_mstp_cist_port_table(vty->index, argv[0], DEF_BPDU_STATUS);
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_bpdu,
      cli_no_mstp_bpdu_cmd,
      "no spanning-tree (bpdu-guard | root-guard | loop-guard | bpdu-filter)",
      NO_STR
      SPAN_TREE
      BPDU_GUARD
      ROOT_GUARD
      LOOP_GUARD
      BPDU_FILTER) {

    cli_set_mstp_cist_port_table(vty->index, argv[0], DEF_BPDU_STATUS);
    return CMD_SUCCESS;
}

DEFUN(cli_mstp_bridge_priority,
      cli_mstp_bridge_priority_cmd,
      "spanning-tree priority <0-61440>",
      SPAN_TREE
      BRIDGE_PRIORITY
      "Enter an integer number\n") {

    cli_set_mstp_cist_table( MSTP_BRIDGE_PRIORITY, atoi(argv[0]));
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_bridge_priority,
      cli_no_mstp_bridge_priority_cmd,
      "no spanning-tree priority [<0-61440>]",
      NO_STR
      SPAN_TREE
      BRIDGE_PRIORITY
      "Enter an integer number\n") {

    cli_set_mstp_cist_table( MSTP_BRIDGE_PRIORITY, DEF_BRIDGE_PRIORITY);
    return CMD_SUCCESS;
}

DEFUN(cli_mstp_inst_priority,
      cli_mstp_inst_priority_cmd,
      "spanning-tree instance <1-64> priority <0-224>",
      SPAN_TREE
      MST_INST
      "Enter an integer number\n"
      INST_PRIORITY
      "Enter an integer number\n") {

    cli_set_mstp_inst(NULL, MSTP_BRIDGE_PRIORITY, atoi(argv[0]), atoi(argv[1]));
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_inst_priority,
      cli_no_mstp_inst_priority_cmd,
      "no spanning-tree instance <1-64> priority [<0-224>]",
      NO_STR
      SPAN_TREE
      MST_INST
      "Enter an integer number\n"
      INST_PRIORITY
      "Enter an integer number\n") {

    cli_set_mstp_inst(NULL, MSTP_BRIDGE_PRIORITY, atoi(argv[0]),
                        DEF_BRIDGE_PRIORITY);
    return CMD_SUCCESS;
}

DEFUN(cli_mstp_inst_cost,
      cli_mstp_inst_cost_cmd,
      "spanning-tree instance <1-64> cost <1-200000000>",
      SPAN_TREE
      MST_INST
      "Enter an integer number\n"
      "Specify a standard to use when calculating the default pathcost"
      "Enter an integer number\n") {

    cli_set_mstp_inst(vty->index, MSTP_PORT_COST, atoi(argv[0]), atoi(argv[1]));
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_inst_cost,
      cli_no_mstp_inst_cost_cmd,
      "no spanning-tree instance <1-64> cost [<1-200000000>]",
      NO_STR
      SPAN_TREE
      MST_INST
      "Enter an integer number\n"
      "Specify a standard to use when calculating the default pathcost"
      "Enter an integer number\n") {

    cli_set_mstp_inst(vty->index, MSTP_PORT_COST, atoi(argv[0]),
                                                  DEF_MSTP_COST);
    return CMD_SUCCESS;
}

DEFUN(cli_mstp_inst_port_priority,
      cli_mstp_inst_port_priority_cmd,
      "spanning-tree instance <1-64> port-priority <0-224>",
      SPAN_TREE
      MST_INST
      "Enter an integer number\n"
      PORT_PRIORITY
      "Enter an integer number\n") {

    cli_set_mstp_inst(vty->index, MSTP_PORT_PRIORITY, atoi(argv[0]),
                                                      atoi(argv[1]));
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_inst_port_priority,
      cli_no_mstp_inst_port_priority_cmd,
      "no spanning-tree instance <1-64> port-priority [<0-224>]",
      SPAN_TREE
      MST_INST
      "Enter an integer number\n"
      PORT_PRIORITY
      "Enter an integer number\n") {

    cli_set_mstp_inst(vty->index, MSTP_PORT_PRIORITY, atoi(argv[0]),
                                      DEF_MSTP_PORT_PRIORITY);
    return CMD_SUCCESS;
}

DEFUN(cli_mstp_hello,
      cli_mstp_hello_cmd,
      "spanning-tree hello-time <2-10>",
      SPAN_TREE
      "Set message transmission interval in seconds on the port\n"
      "Enter an integer number\n") {

    cli_set_mstp_cist_table( MSTP_HELLO_TIME, atoi(argv[0]));
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_hello,
      cli_no_mstp_hello_cmd,
      "no spanning-tree hello-time [<2-10>]",
      NO_STR
      SPAN_TREE
      "Set message transmission interval in seconds on the port\n"
      "Enter an integer number\n") {

    cli_set_mstp_cist_table( MSTP_HELLO_TIME, DEF_HELLO_TIME);
    return CMD_SUCCESS;
}

DEFUN(cli_mstp_forward_delay,
      cli_mstp_forward_delay_cmd,
      "spanning-tree forward-delay <4-30>",
      SPAN_TREE
      "Set time the switch waits between transitioning from listening to learning and from learning to forrwarding\n"
      "Enter an integer number\n") {

    cli_set_mstp_cist_table( MSTP_FORWARD_DELAY, atoi(argv[0]));
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_forward_delay,
      cli_no_mstp_forward_delay_cmd,
      "no spanning-tree forward-delay [<4-30>]",
      NO_STR
      SPAN_TREE
      "Set time the switch waits between transitioning from listening to learning and from learning to forrwarding\n"
      "Enter an integer number\n") {

    cli_set_mstp_cist_table( MSTP_FORWARD_DELAY, DEF_FORWARD_DELAY);
    return CMD_SUCCESS;
}

DEFUN(cli_mstp_max_hops,
      cli_mstp_max_hops_cmd,
      "spanning-tree max-hops <1-40>",
      SPAN_TREE
      "Set the max number of hops in a region before the MST BPDU is discarded and the information held for a port is\n"
      "Enter an integer number\n") {

    cli_set_mstp_cist_table( MSTP_MAX_HOP_COUNT, atoi(argv[0]));
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_max_hops,
      cli_no_mstp_max_hops_cmd,
      "no spanning-tree max-hops [<1-40>]",
      NO_STR
      SPAN_TREE
      "Set the max number of hops in a region before the MST BPDU is discarded and the information held for a port is\n"
      "Enter an integer number\n") {

    cli_set_mstp_cist_table( MSTP_MAX_HOP_COUNT, DEF_MAX_HOPS);
    return CMD_SUCCESS;
}

DEFUN(cli_mstp_max_age,
      cli_mstp_max_age_cmd,
      "spanning-tree max-age <6-40>",
      SPAN_TREE
      "Set maximum age of received STP information before it is discarded\n"
      "Enter an integer number\n") {

    cli_set_mstp_cist_table( MSTP_MAX_AGE, atoi(argv[0]));
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_max_age,
      cli_no_mstp_max_age_cmd,
      "no spanning-tree max-age [<6-40>]",
      NO_STR
      SPAN_TREE
      "Set maximum age of received STP information before it is discarded\n"
      "Enter an integer number\n") {

    cli_set_mstp_cist_table( MSTP_MAX_AGE, DEF_MAX_AGE);
    return CMD_SUCCESS;
}

DEFUN(cli_mstp_transmit_hold_count,
      cli_mstp_transmit_hold_count_cmd,
      "spanning-tree transmit-hold-count <1-10>",
      SPAN_TREE
      "Sets the transmit hold count performance parameter\n"
      "Enter an integer number\n") {

    cli_set_mstp_cist_table( MSTP_TX_HOLD_COUNT, atoi(argv[0]));
    return CMD_SUCCESS;
}

DEFUN(cli_no_mstp_transmit_hold_count,
      cli_no_mstp_transmit_hold_count_cmd,
      "no spanning-tree transmit-hold-count [<1-10>]",
      NO_STR
      SPAN_TREE
      "Sets the transmit hold count performance parameter\n"
      "Enter an integer number\n") {

    cli_set_mstp_cist_table( MSTP_TX_HOLD_COUNT, atoi(argv[0]));
    return CMD_SUCCESS;
}

/* MSTP Show commands*/
DEFUN(show_spanning_tree,
      show_spanning_tree_cmd,
      "show spanning-tree",
      SHOW_STR
      SPAN_TREE) {

    cli_show_spanning_tree_config();
    return CMD_SUCCESS;
}

DEFUN(show_mstp_config,
      show_mstp_config_cmd,
      "show spanning-tree mst-config",
      SHOW_STR
      SPAN_TREE
      "Show multiple spanning tree region configuration.\n") {
    cli_show_mstp_config();
    return CMD_SUCCESS;
}

DEFUN(show_running_config_mstp,
      show_running_config_mstp_cmd,
      "show running-config spanning-tree",
      SHOW_STR
      "Show the switch running configuration.\n"
      SPAN_TREE) {
    cli_show_mstp_running_config();
    return CMD_SUCCESS;
}

DEFUN(show_spanning_mst,
      show_spanning_mst_cmd,
      "show spanning-tree mst",
      SHOW_STR
      SPAN_TREE
      MST_INST) {
    cli_show_mst();
    return CMD_SUCCESS;
}

void
cli_pre_init() {

    ovsdb_idl_add_column(idl, &ovsrec_bridge_col_mstp_instances);
    ovsdb_idl_add_column(idl, &ovsrec_bridge_col_mstp_common_instance);
    ovsdb_idl_add_column(idl, &ovsrec_bridge_col_mstp_enable);
    ovsdb_idl_add_column(idl, &ovsrec_bridge_col_other_config);

    ovsdb_idl_add_table(idl, &ovsrec_table_mstp_instance);
    ovsdb_idl_add_table(idl, &ovsrec_table_mstp_instance_port);
    ovsdb_idl_add_table(idl, &ovsrec_table_mstp_common_instance);
    ovsdb_idl_add_table(idl, &ovsrec_table_mstp_common_instance_port);

    /* MSTP Instance Table. */
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_col_topology_change_disable);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_col_time_since_top_change);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_col_hardware_grp_id);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_col_designated_root);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_col_root_port);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_col_priority);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_col_mstp_instance_ports);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_col_bridge_identifier);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_col_root_path_cost);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_col_top_change_cnt);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_col_vlans);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_col_root_priority);

    /* mstp instance port table */
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_port_col_designated_bridge);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_port_col_port_role);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_port_col_designated_root);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_port_col_port_priority);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_port_col_admin_path_cost);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_port_col_designated_bridge_priority);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_port_col_port_state);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_port_col_designated_root_priority);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_port_col_designated_cost);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_port_col_port);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_instance_port_col_designated_port);

    /* mstp common instance table */
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_remaining_hops);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_mstp_common_instance_ports);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_forward_delay_expiry_time);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_regional_root);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_oper_tx_hold_count);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_max_age);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_max_hop_count);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_designated_root);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_priority);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_root_path_cost);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_root_port);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_root_priority);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_hello_time);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_cist_path_cost);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_oper_max_age);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_oper_hello_time);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_top_change_cnt);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_vlans);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_bridge_identifier);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_time_since_top_change);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_hardware_grp_id);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_hello_expiry_time);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_oper_forward_delay);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_col_forward_delay);

    /* mstp common instance port table */
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_port_role);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_protocol_migration_enable);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_bpdu_filter_disable);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_admin_edge_port_disable);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_port_path_cost);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_port);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_designated_port);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_root_guard_disable);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_designated_bridge);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_designated_path_cost);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_designated_root);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_bpdu_guard_disable);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_mstp_statistics);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_port_hello_time);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_link_type);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_admin_path_cost);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_cist_path_cost);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_port_priority);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_bpdus_rx_enable);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_loop_guard_disable);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_bpdus_tx_enable);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_cist_regional_root_id);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_restricted_port_tcn_disable);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_oper_edge_port);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_restricted_port_role_disable);
    ovsdb_idl_add_column(idl, &ovsrec_mstp_common_instance_port_col_port_state);
}

void cli_post_init(void) {

    vtysh_ret_val retval = e_vtysh_error;

    /* Bridge Table Config */
    install_element(CONFIG_NODE, &cli_mstp_func_cmd);
    install_element(CONFIG_NODE, &cli_mstp_func_enable_cmd);
    install_element(CONFIG_NODE, &cli_no_mstp_func_cmd);
    install_element(CONFIG_NODE, &cli_no_mstp_func_enable_cmd);
    install_element(CONFIG_NODE, &cli_mstp_config_name_cmd);
    install_element(CONFIG_NODE, &cli_mstp_config_rev_cmd);
    install_element(CONFIG_NODE, &cli_no_mstp_config_rev_cmd);
    install_element(CONFIG_NODE, &cli_no_mstp_config_name_cmd);

    /* CIST table config */
    install_element(CONFIG_NODE, &cli_mstp_hello_cmd);
    install_element(CONFIG_NODE, &cli_mstp_forward_delay_cmd);
    install_element(CONFIG_NODE, &cli_mstp_max_age_cmd);
    install_element(CONFIG_NODE, &cli_mstp_max_hops_cmd);
    install_element(CONFIG_NODE, &cli_no_mstp_hello_cmd);
    install_element(CONFIG_NODE, &cli_mstp_transmit_hold_count_cmd);
    install_element(CONFIG_NODE, &cli_no_mstp_forward_delay_cmd);
    install_element(CONFIG_NODE, &cli_no_mstp_max_age_cmd);
    install_element(CONFIG_NODE, &cli_no_mstp_max_hops_cmd);
    install_element(CONFIG_NODE, &cli_no_mstp_transmit_hold_count_cmd);
    install_element(CONFIG_NODE, &cli_mstp_bridge_priority_cmd);
    install_element(CONFIG_NODE, &cli_no_mstp_bridge_priority_cmd);

    /* CIST port table config*/
    install_element(INTERFACE_NODE, &cli_mstp_bpdu_enable_cmd);
    install_element(INTERFACE_NODE, &cli_mstp_bpdu_cmd);
    install_element(INTERFACE_NODE, &cli_no_mstp_bpdu_cmd);
    install_element(INTERFACE_NODE, &cli_no_mstp_bpdu_enable_cmd);
    install_element(INTERFACE_NODE, &cli_mstp_port_type_cmd);
    install_element(INTERFACE_NODE, &cli_no_mstp_port_type_admin_cmd);
    install_element(INTERFACE_NODE, &cli_no_mstp_port_type_cmd);

    /* MSTP Inst Table */
    install_element(CONFIG_NODE, &cli_mstp_inst_vlanid_cmd);
    install_element(CONFIG_NODE, &cli_no_mstp_inst_vlanid_cmd);
    install_element(CONFIG_NODE, &cli_no_mstp_inst_cmd);
    install_element(CONFIG_NODE, &cli_mstp_inst_priority_cmd);
    install_element(CONFIG_NODE, &cli_no_mstp_inst_priority_cmd);

    /* MSTP Inst port Table */
    install_element(CONFIG_NODE, &cli_mstp_inst_port_priority_cmd);
    install_element(CONFIG_NODE, &cli_no_mstp_inst_port_priority_cmd);
    install_element(CONFIG_NODE, &cli_mstp_inst_cost_cmd);
    install_element(CONFIG_NODE, &cli_no_mstp_inst_cost_cmd);

    /* show commands */
    install_element(ENABLE_NODE, &show_spanning_tree_cmd);
    install_element(ENABLE_NODE, &show_spanning_mst_cmd);
    install_element(ENABLE_NODE, &show_mstp_config_cmd);
    install_element(ENABLE_NODE, &show_running_config_mstp_cmd);

    retval = install_show_run_config_subcontext(e_vtysh_config_context,
                            e_vtysh_config_context_mstp,
                            &vtysh_config_context_mstp_clientcallback,
                            NULL, NULL);

    if(e_vtysh_ok != retval)
    {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                "MSTP context unable to add config callback");
        assert(0);
    }

    retval = install_show_run_config_subcontext(e_vtysh_interface_context,
                            e_vtysh_interface_context_mstp,
                            &vtysh_intf_context_mstp_clientcallback,
                            NULL, NULL);

    if(e_vtysh_ok != retval)
    {
        vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_ERR,
                "MSTP context unable to add config callback");
        /*assert(0);*/
    }
}
