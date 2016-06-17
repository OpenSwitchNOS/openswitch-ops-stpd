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

import pytest
import re
from opstestfw import *
from opstestfw.switch.CLI import *
from opstestfw.switch import *
from time import sleep
import base64, cStringIO
import os

#
# The purpose of this test is to test
# functionality of DHCP-Relay

# Topology definition
topoDict = {"topoExecution": 1000,
            "topoTarget": "dut01 dut02 dut03",
            "topoDevices": "dut01 dut02 dut03",
            "topoLinks": "lnk01:dut01:dut02,lnk02:dut02:dut03,\
                          lnk03:dut03:dut01",
            "topoFilters": "dut01:system-category:switch,\
                            dut02:system-category:switch,\
                            dut03:system-category:switch"}

def add_delete_bridge_stp_executable(**kwargs):
    op = kwargs.get('op', None)
    file_path = '/sbin/bridge-stp'

    # Cleanup the executable bridge-stp
    if 'delete' in op:
        os.system("rm -f /sbin/bridge-stp")
        return

    # Binary of the executable generated through the following C code
    #
    #  #include <stdio.h>
    #  #include <stdlib.h>
    #  #include <string.h>

    #  #define BRIDGE "bridge-sim"
    #
    #  int main(int argc, char* argv[])
    #  {
    #      if (argc < 2)
    #          exit(1);
    #
    #      if (strncmp(argv[1], BRIDGE, strlen(BRIDGE)) == 0)
    #         exit(0);
    #
    #      exit(1);
    #  }
    #

    f = cStringIO.StringIO(base64.decodestring("""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
Xf7//1QAAAAAQQ4QhgJDDQYAAAAAAAAARAAAAGQAAACg/v//ZQAAAABCDhCPAkUOGI4DRQ4gjQRF
DiiMBUgOMIYGSA44gwdNDkBsDjhBDjBBDihCDiBCDhhCDhBCDggAFAAAAKwAAADI/v//AgAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAUAVAAAAAAAAwBUAAAAAAAAAAAAAAAAAAAQAAAAAAAAABAAAAAAAAAAwAAAAAAAAA
GARAAAAAAAANAAAAAAAAAFQGQAAAAAAAGQAAAAAAAAAQDmAAAAAAABsAAAAAAAAACAAAAAAAAAAa
AAAAAAAAABgOYAAAAAAAHAAAAAAAAAAIAAAAAAAAAPX+/28AAAAAmAJAAAAAAAAFAAAAAAAAADAD
QAAAAAAABgAAAAAAAAC4AkAAAAAAAAoAAAAAAAAARQAAAAAAAAALAAAAAAAAABgAAAAAAAAAFQAA
AAAAAAAAAAAAAAAAAAMAAAAAAAAAABBgAAAAAAACAAAAAAAAAGAAAAAAAAAAFAAAAAAAAAAHAAAA
AAAAABcAAAAAAAAAuANAAAAAAAAHAAAAAAAAAKADQAAAAAAACAAAAAAAAAAYAAAAAAAAAAkAAAAA
AAAAGAAAAAAAAAD+//9vAAAAAIADQAAAAAAA////bwAAAAABAAAAAAAAAPD//28AAAAAdgNAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACgOYAAAAAAA
AAAAAAAAAAAAAAAAAAAAAFYEQAAAAAAAZgRAAAAAAAB2BEAAAAAAAIYEQAAAAAAAAAAAAAAAAAAA
AAAAAAAAAEdDQzogKFVidW50dSA0LjguNC0ydWJ1bnR1MX4xNC4wNC4zKSA0LjguNAAALnN5bXRh
YgAuc3RydGFiAC5zaHN0cnRhYgAuaW50ZXJwAC5ub3RlLkFCSS10YWcALm5vdGUuZ251LmJ1aWxk
LWlkAC5nbnUuaGFzaAAuZHluc3ltAC5keW5zdHIALmdudS52ZXJzaW9uAC5nbnUudmVyc2lvbl9y
AC5yZWxhLmR5bgAucmVsYS5wbHQALmluaXQALnRleHQALmZpbmkALnJvZGF0YQAuZWhfZnJhbWVf
aGRyAC5laF9mcmFtZQAuaW5pdF9hcnJheQAuZmluaV9hcnJheQAuamNyAC5keW5hbWljAC5nb3QA
LmdvdC5wbHQALmRhdGEALmJzcwAuY29tbWVudAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAbAAAAAQAAAAIAAAAAAAAA
OAJAAAAAAAA4AgAAAAAAABwAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAIwAAAAcAAAAC
AAAAAAAAAFQCQAAAAAAAVAIAAAAAAAAgAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAADEA
AAAHAAAAAgAAAAAAAAB0AkAAAAAAAHQCAAAAAAAAJAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAA
AAAAAABEAAAA9v//bwIAAAAAAAAAmAJAAAAAAACYAgAAAAAAABwAAAAAAAAABQAAAAAAAAAIAAAA
AAAAAAAAAAAAAAAATgAAAAsAAAACAAAAAAAAALgCQAAAAAAAuAIAAAAAAAB4AAAAAAAAAAYAAAAB
AAAACAAAAAAAAAAYAAAAAAAAAFYAAAADAAAAAgAAAAAAAAAwA0AAAAAAADADAAAAAAAARQAAAAAA
AAAAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAABeAAAA////bwIAAAAAAAAAdgNAAAAAAAB2AwAAAAAA
AAoAAAAAAAAABQAAAAAAAAACAAAAAAAAAAIAAAAAAAAAawAAAP7//28CAAAAAAAAAIADQAAAAAAA
gAMAAAAAAAAgAAAAAAAAAAYAAAABAAAACAAAAAAAAAAAAAAAAAAAAHoAAAAEAAAAAgAAAAAAAACg
A0AAAAAAAKADAAAAAAAAGAAAAAAAAAAFAAAAAAAAAAgAAAAAAAAAGAAAAAAAAACEAAAABAAAAAIA
AAAAAAAAuANAAAAAAAC4AwAAAAAAAGAAAAAAAAAABQAAAAwAAAAIAAAAAAAAABgAAAAAAAAAjgAA
AAEAAAAGAAAAAAAAABgEQAAAAAAAGAQAAAAAAAAaAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAA
AAAAAIkAAAABAAAABgAAAAAAAABABEAAAAAAAEAEAAAAAAAAUAAAAAAAAAAAAAAAAAAAABAAAAAA
AAAAEAAAAAAAAACUAAAAAQAAAAYAAAAAAAAAkARAAAAAAACQBAAAAAAAAMIBAAAAAAAAAAAAAAAA
AAAQAAAAAAAAAAAAAAAAAAAAmgAAAAEAAAAGAAAAAAAAAFQGQAAAAAAAVAYAAAAAAAAJAAAAAAAA
AAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAKAAAAABAAAAAgAAAAAAAABgBkAAAAAAAGAGAAAAAAAA
DwAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAACoAAAAAQAAAAIAAAAAAAAAcAZAAAAAAABw
BgAAAAAAADQAAAAAAAAAAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAtgAAAAEAAAACAAAAAAAAAKgG
QAAAAAAAqAYAAAAAAAD0AAAAAAAAAAAAAAAAAAAACAAAAAAAAAAAAAAAAAAAAMAAAAAOAAAAAwAA
AAAAAAAQDmAAAAAAABAOAAAAAAAACAAAAAAAAAAAAAAAAAAAAAgAAAAAAAAAAAAAAAAAAADMAAAA
DwAAAAMAAAAAAAAAGA5gAAAAAAAYDgAAAAAAAAgAAAAAAAAAAAAAAAAAAAAIAAAAAAAAAAAAAAAA
AAAA2AAAAAEAAAADAAAAAAAAACAOYAAAAAAAIA4AAAAAAAAIAAAAAAAAAAAAAAAAAAAACAAAAAAA
AAAAAAAAAAAAAN0AAAAGAAAAAwAAAAAAAAAoDmAAAAAAACgOAAAAAAAA0AEAAAAAAAAGAAAAAAAA
AAgAAAAAAAAAEAAAAAAAAADmAAAAAQAAAAMAAAAAAAAA+A9gAAAAAAD4DwAAAAAAAAgAAAAAAAAA
AAAAAAAAAAAIAAAAAAAAAAgAAAAAAAAA6wAAAAEAAAADAAAAAAAAAAAQYAAAAAAAABAAAAAAAAA4
AAAAAAAAAAAAAAAAAAAACAAAAAAAAAAIAAAAAAAAAPQAAAABAAAAAwAAAAAAAAA4EGAAAAAAADgQ
AAAAAAAAEAAAAAAAAAAAAAAAAAAAAAgAAAAAAAAAAAAAAAAAAAD6AAAACAAAAAMAAAAAAAAASBBg
AAAAAABIEAAAAAAAAAgAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAA/wAAAAEAAAAwAAAA
AAAAAAAAAAAAAAAASBAAAAAAAAArAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAABAAAAAAAAABEAAAAD
AAAAAAAAAAAAAAAAAAAAAAAAAHMQAAAAAAAACAEAAAAAAAAAAAAAAAAAAAEAAAAAAAAAAAAAAAAA
AAABAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAAGQAAAAAAADAGAAAAAAAAHQAAAC0AAAAIAAAAAAAA
ABgAAAAAAAAACQAAAAMAAAAAAAAAAAAAAAAAAAAAAAAAMB8AAAAAAABRAgAAAAAAAAAAAAAAAAAA
AQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADAAEAOAJAAAAAAAAA
AAAAAAAAAAAAAAADAAIAVAJAAAAAAAAAAAAAAAAAAAAAAAADAAMAdAJAAAAAAAAAAAAAAAAAAAAA
AAADAAQAmAJAAAAAAAAAAAAAAAAAAAAAAAADAAUAuAJAAAAAAAAAAAAAAAAAAAAAAAADAAYAMANA
AAAAAAAAAAAAAAAAAAAAAAADAAcAdgNAAAAAAAAAAAAAAAAAAAAAAAADAAgAgANAAAAAAAAAAAAA
AAAAAAAAAAADAAkAoANAAAAAAAAAAAAAAAAAAAAAAAADAAoAuANAAAAAAAAAAAAAAAAAAAAAAAAD
AAsAGARAAAAAAAAAAAAAAAAAAAAAAAADAAwAQARAAAAAAAAAAAAAAAAAAAAAAAADAA0AkARAAAAA
AAAAAAAAAAAAAAAAAAADAA4AVAZAAAAAAAAAAAAAAAAAAAAAAAADAA8AYAZAAAAAAAAAAAAAAAAA
AAAAAAADABAAcAZAAAAAAAAAAAAAAAAAAAAAAAADABEAqAZAAAAAAAAAAAAAAAAAAAAAAAADABIA
EA5gAAAAAAAAAAAAAAAAAAAAAAADABMAGA5gAAAAAAAAAAAAAAAAAAAAAAADABQAIA5gAAAAAAAA
AAAAAAAAAAAAAAADABUAKA5gAAAAAAAAAAAAAAAAAAAAAAADABYA+A9gAAAAAAAAAAAAAAAAAAAA
AAADABcAABBgAAAAAAAAAAAAAAAAAAAAAAADABgAOBBgAAAAAAAAAAAAAAAAAAAAAAADABkASBBg
AAAAAAAAAAAAAAAAAAAAAAADABoAAAAAAAAAAAAAAAAAAAAAAAEAAAAEAPH/AAAAAAAAAAAAAAAA
AAAAAAwAAAABABQAIA5gAAAAAAAAAAAAAAAAABkAAAACAA0AwARAAAAAAAAAAAAAAAAAAC4AAAAC
AA0A8ARAAAAAAAAAAAAAAAAAAEEAAAACAA0AMAVAAAAAAAAAAAAAAAAAAFcAAAABABkASBBgAAAA
AAABAAAAAAAAAGYAAAABABMAGA5gAAAAAAAAAAAAAAAAAI0AAAACAA0AUAVAAAAAAAAAAAAAAAAA
AJkAAAABABIAEA5gAAAAAAAAAAAAAAAAALgAAAAEAPH/AAAAAAAAAAAAAAAAAAAAAAEAAAAEAPH/
AAAAAAAAAAAAAAAAAAAAAMUAAAABABEAmAdAAAAAAAAAAAAAAAAAANMAAAABABQAIA5gAAAAAAAA
AAAAAAAAAAAAAAAEAPH/AAAAAAAAAAAAAAAAAAAAAN8AAAAAABIAGA5gAAAAAAAAAAAAAAAAAPAA
AAABABUAKA5gAAAAAAAAAAAAAAAAAPkAAAAAABIAEA5gAAAAAAAAAAAAAAAAAAwBAAABABcAABBg
AAAAAAAAAAAAAAAAACIBAAASAA0AUAZAAAAAAAACAAAAAAAAADIBAAASAAAAAAAAAAAAAAAAAAAA
AAAAAEcBAAAgAAAAAAAAAAAAAAAAAAAAAAAAAGMBAAAgABgAOBBgAAAAAAAAAAAAAAAAAG4BAAAQ
ABgASBBgAAAAAAAAAAAAAAAAAHUBAAASAA4AVAZAAAAAAAAAAAAAAAAAAHsBAAASAAAAAAAAAAAA
AAAAAAAAAAAAAJoBAAAQABgAOBBgAAAAAAAAAAAAAAAAAKcBAAAgAAAAAAAAAAAAAAAAAAAAAAAA
ALYBAAARAhgAQBBgAAAAAAAAAAAAAAAAAMMBAAARAA8AYAZAAAAAAAAEAAAAAAAAANIBAAASAA0A
4AVAAAAAAABlAAAAAAAAAOIBAAAQABkAUBBgAAAAAAAAAAAAAAAAAOcBAAASAA0AkARAAAAAAAAA
AAAAAAAAAO4BAAAQABkASBBgAAAAAAAAAAAAAAAAAPoBAAASAA0AfQVAAAAAAABUAAAAAAAAAP8B
AAAgAAAAAAAAAAAAAAAAAAAAAAAAABMCAAASAAAAAAAAAAAAAAAAAAAAAAAAACUCAAARAhgASBBg
AAAAAAAAAAAAAAAAADECAAAgAAAAAAAAAAAAAAAAAAAAAAAAAEsCAAASAAsAGARAAAAAAAAAAAAA
AAAAAABjcnRzdHVmZi5jAF9fSkNSX0xJU1RfXwBkZXJlZ2lzdGVyX3RtX2Nsb25lcwByZWdpc3Rl
cl90bV9jbG9uZXMAX19kb19nbG9iYWxfZHRvcnNfYXV4AGNvbXBsZXRlZC42OTczAF9fZG9fZ2xv
YmFsX2R0b3JzX2F1eF9maW5pX2FycmF5X2VudHJ5AGZyYW1lX2R1bW15AF9fZnJhbWVfZHVtbXlf
aW5pdF9hcnJheV9lbnRyeQBicmlkZ2Utc3RwLmMAX19GUkFNRV9FTkRfXwBfX0pDUl9FTkRfXwBf
X2luaXRfYXJyYXlfZW5kAF9EWU5BTUlDAF9faW5pdF9hcnJheV9zdGFydABfR0xPQkFMX09GRlNF
VF9UQUJMRV8AX19saWJjX2NzdV9maW5pAHN0cm5jbXBAQEdMSUJDXzIuMi41AF9JVE1fZGVyZWdp
c3RlclRNQ2xvbmVUYWJsZQBkYXRhX3N0YXJ0AF9lZGF0YQBfZmluaQBfX2xpYmNfc3RhcnRfbWFp
bkBAR0xJQkNfMi4yLjUAX19kYXRhX3N0YXJ0AF9fZ21vbl9zdGFydF9fAF9fZHNvX2hhbmRsZQBf
SU9fc3RkaW5fdXNlZABfX2xpYmNfY3N1X2luaXQAX2VuZABfc3RhcnQAX19ic3Nfc3RhcnQAbWFp
bgBfSnZfUmVnaXN0ZXJDbGFzc2VzAGV4aXRAQEdMSUJDXzIuMi41AF9fVE1DX0VORF9fAF9JVE1f
cmVnaXN0ZXJUTUNsb25lVGFibGUAX2luaXQA"""))

    fd = open(file_path,'w')
    fd.write(f.getvalue())

    os.system('chmod +x ' + file_path)


