# -*- coding: utf-8 -*-
#
# Copyright (C) 2015-2016 Hewlett Packard Enterprise Development LP
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

"""
OpenSwitch Test for interface lag commands.
"""

from __future__ import unicode_literals, absolute_import
from __future__ import print_function, division

import time

TOPOLOGY = """
#
# +-------+     +-------+
# |       |     |       |
# |       +-----+       |
# | Sw1   +-----+   Sw2 |
# |       |     |       |
# +-------+     +-------+
#
# Nodes
[type=openswitch name="OpenSwitch 1"] sw1
[type=openswitch name="OpenSwitch 2"] sw2

# Links
sw1:1 -- sw2:1
sw1:2 -- sw2:2
"""


def validate_turn_on_interfaces(sw, interfaces):
    for intf in interfaces:
        output = sw.libs.vtysh.show_interface(intf)
        assert output['interface_state'] == 'up',\
            "Interface state for " + intf + " is down"


def lag_no_shutdown(sw, lag_id):
    with sw.libs.vtysh.ConfigInterfaceLag(lag_id) as ctx:
        ctx.no_shutdown()


def create_lag_active(sw, lag_id):
    with sw.libs.vtysh.ConfigInterfaceLag(lag_id) as ctx:
        ctx.lacp_mode_active()


def create_lag_passive(sw, lag_id):
    with sw.libs.vtysh.ConfigInterfaceLag(lag_id) as ctx:
        ctx.lacp_mode_passive()


def lag_no_routing(sw, lag_id):
    with sw.libs.vtysh.ConfigInterfaceLag(lag_id) as ctx:
        ctx.no_routing()


def create_lag(sw, lag_id, lag_mode):
    with sw.libs.vtysh.ConfigInterfaceLag(lag_id) as ctx:
        if(lag_mode == 'active'):
            ctx.lacp_mode_active()
        elif(lag_mode == 'passive'):
            ctx.lacp_mode_passive()
        elif(lag_mode == 'off'):
            pass
        else:
            assert False, 'Invalid mode %s for LAG' % (lag_mode)
    lag_name = "lag" + lag_id
    output = sw.libs.vtysh.show_lacp_aggregates(lag_name)
    assert lag_mode == output[lag_name]['mode'],\
        "Unable to create and validate LAG"


def associate_interface_to_lag(sw, interface, lag_id):
    with sw.libs.vtysh.ConfigInterface(interface) as ctx:
        ctx.lag(lag_id)
    lag_name = "lag" + lag_id
    output = sw.libs.vtysh.show_lacp_aggregates(lag_name)
    assert interface in output[lag_name]['interfaces'],\
        "Unable to associate interface to lag"


def turn_on_interface(sw, interface):
    with sw.libs.vtysh.ConfigInterface(interface) as ctx:
        ctx.no_shutdown()


def turn_off_interface(sw, interface):
    with sw.libs.vtysh.ConfigInterface(interface) as ctx:
        ctx.shutdown()


def wait_until_interface_up(switch, portlbl, timeout=30, polling_frequency=1):
    """
    Wait until the interface, as mapped by the given portlbl, is marked as up.

    :param switch: The switch node.
    :param str portlbl: Port label that is mapped to the interfaces.
    :param int timeout: Number of seconds to wait.
    :param int polling_frequency: Frequency of the polling.
    :return: None if interface is brought-up. If not, an assertion is raised.
    """
    for i in range(timeout):
        status = switch.libs.vtysh.show_interface(portlbl)
        if status['interface_state'] == 'up':
            break
        time.sleep(polling_frequency)
    else:
        assert False, (
            'Interface {}:{} never brought-up after '
            'waiting for {} seconds'.format(
                switch.identifier, portlbl, timeout
            )
        )


def config_mstp_bpdu(sw, intf, value):
    with sw.libs.vtysh.ConfigInterface(intf) as ctx:
        ctx.spanning_tree_bpdu_guard(value)
        ctx.spanning_tree_loop_guard(value)
        ctx.spanning_tree_root_guard(value)
        ctx.spanning_tree_bpdu_filter(value)


