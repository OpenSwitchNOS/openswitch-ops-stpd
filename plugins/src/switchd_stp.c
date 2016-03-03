/** @file stp.c
 */

/* Copyright (C) 2016 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include "hash.h"
#include "hmap.h"
#include "shash.h"
#include "vswitch-idl.h"
#include "openswitch-idl.h"
#include "ofproto/ofproto.h"
#include "openvswitch/vlog.h"
#include "plugin-extensions.h"
#include "switchd_stp_plugin.h"
#include "switchd_stp.h"

VLOG_DEFINE_THIS_MODULE(stp);

/* temp port strcuture decalred here to avoid compilation */
struct bridge_port {
    struct hmap_node hmap_node;
    void *br;
    char *name;
    const struct ovsrec_port *cfg;
    struct ovs_list ifaces;
    int bond_hw_handle;
};

struct hmap all_mstp_instances = HMAP_INITIALIZER(&all_mstp_instances);

/*------------------------------------------------------------------------------
| Function:  get_port_state_from_string
| Description: get the port state
| Parameters[in]: portstate_str: string  contains port name
| Parameters[out]: port_state:- object conatins port state enum mstp_instance_port_state
| Return: True if valid port state else false.
-----------------------------------------------------------------------------*/
bool
get_port_state_from_string(const char *portstate_str, int *port_state)
{
    if (!portstate_str || !port_state) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return false;
    }

    if (!strcmp(portstate_str,
                OVSREC_MSTP_COMMON_INSTANCE_PORT_PORT_STATE_BLOCKING)) {
        *port_state = MSTP_INST_PORT_STATE_BLOCKED;
    } else if (!strcmp(portstate_str,
                       OVSREC_MSTP_INSTANCE_PORT_PORT_STATE_DISABLED)) {
        *port_state = MSTP_INST_PORT_STATE_DISABLED;
    } else if (!strcmp(portstate_str,
                       OVSREC_MSTP_INSTANCE_PORT_PORT_STATE_LEARNING)) {
        *port_state = MSTP_INST_PORT_STATE_LEARNING;
    } else if (!strcmp(portstate_str,
                       OVSREC_MSTP_INSTANCE_PORT_PORT_STATE_FORWARDING)) {
        *port_state = MSTP_INST_PORT_STATE_FORWARDING;
    } else {
        return false;
    }
    return true;
}

/*-----------------------------------------------------------------------------
| Function:  mstp_br_port_lookup
| Description:  lookup given port name in struct bridge hmap ports.
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
|                         name:- port name to lookup
| Parameters[out]: None
| Return:  returns bridge port object
-----------------------------------------------------------------------------*/
static struct bridge_port *
mstp_br_port_lookup(const struct blk_params *br, const char *name)
{
    struct bridge_port *port;

    if (!br || !name) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return NULL;
    }

    HMAP_FOR_EACH_WITH_HASH (port, hmap_node, hash_string(name, 0),
                             br->ports) {
        if (!strcmp(port->name, name)) {
            return port;
        }
    }
    return NULL;
}

/*-----------------------------------------------------------------------------
| Function:  mstp_cist_and_instance_vlan_add
| Description:  Add vlan to cist or msti
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[in]: mstp_instance object
| Parameters[in]: ovsrec_vlan object
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
mstp_cist_and_instance_vlan_add(const struct blk_params *br,
                                       struct mstp_instance *msti,
                                       const struct ovsrec_vlan *vlan_cfg )
{
    struct mstp_instance_vlan *new_vlan = NULL;

    VLOG_DBG("%s: called", __FUNCTION__);

    if (!br || !msti || !vlan_cfg) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return;
    }

    /* Allocate structure to save state information for this VLAN. */
    new_vlan = xzalloc(sizeof(struct mstp_instance_vlan));

    hmap_insert(&msti->vlans, &new_vlan->hmap_node,
                hash_string(vlan_cfg->name, 0));

    new_vlan->vid = (int)vlan_cfg->id;
    new_vlan->name = xstrdup(vlan_cfg->name);
    VLOG_DBG("ofproto add vlan %d to stg %d", new_vlan->vid,
                                              msti->hw_stg_id);
    ofproto_add_stg_vlan(br->ofproto, msti->hw_stg_id, new_vlan->vid);

}