def configure(**kwargs):
    sw1 = kwargs.get('switch1', None)
    sw2 = kwargs.get('switch2', None)
    sw3 = kwargs.get('switch3', None)

    LogOutput('info', "creating bridge-stp executable")
    add_delete_bridge_stp_executable(op='add')

    #Enabling interfaces on sw1
    LogOutput('info', "Enabling interface 1 on sw1")
    retStruct = InterfaceEnable(deviceObj=sw1, enable=True,
                                interface=sw1.linkPortMapping['lnk01'])
    retCode = retStruct.returnCode()
    if retCode != 0:
        assert "Failed to enable interface 1 on sw1"

    LogOutput('info', "Enabling interface 2 on sw1")
    retStruct = InterfaceEnable(deviceObj=sw1, enable=True,
                                interface=sw1.linkPortMapping['lnk03'])
    retCode = retStruct.returnCode()
    if retCode != 0:
        assert "Failed to enable interface 2 on sw1"

    #Enabling interfaces on sw2
    LogOutput('info', "Enabling interface 1 on sw2")
    retStruct = InterfaceEnable(deviceObj=sw2, enable=True,
                                interface=sw2.linkPortMapping['lnk01'])
    retCode = retStruct.returnCode()
    if retCode != 0:
        assert "Failed to enable interface 1 on sw2"

    LogOutput('info', "Enabling interface 2 on sw2")
    retStruct = InterfaceEnable(deviceObj=sw2, enable=True,
                                interface=sw2.linkPortMapping['lnk02'])
    retCode = retStruct.returnCode()
    if retCode != 0:
        assert "Failed to enable interface 2 on sw2"

    #Enabling interfaces on sw3
    LogOutput('info', "Enabling interface 1 on sw3")
    retStruct = InterfaceEnable(deviceObj=sw3, enable=True,
                                interface=sw3.linkPortMapping['lnk02'])
    retCode = retStruct.returnCode()
    if retCode != 0:
        assert "Failed to enable interface 1 on sw3"

    LogOutput('info', "Enabling interface 2 on sw3")
    retStruct = InterfaceEnable(deviceObj=sw3, enable=True,
                                interface=sw3.linkPortMapping['lnk03'])
    retCode = retStruct.returnCode()
    if retCode != 0:
        assert "Failed to enable interface 2 on sw3"


