#!/usr/bin/python

# (c) Copyright 2016 Hewlett Packard Enterprise Development LP
# All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

from time import sleep
from pytest import mark
#
# The purpose of this test is to test
# functionality of STP in docker environment

# Topology definition
TOPOLOGY = """
#                                        +---------+
#                         +-------------->   hs1   |
#                         |              |         |
#                         |              +---------+
#                         |
#                    +----+------+
#      +------------->   ops1    <------------+
#      |             +-----------+            |
#      |                                      |
#+-----v-----+                           +----v-----+
#|   ops2    <--------------------------->   ops3   |
#+-----+-----+                           +----+-----+
#      |                                      |
#+-----v-----+                           +----v-----+
#|    hs2    |                           |   hs3    |
#|           |                           |          |
#+-----------+                           +----------+
#
# Nodes
[type=openswitch name="OpenSwitch 1"] ops1
[type=openswitch name="OpenSwitch 2"] ops2
[type=openswitch name="OpenSwitch 3"] ops3
[type=oobmhost image="ubuntu_stp:latest" name="Host 1"] hs1
[type=oobmhost image="ubuntu_stp:latest" name="Host 2"] hs2
[type=oobmhost image="ubuntu_stp:latest" name="Host 3"] hs3

# Ports
[force_name=oobm] ops1:sp1
[force_name=oobm] ops2:sp2
[force_name=oobm] ops3:sp3

#Links
ops1:sp1 -- hs1:if01
ops2:sp2 -- hs2:if01
ops3:sp3 -- hs3:if01
ops1:p2  -- ops2:p1
ops2:p2  -- ops3:p1
ops3:p2  -- ops1:p1
"""


# Get the ip of the switch
def getswitchip(switch, step):
    """ This function is to get switch IP addess
    """
    out = switch("ifconfig eth0", shell="bash")
    switchipaddress = out.split("\n")[1].split()[1][5:]
    return switchipaddress


# Get the ip of the host
def gethostip(host, step):
    """ This function is to get host IP addess
    """
    out = host("ifconfig %s" % host.ports["if01"])
    host_ipaddress = out.split("\n")[1].split()[1][5:]
    return host_ipaddress


# Configrue the Interfaces on the Switches and Enable STP
def configure_interfaces(sw1, sw2, sw3, step):
    step('configuring switches')

    # Enabling interfaces on sw1
    step('Enabling interface 1 on sw1')
    with sw1.libs.vtysh.ConfigInterface('1') as ctx:
        ctx.no_shutdown()
        ctx.no_routing()

    step('Enabling interface 2 on sw1')
    with sw1.libs.vtysh.ConfigInterface('2') as ctx:
        ctx.no_shutdown()
        ctx.no_routing()

    with sw1.libs.vtysh.ConfigInterfaceVlan('1') as ctx:
        ctx.no_shutdown()
        ctx.ip_address('192.168.10.1/24')

    # Enabling interfaces on sw2
    step('Enabling interface 1 on sw2')
    with sw2.libs.vtysh.ConfigInterface('1') as ctx:
        ctx.no_shutdown()
        ctx.no_routing()

    step('Enabling interface 2 on sw2')
    with sw2.libs.vtysh.ConfigInterface('2') as ctx:
        ctx.no_shutdown()
        ctx.no_routing()

    with sw2.libs.vtysh.ConfigInterfaceVlan('1') as ctx:
        ctx.no_shutdown()
        ctx.ip_address('192.168.10.2/24')

    # Enabling interfaces on sw3
    step('Enabling interface 1 on sw3')
    with sw3.libs.vtysh.ConfigInterface('1') as ctx:
        ctx.no_shutdown()
        ctx.no_routing()

    step('Enabling interface 2 on sw3')
    with sw3.libs.vtysh.ConfigInterface('2') as ctx:
        ctx.no_shutdown()
        ctx.no_routing()

    with sw3.libs.vtysh.ConfigInterfaceVlan('1') as ctx:
        ctx.no_shutdown()
        ctx.ip_address('192.168.10.3/24')

    sleep(30)

    with sw1.libs.vtysh.Configure() as ctx:
        ctx.spanning_tree()

    with sw2.libs.vtysh.Configure() as ctx:
        ctx.spanning_tree()

    with sw3.libs.vtysh.Configure() as ctx:
        ctx.spanning_tree()

    sleep(30)