/*-----------------------------------------------------------------------------
| Function:  mstp_cist_and_instance_vlan_delete
| Description: delete vlan from cist or msti
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[in]: mstp_instance object
| Parameters[in]: mstp_instance_vlan object
| Parameters[out]: None
| Return:
-----------------------------------------------------------------------------*/
void
mstp_cist_and_instance_vlan_delete(const struct blk_params *br,
                                         struct mstp_instance *msti,
                                         struct mstp_instance_vlan *vlan)
{
    int vid;

    VLOG_DBG("%s: called", __FUNCTION__);

    if (!br || !msti || !vlan) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return;
    }

    VLOG_DBG("ofproto delete vlan %d from stg %d",vlan->vid,
                                          msti->instance_id);
    vid = vlan->vid;
    hmap_remove(&msti->vlans, &vlan->hmap_node);
    free(vlan->name);
    free(vlan);

    /* TODO call ofproto api to delete vlan */
    ofproto_remove_stg_vlan(br->ofproto, msti->hw_stg_id, vid);
}

/*-----------------------------------------------------------------------------
| Function: mstp_cist_and_instance_vlan_lookup
| Description: find vlan in cist/mst
| Parameters[in]: mstp_instance object
| Parameters[in]: vlan name
| Parameters[out]: None
| Return: mstp_instance_vlan object
-----------------------------------------------------------------------------*/
static struct mstp_instance_vlan *
mstp_cist_and_instance_vlan_lookup(const struct mstp_instance *msti,
                                          const char *name)
{
    struct mstp_instance_vlan *vlan;

    VLOG_DBG("%s: called", __FUNCTION__);

    if (!msti || !name) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return NULL;
    }

    HMAP_FOR_EACH_WITH_HASH (vlan, hmap_node, hash_string(name, 0),
                             &msti->vlans) {
        if (!strcmp(vlan->name, name)) {
            return vlan;
        }
    }
    return NULL;
}

/*-----------------------------------------------------------------------------
| Function: mstp_instance_add_del_vlans
| Description: add or delete vlan in mstp instance
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[in]: mstp_instance object
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
mstp_instance_add_del_vlans(const struct blk_params *br,
                                  struct mstp_instance *msti)
{
    size_t i;
    struct mstp_instance_vlan *vlan, *next;
    struct shash sh_idl_vlans;
    struct shash_node *sh_node;

    VLOG_DBG("%s: called", __FUNCTION__);

    if (!msti || !br) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return;
    }

    /* Collect all Instance VLANs present in the DB. */
    shash_init(&sh_idl_vlans);
    for (i = 0; i < msti->cfg.msti_cfg->n_vlans; i++) {
        const char *name = msti->cfg.msti_cfg->vlans[i]->name;
        if (!shash_add_once(&sh_idl_vlans, name,
                            msti->cfg.msti_cfg->vlans[i])) {
            VLOG_WARN("mstp instance id %d: %s specified twice as msti VLAN",
                      msti->instance_id, name);
        }
    }

    /* Delete old Instance VLANs. */
    HMAP_FOR_EACH_SAFE (vlan, next, hmap_node, &msti->vlans) {
        const struct ovsrec_vlan *vlan_cfg;

        vlan_cfg = shash_find_data(&sh_idl_vlans, vlan->name);
        if (!vlan_cfg) {
            VLOG_DBG("Found a deleted vlan %s in msti %d", vlan->name,
                     msti->instance_id);
            /* Need to update ofproto now since this VLAN
                       * won't be around for the "check for changes"
                       * loop below. */
            mstp_cist_and_instance_vlan_delete(br, msti, vlan);
        }
    }

    /* Add new VLANs. */
    SHASH_FOR_EACH (sh_node, &sh_idl_vlans) {
        vlan = mstp_cist_and_instance_vlan_lookup(msti, sh_node->name);
        if (!vlan) {
            VLOG_DBG("Found an added vlan %s in msti %d", sh_node->name,
                     msti->instance_id);
            mstp_cist_and_instance_vlan_add(br, msti, sh_node->data);
        }
    }

    /* Destroy the shash of the IDL vlans */
    shash_destroy(&sh_idl_vlans);

}