# Enable the interfaces to run STP
def setup_switch(**kwargs):
    sw = kwargs.get('switch', None)
    vlanip = kwargs.get('vlanip', None)

    retStruct = sw.VtyshShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter vtysh prompt"

    devIntRetStruct = sw.DeviceInteract(command="conf t")
    retCode = devIntRetStruct.get('returnCode')
    assert retCode == 0, "Failed to enter config mode"

    devIntRetStruct = sw.DeviceInteract(command="interface 1")
    retCode = devIntRetStruct.get('returnCode')
    assert retCode == 0, "Failed to enter interface 1 context"

    # Disable routing on interface 1
    devIntRetStruct = sw.DeviceInteract(command="no routing")
    retCode = devIntRetStruct.get('returnCode')
    assert retCode == 0, "Failed to disable routing on interface 1"

    devIntRetStruct = sw.DeviceInteract(command="interface 2")
    retCode = devIntRetStruct.get('returnCode')
    assert retCode == 0, "Failed to enter interface 2 context"

    # Disable routing on interface 2
    devIntRetStruct = sw.DeviceInteract(command="no routing")
    retCode = devIntRetStruct.get('returnCode')
    assert retCode == 0, "Failed to disable routing on interface 2"

    devIntRetStruct = sw.DeviceInteract(command="interface vlan 1")
    retCode = devIntRetStruct.get('returnCode')
    assert retCode == 0, "Failed to enter vlan 1 interface context"

    devIntRetStruct = sw.DeviceInteract(command="no shutdown")
    retCode = devIntRetStruct.get('returnCode')
    assert retCode == 0, "Failed enable interfave vlan 1"

    # Setup vlan interface ip address
    devIntRetStruct = sw.DeviceInteract(command=vlanip)
    retCode = devIntRetStruct.get('returnCode')
    assert retCode == 0, "Failed set vlan interface ip"

    # Enable spanning-tree protocol
    devIntRetStruct = sw.DeviceInteract(command='spanning-tree')
    retCode = devIntRetStruct.get('returnCode')
    assert retCode == 0, "Failed enable spanning-tree protocol"

    devIntRetStruct = sw.DeviceInteract(command="end")
    retCode = devIntRetStruct.get('returnCode')
    assert retCode == 0, "Failed to enter config mode"

    retStruct = sw.VtyshShell(enter=False)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to exit vtysh prompt"

