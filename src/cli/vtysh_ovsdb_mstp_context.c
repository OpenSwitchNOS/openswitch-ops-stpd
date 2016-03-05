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
 *    File               : vtysh_ovsdb_mstp_context.c
 *    Description        : MSTP Protocol show running config API
 ******************************************************************************/
#include "vtysh/vty.h"
#include "vtysh/vector.h"
#include "vswitch-idl.h"
#include "openswitch-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "vtysh_ovsdb_mstp_context.h"
#include "mstp_vty.h"

int
vtysh_ovsdb_parse_mstp_global_config(vtysh_ovsdb_cbmsg_ptr p_msg) {
    const struct ovsrec_bridge *bridge_row = NULL;
    const struct ovsrec_system *system_row = NULL;
    const struct ovsrec_mstp_common_instance *cist_row = NULL;
    const struct ovsrec_mstp_instance *mstp_row = NULL;
    const struct ovsrec_mstp_instance_port *mstp_port_row = NULL;
    const char *data = NULL;
    size_t i = 0, j = 0;

    system_row = ovsrec_system_first(p_msg->idl);
    if (!system_row) {
        return e_vtysh_error;
    }

    /* Bridge configs */
    bridge_row = ovsrec_bridge_first(p_msg->idl);
    if (bridge_row) {
        if (bridge_row->mstp_enable &&
                (*bridge_row->mstp_enable != DEF_ADMIN_STATUS)) {
            vtysh_ovsdb_cli_print(p_msg, "spanning-tree enable\n");
        }

        data = smap_get(&bridge_row->other_config, MSTP_CONFIG_NAME);
        if (data && (!VTYSH_STR_EQ(data, system_row->system_mac))) {
            vtysh_ovsdb_cli_print(p_msg, "spanning-tree config-name %s\n", data);
        }

        data = smap_get(&bridge_row->other_config, MSTP_CONFIG_REV);
        if (data && (atoi(DEF_CONFIG_REV) != atoi(data))) {
            vtysh_ovsdb_cli_print(p_msg, "spanning-tree config-revision %d\n", atoi(data));
        }

        /* Loop for all instance in bridge table */
        for (i=0; i < bridge_row->n_mstp_instances; i++) {
            mstp_row = bridge_row->value_mstp_instances[i];
            vtysh_ovsdb_cli_print(p_msg, "spanning-tree instance %ld vlan %ld",
                    bridge_row->key_mstp_instances[i],mstp_row->vlans[0]->id );

            /* Loop for all vlans in one MST instance table */
            for (j=1; j<mstp_row->n_vlans; j++) {
                vtysh_ovsdb_cli_print(p_msg, ",%ld", mstp_row->vlans[j]->id);
            }
            vtysh_ovsdb_cli_print(p_msg, "\n" );

            if (mstp_row->priority &&
                    (*mstp_row->priority != DEF_BRIDGE_PRIORITY)) {
                vtysh_ovsdb_cli_print(p_msg, "spanning-tree instance %ld priority %ld\n",
                bridge_row->key_mstp_instances[i], *mstp_row->priority);
            }

            /* Loop for all ports in the instance table */
            for (j=0; j<mstp_row->n_mstp_instance_ports; j++) {
                mstp_port_row = mstp_row->mstp_instance_ports[j];
                if (mstp_port_row->port_priority &&
                   (*mstp_port_row->port_priority != DEF_MSTP_PORT_PRIORITY)) {
                    vtysh_ovsdb_cli_print(p_msg, "spanning-tree instance %ld port-priority %ld\n",
                            bridge_row->key_mstp_instances[i],
                            *mstp_port_row->port_priority);
                }
                if (mstp_port_row->admin_path_cost &&
                        (*mstp_port_row->admin_path_cost != DEF_MSTP_COST)) {
                    vtysh_ovsdb_cli_print(p_msg, "spanning-tree instance %ld cost %ld\n",
                            bridge_row->key_mstp_instances[i],
                            *mstp_port_row->admin_path_cost);
                }
            }
        }
    }

    /* CIST configs */
    cist_row = ovsrec_mstp_common_instance_first (p_msg->idl);
    if (cist_row) {
        if (cist_row->priority &&
                *cist_row->priority != DEF_BRIDGE_PRIORITY) {
            vtysh_ovsdb_cli_print(p_msg, "spanning-tree priority %ld\n", *cist_row->priority);
        }
        if (cist_row->forward_delay &&
                *cist_row->forward_delay != DEF_FORWARD_DELAY) {
            vtysh_ovsdb_cli_print(p_msg, "spanning-tree forward-delay %ld\n",
                    *cist_row->forward_delay);
        }
        if (cist_row->max_age && *cist_row->max_age != DEF_MAX_AGE) {
            vtysh_ovsdb_cli_print(p_msg, "spanning-tree max-age %ld\n", *cist_row->max_age);
        }
        if (cist_row->max_hop_count &&
                *cist_row->max_hop_count != DEF_MAX_HOPS) {
            vtysh_ovsdb_cli_print(p_msg, "spanning-tree max-hops %ld\n",
                    *cist_row->max_hop_count);
        }
    }
    return e_vtysh_ok;
}

void
vtysh_ovsdb_parse_mstp_intf_config(vtysh_ovsdb_cbmsg_ptr p_msg) {
    const struct ovsrec_mstp_common_instance *cist_row = NULL;
    const struct ovsrec_mstp_common_instance_port *cist_port_row = NULL;
    size_t i = 0;

    cist_row = ovsrec_mstp_common_instance_first (p_msg->idl);
    if (cist_row) {
        /* CIST port configs */
        for (i=0; i < cist_row->n_mstp_common_instance_ports; i++) {
            cist_port_row = cist_row->mstp_common_instance_ports[i];
            if (cist_port_row->loop_guard_disable &&
                    *cist_port_row->loop_guard_disable != DEF_BPDU_STATUS) {
                vtysh_ovsdb_cli_print(p_msg, "spanning-tree loop-guard enable\n");
            }
            if (cist_port_row->root_guard_disable &&
                    *cist_port_row->root_guard_disable != DEF_BPDU_STATUS) {
                vtysh_ovsdb_cli_print(p_msg, "spanning-tree root-guard enable\n");
            }
            if (cist_port_row->bpdu_guard_disable &&
                    *cist_port_row->bpdu_guard_disable != DEF_BPDU_STATUS) {
                vtysh_ovsdb_cli_print(p_msg, "spanning-tree bpdu-guard enable\n");
            }
            if (cist_port_row->bpdu_filter_disable &&
                    *cist_port_row->bpdu_filter_disable != DEF_BPDU_STATUS) {
                vtysh_ovsdb_cli_print(p_msg, "spanning-tree bpdu-filter enable\n");
            }
            if (cist_port_row->admin_edge_port_disable &&
                  *cist_port_row->admin_edge_port_disable != DEF_ADMIN_EDGE) {
                vtysh_ovsdb_cli_print(p_msg,"spanning-tree port-type admin-edge\n");
            }
        }
    }
}

vtysh_ret_val vtysh_config_context_mstp_clientcallback(void *p_private) {
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;

    vtysh_ovsdb_parse_mstp_global_config(p_msg);
    return e_vtysh_ok;
}

vtysh_ret_val vtysh_intf_context_mstp_clientcallback(void *p_private) {
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;

    vtysh_ovsdb_parse_mstp_intf_config(p_msg);
    return e_vtysh_ok;
}