/*------------------------------------------------------------------------------
| Function:  mstp_cist_and_instance_set_port_state
| Description:  set port state in cist/msti
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[in]: mstp_instance object
| Parameters[in]: mstp_instance_port object
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
mstp_cist_and_instance_set_port_state(const struct blk_params *br,
                                            struct mstp_instance *msti,
                                          struct mstp_instance_port *mstp_port)
{
    struct bridge_port *port;

    VLOG_DBG("%s: called", __FUNCTION__);

    if (!msti || !br || !mstp_port) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return;
    }

    /* Get port info */
    port = mstp_br_port_lookup(br, mstp_port->name);
    if (port == NULL) {
        VLOG_DBG("Failed to get port cfg for %s", mstp_port->name);
        return;
    }

    ofproto_set_stg_port_state(br->ofproto, port, msti->hw_stg_id,
                               mstp_port->stp_state, true);
}

/*------------------------------------------------------------------------------
| Function:   mstp_instance_port_add
| Description: add port to msti
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[in]: mstp_instance_object
| Parameters[in]:  ovsrec_mstp_instance_port object
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
mstp_instance_port_add(const struct blk_params *br,
                            struct mstp_instance *msti,
                         const struct ovsrec_mstp_instance_port *inst_port_cfg)
{
        struct mstp_instance_port *new_port = NULL;
        bool retval = false;
        int port_state;

        VLOG_DBG("%s: called", __FUNCTION__);

        if (!msti || !br || !inst_port_cfg) {
            VLOG_DBG("%s: invalid param", __FUNCTION__);
            return;
        }

        /* Allocate structure to save state information for this port. */
        new_port = xzalloc(sizeof(struct mstp_instance_port));

        hmap_insert(&msti->ports, &new_port->hmap_node,
                    hash_string(inst_port_cfg->port->name, 0));

        new_port->name = xstrdup(inst_port_cfg->port->name);

        retval = get_port_state_from_string(inst_port_cfg->port_state,
                                            &port_state);
        if(false == retval) {
            VLOG_DBG("%s:invalid inst id %d port %s state %s", __FUNCTION__,
                     msti->instance_id, new_port->name, inst_port_cfg->port_state);
            new_port->stp_state = MSTP_INST_PORT_STATE_INVALID;;
            new_port->cfg.msti_port_cfg = inst_port_cfg;
            return;
        }
        new_port->stp_state = port_state;
        new_port->cfg.msti_port_cfg = inst_port_cfg;

        mstp_cist_and_instance_set_port_state(br, msti, new_port);
}

/*-----------------------------------------------------------------------------
| Function:  mstp_cist_and_instance_port_delete
| Description: delete port from cist/msti
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[in]: mstp_instance object
| Parameters[in]: mstp_instance_port object
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
mstp_cist_and_instance_port_delete(const struct blk_params *br,
                                         struct mstp_instance *msti,
                                         struct mstp_instance_port *port)
{

    VLOG_DBG("%s: called", __FUNCTION__);
    if (!msti || !br || !port) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return;
    }

    if (port) {
        hmap_remove(&msti->ports, &port->hmap_node);
        free(port->name);
        free(port);
    }

}

/*-----------------------------------------------------------------------------
| Function:  mstp_cist_and_instance_port_lookup
| Description: find port in cist/msti
| Parameters[in]:mstp_instance object
| Parameters[in]: port name
| Parameters[out]: None
| Return:
-----------------------------------------------------------------------------*/
static struct mstp_instance_port *
mstp_cist_and_instance_port_lookup(const struct mstp_instance *msti,
                                          const char *name)
{
    struct mstp_instance_port *port;

    VLOG_DBG("%s: called", __FUNCTION__);

    if (!msti || !name) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return NULL;
    }

    HMAP_FOR_EACH_WITH_HASH (port, hmap_node, hash_string(name, 0),
                             &msti->ports) {
        if (!strcmp(port->name, name)) {
            return port;
        }
    }
    return NULL;
}

