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
OpenSwitch Test for vlan related configurations.
"""

from __future__ import unicode_literals, absolute_import
from __future__ import print_function, division

from .helpers import wait_until_interface_up
from pytest import mark

import re
import time

TOPOLOGY = """
#                    +-----------+
#      +------------->   ops1    <------------+
#      |             +-----------+            |
#      |                                      |
#+-----v-----+                           +----v-----+
#|   ops2    <--------------------------->   ops3   |
#+-----------+                           +----------+
#
# Nodes
[type=openswitch name="OpenSwitch 1"] ops1
[type=openswitch name="OpenSwitch 2"] ops2
[type=openswitch name="OpenSwitch 3"] ops3

# Links
ops1:1 -- ops2:1
ops1:2 -- ops3:1
ops2:2 -- ops3:2

"""

HELLO_TIME = 2
REGION_1 = "Region-One"
INT1 = '1'
INT2 = '2'


@mark.platform_incompatible(['docker'])
def test_cist_single_region_root_elect(topology):
    """
    Test that a cist in single region is functional with a OpenSwitch switch.

    Build a topology of three switch and connection made as shown in topology.
    Setup a spanning tree configuration on all the three switch so that all the
    switch are in same region. Now enable spanning tree and check loop is
    resolved and cist root selected.
    """
    ops1 = topology.get('ops1')
    ops2 = topology.get('ops2')
    ops3 = topology.get('ops3')

    assert ops1 is not None
    assert ops2 is not None
    assert ops3 is not None

    for ops in [ops1, ops2, ops3]:
        with ops.libs.vtysh.ConfigInterface(INT1) as ctx:
            ctx.no_routing()
            ctx.no_shutdown()

        with ops.libs.vtysh.ConfigInterface(INT2) as ctx:
            ctx.no_routing()
            ctx.no_shutdown()

        for switch, portlbl in [(ops, INT1), (ops, INT2)]:
            wait_until_interface_up(switch, portlbl)

        with ops.libs.vtysh.Configure() as ctx:
            ctx.spanning_tree_config_name(REGION_1)
            ctx.spanning_tree_config_revision(8)
            ctx.spanning_tree_hello_time(HELLO_TIME)
            ctx.spanning_tree()

    # Covergence should happen with HELLO_TIME * 2
    time.sleep(HELLO_TIME * 2)

    ops1_show = ops1.libs.vtysh.show_spanning_tree()
    result = ops1.send_command('ovs-vsctl list system | grep system_mac',
                               shell='bash')
    result = re.search('\s*system_mac\s*:\s*"(?P<sys_mac>.*)"', result)
    result = result.groupdict()
    ops1_mac = result['sys_mac']

    ops2_show = ops2.libs.vtysh.show_spanning_tree()
    result = ops2.send_command('ovs-vsctl list system | grep system_mac',
                               shell='bash')
    result = re.search('\s*system_mac\s*:\s*"(?P<sys_mac>.*)"', result)
    result = result.groupdict()
    ops2_mac = result['sys_mac']

    ops3_show = ops3.libs.vtysh.show_spanning_tree()
    result = ops3.send_command('ovs-vsctl list system | grep system_mac',
                               shell='bash')
    result = re.search('\s*system_mac\s*:\s*"(?P<sys_mac>.*)"', result)
    result = result.groupdict()
    ops3_mac = result['sys_mac']

    ops1_mac_int = int(ops1_mac.replace(':', ''), 16)
    ops2_mac_int = int(ops2_mac.replace(':', ''), 16)
    ops3_mac_int = int(ops3_mac.replace(':', ''), 16)

    root = ops3_mac
    if (ops1_mac_int < ops2_mac_int) and (ops1_mac_int < ops3_mac_int):
        root = ops1_mac
    elif (ops2_mac_int < ops3_mac_int):
        root = ops2_mac

    assert(root == ops1_show['root_mac_address']), \
        "Root bridge mac is updated incorrectly"

    assert(root == ops2_show['root_mac_address']), \
        "Root bridge mac is updated incorrectly"

    assert(root == ops3_show['root_mac_address']), \
        "Root bridge mac is updated incorrectly"

    forwarding = 0
    blocking = 0

    for ops_show in [ops1_show, ops2_show, ops3_show]:
        if ops_show['1']['State'] == 'Forwarding':
            forwarding = forwarding + 1
        elif ops_show['1']['State'] == 'Blocking':
            blocking = blocking + 1

        if ops_show['2']['State'] == 'Forwarding':
            forwarding = forwarding + 1
        elif ops_show['2']['State'] == 'Blocking':
            blocking = blocking + 1

    assert(forwarding == 5), \
        "Port state has not updated correctly"

    assert(blocking == 1), \
        "Port state has not updated correctly"

    for ops_show in [ops1_show, ops2_show, ops3_show]:
        if ops_show['root'] == 'yes':
            root_show = ops_show
            assert(ops_show['1']['role'] == 'Designated'), \
                "Port role has not updated correctly"

            assert(ops_show['2']['role'] == 'Designated'), \
                "Port role has not updated correctly"

            assert(ops_show['1']['State'] == 'Forwarding'), \
                "Port state has not updated correctly"

            assert(ops_show['2']['State'] == 'Forwarding'), \
                "Port state has not updated correctly"
        else:
            if (ops_show['1']['role'] == 'Designated'):
                assert(ops_show['1']['State'] == 'Forwarding'), \
                    "Port state has not updated correctly"
            elif (ops_show['1']['role'] == 'Root'):
                assert(ops_show['1']['State'] == 'Forwarding'), \
                    "Port state has not updated correctly"
            elif (ops_show['1']['role'] == 'Alternate'):
                assert(ops_show['1']['State'] == 'Blocking'), \
                    "Port state has not updated correctly"

    for ops_show in [ops1_show, ops2_show, ops3_show]:
        assert(root_show['bridge_max_age'] == ops_show['root_max_age']), \
            "Root bridge max age is updated incorrectly"

        assert(root_show['bridge_forward_delay'] ==
               ops_show['root_forward_delay']), \
            "Root bridge forward delay is updated incorrectly"

        assert(root_show['bridge_priority'] == ops_show['root_priority']), \
            "Root bridge priority is updated incorrectly"

        assert(root_show['bridge_mac_address'] ==
               ops_show['root_mac_address']), \
            "Root bridge mac is updated incorrectly"

    # TODO: Comments to be removed once dynamic config is fixed
    '''
    priority = 8
    root_sw = None
    for ops in [ops1, ops2, ops3]:
        ops_show = ops.libs.vtysh.show_spanning_tree()
        if root == ops_show['bridge_mac_address']:
            with ops.libs.vtysh.Configure() as ctx:
                ctx.spanning_tree_priority(15)
        else:
            with ops.libs.vtysh.Configure() as ctx:
                ctx.spanning_tree_priority(priority)
            if not root_sw:
                root_sw = ops
            priority = priority + 1

    # Covergence should happen with HELLO_TIME * 2
    time.sleep(HELLO_TIME * 30)

    result = root_sw.libs.vtysh.show_interface(INT1)
    root = result['mac_address']

    ops1_show = ops1.libs.vtysh.show_spanning_tree()

    ops2_show = ops2.libs.vtysh.show_spanning_tree()

    ops3_show = ops3.libs.vtysh.show_spanning_tree()

    assert(root == ops1_show['root_mac_address']), \
        "Root bridge mac is updated incorrectly"

    assert(root == ops2_show['root_mac_address']), \
        "Root bridge mac is updated incorrectly"

    assert(root == ops3_show['root_mac_address']), \
        "Root bridge mac is updated incorrectly"

    '''

    for ops in [ops1, ops2, ops3]:
        with ops.libs.vtysh.Configure() as ctx:
            ctx.no_spanning_tree_config_name()
            ctx.no_spanning_tree_config_revision()
            ctx.no_spanning_tree_hello_time()
            ctx.no_spanning_tree()
