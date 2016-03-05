#!/usr/bin/python

# (c) Copyright 2015 Hewlett Packard Enterprise Development LP
#
# GNU Zebra is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# GNU Zebra is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Zebra; see the file COPYING.  If not, write to the Free
# Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

import time
import pytest
import re
from  opstestfw import *
from opstestfw.switch.CLI import *
from opstestfw.switch import *

# Topology definition
topoDict = {"topoExecution": 1000,
            "topoTarget": "dut01",
            "topoDevices": "dut01",
            "topoFilters": "dut01:system-category:switch"}

#topoDict = {"topoExecution": 1000,
#            "topoTarget": "dut01",
#            "topoDevices": "dut01 wrkston01",
#            "topoLinks": "lnk01:dut01:wrkston01",
#            "topoFilters": "dut01:system-category:switch,\
#                    wrkston01:system-category:workstation"}
#

def MSTPCliTest(**kwargs):
    device1 = kwargs.get('device1',None)
#    device2 = kwargs.get('device2', None)

    #Case:1 Test "Spanning-tree"(Default-enable) command from CLI to OVS-VSCTL
    device1.VtyshShell(enter=True)
    device1.ConfigVtyShell(enter=True)
    retStructure = device1.DeviceInteract(command="spanning-tree")
    retStructure = device1.DeviceInteract(command="exit")
    retStructure = device1.DeviceInteract(command="show running-config spanning-tree")
    cmdOut = retStructure.get('buffer')
    assert 'spanning-tree enable' in cmdOut, "Case:1 Test to enable MSTP failed"

    retStructure = device1.DeviceInteract(command="exit")
    retStructure = device1.DeviceInteract(command="ovs-vsctl list bridge")
    cmdOut = retStructure.get('buffer')
    assert 'true' in cmdOut, "Case:1 Test to enable spanning-tree by default failed"

    #Case:2 Test "no Spanning-tree" command from CLI to OVS-VSCTL
    retStructure = device1.DeviceInteract(command="vtysh")
    retStructure = device1.DeviceInteract(command="conf t")
    retStructure = device1.DeviceInteract(command="no spanning-tree")
    retCode = retStructure.get('returnCode')
    assert retCode == 0, "Case:2 - Failed to disable spanning-tree"

    retStructure = device1.DeviceInteract(command="exit")
    retStructure = device1.DeviceInteract(command="show running-config spanning-tree")
    cmdOut = retStructure.get('buffer')
    assert 'spanning-tree enable' not in cmdOut, "Case:2 Test to enable MSTP failed"

    retStructure = device1.DeviceInteract(command="exit")

    retStructure = device1.DeviceInteract(command="ovs-vsctl list bridge")
    retCode = retStructure.get('returnCode')
    assert retCode == 0, "Case:2 - Failed to run ovs-vsctl"
    cmdOut = retStructure.get('buffer')
    assert 'false' in cmdOut, "Case:2 Test to disable spanning-tree by default failed"

    #Case:3 Test "Spanning-tree enable" command from CLI to OVS-VSCTL
    retStructure = device1.DeviceInteract(command="vtysh")
    retStructure = device1.DeviceInteract(command="conf t")
    retStructure = device1.DeviceInteract(command="spanning-tree enable")
    retStructure = device1.DeviceInteract(command="exit")
    retStructure = device1.DeviceInteract(command="show running-config spanning-tree")
    cmdOut = retStructure.get('buffer')
    assert 'spanning-tree enable' in cmdOut, "Case:3 Test to enable MSTP failed"

    retStructure = device1.DeviceInteract(command="exit")
    retStructure = device1.DeviceInteract(command="ovs-vsctl list bridge")
    cmdOut = retStructure.get('buffer')
    assert 'true' in cmdOut, "Case:3 Test to enable spanning-tree by default failed"

    #Case:4 Test "Spanning-tree disable" command from CLI to OVS-VSCTL
    retStructure = device1.DeviceInteract(command="vtysh")
    retStructure = device1.DeviceInteract(command="conf t")
    retStructure = device1.DeviceInteract(command="spanning-tree disable")
    retStructure = device1.DeviceInteract(command="exit")
    retStructure = device1.DeviceInteract(command="show running-config spanning-tree")
    cmdOut = retStructure.get('buffer')
    assert 'spanning-tree enable' not in cmdOut, "Case:4 Test to disable MSTP failed"

    retStructure = device1.DeviceInteract(command="exit")
    retStructure = device1.DeviceInteract(command="ovs-vsctl list bridge")
    cmdOut = retStructure.get('buffer')
    assert 'false' in cmdOut, "Case:4 Test to disable spanning-tree by default failed"

    #Case:5 Test enable spanning-tree from OVS-VSCTL
    retStructure = device1.DeviceInteract(command="ovs-vsctl set bridge \
            bridge_normal mstp_enable=true")

    retStructure = device1.DeviceInteract(command="ovs-vsctl list bridge")
    cmdOut = retStructure.get('buffer')
    assert 'true' in cmdOut, "Case:5 Test to enable spanning-tree failed"

    retStructure = device1.DeviceInteract(command="vtysh")
    retStructure = device1.DeviceInteract(command="show running-config spanning-tree")
    cmdOut = retStructure.get('buffer')
    assert 'spanning-tree enable' in cmdOut,"Case:5 Failed to enable MSTP"
    retStructure = device1.DeviceInteract(command="exit")

    #Case:6 Test disable spanning-tree from OVS-VSCTL
    retStructure = device1.DeviceInteract(command="ovs-vsctl set bridge \
            bridge_normal mstp_enable=false")

    retStructure = device1.DeviceInteract(command="ovs-vsctl list bridge")
    cmdOut = retStructure.get('buffer')
    assert 'false' in cmdOut, "Case:6 Test to disable spanning-tree failed"

    retStructure = device1.DeviceInteract(command="vtysh")
    retStructure = device1.DeviceInteract(command="show running-config spanning-tree")
    cmdOut = retStructure.get('buffer')
    assert 'spanning-tree enable' not in cmdOut, "Case:6 Failed to disable MSTP"
    retStructure = device1.DeviceInteract(command="exit")

    #Case:7 Test spanning-tree config-revision via CLI
    retStructure = device1.DeviceInteract(command="vtysh")
    retStructure = device1.DeviceInteract(command="conf t")
    retStructure = device1.DeviceInteract(command="spanning-tree \
                                                            config-revision 5")
    retStructure = device1.DeviceInteract(command="exit")
    retStructure = device1.DeviceInteract(command="show running-config \
                                                            spanning-tree")
    cmdOut = retStructure.get('buffer')
    assert 'spanning-tree config-revision 5' in cmdOut, "Case:7 Test to set \
                                                        config-revision failed"

    retStructure = device1.DeviceInteract(command="exit")
    retStructure = device1.DeviceInteract(command="ovs-vsctl list bridge")
    cmdOut = retStructure.get('buffer')
    assert 'mstp_config_revision="5"' in cmdOut, "Case:7 Test to set \
                                                config-revision via CLI failed"

    #Case:8 Test no spanning-tree config-revision from CLI
    retStructure = device1.DeviceInteract(command="vtysh")
    retStructure = device1.DeviceInteract(command="conf t")
    retStructure = device1.DeviceInteract(command="no spanning-tree \
                                                            config-revision")
    retStructure = device1.DeviceInteract(command="exit")
    retStructure = device1.DeviceInteract(command="show running-config \
                                                            spanning-tree")
    cmdOut = retStructure.get('buffer')
    assert 'spanning-tree config-revision' not in cmdOut, "Case:8 Test to reset\
                                                        config-revision failed"

    retStructure = device1.DeviceInteract(command="exit")
    retStructure = device1.DeviceInteract(command="ovs-vsctl list bridge")
    cmdOut = retStructure.get('buffer')
    assert 'mstp_config_revision="0"' in cmdOut, "Case:8 Test to reset \
                                                config-revision via CLI failed"

    #Case:9 Test spanning-tree config-revision via OVS_VSCTL
    retStructure = device1.DeviceInteract(command='ovs-vsctl set bridge\
            bridge_normal other_config={mstp_config_revision="44"}')

    retStructure = device1.DeviceInteract(command="ovs-vsctl list bridge")
    cmdOut = retStructure.get('buffer')
    assert 'mstp_config_revision="44"' in cmdOut, "Case:9 Test to set\
            config-revision via OVS_VSCTL failed"

    retStructure = device1.DeviceInteract(command="vtysh")
    retStructure = device1.DeviceInteract(command="show running-config spanning-tree")
    cmdOut = retStructure.get('buffer')
    assert 'spanning-tree config-revision 44' in cmdOut, "Case:9 Test to set\
                                         config-revision via OVS_VSCTL failed"

    #Case:10 Test spanning-tree config-name from CLI
    retStructure = device1.DeviceInteract(command="vtysh")
    retStructure = device1.DeviceInteract(command="conf t")
    retStructure = device1.DeviceInteract(command="spanning-tree config-name MST1")
    retStructure = device1.DeviceInteract(command="exit")
    retStructure = device1.DeviceInteract(command="show running-config spanning-tree")
    cmdOut = retStructure.get('buffer')
    assert 'spanning-tree config-name MST1' in cmdOut, "Case:10 Test to set config name failed"

    retStructure = device1.DeviceInteract(command="exit")
    retStructure = device1.DeviceInteract(command="ovs-vsctl list bridge")
    cmdOut = retStructure.get('buffer')
    assert 'mstp_config_name="MST1"' in cmdOut, "Case:10 Test to set config-name via CLI failed"

    #Case:11 Test no spanning-tree config-name from CLI
    retStructure = device1.DeviceInteract(command="vtysh")
    retStructure = device1.DeviceInteract(command="conf t")
    retStructure = device1.DeviceInteract(command="no spanning-tree config-name")
    retStructure = device1.DeviceInteract(command="exit")
    retStructure = device1.DeviceInteract(command="show running-config spanning-tree")
    cmdOut = retStructure.get('buffer')
    assert 'spanning-tree config-name' not in cmdOut, "Case:11 Test to reset config-name  failed"

    retStructure = device1.DeviceInteract(command="exit")
    retStructure = device1.DeviceInteract(command="ovs-vsctl list bridge")
    cmdOut = retStructure.get('buffer')
    assert 'mstp_config_name' in cmdOut, "Case:11 Test to reset config-name via CLI failed"

class Test_mstp_cli:
    def setup_class (cls):
        # Test object will parse command line and formulate the env
        Test_mstp_cli.testObj = testEnviron(topoDict=topoDict)
        #    Get topology object
        Test_mstp_cli.topoObj = Test_mstp_cli.testObj.topoObjGet()

    def teardown_class (cls):
        Test_mstp_cli.topoObj.terminate_nodes()

    def test_mstp_cli(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        #wrkston1Obj = self.topoObj.deviceObjGet(device="wrkston01")
        #retValue = MSTPCliTest(device1=dut01Obj, device2=wrkston1Obj)
        retValue = MSTPCliTest(device1=dut01Obj)