/*------------------------------------------------------------------------------
| Function:  mstp_instance_add_del_ports
| Description: add/del ports from msti
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[in]: mstp_instance object
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
mstp_instance_add_del_ports(const struct blk_params *br,
                                  struct mstp_instance *msti)
{
    size_t i;
    struct mstp_instance_port *inst_port, *next;
    struct shash sh_idl_ports;
    struct shash_node *sh_node;

    VLOG_DBG("%s: called", __FUNCTION__);

    if (!msti || !br) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return;
    }

    /* Collect all Instance Ports present in the DB. */
    shash_init(&sh_idl_ports);
    for (i = 0; i < msti->cfg.msti_cfg->n_mstp_instance_ports; i++) {
        const char *name = msti->cfg.msti_cfg->mstp_instance_ports[i]->port->name;
        if (!shash_add_once(&sh_idl_ports, name,
                            msti->cfg.msti_cfg->mstp_instance_ports[i])) {
            VLOG_WARN("mstp instance id %d: %s specified twice as msti port",
                      msti->instance_id, name);
        }
    }

    /* Delete old Instance Ports. */
    HMAP_FOR_EACH_SAFE (inst_port, next, hmap_node, &msti->ports) {
        const struct ovsrec_mstp_instance_port *port_cfg;

        port_cfg = shash_find_data(&sh_idl_ports, inst_port->name);
        if (!port_cfg) {
            VLOG_DBG("Found a deleted Port %s in instance %d",
                     inst_port->name, msti->instance_id);
            mstp_cist_and_instance_port_delete(br, msti, inst_port);
        }
    }

    /* Add new instance ports. */
    SHASH_FOR_EACH (sh_node, &sh_idl_ports) {
        inst_port = mstp_cist_and_instance_port_lookup(msti, sh_node->name);
        if (!inst_port) {
            VLOG_DBG("Found an added Port %s for instance %d",
                     sh_node->name, msti->instance_id);
            mstp_instance_port_add(br, msti, sh_node->data);
        }
    }

    /* Destroy the shash of the IDL ports */
    shash_destroy(&sh_idl_ports);

}

/*-----------------------------------------------------------------------------
| Function:  mstp_instance_create
| Description:  create new mstp instance  object
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[in]: inst_id : instance id
| Parameters[in]: ovsrec_mstp_instance object
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
mstp_instance_create(const struct blk_params *br, int inst_id,
                         const struct ovsrec_mstp_instance *msti_cfg)
{
    struct mstp_instance *msti;
    int stg;

    VLOG_DBG("%s: called", __FUNCTION__);

    if (!msti_cfg || !br) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return;
    }

    if (false == MSTP_INST_VALID(inst_id)) {
        VLOG_DBG("%s: invalid instance id %d", __FUNCTION__, inst_id);
        return;
    }

    msti = xzalloc(sizeof *msti);
    msti->instance_id = inst_id;
    msti->cfg.msti_cfg= msti_cfg;
    hmap_init(&msti->vlans);
    hmap_init(&msti->ports);
    hmap_insert(&all_mstp_instances, &msti->node, hash_int(inst_id, 0));

    ofproto_create_stg(br->ofproto,&stg);
    msti->hw_stg_id = stg;
    /* msti_cfg->hardware_grp_id = stg; */

    mstp_instance_add_del_vlans(br, msti);
    mstp_instance_add_del_ports(br, msti);
}

/*-----------------------------------------------------------------------------
| Function:  mstp_instance_delete
| Description: delete instance from msti
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[in]: mstp_instance object
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
mstp_instance_delete(const struct blk_params* br,
                         struct mstp_instance *msti)
{
    if (!msti || !br) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return;
    }

    if (msti) {
        hmap_remove(&all_mstp_instances, &msti->node);
        hmap_destroy(&msti->vlans);
        hmap_destroy(&msti->ports);
        free(msti);

        ofproto_delete_stg(br->ofproto, msti->hw_stg_id);
    }

}

/*-----------------------------------------------------------------------------
| Function:  mstp_cist_and_instance_lookup
| Description: find instance in mstp_instances data
| Parameters[in]: inst_id:- instance
| Parameters[out]: None
| Return: mstp_instance object
-----------------------------------------------------------------------------*/
static struct mstp_instance *
mstp_cist_and_instance_lookup(int inst_id)
{
    struct mstp_instance *msti;

    if (false == MSTP_CIST_INST_VLAID(inst_id)) {
        VLOG_DBG("%s: invalid instance id %d", __FUNCTION__, inst_id);
        return NULL;
    }

    HMAP_FOR_EACH_WITH_HASH (msti, node, hash_int(inst_id, 0),
                             &all_mstp_instances) {
        if (inst_id == msti->instance_id) {
            return msti;
        }
    }
    return NULL;
}