def reset_mstp_bpdu(sw, intf):
    with sw.libs.vtysh.ConfigInterface(intf) as ctx:
        ctx.no_spanning_tree_bpdu_guard()
        ctx.no_spanning_tree_loop_guard()
        ctx.no_spanning_tree_root_guard()
        ctx.no_spanning_tree_bpdu_filter()


def config_mstp_intf(sw, intf, cost, priority, port_type):
    with sw.libs.vtysh.ConfigInterface(intf) as ctx:
        ctx.spanning_tree_port_type(port_type)
    sw('conf t', shell='vtysh')
    sw('interface '+intf, shell='vtysh')
    sw('spanning-tree cost '+cost, shell='vtysh')
    sw('spanning-tree port-priority '+priority, shell='vtysh')
    sw('end', shell='vtysh')


def reset_mstp_intf(sw, intf):
    with sw.libs.vtysh.ConfigInterface(intf) as ctx:
        ctx.no_spanning_tree_port_type()
    sw('conf t', shell='vtysh')
    sw('interface '+intf, shell='vtysh')
    sw('no spanning-tree cost', shell='vtysh')
    sw('no spanning-tree port-priority', shell='vtysh')
    sw('end', shell='vtysh')


def validate_mstp_show_run(sw, cmd, required):
    out = sw('show running-config spanning-tree', shell='vtysh')
    found = False
    lines = out.split('\n')
    for line in lines:
        if cmd in line:
            found = True
            break
    if(required):
        assert(found is True), \
            cmd + " cmd not found in show running-config spanning-tree"
    else:
        assert(found is False), \
            cmd + " cmd found in show running-config spanning-tree"
    out = sw('show running-config', shell='vtysh')
    found = False
    lines = out.split('\n')
    for line in lines:
        if cmd in line:
            found = True
            break
    if(required):
        assert(found is True), \
            cmd + " cmd not found in show running-config"
    else:
        assert(found is False), \
            cmd + " cmd found in show running-config"