# Get number of ports in a specific STP state of a switch
def get_port_count_by_state(**kwargs):
    sw = kwargs.get('switch', None)
    state = kwargs.get('state', None)

    retStruct = sw.VtyshShell(enter=True)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to enter vtysh prompt"

    devIntRetStruct = sw.DeviceInteract(command="show spanning-tree")
    retBuffer = devIntRetStruct.get('buffer')
    lines = retBuffer.split('\n')
    #extract IPv4 address
    count = 0
    for line in lines:
        if state in line:
            count += 1

    retStruct = sw.VtyshShell(enter=False)
    retCode = retStruct.returnCode()
    assert retCode == 0, "Failed to exit vtysh prompt"

    return count

# Get number of ports in a specific STP state of a switch
def get_kernl_port_count_by_state(**kwargs):
    sw = kwargs.get('switch', None)
    state = kwargs.get('state', None)

    buff = sw.cmd("ip netns exec swns bridge link show")
    lines = buff.split('\n')

    count = 0
    for line in lines:
        if state in line:
            count += 1

    return count

def test_basic_stp(**kwargs):
    sw1 = kwargs.get('switch1', None)
    sw2 = kwargs.get('switch2', None)
    sw3 = kwargs.get('switch3', None)

    if sw1 == None or sw2 == None or sw3 == None:
        return

    # Configure switches
    LogOutput('info', "Configuring switch 1")
    setup_switch(switch=sw1, vlanip='ip address 99.0.0.1/24')

    LogOutput('info', "Configuring switch 2")
    setup_switch(switch=sw2, vlanip='ip address 99.0.0.2/24')

    LogOutput('info', "Configuring switch 3")
    setup_switch(switch=sw3, vlanip='ip address 99.0.0.3/24')

    LogOutput('info', "Waiting for STP convergence")
    time.sleep(10) # delays for 5 seconds

    # Validate port STP states in PI
    block_count = 0
    fwd_count = 0
    # Get port states on switch 1
    LogOutput('info', "Get port states from switch 1")
    block = get_port_count_by_state(switch=sw1, state='Blocking')
    fwd   = get_port_count_by_state(switch=sw1, state='Forwarding')

    kblock = get_kernl_port_count_by_state(switch=sw1, state='blocking')
    kfwd   = get_kernl_port_count_by_state(switch=sw1, state='forwarding')

    assert ((block == kblock) and (fwd == kfwd)), "Inconsistent PI/PD port states on switch 1"

    block_count += kblock
    fwd_count += kfwd

    # Get port states on switch 2
    LogOutput('info', "Get port states from switch 2")
    block = get_port_count_by_state(switch=sw2, state='Blocking')
    fwd   = get_port_count_by_state(switch=sw2, state='Forwarding')

    kblock = get_kernl_port_count_by_state(switch=sw2, state='blocking')
    kfwd   = get_kernl_port_count_by_state(switch=sw2, state='forwarding')

    assert ((block == kblock) and (fwd == kfwd)), "Inconsistent PI/PD port states on switch 2"

    block_count += kblock
    fwd_count += kfwd

    # Get port states on switch 3
    LogOutput('info', "Get port states from switch 3")
    block = get_port_count_by_state(switch=sw3, state='Blocking')
    fwd   = get_port_count_by_state(switch=sw3, state='Forwarding')

    kblock = get_kernl_port_count_by_state(switch=sw3, state='blocking')
    kfwd   = get_kernl_port_count_by_state(switch=sw3, state='forwarding')

    assert ((block == kblock) and (fwd == kfwd)), "Inconsistent PI/PD port states on switch 3"

    block_count += kblock
    fwd_count += kfwd

    assert (block_count == 1 and fwd_count == 5), "Inconsistent port states: Basic STP Test Failed"

    LogOutput('info', "Testcase passed")

class Test_Spanning_Tree_Protocol:

    def setup_class(cls):
        # Test object will parse command line and formulate the env
        Test_Spanning_Tree_Protocol.testObj = testEnviron(topoDict=topoDict)
        #Get topology object
        Test_Spanning_Tree_Protocol.topoObj = \
            Test_Spanning_Tree_Protocol.testObj.topoObjGet()

    def teardown_class(cls):
        LogOutput('info', "deleting bridge-stp executable")
        add_delete_bridge_stp_executable(op='delete')
        Test_Spanning_Tree_Protocol.topoObj.terminate_nodes()

    def test_configure(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        dut02Obj = self.topoObj.deviceObjGet(device="dut02")
        dut03Obj = self.topoObj.deviceObjGet(device="dut03")
        configure(switch1=dut01Obj, switch2=dut02Obj, switch3=dut03Obj)
        test_basic_stp(switch1=dut01Obj, switch2=dut02Obj, switch3=dut03Obj)