/*-----------------------------------------------------------------------------
| Function:  mstp_add_del_instances
| Description: add/del instances from msti
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
mstp_add_del_instances(const struct blk_params *br)
{
    struct mstp_instance *msti, *next_msti;
    struct shash new_msti;
    const struct ovsrec_bridge *bridge_row = br->cfg;
    size_t i;

    VLOG_DBG("%s: called", __FUNCTION__);

    if (!br) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return;
    }

    /* Collect new instance  id's */
    shash_init(&new_msti);

    for (i = 0; i < bridge_row->n_mstp_instances; i++) {
        static struct vlog_rate_limit rl = VLOG_RATE_LIMIT_INIT(1, 5);
        const struct ovsrec_mstp_instance *msti_cfg =
                                   bridge_row->value_mstp_instances[i];
        int inst_id = bridge_row->key_mstp_instances[i];
        char inst_id_string[10];

        snprintf(inst_id_string, sizeof(inst_id_string), "%d", inst_id);
        if (!shash_add_once(&new_msti, inst_id_string, msti_cfg)) {
            VLOG_WARN_RL(&rl, "inst id %s specified twice", inst_id_string);
        }
    }

    /* Get rid of deleted instid's */
    HMAP_FOR_EACH_SAFE (msti, next_msti, node, &all_mstp_instances) {
        char inst_id_string[10];
        if (msti->instance_id != MSTP_CIST) {
            snprintf(inst_id_string, sizeof(inst_id_string), "%d",
                     msti->instance_id);
            msti->cfg.msti_cfg = shash_find_data(&new_msti, inst_id_string);
            if (!msti->cfg.msti_cfg) {
                VLOG_DBG("found deleted instance %d",msti->instance_id);
                mstp_instance_delete(br, msti);
            }
        }
    }

    /* Add new instancess. */
    for (i = 0; i < bridge_row->n_mstp_instances; i++) {
        int inst_id = bridge_row->key_mstp_instances[i];
        const struct ovsrec_mstp_instance *msti_cfg =
                     bridge_row->value_mstp_instances[i];
        struct mstp_instance *msti = mstp_cist_and_instance_lookup(inst_id);
        if (!msti) {
            VLOG_DBG("Found added instance %d", inst_id);
            mstp_instance_create(br, inst_id, msti_cfg);
        }
    }

    shash_destroy(&new_msti);
}


/*-----------------------------------------------------------------------------
| Function:   mstp_cist_port_add
| Description: add port to cist
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[in]: mstp_instance object
| Parameters[in]: ovsrec_mstp_common_instance_port object
| Parameters[out]: None
| Return:
-----------------------------------------------------------------------------*/
void
mstp_cist_port_add(const struct blk_params *br, struct mstp_instance *msti,
                 const struct ovsrec_mstp_common_instance_port *cist_port_cfg )
{
        struct mstp_instance_port *new_port = NULL;
        bool retval = false;
        int port_state;

        if (!msti || !br || !cist_port_cfg) {
            VLOG_DBG("%s: invalid param", __FUNCTION__);
            return;
        }

        /* Allocate structure to save state information for this port. */
        new_port = xzalloc(sizeof(struct mstp_instance_port));

        hmap_insert(&msti->ports, &new_port->hmap_node,
                    hash_string(cist_port_cfg->port->name, 0));

        new_port->name = xstrdup(cist_port_cfg->port->name);

        retval = get_port_state_from_string(cist_port_cfg->port_state,
                                            &port_state);
        if (false == retval) {
            VLOG_DBG("%s:invalid CIST port %s state %s", __FUNCTION__,
                     new_port->name, cist_port_cfg->port_state);
            new_port->stp_state = MSTP_INST_PORT_STATE_INVALID;;
            new_port->cfg.cist_port_cfg = cist_port_cfg;
            return;
        }

        new_port->stp_state = port_state;
        new_port->cfg.cist_port_cfg = cist_port_cfg;

        mstp_cist_and_instance_set_port_state(br, msti, new_port);
}