# The Workstation container has bridgeStp.py script under /var/www/.
# Through wget we get the script to the switch and we execute it
# so that we get binary bridge.
def copy_bridge_stp_script(switch, host, step):
    switchip = getswitchip(switch, step)
    hostip = gethostip(host, step)
    ping = host.libs.ping.ping(1, switchip)
    assert (ping['transmitted'] == ping['received'] == 1),\
        "Ping between host and switch failed"

    stpurl = "wget http://" + hostip + "/bridgeStp.py"
    host("lighttpd -f /etc/lighttpd/lighttpd.conf")

    switch(stpurl, shell="bash")
    switch("python bridgeStp.py", shell="bash")


# Get number of ports in a specific STP state of a switch
def get_kernl_port_count_by_state(switch, state):
    buff = switch("ip netns exec swns bridge link show", shell='bash')
    lines = buff.split('\n')

    count = 0
    for line in lines:
        if state in line:
            count += 1

    return count


# Get the STP information through show spanning tree &
# through the bridge sim
def check_basic_stp(sw1, sw2, sw3, step):
    # Validate port STP states in PI
    block_count = 0
    fwd_count = 0
    block = 0
    forward = 0
    sleep(60)
    # Get port states on switch 1
    step('Get port states from switch 1')
    ops1_show = sw1.libs.vtysh.show_spanning_tree()
    if ops1_show['1']['State'] == 'Forwarding':
            forward = forward + 1
    elif ops1_show['1']['State'] == 'Blocking':
            block = block + 1

    if ops1_show['2']['State'] == 'Forwarding':
            forward = forward + 1
    elif ops1_show['2']['State'] == 'Blocking':
            block = block + 1

    kblock = get_kernl_port_count_by_state(sw1, 'blocking')
    kfwd = get_kernl_port_count_by_state(sw1, 'forwarding')

    block_count += kblock
    fwd_count += kfwd

    # Get port states on switch 2
    step('Get port states from switch 2')
    ops2_show = sw2.libs.vtysh.show_spanning_tree()
    if ops2_show['1']['State'] == 'Forwarding':
            forward = forward + 1
    elif ops2_show['1']['State'] == 'Blocking':
            block = block + 1

    if ops2_show['2']['State'] == 'Forwarding':
            forward = forward + 1
    elif ops2_show['2']['State'] == 'Blocking':
            block = block + 1

    kblock = get_kernl_port_count_by_state(sw2, 'blocking')
    kfwd = get_kernl_port_count_by_state(sw2, 'forwarding')

    block_count += kblock
    fwd_count += kfwd

    # Get port states on switch 3
    step('Get port states from switch 3')
    ops3_show = sw3.libs.vtysh.show_spanning_tree()
    if ops3_show['1']['State'] == 'Forwarding':
            forward = forward + 1
    elif ops3_show['1']['State'] == 'Blocking':
            block = block + 1

    if ops3_show['2']['State'] == 'Forwarding':
            forward = forward + 1
    elif ops3_show['2']['State'] == 'Blocking':
            block = block + 1

    kblock = get_kernl_port_count_by_state(sw3, 'blocking')
    kfwd = get_kernl_port_count_by_state(sw3, 'forwarding')

    block_count += kblock
    fwd_count += kfwd

    assert (block == 1 and forward == 5),\
        "Inconsistent port states in DB: Basic STP Test Failed"
    assert (block_count == 1 and fwd_count == 5),\
        "Inconsistent port states in kernel: Basic STP Test Failed"
    step('Testcase passed')


@mark.timeout(7200)
@mark.platform_incompatible(['ostl'])
def test_basic_cist_functionality(topology, step):
    ops1 = topology.get('ops1')
    ops2 = topology.get('ops2')
    ops3 = topology.get('ops3')
    hs1 = topology.get('hs1')
    hs2 = topology.get('hs2')
    hs3 = topology.get('hs3')

    assert ops1 is not None
    assert ops2 is not None
    assert ops3 is not None
    assert hs1 is not None
    assert hs2 is not None
    assert hs3 is not None

    configure_interfaces(ops1, ops2, ops3, step)
    copy_bridge_stp_script(ops1, hs1, step)
    copy_bridge_stp_script(ops2, hs2, step)
    copy_bridge_stp_script(ops3, hs3, step)
    check_basic_stp(ops1, ops2, ops3, step)