def test_mstp_with_lag(topology):
    sw1 = topology.get('sw1')
    sw2 = topology.get('sw2')
    sw1_lag_id = '10'
    sw2_lag_id = '10'

    assert sw1 is not None
    assert sw2 is not None

    p11 = sw1.ports['1']
    p12 = sw1.ports['2']
    p21 = sw2.ports['1']
    p22 = sw2.ports['2']

    print("Turning on all interfaces used in this test")
    ports_sw1 = [p11, p12]
    for port in ports_sw1:
        turn_on_interface(sw1, port)

    ports_sw2 = [p21, p22]
    for port in ports_sw2:
        turn_on_interface(sw2, port)

    print("Verify all interface are up")
    validate_turn_on_interfaces(sw1, ports_sw1)
    validate_turn_on_interfaces(sw2, ports_sw2)

    print("craete l2 lag in both switches")
    lag_no_routing(sw1, sw1_lag_id)
    lag_no_routing(sw2, sw1_lag_id)

    print("Create LAG in both switches")
    create_lag(sw1, sw1_lag_id, 'active')
    create_lag(sw2, sw2_lag_id, 'active')

    print("Associate interfaces [1, 2] to LAG in both switches")
    associate_interface_to_lag(sw1, p11, sw1_lag_id)
    associate_interface_to_lag(sw1, p12, sw1_lag_id)
    associate_interface_to_lag(sw2, p21, sw2_lag_id)
    associate_interface_to_lag(sw2, p22, sw2_lag_id)

    print("Enable lag in both siwthces")
    lag_no_shutdown(sw1, sw1_lag_id)
    lag_no_shutdown(sw2, sw2_lag_id)

    """
    Case 1:
        Check setting BPDU guard and filter commands.
    """

    print("Setting bpdu guard and filter via CLI")
    for sw in [sw1, sw2]:
        config_mstp_bpdu(sw, "lag 10", "enable")

    for sw in [sw1, sw2]:
        validate_mstp_show_run(sw, 'spanning-tree bpdu-guard enable', True)
        validate_mstp_show_run(sw, 'spanning-tree root-guard enable', True)
        validate_mstp_show_run(sw, 'spanning-tree bpdu-filter enable', True)
        validate_mstp_show_run(sw, 'spanning-tree loop-guard enable', True)

    """
    Case 2: Check resetting BPDU guard and filter commands by no commands.
    """
    print("Resetting bpdu guard and filter via no commands in CLI")
    for sw in [sw1, sw2]:
        reset_mstp_bpdu(sw, "lag 10")

    for sw in [sw1, sw2]:
        validate_mstp_show_run(sw, 'spanning-tree bpdu-guard', False)
        validate_mstp_show_run(sw, 'spanning-tree root-guard', False)
        validate_mstp_show_run(sw, 'spanning-tree bpdu-filter', False)
        validate_mstp_show_run(sw, 'spanning-tree loop-guard', False)

    """
    Case 3: Check setting of interface cost, priority, type via CLI for CIST.
    """
    print("Setting interface cost, priority, type via CLI for CIST")
    for sw in [sw1, sw2]:
        config_mstp_intf(sw, "lag 10", "2000", "12", "admin-edge")

    for sw in [sw1, sw2]:
        validate_mstp_show_run(sw, 'spanning-tree cost 2000', True)
        validate_mstp_show_run(sw, 'spanning-tree port-priority 12', True)
        validate_mstp_show_run(sw, 'spanning-tree port-type admin-edge', True)

    """
    Case 4: Check resetting interface cost, priority, type via CLI for CIST.
    """
    print("Resetting interface cost, priority, type via CLI for CIST")
    for sw in [sw1, sw2]:
        reset_mstp_intf(sw, "lag 10")

    for sw in [sw1, sw2]:
        validate_mstp_show_run(sw, 'spanning-tree cost 2000', False)
        validate_mstp_show_run(sw, 'spanning-tree port-priority 12', False)
        validate_mstp_show_run(sw, 'spanning-tree port-type admin-edge', False)

    """
    Case 5: Check setting of interface cost, priority, type via CLI for MSTI.
    """
    print("Creating new MSTI via CLI")
    for sw in [sw1, sw2]:
        for vid in ['10', '20']:
            with sw.libs.vtysh.ConfigVlan(vid) as ctx:
                ctx.no_shutdown()
            with sw.libs.vtysh.Configure() as ctx:
                ctx.spanning_tree_instance_vlan('1', vid)

    print("Setting interface cost, priority, type via CLI for MSTI")
    for sw in [sw1, sw2]:
        with sw.libs.vtysh.ConfigInterface("lag 10") as ctx:
            ctx.spanning_tree_instance_cost('1', '2000')
            ctx.spanning_tree_instance_port_priority('1', '12')

    for sw in [sw1, sw2]:
        validate_mstp_show_run(sw, 'spanning-tree instance 1 cost 2000', True)
        validate_mstp_show_run(sw, 'spanning-tree instance 1 port-priority 12',
                               True)

    """
    Case 6: Check resetting interface cost, priority, type via CLI for MSTI.
    """
    print("Resetting interface cost, priority, type via CLI for MSTI")
    for sw in [sw1, sw2]:
        with sw.libs.vtysh.ConfigInterface("lag 10") as ctx:
            ctx.no_spanning_tree_instance_cost('1')
            ctx.no_spanning_tree_instance_port_priority('1')

    for sw in [sw1, sw2]:
        validate_mstp_show_run(sw, 'spanning-tree instance 1 cost 2000', False)
        validate_mstp_show_run(sw, 'spanning-tree instance 1 port-priority 12',
                               False)

    print("mstp lag test passed")