/*-----------------------------------------------------------------------------
| Function:  mstp_cist_add_del_ports
| Description: add/del ports in cist
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[in]: mstp_instance object
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
mstp_cist_add_del_ports(const struct blk_params *br,
                             struct mstp_instance *msti)
{
    size_t i;
    struct mstp_instance_port *inst_port, *next;
    struct shash sh_idl_ports;
    struct shash_node *sh_node;

    VLOG_DBG("%s: called", __FUNCTION__);

    if (!msti || !br) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return;
    }

    /* Collect all Instance Ports present in the DB. */
    shash_init(&sh_idl_ports);
    for (i = 0; i < msti->cfg.cist_cfg->n_mstp_common_instance_ports; i++) {
        const char *name =
                    msti->cfg.cist_cfg->mstp_common_instance_ports[i]->port->name;
        if (!shash_add_once(&sh_idl_ports, name,
                          msti->cfg.cist_cfg->mstp_common_instance_ports[i])) {
            VLOG_WARN("instance id %d: %s specified twice as CIST Port",
                      msti->instance_id, name);
        }
    }

    /* Delete old Instance Ports. */
    HMAP_FOR_EACH_SAFE (inst_port, next, hmap_node, &msti->ports) {
        const struct ovsrec_mstp_common_instance_port *port_cfg;

        port_cfg = shash_find_data(&sh_idl_ports, inst_port->name);
        if (!port_cfg) {
            VLOG_DBG("Found a deleted Port %s in CIST", inst_port->name);
            mstp_cist_and_instance_port_delete(br, msti, inst_port);
        }
    }

    /* Add new VLANs. */
    SHASH_FOR_EACH (sh_node, &sh_idl_ports) {
        inst_port = mstp_cist_and_instance_port_lookup(msti, sh_node->name);
        if (!inst_port) {
            VLOG_DBG("Found an added Port %s in CIST", sh_node->name);
            mstp_cist_port_add(br, msti, sh_node->data);
        }
    }

    /* Destroy the shash of the IDL vlans */
    shash_destroy(&sh_idl_ports);

}

/*-----------------------------------------------------------------------------
| Function:  mstp_cist_add_del_vlans
| Description: add/del vlans in cist
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[in]: mstp_instance object
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
mstp_cist_add_del_vlans(const struct blk_params *br,
                             struct mstp_instance *msti)
{
    size_t i;
    struct mstp_instance_vlan *vlan, *next;
    struct shash sh_idl_vlans;
    struct shash_node *sh_node;

    VLOG_DBG("%s: called", __FUNCTION__);

    if (!msti || !br) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return;
    }

    /* Collect all Instance VLANs present in the DB. */
    shash_init(&sh_idl_vlans);
    for (i = 0; i < msti->cfg.cist_cfg->n_vlans; i++) {
        const char *name = msti->cfg.cist_cfg->vlans[i]->name;
        if (!shash_add_once(&sh_idl_vlans, name,
                            msti->cfg.cist_cfg->vlans[i])) {
            VLOG_WARN("mstp instance id %d: %s specified twice as cist VLAN",
                      msti->instance_id, name);
        }
    }

    /* Delete old Instance VLANs. */
    HMAP_FOR_EACH_SAFE (vlan, next, hmap_node, &msti->vlans) {
        const struct ovsrec_vlan *vlan_cfg;

        vlan_cfg = shash_find_data(&sh_idl_vlans, vlan->name);
        if (!vlan_cfg) {
            VLOG_DBG("Found a deleted VLAN in CIST%s", vlan->name);
            /* Need to update ofproto now since this VLAN
                       * won't be around for the "check for changes"
                       * loop below. */
            mstp_cist_and_instance_vlan_delete(br, msti, vlan);
        }
    }

    /* Add new VLANs. */
    SHASH_FOR_EACH (sh_node, &sh_idl_vlans) {
        vlan = mstp_cist_and_instance_vlan_lookup(msti, sh_node->name);
        if (!vlan) {
            VLOG_DBG("Found an added VLAN in CIST%s", sh_node->name);
            mstp_cist_and_instance_vlan_add(br, msti, sh_node->data);
        }
    }

    /* Destroy the shash of the IDL vlans */
    shash_destroy(&sh_idl_vlans);
}


/*-----------------------------------------------------------------------------
| Function:  mstp_cist_create
| Description: create cist instance
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[in]:  ovsrec_mstp_common_instance object
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
mstp_cist_create(const struct blk_params *br,
                    const struct ovsrec_mstp_common_instance *msti_cist_cfg)
{
    struct mstp_instance *msti;

    VLOG_DBG("%s: called", __FUNCTION__);
    if (!msti_cist_cfg || !br) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return;
    }

    msti = xzalloc(sizeof *msti);
    msti->instance_id = MSTP_CIST;
    msti->cfg.cist_cfg= msti_cist_cfg;
    hmap_init(&msti->vlans);
    hmap_init(&msti->ports);
    hmap_insert(&all_mstp_instances, &msti->node, hash_int(MSTP_CIST, 0));

    msti->hw_stg_id = MSTP_DEFAULT_STG_GROUP;

    mstp_cist_add_del_vlans(br, msti);
    mstp_cist_add_del_ports(br, msti);
}

/*-----------------------------------------------------------------------------
| Function:  mstp_cist_add_update
| Description: check vlans, ports add/deleted in cist
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
mstp_cist_add_update(const struct blk_params *br)
{
    struct mstp_instance *msti;
    const struct ovsrec_mstp_common_instance *msti_cist_cfg;

    VLOG_DBG("%s: called", __FUNCTION__);

    if (!br) {
        VLOG_DBG("%s: invalid bridge param", __FUNCTION__);
        return;
    }

    if (!br->cfg) {
        VLOG_DBG("%s: invalid bridge config param", __FUNCTION__);
        return;
    }

    msti_cist_cfg = br->cfg->mstp_common_instance;
    if (!msti_cist_cfg) {
        VLOG_DBG("%s: invalid mstp common instance config  param",
                 __FUNCTION__);
        return;
    }

    msti = mstp_cist_and_instance_lookup(MSTP_CIST);
    if (!msti) {
        const struct ovsrec_mstp_common_instance *msti_cist_cfg =
                                                br->cfg->mstp_common_instance;
        VLOG_DBG("Creating CIST");
        mstp_cist_create(br, msti_cist_cfg);
        return;
    }
    else {
        /* update  CIST vlans and ports */
        /* check if any vlans added or deleted */
        mstp_cist_add_del_vlans(br, msti);

        /* check if any l2 ports added or deleted */
        mstp_cist_add_del_ports(br, msti);
    }

}

/*-----------------------------------------------------------------------------
| Function:  mstp_cist_update_port_state
| Description:  update port state in cist
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[in]: mstp_instance object
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
mstp_cist_update_port_state(struct blk_params *br_blk_params,
                                 struct mstp_instance *msti)
{
     struct mstp_instance_port *inst_port;
     int new_port_state;
     bool retval;

    if (!msti || !br_blk_params) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return;
    }

    /* Check for changes in the VLAN row entries. */
    HMAP_FOR_EACH (inst_port, hmap_node, &msti->ports) {
        const struct ovsrec_mstp_common_instance_port *inst_port_row =
                                                 inst_port->cfg.cist_port_cfg;

        /* Check for changes to row. */
        if (OVSREC_IDL_IS_ROW_INSERTED(inst_port_row,
            ovsdb_idl_get_seqno(br_blk_params->idl)) ||
            OVSREC_IDL_IS_ROW_MODIFIED(inst_port_row,
            ovsdb_idl_get_seqno(br_blk_params->idl))) {

            // Check for port state changes.
            retval =  get_port_state_from_string(inst_port_row->port_state,
                                                 &new_port_state);
            if (false == retval) {
                VLOG_DBG("mstp_cist_update_port_state - invalid port state");
                return;
            }

            if(new_port_state != inst_port->stp_state) {
                VLOG_DBG("Set CIST port state to %s",
                         inst_port_row->port_state);
                inst_port->stp_state = new_port_state;
                mstp_cist_and_instance_set_port_state(br_blk_params,
                                                      msti, inst_port);
            }
            else {
                VLOG_DBG("No change in CIST port %s state" ,inst_port->name);
            }
        }
    }
}

/*-----------------------------------------------------------------------------
| Function:  mstp_instance_update_port_state
| Description: updates port state in msti
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[in]: mstp_instance object
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
mstp_instance_update_port_state(struct blk_params *br_blk_params,
                                      struct mstp_instance *msti)
{
     struct mstp_instance_port *inst_port;
     int new_port_state;
     bool retval;

    if (!msti || !br_blk_params) {
        VLOG_DBG("%s: invalid param", __FUNCTION__);
        return;
    }

    /* Check for changes in the VLAN row entries. */
    HMAP_FOR_EACH (inst_port, hmap_node, &msti->ports) {
        const struct ovsrec_mstp_instance_port *inst_port_row =
                                                inst_port->cfg.msti_port_cfg;

        /* Check for changes to row. */
        if (OVSREC_IDL_IS_ROW_INSERTED(inst_port_row,
            ovsdb_idl_get_seqno(br_blk_params->idl)) ||
            OVSREC_IDL_IS_ROW_MODIFIED(inst_port_row,
            ovsdb_idl_get_seqno(br_blk_params->idl))) {

            // Check for port state changes.
            retval =  get_port_state_from_string(inst_port_row->port_state,
                                                 &new_port_state);
            if (false == retval) {
                VLOG_DBG("mstp_instance_update_port_state-invalid port state");
                continue;
            }

            if(new_port_state != inst_port->stp_state) {
                VLOG_DBG("Set mstp instance %d port %s state to %s",
                         msti->instance_id, inst_port->name,
                         inst_port_row->port_state);
                inst_port->stp_state = new_port_state;
                mstp_cist_and_instance_set_port_state(br_blk_params,
                                                      msti, inst_port);
            }
            else {
                VLOG_DBG("No chnage in mstp instance %d port %s state" ,
                         msti->instance_id, inst_port->name);
            }
        }
    }

}

/*-----------------------------------------------------------------------------
| Function:  stp_reconfigure
| Description: checks for vlans,ports added/deleted in msti/cist
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
stp_reconfigure(struct blk_params* br_blk_param)
{

    VLOG_DBG("stp_reconfigure called");
    if(!br_blk_param || !br_blk_param->idl || !br_blk_param->ofproto
        || !br_blk_param->ports || !br_blk_param->cfg) {
        VLOG_DBG("invalid blk param object");
        return;
    }

    mstp_cist_add_update(br_blk_param);
    mstp_add_del_instances(br_blk_param);
}

/*-----------------------------------------------------------------------------
| Function:   stp_update_port_states
| Description:  update port states in cist/msti
| Parameters[in]: blk params :-object contains idl, ofproro, bridge cfg, bridge ports hmap
| Parameters[out]: None
| Return: None
-----------------------------------------------------------------------------*/
void
stp_update_port_states(struct blk_params* br_blk_param)
{
       struct mstp_instance *msti, *next_msti;

    VLOG_DBG("stp_port_update_states called");
    if(!br_blk_param || !br_blk_param->idl || !br_blk_param->ofproto
        || !br_blk_param->ports || !br_blk_param->cfg) {
        VLOG_DBG("invalid blk param object");
        return;
    }

    /* Get rid of deleted instid's */
    HMAP_FOR_EACH_SAFE (msti, next_msti, node, &all_mstp_instances) {
        if (msti->instance_id != MSTP_CIST) {
            mstp_cist_update_port_state(br_blk_param, msti);
        }
        else {
            mstp_instance_update_port_state(br_blk_param, msti);
        }
    }
}
