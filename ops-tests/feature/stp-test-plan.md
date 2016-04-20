# MSTP Test Cases
## Contents

- [Diagrammatic Conventions Used in the Document](#diagrammatic-conventions-used-in-the-document)
- [1 Protocol Parameters Configuration](#1-protocol-parameters-configuration)
	- [1.1 Default Spanning Tree Configuration](#1.1-default-spanning-tree-configuration)
	- [1.2 Default MSTP Configuration](#1.2-default-mstp-configuration)
	- [1.3 MST Configuration Identifier](#1.3-mst-configuration-identifier)
	- [1.4 Common CIST and MSTIs Parameters](#1.4-common-cist-and-mstis-parameters)
	- [1.5 CIST Parameters](#1.5-cist-parameters)
	- [1.6 MST Instance Creation/Deletion](#1.6-mst-instance-creation/deletion)
	- [1.7 MST Instance Parameters](#1.7-mst-instance-parameters)
	- [1.8 Configuration Constrains](#1.8-configuration-constrains)
	- [1.9 MSTP and Static VLANs](#1.9-mstp-and-static-vlans)
- [2 Protocol Operation](#2-protocol-operation)
	- [2.1 Enabling/Disabling Protocol](#2.1-enabling/disabling-protocol)
	- [2.2 Initial Protocol Operation](#2.2-initial-protocol-operation)
	- [2.3 CIST Priority Vector - MST BPDU Format](#2.3-cist-priority-vector---mst-bpdu-format)
	- [2.4 CIST Root Bridge Election](#2.4-cist-root-bridge-election)
	- [2.5 MST Region Join - Formation](#2.5-mst-region-join---formation)
	- [2.6 MST Region Leave - Separation](#2.6-mst-region-leave---separation)
	- [2.7 CIST Regional Root Bridge Election](#2.7-cist-regional-root-bridge-election)
	- [2.8 CIST Ports Roles and States Assignment - loop resolving](#2.8-cist-ports-roles-and-states-assignment---loop-resolving)
	- [2.9 CIST Root Bridge Parameters Propagation](#2.9-cist-root-bridge-parameters-propagation)
	- [2.10 MST0 Regional Root Parameters Propagation](#2.10-mst0-regional-root-parameters-propagation)
	- [2.11 MSTI Priority Vector - MSTI Configuration Message](#2.11-msti-priority-vector---msti-configuration-message)
	- [2.12 MSTI Regional Root Bridge Election](#2.12-msti-regional-root-bridge-election)
	- [2.13 MSTI Ports Roles and States Assignment (loop resolving)](#2.13-msti-ports-roles-and-states-assignment---loop-resolving)
	- [2.14 Fault Tolerance (CIST)](#2.14-fault-tolerance---cist)
	- [2.15 Fault Tolerance (MSTI)](#2.15-fault-tolerance---msti)
	- [2.16 Topology Changes Detection (CIST)](#2.16-topology-changes-detection---cist)
	- [2.17 Topology Changes Propagation (CIST)](#2.17-topology-changes-propagation---cist)
	- [2.18 Topology Changes Detection (MSTI)](#2.18-topology-changes-detection---msti)
	- [2.19 Topology Changes Propagation (MSTI)](#2.19-topology-changes-propagation---msti)

### Diagrammatic Conventions Used in the Document
```ditaa

[R]------- Root Port

[D]------- Designated Port

---X-----  Alternate Port

--XXX----- Backup Port

[M]------  Master Port  (applicable to an MSTI only)

[E]------  Edge Port

8:A	Bridge Identifier comprising of Priority and MAC Address components. In this examplePriority component was configured via CLI with the use of step 8, i.e. corresponds to the real value of  8*4096=32768. “A” is a MAC address of the Bridge. When MAC addresses of two different Bridges are being compared “A” means it is lower (better) than “B”, “B” is lower (better) than “C” and so on.

8:1	Port Identifier comprising of Priority and Port Number components. In this example Priority component was configured via CLI with the use of step 8, i.e. corresponds to the real value of  8*16=128. “1”is the number Bridge uses to identify a port, i.e. first port on the slot A.

MST Region A	MST Configuration Identifier information comprising of name, revision number and digest value a switch is using to advertise it to other switches in order to form MST Region.

```

###	1 Protocol Parameters Configuration
#### 1.1 Default Spanning Tree Configuration
#### Setup
Single switch
#### Description
1. Clear switch’s configuration by using **“erase startup-config”** CLI command.
2. After reboot use **“show spanning-tree”** CLI command to verify spanning tree configuration. Spanning tree protocol must be disabled.

#### 1.2 Default MSTP Configuration
#### Setup
Single switch
#### Description
i1. Enable spanning tree protocol **"(conf t)spanning-tree"**.
2. Enable any one interface and make it as L2 interface.
3. Use **“show spanning-tree mst“** and **“show spanning-tree mst-config“** CLI commands to verify default configuration set for MSTP. The summary of default configuration should be as follows:


"show spanning-tree mst"

|Per-Bridge Parameters|Default Value|
|:-----------|:---------- |
| MST Mapped VLANs| 1 |
| Max Age|20 |
| Forward Delay|15 |
| Bridge Hello Time| 2|
| Switch Priority|8 |

|Per-Port Parameters| Default Value
|:-----------|:----------|
|Path Cost| 0|
|Priority| 8|
|Type| p2p|
|Role| Diabled|
|State| Blocking|

"show spanning-tree mst-config"

|Parameters	 | Default Value |
|:---------------|:----------
| MST config ID| Mac_Address|
| MST config revision|0 |
| MST config digest| |
|Number of instances|0|

| Instance ID   |  Member VLANs
| :------------ :----------------------------------
| 0             | 1-4095


Verify that there are no any MST Instances configured, only MST0 must exist (e.g. use show spanning-tree mst CLI command).

####1.3 MST Configuration Identifier
#### Setup
Single switch
#### Description
1. Verify that MST Configuration Name is a textual string containing the Hexadecimal Representation of the switch’s MAC address, MST Configuration Revision is “0” and MST configuration digest value is “0xAC36177F50283CD4B83821D8AB26DE62”(use “show spanning-tree mst-config” CLI command).
2. Verify that MST Configuration name can be set to any ASCII textual string that is not longer than 32 characters (NOTE: ‘~’ character can not be used as it has special interpretation in the switch’s code).
3. Verify that MST Configuration Revision number can be set to any value in range of 0-65535.
4. Create a few new VLANs. Verify that MST Configuration digest value has not changed.
5. Verify that MST Configuration digest value has not changed after VLANs removal.

####1.4 Common CIST and MSTIs Parameters
#### Setup
Single switch
#### Description
1. Configure the following parameters: forward-delay, hello-time, max-hops, maximum-age.
2. Verify all your changes via CLI show commands.
3. Save configuration and reboot the switch.
4. Verify that all your changes remained as they were before switch reboot.

####1.5 CIST Parameters
#### Setup
Single switch
#### Description
1. Configure CIST parameters: priority.
2. Verify your changes.
3. Configure CIST port parameters: edge-port, hello-time, path-cost, priority.
4. Verify your changes.
5. Save configuration and reboot the switch.
6. Verify that all your changes remained as they were before switch reboot.

####1.6 MST Instance Creation/Deletion
#### Setup
Single switch
#### Description
1. Create new VLAN entries so that total number of configured VLANs is equal max number of vlan’s allowed in switch.
2. Verify that by default all new VLANs are being mapped to the MST0 Instance (use “show spanning-tree mst” CLI command).
3. Create new MSTIs so that each MSTI has one VLAN associated with it (total number of MSTI entries will be equal to the number of newly configured instances).
4. Verify that each VLAN associated with an MSTI is not mapped to the CIST anymore.
5. Verify that after each MST Instance creation the MST Configuration digest value has been changed.
6. Verify that non-existing VLAN(s) cannot be associated with an MST Instance.
7. Verify that a VLAN cannot be mapped more than to one MST Instance at time.
8. Save configuration and reboot the switch. After switch reboot you should see all your changes are being effective.
9. Unmap a VLAN from the existing MST Instance and verify this VLAN belongs to the MST0 again.
10. Verify that the changes in VLANs to MSTI mapping cause the change in MST Configuration digest value.
11. Verify that MST Instance cannot exist without any VLAN being mapped to it (try to unmap all VLANs from an MST Instance).
12.  Remove existing MST Instance.
13. Verify that all VLANs that were associated with the removed MSTI are mapped back to the MST0.
14. Verify that after each MST Instance deletion the MST Configuration digest value has changed.
15. Verify that you cannot remove non-existing MSTI.
16. Save configuration and reboot the switch. After switch reboot you should see all your changes are being effective.

####1.7 MST Instance Parameters
#### Setup
Single switch
#### Description
1. Configure parameters for an existing MSTI: priority, vlan.
2. Verify your changes.
3. Configure port parameters for an existing MSTI: path-cost, priority.
4. Verify your changes.
5. Try to configure parameters for non-existing MSTI: priority. You should see complain message that instance does not exist. Verify no configuration changes occurred.
6. Try to configure port parameters for non-existing MSTI: path-cost, priority. You should see complain message that instance does not exist. Verify no configuration changes occurred.
7. Save configuration and reboot the switch.
8. Verify that all your changes remained as they were before switch reboot.

####1.8 Configuration Constrains
#### Setup
Single switch
#### Description
1. Verify that all MSTP parameters restricted on valid ranges cannot be misconfigured (these are config-name, config-revision, forward-delay, hello-time, max-hops, maximum-age, priority)
2. Verify that configuration values for Max Age, Fwd Delay and Hello Time parameters are constrained by the following equitations:
Max Age <= 2x(Fwd Delay – 1)
Max Age >= 2x(Hello Time + 1)

3. Verify that Hello Time configured on per-Port basis also complies with the above constrains.
4. Verify that no more that “MAX supported” MSTIs can be configured on the switch (currently up to 64 MSTIs can be configured).

####1.9 MSTP and Static VLANs
#### Setup
Single switch
#### Description
1. Create few VLANs, e.g. VLAN 100, 200, 300, 400.
2. Verify that all created VLANs are associated with MST0 Instance.
3. Create new MST instance with all newly created VLANs being mapped to it, e.g. “spanning-tree instance 1 vlan 100 200 300 400”.
4. Verify that VLANs 100 200 300 400 are associated with new MST Instance and are unmapped from the MST0. Verify that MST Configuration digest value has been changed.
5. Delete an existing VLAN, e.g. “no vlan 400”.
6. Verify that deleted VLAN has been disassociated from the MST Instance and MST Configuration Digest value has been updated.
7. Repeat step 6 for all VLANs currently mapped to the MSTI. Check that Configuration Digest value is being updated on each successful VLAN deletion
8. Verify that a VLAN deletion cannot produce effect of “empty” MSTI, i.e. no MSTI must exist without associated VLANs.

### 2 Protocol Operation
#### 2.1 Enabling/Disabling Protocol
#### Setup
##### Topology diagram
```ditaa
+----------------+                                +-----------------+
|                |                                |                 |
|     S2         <------------------------------->|        H1       |
|                |                                |                 |
+----------------+                                +-----------------+

H1 station running a “packet analyzer”software application
```
#### Description
1. Use **"show spanning-tree"** to verify protocol is disabled.
2. Start packet capture.
3. You should not see any spanning tree packets coming out of the box.
4. Enable protocol via CLI **"#switch(conf t)spanning-tree"**.
5. Check that packet analyzer is capturing spanning tree packets. Verify that packets are being sent continuously with the default Hello Time interval (2 sec).
6. Disable protocol via CLI.
7. Check that packet analyzer stopped capturing spanning tree packets

####2.2 Initial Protocol Operation
#### Setup
##### Topology diagram
```ditaa
+-------------+        +--------+         +---------------+
|             +--------+ HUB    +---------+               |
|     8:A     |        |        |         |    8:B        |
|             |        +--------+         |               |
|             |             |             |               |
+-------------+             |             +---------------+
                      +------------+
                      |            |
                      |  H1        |
                      |  Packet analyzer
                      +------------+

```
#### Description
1. Start packet capture.
2. You should not see any spanning tree packets coming out of the boxes.
3. On both switches enable protocol via CLI.
4. Check that packet analyzer is capturing spanning tree packets. Verify that initially both switches send packets with the default Hello Time interval. In about 2 Hello Time intervals “8:B” switch should stop sending packets while the **“8:A”** switch still continue send packets with the Hello Time interval.
5. Look at the captured packets both switches initially sent and verify both switches claim themselves as a root.
6. Disconnect and than reconnect link on the **“8:B”** switch.
7. Check that switch **“8:B”** starts sending packets again and than stops it in a few seconds. Look at the captured packets and verify switch claims itself to be a root.
8. Using CLI show commands verify that switch **“8:A”** is known as a CST root on both switches.

####2.3 CIST Priority Vector - MST BPDU Format
#### Setup
##### Topology diagram
```ditaa
+----------------+                                +-----------------+
|                |                                |                 |
|     S2         <------------------------------->|        H1       |
|                |                                |                 |
+----------------+                                +-----------------+

H1 station running a “packet analyzer”software application

```
#### Description
1. Capture a few spanning tree packets coming out of the switch.
2. Verify that the structure of captured BPDUs matches the following:

```ditaa
                                      Octet#
+--------------------------------+
|                                |   1-2
|    Protocol Identifier         |
+--------------------------------+
|                                |  3
|    Protocol Version Identi ier |
+--------------------------------+
|                                |  4
|    BPDU Type                   |
+--------------------------------+
|                                |  5
|   CIST Flags                   |
+--------------------------------+
|                                |  6-13
|  CIST Root Identifier          |
+--------------------------------+
|                                |  14-17
|  CIST External Path Cost       |
+--------------------------------+
|                                |  18-25
| CIST Regional Root Identifier  |
+--------------------------------+
|                                |  26-27
| CIST Port Identifier           |
+--------------------------------+
|                                |  28-29
|  Message Age                   |
+--------------------------------+
|                                |  30-31
|  Max Age                       |
+--------------------------------+
|                                |  32-33
|  Hello Time                    |
+--------------------------------+
|                                |  34-35
|  Forward Delay                 |
+--------------------------------+
|                                |  36
|  Version 1 Length              |
+--------------------------------+
|                                |  37-38
|  Version 3 Length              |
+--------------------------------+
|  MST Configuration Identifier  |  39-89
|                                |
+--------------------------------+
|                                |  90-93
| CIST Internal Root Path Cost   |
+--------------------------------+
|                                |  94-101
| CIST Bridge Identifier         |
+--------------------------------+

```

3. Verify that all fields in a captured BPDU contain correct Bridge/Port ID and configuration information.
NOTE: For convenience, use packet analyzer that supports MST BPDU decription, e.g. latest version of the “Ethereal – Network Protocol Analyzer “ (www.ethereal.com).


####2.4 CIST Root Bridge Election
#### Setup
##### Topology diagram

```ditaa
Topology I:

                  +--------------------------------------------+
                  |                    MST Region One          |
+-------------+   |  +--------------+       +-------------+    |    +-----------+
|             |   |  |              |       |             |    |    |      **   |
|             |   |  |  8:B         |       | 8:C         |    |    | 7:D       |
|  8:A       [R]----[D] MST Bridge [R]----[D] MST Bridge [R]-------[D] MSTP Bridge
| MSTP Bridge |   |  |              |       |             |    |    |           |
|             |   |  |              |       |             |    |    |           |
+-------------+   |  +--------------+       +-------------+    |    +-----------+
                  +--------------------------------------------+

Topology II:

                  +--------------------------------------------+
                  |                    MST Region One          |
+-------------+   |  +--------------+       +-------------+    |    +-----------+
|     **      |   |  |              |       |             |    |    |           |
|             |   |  |  8:B         |       | 8:C         |    |    | 8:D       |
|  8:A       [R]----[D] MST Bridge [R]----[D] MST Bridge [R]-------[D] MSTP Bridge
| MSTP Bridge |   |  |              |       |             |    |    |           |
|             |   |  |              |       |             |    |    |           |
+-------------+   |  +--------------+       +-------------+    |    +-----------+
                  +--------------------------------------------+

Topology III:

                  +--------------------------------------------+
                  |                    MST Region One          |
+-------------+   |  +--------------+       +-------------+    |    +-----------+
|             |   |  |              |       |       **    |    |    |           |
|             |   |  |  8:B         |       | 7:C         |    |    | 8:D       |
|  8:A       [R]----[D] MST Bridge [R]----[D] MST Bridge [R]-------[D] MSTP Bridge
| MSTP Bridge |   |  |              |       |             |    |    |           |
|             |   |  |              |       |             |    |    |           |
+-------------+   |  +--------------+       +-------------+    |    +-----------+
                  +--------------------------------------------+

Topology IV:

+----------------------------------------+ +-------------------------------------------+
|               MST Region One           | |                      MST Region Two       |
|  +-------------+      +--------------+ | |   +-------------+         +-----------+   |
|  |             |      |              | | |   |       **    |         |           |   |
|  |             |      |  8:B         | | |   | 7:C         |         | 8:D       |   |
|  |  8:A       [R]----[D] MST Bridge [R]----[D] MST Bridge [R]-------[D] MSTP Bridge  |
|  | MSTP Bridge |      |              | | |   |             |         |           |   |
|  |             |      |              | | |   |             |         |           |   |
|  +-------------+      +--------------+ | |   +-------------+         +-----------+   |
+----------------------------------------+ +-------------------------------------------+


 NOTE: a switch shown ** within it is the CST Root for the spanning tree

```
#### Description
1. On all switches, starting from default configuration as required by the corresponding topology, spanning tree status to “enabled” , no MSTIs configured
2. Starting from the Topology I example and continue through up to the Topology example IV
do the configuration changes as shown on the corresponding picture for every topology.
3. Verify that in every configuration /topology example the active spanning tree topology matches to that depicted. Use CLI commands for configuration changes and for displaying spanning tree status information. Every topology should be built and become stable in about **“2xHello Time”** period.

NOTE: a switch shown ** within it is the CST Root for the spanning tree

####2.5 MST Region Join - Formation
#### Setup
##### Topology diagram

```ditaa
Topology I:
+-----------------+    +-----------------+    +------------------+     +----------------+
| MST Region A    |    | MST Region B    |    | MST Region C     |     | MST Region D   |
| +-------------+ |    |+--------------+ |    | +-------------+  |     |+-----------+   |
| |       **    | |    ||              | |    | |             |  |     ||           |   |
| |  8:A        | |    ||  8:B         | |    | | 8:C         |  |     || 8:D       |   |
| | MSTP Bridge[D]_----[R] MST Bridge [D]------[R] MST Bridge [D]-------[R] MSTP Bridge |
| |             | |    ||              | |    | |             |  |     ||           |   |
| |             | |    ||              | |    | |             |  |     ||           |   |
| +-------------+ |    |+--------------+ |    | +-------------+  |     |+-----------+   |
+-----------------+    +-----------------+    +------------------+     +----------------+

Topology II:
+----------------------------------------+    +------------------+     +----------------+
| MST Region A                           |    | MST Region C     |     | MST Region D   |
| +-------------+       +--------------+ |    | +-------------+  |     +------------+   |
| |     **      |       |              | |    | |             |  |     ||           |   |
| |             |       |  8:B         | |    | | 8:C         |  |     |+ 8:D       +   |
| |  8:A       [D]-----[R] MST Bridge [D]------[R] MST Bridge [D]------[R] MSTP Bridge  |
| | MSTP Bridge |       |              | |    | |             |  |     |           +   |
| |             |       |              | |    | |             |  |     ||           |   |
| +-------------+       +--------------+ |    | +-------------+  |     |------------+   |
+----------------------------------------+    +------------------+     +----------------+

Topology III:
+----------------------------------------------------------------+     +----------------+
| MST Region A                                                   |     | MST Region D   |
|                                                                |     |                |
| +-------------+       ---------------+        +-------------+  |     |+-----------+   |
| |       **    |       |              |        |             |  |     ||           |   |
| |  8:A        |       |  8:B         |        | 8:C         +  |     || 8:D       |   |
| | MSTP Bridge[D]-----[R] MST Bridge [D]------[R] MST Bridge [D]-------[R] MSTP Bridge |
| |             |       |              |        |             |  |     ||           |   |
| |             |       |              |        |             |  |     ||           |   |
| +-------------+       +--------------+        +-------------+  |     |------------+   |
+----------------------------------------------------------------+     +----------------+

Topology IV:
+---------------------------------------------------------------------------------------+
| MST Region A                                                                          |
|                                                                                       |
| +-------------+       ---------------+        +-------------+         ------------+   |
| |       **    |       |              |        |             |         |           |   |
| |  8:A        |       |  8:B         |        | 8:C         |         | 8:D       |   |
| | MSTP Bridge[D]-----[R] MST Bridge [D]------[R] MST Bridge [D]-------[R] MSTP Bridge |
| |             |       |              |        |             |         |           |   |
| |             |       |              |        |             |         |           |   |
| +-------------+       +--------------+        +-------------+         +-----------+   |
+---------------------------------------------------------------------------------------+

NOTE: a switch shown ** within it is the CST Root for the spanning tree
```
#### Configuration
On all switches, starting from default configuration as required by the corresponding topology, spanning tree status to “enabled” , no MSTIs configured.
#### Description
1. Starting from the Topology I example verify that initial active topology matches to that shown on the picture. On every switch in the topology the default MST Configuration name should be a text string using the hexadecimal representation of the switch’s MAC address, MST Configuration revision should be set “0” and digest value should be “0xAC36177F50283CD4B83821D8AB26DE62”.
2. Verify that after configuring identical values for the MST Configuration Identifier parameters (config-name, config-revision ) the switches join (form) common MST Region. Use the sequence shown in the topology examples I-IV.
3. Verify that each switch located within the MST Region leaves that region when switch’s MST Configuration Identifier parameters (config-name, config-revision ) changed to the values different from what other switches have, e.g. starting from switch “8:D” reconfiguration verify that topology IV gradually transforms to the topology I.
4. Repeat step 2 and verify that the topology transforms back to example IV.
5. Check that transitioning from one active topology to another occurs within “2xHello Time” period.

NOTE: a switch shown ** within it is the CST Root for the spanning tree

####2.6 MST Region Leave - Separation
#### Setup
##### Topology diagram

```ditaa
Topology I:
+---------------------------------------------------------------------------------------+
| MST Region A                                                                          |
|                                                                                       |
| +-------------+       ---------------+        +-------------+         ------------+   |
| |       **    |       |              |        |             |         |           |   |
| |  8:A        +       +  8:B         +        + 8:C         +         + 8:D       +   |
| | MSTP Bridge[D]+---+[R] MST Bridge [D]+----+[R] MST Bridge [D]+-----+[R] MSTP Bridge |
| |             +       +              +        +             +         +           +   |
| |             |       |              |        |             |         |           |   |
| +-------------+       +--------------+        +-------------+         +-----------+   |
+---------------------------------------------------------------------------------------+


Topology II:
+----------------------------------------------------------------+     +----------------+
| MST Region A                                                   |     | MST Region B   |
|                                                                |     |                |
| +-------------+       ---------------+        +-------------+  |     |+-----------+   |
| |       **    |       |              |        |             |  |     ||           |   |
| |  8:A        +       +  8:B         +        + 8:C         +  |     |+ 8:D       +   |
| | MSTP Bridge[D]+---+[R] MST Bridge [D]+----+[R] MST Bridge [D]+-----+[R] MSTP Bridge |
| |             +       +              +        +             +  |     |+           +   |
| |             |       |              |        |             |  |     ||           |   |
| +-------------+       +--------------+        +-------------+  |     |------------+   |
+----------------------------------------------------------------+     +----------------+


Topology III:
+----------------------------------------+    +------------------+     +----------------+
| MST Region A                           |    | MST Region B     |     | MST Region A   |
|                                        |    |                  |     |                |
| +-------------+       +--------------+ |    | +-------------+  |     +------------+   |
| |       **    |       |              | |    | |             |  |     ||           |   |
| |             +       +  8:B         + |    | + 8:C         +  |     |+ 7:D       +   |
| |  8:A       [D]+---+[R] MST Bridge [D]+----+[R] MST Bridge [D]+----+[R] MSTP Bridge  |
| | MSTP Bridge +       +              + |    | +             +  |     |+           +   |
| |             |       |              | |    | |             |  |     ||           |   |
| +-------------+       +--------------+ |    | +-------------+  |     |------------+   |
+----------------------------------------+    +------------------+     +----------------+

Topology IV:
+-----------------+    +-----------------+    +------------------+     +----------------+
| MST Region A    |    | MST Region B    |    | MST Region C     |     | MST Region D   |
|                 |    |                 |    |                  |     |                |
| +-------------+ |    |+--------------+ |    | +-------------+  |     |+-----------+   |
| |       **    | |    ||              | |    | |             |  |     ||           |   |
| |  8:A        + |    ||  8:B         + |    | + 8:C         +  |     |+ 8:D       +   |
| | MSTP Bridge[D]+---+[R] MST Bridge [D]+----+[R] MST Bridge [D]+-----+[R] MSTP Bridge |
| |             + |    |+              + |    | +             +  |     |+           +   |
| |             | |    ||              | |    | |             |  |     ||           |   |
| +-------------+ |    |+--------------+ |    | +-------------+  |     |+-----------+   |
+-----------------+    +-----------------+    +------------------+     +----------------+

NOTE: a switch shown ** within it is the CST Root for the spanning tree
```
#### Configuration
On all switches, starting from default configuration as required by the corresponding topology, spanning tree status to “enabled” , no MSTIs configured.
#### Description
1. Starting from the topology example I verify that all of switches are in the same MST Region and active topology matches to that shown in the example.
2. Connect and configure switches as shown in example II and III. Verify that without reboot new active topology and MST Regions boundaries matches to that shown on the picture.
3. Connect and configure switches as shown in example IV. Verify that without reboot new active topology and MST Regions boundaries matches to that shown on the picture. MST Region Two is partitioned, i.e. both switches “8:C”and “8:D” are selected as being the Root for that Region.

NOTE: a switch shown ** within it is the CST Root for the spanning tree

####2.7 CIST Regional Root Bridge Election
#### Setup
##### Topology diagram

```ditaa
Topology I:
+---------------------+      +---------------------+
|                     |      |                     |
|  MST Region A       |      |  MST Region C       |
|  +---------------+  |      |  +---------------+  |
|  |       **      |  |      |  |        $$     |  |
|  | 8:A   $$      +  |      |  +  8:C          |  |
|  | MST Bridge   [D]+--------+[R] MST Bridge   |  |
|  |               +  |      |  +               |  |
|  |               |  |      |  |               |  |
|  +-----+[D]+-----+  |      |  +-----+[D]+-----+  |
|          +          |      |          +          |
+----------|----------+      +----------|----------+
           |                            |
           |                            X
           |                            |
+----------|-----------+     +----------|----------+
|          +           |     |          |          |
|  +-----+[R]+-----+   |     |  +-------+-------+  |
|  |          $$   |   |     |  |         $$    |  |
|  | 8:B           +   |     |  +  8:D          |  |
|  | MST Bridge   [D]+--------+[R] MST Bridge   |  |
|  |               +   |     |  +               |  |
|  |               |   |     |  |               |  |
|  +---------------+   |     |  +---------------+  |
|  MST Region B        |     |  MST Region D       |
+----------------------+     +---------------------+

Topology II:
+---------------------+      +---------------------+
|                     |      |                     |
|  MST Region A       |      |  MST Region C       |
|  +---------------+  |      |  +---------------+  |
|  |       **      |  |      |  |        $$     |  |
|  | 8:A   $$      +  |      |  +  8:C          |  |
|  | MST Bridge   [D]+--------+[R] MST Bridge   |  |
|  |               +  |      |  +               |  |
|  |               |  |      |  |               |  |
|  +-----+[D]+-----+  |      |  +-----+[D]+-----+  |
|          +          |      |          +          |
+---------------------+      |          |          |
           |                 |          |          |
           |                 |          |          |
           |                 |          |          |
+----------------------+     |          |          |
|          +           |     |          +          |
|  +-----+[R]+-----+   |     |  +------[R]------+  |
|  |          $$   |   |     |  |               |  |
|  | 8:B           +   |     |  |  8:D          |  |
|  | MST Bridge   [D]+-----X----+  MST Bridge   |  |
|  |               +   |     |  |               |  |
|  |               |   |     |  |               |  |
|  +---------------+   |     |  +---------------+  |
|  MST Region B        |     |                     |
+----------------------+     +---------------------+

Topology III:
+---------------------+      +---------------------+
|  MST Region A       |      |                     |
|  +---------------+  |      |  +---------------+  |
|  |       **      |  |      |  |               |  |
|  | 8:A   $$      +  |      |  |  8:C          |  |
|  | MST Bridge   [D]+---------X|  MST Bridge   |  |
|  |               +  |      |  |               |  |
|  |               |  |      |  |               |  |
|  +-----+[D]+-----+  |      |  +-----+[R]+-----+  |
|          +          |      |          +          |
+----------|----------+      |          |          |
           |                 |          |          |
+----------|-----------------+          |          |
|          +                            +          |
|  +-----+[R]+-----+            +-----+[D]+-----+  |
|  |          $$   |            |               |  |
|  | 8:B           +            |  8:D          |  |
|  | MST Bridge   [D]+--------+[R] MST Bridge   |  |
|  |               +            |               |  |
|  |               |            |               |  |
|  +---------------+            +---------------+  |
|  MST Region B                                    |
+--------------------------------------------------+

Topology IV:
+--------------------------------------------------+
|  MST Region A                                    |
|  +---------------+            +---------------+  |
|  |       **      |            |               |  |
|  | 8:A   $$      +            |  8:C          |  |
|  | MST Bridge   [D]+--------+[R] MST Bridge   |  |
|  |               +            |               |  |
|  |               |            |               |  |
|  +-----+[D]+-----+            +-----+[D]+-----+  |
|          +                            +          |
|          |                            |          |
|          |                            |          |
|          +                            X          |
|  +-----+[R]+-----+            +---------------+  |
|  |               |            |               |  |
|  | 8:B           +            |  8:D          |  |
|  | MST Bridge   [D]+--------+[R] MST Bridge   |  |
|  |               +            |               |  |
|  |               |            |               |  |
|  +---------------+            +---------------+  |
|                                                  |
+--------------------------------------------------+

NOTE: a switch shown ** within it is the CST Root for the spanning tree
NOTE: a switch shown $$ within it is the Regional Root for the spanning tree

```
#### Configuration
On all switches, starting from default configuration as required by the corresponding topology, spanning tree status to “enabled” , no MSTIs configured.
#### Description
1. Starting from the topology example I verify that all switches stay in separate MST Regions and active topology matches to that shown on the picture. Each switch should be selected as the Root of its Region. One switch (“8:A”) should be selected as the CIST Root for all of them.
2. Modifying MST Configuration Identifier on the switches verify that topology I can transition to the topology II, then to the topology III and finally to the topology IV.
3. Modifying MST Configuration Identifier on the switches verify that active topology IV can transition back to the III, than to the II and finally to the initial example I.
4. Check that the convergence time for every expected topology change is within “2xHello Time” interval (“Hello Time” is the value configured on the CIST Root Bridge).

NOTE: a switch shown ** within it is the CST Root for the spanning tree
NOTE: a switch shown $$ within it is the Regional Root for the spanning tree

####2.8 CIST Ports Roles and States Assignment - loop resolving
#### Setup
##### Topology diagram
```ditaa
 Topology I:

   +---------------------+
   |                     |
   |                 7:5 +---+
   |        8:A          | X |
   |                 8:4 +-X-+
   |                     | X
   | 8:1     7:2     8:3 |
   +----------+----------+
     XXX      |     XXX
      |       |      |
   +---------------------+

Topology II:
  +-------------------+
  |      8:A          |
  |                   |
  |                   |
  |  8:1   8:2   7:3  |
  +--[D]---[D]---[D]--+
      |     |     |
      |     |     |
      |     |     |
  +---X-----X----[R]-+
  |  8:3   8:2   8:1 |
  |                  |
  |                  |
  |      8:B         |
  +------------------+


Topology III:
  +------------------+
  |       8:A        |
  |                  |
  |       8:1        |
  +-------[D]--------+
           |
           |
   +-------+---------+
      |          |
      |          |
  +---X---------[R]--+
  |  8:1        7:2  |
  |                  |
  |      8:B         |
  +------------------+

Topology IV:
                        +------------------------+
                        |MST Region One          |
                        |    +----------------+  |
                        |    |    8:A         |  |
           +---------------+[D]8:1 $$     8:1[D]+--------------+
           |            |    |                |  |             |
           |            |    +----------------+  |             |
           |            +------------------------+             |
+------------------------+                          +---------------------+
|          +             |                          |          |          |
|   +----+[R]+-------+   |                          | +------+[R]+------+ |
|   |     8:1        |   |                          | |       8:2       | |
|   |            8:2[D]-------------------------------X 8:1             | |
|   |     8:B        |   |                          | |       8:c       | |
|   +----------------+   |                          | +-----------------+ |
| MST Region Two         |                          | MST Region Three    |
+------------------------+                          +---------------------+

Topology V:
+-----------------------------------------------------------------------------+
|                                                MST Region One               |
|                            +----------------+                               |
|                            |    8:A         |                               |
|          +----------------[D]8:1        8:1[D]+--------------+              |
|          |                 |                |                |              |
|          |                 +----------------+                |              |
+-----------------------------------------------------------------------------+
           |                                                   |
+-----------------------------------------------------------------------------+
|          |                                                   |              |
|   +-----[R]--------+                                +--------X--------+     |
|   |     8:1        |                                |       8:2       |     |
|   |            8:2[D]------------------------------[R]8:1             |     |
|   |     8:B        |                                |       8:c       |     |
|   +----------------+                                +-----------------+     |
|                                                  MST Region Two             |
+-----------------------------------------------------------------------------+

```
#### Configuration
On all switches, starting from default configuration as required by the corresponding topology, spanning tree status to “enabled” , no MSTIs configured.
#### Description
1. Configure and connect a switch ports as shown in topology example I. Verify that port roles and states match to that shown on the picture.
2. Configure ports and connect 2 switches as shown in topology example II. Verify that port roles and states match to that shown on the picture.
3. Configure ports and connect 2 switches as shown in topology example III. Verify that port roles and states match to that shown on the picture.
4. Repeat steps 2-3 when the switches have the same MST Configuration Identifier (i.e. are in the same MST Region). Resulting active topology should not change.
5. Configure switches and their ports as shown in topology examples IV-V. Verify that port roles and states match to that shown on the picture.
6. For every topology example try to reconnect links (disconnect/connect) and verify that topology converges within “2xHello Time” period to the stable state.
NOTE: On links that have assigned Alternate role to one of it ports the transitioning to the Forwarding state for the other (Designated) port may take up to “2*Forward Delay” time interval (30 seconds with default parameters).


NOTE: a switch shown ** within it is the CST Root for the spanning tree
NOTE: a switch shown $$ within it is the Regional Root for the spanning tree

####2.9 CIST Root Bridge Parameters Propagation
#### Setup
##### Topology diagram
```ditaa

+-----------------+   +------------------------------------------+   +-------------------+
| MST Region ONE  |   |                         MST Region TWO   |   | MST Region Three  |
| +-------------- |   | +--------------+        +-------------+  |   |  +------------+   |
| |             | |   | |              |        |             |  |   |  |            |   |
| |   6:D       + |   | +     8:B      +        +    8:A      +  |   |  +    7:C     |   |
| |        8:1[D] +---+[R]8:1      8:2[D]+----+[R]8:1     8:2 [D]+----+[R]8:1        |   |
| |             + |   | +              +        +             +  |   |  +            |   |
| |             | |   | |              |        |             |  |   |  |            |   |
| +-------------+ |   | +--------------+        +-------------+  |   |  +------------+   |
+-----------------+   +------------------------------------------+   +-------------------+

```
#### Configuration
On all switches, starting from default configuration as required by the corresponding topology, spanning tree status to “enabled” , no MSTIs configured.
#### Description
1. Configure and connect switches as shown in the topology example above.
2. Verify that switch “6:D” is known to be the CIST Root on all switches.
3. Verify the operational values of Forward Delay, Max Age, Hello Time, Max Hops parameters on every switch. On each switch all parameters should be identical and equal to the default values of the switch “6:D”.
4. On the CIST Root Bridge (“6:D” switch) change the configuration of Forward Delay, Max Age, Hello Time, Max Hops parameters to the values different from default.
5. Verify that operational values of Forward Delay, Max Age, and Hello Time parameters used by all other switches are equal to those configured on the CIST Root.
6. Verify that on switches “8:B”, “8:A”and “7:C” the value of Max Hops parameter have not been changed from the default.
7. Verify that Message Age parameter of the ‘CistRootTimes‘ held on a switch is incremented by 1 on the switches “8:B” and “8:A”, incremented by 2 on the switch “7:C” and is ‘0’ on the switch “6:D”.
8. Verify that Remaining Hops parameter of the ‘CistRootTimes‘ held on a switch is as follows:
on switch “6:D” it is equal to the new value of Max Hops, on the switches “8:B” and “7:C” it is equal to default (20) value, on the switch “8:A”it is one less (19) then default.

####2.10 MST0 Regional Root Parameters Propagation
#### Setup
##### Topology diagram

```ditaa
+-----------------+   +------------------------------------------------------------------+
| MST Region ONE  |   |                         MST Region TWO                           |
| +-------------+ |   | +--------------+        +-------------+         +------------+   |
| |             | |   | |              |        |             |         |            |   |
| |   6:D       + |   | +     8:B      +        +    8:A      +         +    7:C     |   |
| |        8:1[D] +---+[R]8:1      8:2[D]+----+[R]8:1     8:2 [D]+----+[R]8:1        |   |
| |             + |   | +              +        +             +         +            |   |
| |             | |   | |              |        |             |         |            |   |
| +-------------+ |   | +--------------+        +-------------+         +------------+   |
+-----------------+   +------------------------------------------------------------------+

```
#### Configuration
On all switches, starting from default configuration as required by the corresponding topology, spanning tree status to “enabled” , no MSTIs configured.
#### Description
1. Configure and connect switches as shown in the topology example above.
2. Verify that switch “6:D” is known to be the CIST Root on all switches.
3. Verify that switch “6:D” is the Root of the MST Region One.
4. Verify that  switch “8:B” is known to be the IST Regional Root for the switches located within the MST Region Two.
5. Verify the operational values of Forward Delay, Max Age, Hello Time, Max Hops parameters on every switch. On each switch all parameters should be identical and equal to the default values configured on the switch “6D” (CIST Root).
6. On the switch “8:B” (MST Region Two Regional Root) change the configuration of Forward Delay, Max Age, Hello Time, Max Hops parameters to the values different from default.
7. Verify that on all switches the operational values of Forward Delay, Max Age, and Hello Time parameters remain unchanged and match to the values configured on the switch “6:D” (CIST Root).
8. Verify that Message Age parameter of the ‘CistRootTimes‘ held on the switches “8:B”, “8:A” and “7:C” is greater by one then on the switch “6:D” (CIST Root).
9. Verify that Remaining Hops parameter of the ‘CistRootTimes‘ held on the MST Region Two switches is as follows: on the switch “8:B” (IST Regional Root) it is equal to the new configured value, on the switch “8:A” it is less by one and on the switch “7:C” it is less by two then on the “8:B” switch.
10. Verify that active topology still matches to that shown in the above picture.
11. On the switch “8:B” (IST Regional Root) set Max Hops configuration parameter to the value of 1.
12. Verify that Remaining Hops parameter of the ‘CistRootTimes‘ held on the switch “8:A” is equal to 0.
13. Verify that switch “7:C” has the value of the Remaining Hops equal to default (20) and assumes that it is CST Root and IST Regional Root for the MST Region Two. This is an example of the MSTP parameters misconfiguration.

####2.11 MSTI Priority Vector - MSTI Configuration Message
#### Setup
##### Topology diagram

```ditaa
+----------------+                                +-----------------+
|                |                                |                 |
|     S2         <------------------------------->|        H1       |
|                |                                |                 |
+----------------+                                +-----------------+

H1 station running a “packet analyzer”software application
```
#### Configuration
On all switches, starting from default configuration as required by the corresponding topology, spanning tree status to “enabled”.
#### Description
1. Create MSTIs with VLANs being mapped as follows
| MST Instance ID | Mapped VIDs |
|:-----------|:---------- |
|1  | 1-3|
|7  | 100, 200, 300|
|16 | 400,4094|

2. . Capture a few spanning tree packets coming out of the switch.
3. Verify that all fields in the first 102 octets of the captured BPDU contain correct info. The structure of the whole MST BPDU is shown in the test case (3. CIST Priority Vector).
4. Looking at the captured BPDU verify it contains the correct number of MSTI Configuration Messages (3 in this example) located starting from the octet #103 of the BPDU.
5. Check that each MSTI Configuration Message has the following structure:

|Fields|Octet#|
|:-----------|:---------- |
| MSTI Flags | 1 |
|MSTI Regional Root Identifier | 2-9|
|MSTI Internal Root Path Cost  | 10-13|
|MSTI Bridge Priority | 14|
|MSTI Port priority | 15|
|MSTI Remaining Hops | 16|

Verify that every MSTI Configuration Message contains correct information

####2.12 MSTI Regional Root Bridge Election
#### Setup
##### Topology diagram

```ditaa
Topology I:
+-------------------------------------------------------------------------------------+
|                                                              MST Region One: MST0   |
| +-------------+       +--------------+        +-------------+         +-----------+ |
| |             |       |              |        |             |         |           | |
| |  8:A        +       +  8:B         +        + 8:C         +         + 8:D       | |
| |  $$        [D]+---+[R]            [D]+----+[R]            [D]+-----+[R]         | |
| |             +       +              +        +             +         +           | |
| |             |       |              |        |             |         |           | |
| +-------------+       +--------------+        +-------------+         +-----------+ |
+-------------------------------------------------------------------------------------+
+-------------------------------------------------------------------------------------+
|                                                             MST Region One: MST1    |
| +-------------+       +--------------+        +-------------+         +-----------+ |
| |             |       |              |        |             |         |           | |
| |  8:A        +       +  7:B         +        + 8:C         +         + 8:D       | |
| |            [R]+---+[D] $$         [D]+----+[R]            [D]+-----+[R]         | |
| |             +       +              +        +             +         +           | |
| |             |       |              |        |             |         |           | |
| +-------------+       +--------------+        +-------------+         +-----------+ |
+-------------------------------------------------------------------------------------+
+-------------------------------------------------------------------------------------+
|                                                             MST Region One: MST2    |
| +-------------+       +--------------+        +-------------+         +-----------+ |
| |             |       |              |        |             |         |           | |
| |  8:A        +       +  8:B         +        + 7:C         +         + 8:D       | |
| |            [R]+---+[D]            [R]+----+[D] $$         [D]+-----+[R]         | |
| |             +       +              +        +             +         +           | |
| |             |       |              |        |             |         |           | |
| +-------------+       +--------------+        +-------------+         +-----------+ |
+-------------------------------------------------------------------------------------+
+-------------------------------------------------------------------------------------+
|                                                             MST Region One: MST3    |
| +-------------+       +--------------+        +-------------+         +-----------+ |
| |             |       |              |        |             |         |           | |
| |  8:A        +       +  8:B         +        + 8:C         +         + 7:D       | |
| |            [R]+---+[D]            [R]+----+[D]            [R]+-----+[D] $$      | |
| |             +       +              +        +             +         +           | |
| |             |       |              |        |             |         |           | |
| +-------------+       +--------------+        +-------------+         +-----------+ |
+-------------------------------------------------------------------------------------+

Topology II:
+-----------------------------------------------------------------------------+
|                                                MST Region One: MSTI 0       |
|                            +----------------+                               |
|                            |    8:A         |                               |
|          +---------------+[D]   $$         [D]+--------------+              |
|          |                 |                |                |              |
|          |                 +----------------+                |              |
|          |                                                   |              |
|          +                                                   +              |
|   +-----[R]--------+                                +-------[R]-------+     |
|   |                |                                |                 |     |
|   |     9:B        +-X----------------------------+[D]    8:C         |     |
|   |                |                                |                 |     |
|   +----------------+                                +-----------------+     |
|                                                                             |
+-----------------------------------------------------------------------------+
+-----------------------------------------------------------------------------+
|                                                 MST Region One: MSTI 1      |
|                            +----------------+                               |
|                            |     8:A        |                               |
|          +---------------+[R]              [D]---------------+              |
|          |                 |                |                |              |
|          |                 +----------------+                |              |
|          |                                                   |              |
|          |                                                   X              |
|   +-----[D]--------+                                +-----------------+     |
|   |                |                                |                 |     |
|   |    7:B        [D]-----------------------------+[R]    8:C         |     |
|   |    $$          |                                |                 |     |
|   +----------------+                                +-----------------+     |
|                                                                             |
+-----------------------------------------------------------------------------+
+-----------------------------------------------------------------------------+
|                                                  MST Region One: MSTI 2     |
|                            +----------------+                               |
|                            |     8:A        |                               |
|          +----------------[D]              [R]---------------+              |
|          |                 |                |                |              |
|          |                 +----------------+                |              |
|          |                                                   |              |
|          X                                                   |              |
|   +----------------+                                +-------[D]-------+     |
|   |                |                                |                 |     |
|   |    8:B        [R]------------------------------[D]     7:C        |     |
|   |                |                                |      $$         |     |
|   +----------------+                                +-----------------+     |
|                                                                             |
+-----------------------------------------------------------------------------+


NOTE: a switch shown $$ within it is the Regional Root for the spanning tree
```
#### Configuration
On all switches, starting from default configuration as required by the corresponding topology, spanning tree status to “enabled”.
#### Description
Using topology example I connect 4 switches and create on every switch 3 MSTI entries, MSTI 1, MSTI 2 and MSTI 3.
2. Configure Bridge Priority value for each MSTI as shown in the topology example I.
3. Verify that within MST Region One the active topology built by each spanning tree instance (the IST and MSTIs) matches to what is shown in the topology I example, i.e. each instance has different Regional Root switch. For each spanning tree instance the active topology should converge within “2xHello Time” interval and remain stable for unlimited time.
4. Connect and configure 3 switches as shown in the topology example II.
5. Verify that the active topology built by the IST instance and each MSTI matches to the picture.
6. Repeat steps 1-5 for MST Instances with different IDs, e.g. MSTI 5, MSTI 7, MSTI 16.

NOTE: a switch shown $$ within it is the Regional Root for the spanning tree

####2.13 MSTI Ports Roles and States Assignment - loop resolving
#### Setup
##### Topology diagram

```ditaa
 Topology I:
+--------------------------------+
|MST Region One: MSTI 1          |
|   +---------------------+      |
|   |                     |      |
|   |                 7:5 +---+  |
|   |        8:A          | X |  |
|   |                 8:4 +-X-+  |
|   |                     | X    |
|   | 8:1     7:2     8:3 |      |
|   +----------+----------+      |
|     XXX      |     XXX         |
|      |       |      |          |
|   +------------------------+   |
+--------------------------------+

 Topology II:
+--------------------------+
|MST Region One: MSTI 1    |
|  +-------------------+   |
|  |      8:A          |   |
|  |      $$           |   |
|  |                   |   |
|  |  8:1   8:2   7:3  |   |
|  +--[D]---[D]---[D]--+   |
|      |     |     |       |
|      |     |     |       |
|      |     |     |       |
|  +---X-----X-----X--+    |
|  |  8:3   8:2   8:1 |    |
|  |                  |    |
|  |                  |    |
|  |      8:B         |    |
|  +------------------+    |
+--------------------------+

 Topology III:
+--------------------------+
|MST Region One: MSTI 1    |
|  +------------------+    |
|  |      8:A         |    |
|  |      $$          |    |
|  |       8:1        |    |
|  +-------[D]--------+    |
|           |              |
|           |              |
|   +-------+---------+    |
|      |          |        |
|      |          |        |
|  +---X---------[R]--+    |
|  |   8:1       7:2  |    |
|  |                  |    |
|  |      8:B         |    |
|  +------------------+    |
+--------------------------+

Topology IV:
+-----------------------------------------------------------------------------+
|                                                MST Region One: MSTI 1       |
|                            +----------------+                               |
|                            |    8:A  $$     |                               |
|          +----------------[D]8:1 $$     8:1[D]+--------------+              |
|          |                 |                +                |              |
|          |                 +----------------+                |              |
|          |                                                   |              |
|          |                                                   |              |
|   +-----[R]--------+                                +------+[R]+------+     |
|   |     8:1        |                                |       8:2       |     |
|   |            8:2[D]-------------------------------X 8:1             |     |
|   |     8:B        |                                |       8:c       |     |
|   +----------------+                                +-----------------+     |
|                                                                             |
+-----------------------------------------------------------------------------+

Topology V:
+-----------------------------------------------------------------------------+
|                                                MST Region One: MSTI 1       |
|                            +----------------+                               |
|                            |    8:A  $$     |                               |
|          +----------------[D]8:1 $$     8:1[D]+--------------+              |
|          |                 |                |                |              |
|          |                 +----------------+                |              |
+-----------------------------------------------------------------------------+
           |                                                   |
+-----------------------------------------------------------------------------+
|          |                                                   |              |
|   +-----[M]--------+                                +--------X--------+     |
|   |     8:1        |                                |       8:2       |     |
|   |     $$     8:2[D]------------------------------[R]8:1             |     |
|   |     8:B        |                                |       8:c       |     |
|   +----------------+                                +-----------------+     |
|                                                  MST Region Two: MSTI 1     |
+-----------------------------------------------------------------------------+

NOTE: a switch shown $$ within it is the Regional Root for the spanning tree
```
#### Configuration
On all switches, starting from default configuration, protocol version is set to “mstp”, protocol administrative status is “enabled”.
#### Description
1. Configure and connect a switch ports as shown in the topology example I. Verify that port roles and states match to that shown on the picture.
2. Configure ports and connect 2 switches as shown in the topology example II. Verify that port roles and states match to that shown on the picture.
3. Configure ports and connect 2 switches as shown in the topology example III. Verify that port roles and states match to that shown on the picture.
4. Repeat steps 2-3 for MSTI with different IDs, e.g. MSTI 3, MSTI 16. Resulting active topology should be identical for any MSTI.
5. Configure switches and their ports as shown in topology examples IV-V. Verify that port roles and states match to that shown on the picture.
6. For every topology example try to reconnect links (disconnect/connect) and verify that topology converges within “2xHello Time” period to the stable state.
7. For every configuration and topology example reboot the switches and verify after reboot that port roles and states match to that depicted in the example
NOTE: a switch shown $$ within it is the Regional Root for the spanning tree

####2.14 Fault Tolerance - CIST
#### Setup
##### Topology diagram

```ditaa
Topology I:
+-----------------------------------+
|                                   |
|     MST Region One                |
|  +---------------------------+    |
|  |                           |    |        +--------------+
|  |      8:A                  |    |        |  Host H1     |
|  |                          [E]------------>              |
|  |                           |    |        |              |
|  |                           |    |        +--------------+
|  | 8:1  8:2  8:3  8:4  8:5   |    |
|  +-[D]--[D]--[D]--[D]--[D]---+    |
|     |    |    |    |    |         |
+-----------------------------------+
      |    |    |    |    |
      |    |    |    |    |
+-----|----|----|----|----|---------+
|     |    X    X    X    X         |
|  +-[R]------------------------+   |
|  | 8:1  8:2  8:3  8:4  8:5    |   |         +--------------+
|  |                            |   |         | Host H2      |
|  |                            |  [E]-------->              |
|  |                            |   |         |              |
|  |      8:C                   |   |         +--------------+
|  |                            |   |
|  +----------------------------+   |
|     MST Region Two                |
+-----------------------------------+

Topology II:
             +-----------------------+
             |    MST Region One     |        +----------+
             |   +--------------+    |        |          |
             |   |             [E]------------>          |
             |   |    8:A       |    |        |   Host 1 |
           +----[D]8:1     8:2 [D]----+       +----------+
           | |   +--------------+    ||
           | |                       ||
           | +-----------------------+|
           |                          |
+--------------------+      +---------|-------------+
|          |         |      |         |             |
|    +----[R]----+   |      |   +----[R]-----+      |
|    |    8:1    |   |      |   |    8:1     |      |
|    |8:B    8:2[D]-----------X-|8:2    8:C  |      |
|    |   8:3     |   |      |   |    8:3     |      |
|    +---[D]-----+   |      |   +----[D]-----+      |
|         |          |      |         |             |
|MST Region Two      |      | MST Region Three      |
+--------------------+      +-----------------------+
          |                           |
          |   +--------------------+  |
          |   |                    |  |
          |   |  +---------------+ |  |
          +-----[R]8:1        8:2|X---+        +-----------+
              |  |              [E]----------->| Host 2    |
              |  |    8:D        | |           |           |
              |  +---------------+ |           +-----------+
              |                    |
              |   MST Region Four  |
              +--------------------+

```
#### Configuration
On all switches, starting from default configuration as required by the corresponding topology, spanning tree status to “enabled”.
#### Description
1. Configure switches and connect the switches ports as shown in the topology example I. Verify that port roles and states match to that shown on the picture.
2. Start “ping” Host 1 <-> Host 2. Verify that on both hosts each ICMP Echo Request packet is echoed back via an ICMP Echo Response packet, i.e. connectivity between hosts is established.
3. Starting from port “8:1”, disconnect one by one links between switches “8:A”and “8:B”.
Verify that “ping” still succeeds and connectivity recovery time does not exceeds the 2 * Hello Time interval. Doing links disconnection check that all ports states and roles matches to what is expected.
4. One by one restore disconnected links back and verify that “ping” still succeeds and connectivity recovery time does not exceeds the 2 * Hello Time interval. Doing links disconnection check that all ports states and roles matches to what is expected.
6. Configure switches and connect the switches ports as shown in the topology example II. Verify that port roles and states match to that shown on the picture.
7. Start “ping” Host 1 <-> Host 2. Verify that on both hosts each ICMP Echo Request packet is echoed back via an ICMP Echo Response packet, i.e. connectivity between hosts is established.
8. Randomly break links between switches so that theoretically at least one path is always available between the hosts. Verify that “ping” still succeeds and connectivity recovery time does not exceeds the 2 * Hello Time interval. Doing links disconnection check that all ports states and roles matches to what is expected.
9. Restore initial topology example II. Verify that “ping” still succeeds and connectivity recovery time does not exceeds the 2 * Hello Time interval. Verify that port roles and states match to that shown on the picture.
10. Shutdown (power off) switch “8:B”. Verify that “ping” still succeeds and connectivity recovery time does not exceeds the 2 * Hello Time interval.
11. Bring up (power on) switch “8:B”. Verify that “ping” still succeeds and connectivity recovery time does not exceeds the 2 * Hello Time interval.
12. Repeat steps 10-11 with the switch “8:C”.

####2.15 Fault Tolerance - MSTI
#### Setup
##### Topology diagram

```ditaa
Topology I:
+-----------------------------------+
|     MST Region One                |
|  +---------------------------+    |
|  |                           |    |        +--------------+
|  |      8:A                  |    |        |  Host H1     |
|  |                          [E]----------->|              |
|  |                           |    |        +--------------+
|  | 8:1  8:2  8:3  8:4  8:5   |    |
|  +-[D]--[D]--[D]--[D]--[D]---+    |
|     |    |    |    |    |         |
|     |    |    |    |    |         |
|     |    |    |    |    |         |
|     |    X    X    X    X         |
|  +-[R]------------------------+   |
|  | 8:1  8:2  8:3  8:4  8:5    |   |         +--------------+
|  |                            |   |         | Host H2      |
|  |                            |  [E]------->|              |
|  |                            |   |         |              |
|  |      8:C                   |   |         +--------------+
|  +----------------------------+   |
+-----------------------------------+

Topology II:
+--------------------------------------------+
|   MST Region One                           |
|                                            |   +----------+
|                   +--------------+         |   |          |
|                   |             [E]----------->|          |
|                   |    8:A       |         |   |   Host 1 |
|             +----[D]8:1     8:2 [D]----+   |   +----------+
|             |     +--------------+     |   |
|             |                          |   |
|             |                          |   +--------+
|             |                          |            |
|       +----[R]----+              +----[R]-----+     |
|       |    8:1    |              |    8:1     |     |
|       |8:B    8:2[D]------------X|8:2    8:C  |     |
|       |   8:3     |              |    8:3     |     |
|       +---[D]-----+              +----[D]-----+     |
|            |                           |            |
|            |                           |            |
|            |                           |   +--------+
|            |                           |   |
|            |      +---------------+    |   |
|            +-----[R]8:1        8:2|X---+   |    +-----------+
|                   |              [E]----------->| Host 2    |
|                   |    8:D        |        |    |           |
|                   +---------------+        |    +-----------+
+--------------------------------------------+

```
#### Configuration
On all switches, starting from default configuration as required by the corresponding topology, spanning tree status to “enabled” , no MSTIs configured.
#### Description
1. Configure few MSTIs, e.g. MSTI 1, MSTI 7, MSTI 16.
2. For each VLAN that is mapped to an MSTI configure an IP address.
3. Repeat the logic from the previous test and make sure that the connectivity is exist for all VLANs and is recoverable after a link failure within the expected time (“2*Hello Time”, by default that is 4 seconds).

####2.16 Topology Changes Detection - CIST
#### Setup
##### Topology diagram

```ditaa
                          +----------------------+
                          |End station running a |
                          |“packet analyzer”     |
                          +--------+-------------+
                                   |
                                   |
+-----------------+            +---+----+                 +------------------+
|           8:1  [D]-----------+  HUB   +----------------[R] 8:1             |
|   8:A           |            +--------+                 |       8:B        |
|   **            |                                       |                  |
|                 |                                       |                  |
|           8:2  [D]-------------------------------------X|  8:2             |
+-----------------+                                       +------------------+

```
#### Configuration
on all switches, starting from default configuration, protocol version is set to “mstp”, protocol administrative status is “enabled”, participating ports are configured to be non-Edge, CST path-cost for all participating ports is manually configured to 2000000, no MSTIs configured.
#### Description
1. Configure and connect switches as shown in the topology example.
2. Verify that resulting active topology match to that shown on the picture.
3. Disconnect port “8:1” of the switch “8:A” and wait >=30 seconds.
4. Start packets capture.
5. Connect port “8:1” of the switch “8:A”. Looking at the capture buffer wait until switch starts to send BPDUs with the ‘Topology Change’ bit set in the ‘flags’ field.
6. Stop packets capture.
7. Look at all BPDUs transmitted by the switch “8:A” on port “8:1” and verify that the ‘Topology Change’ bit was first set when the ‘Forwarding’ bit was set, i.e. switch detected the topology change when its port “8:1” went to the forwarding state.
8. Repeat steps 3-7 with the port “8:1” of the switch “8:B”. Verify that the result is the same.
9. Disable spanning tree protocol on the “8:A”switch.
10. Start packets capture.
11. Enable spanning tree protocol on the “8:A”switch.
12. Looking at the capture buffer wait until switch “8:A” starts to send BPDUs with the ‘Topology Change’ bit set in the ‘flags’ field.
13. Stop packets capture.
14. Look at all BPDUs transmitted by the switch “8:A” on port “8:1” and verify that the ‘Topology Change’ bit was first set when the ‘Forwarding’ bit was set, i.e. switch detected the topology change when its port “8:1” went to the forwarding state.
15. Verify that TCN messages are being sent no longer than twice of the Hello Time interval.
16. Repeat steps 9-15 with “8:B” switch. Verify that the result is the same.
17. Configure and connect switches as shown in the original topology example. Verify that resulting active topology match to that shown on the picture.
18. Start packets capture.
19. Disconnect any side of the link that connects “8:2” ports of the switches.
20. Wait >=30 seconds.
21. Stop packets capture.
22. In the capture buffer you should not see any BPDU with the ‘Topology Change’ bit set.

####2.17 Topology Changes Propagation - CIST
#### Setup
##### Topology diagram

```ditaa
+-------------------+       +-------------------+                    +------------------+
|      8:A          |       |       8:B         |     +--------+     |      8:C         |
|      **       8:1[D]-----[R]8:2           8:1[D]----+  HUB   +----[R]8:1              |
|                   |       |                   |     +----+---+     |                  |
|  8:2         8:3  |       |                   |          |         |                  |
+--[D]--------------+       +-------------------+          |         +------------------+
    |          XXX                                         |
    |           |                                          |
    |           |                                          |
    +-----------+                                   +------+------------+
                                                    |End station running|
                                                    |“packet analyzer”” |
                                                    +-------------------+
```
#### Configuration
on all switches, starting from default configuration, protocol version is set to “mstp”, protocol administrative status is “enabled”, participating ports are configured to be non-Edge, CST path-cost for all participating ports is manually configured to 2000000, no MSTIs configured.
#### Description
1. Configure and connect switches as shown in the topology example.
2. Verify that resulting active topology match to that shown on the picture.
3. Start packets capture.
4. Disconnect port “8:3”of the switch “8:A” and wait >= 30seconds.
5. Stop packets capture.
6. In the capture buffer you should not see any BPDU with the ‘Topology Change’ bit set.
7. Start packets capture.
8. Connect port “8:3”of the switch “8:A” and wait for about 30 seconds to let the port  “8:2” become forwarding, that will trigger switch “8:A” to generate TCN messages on its port “8:1”.
9. Stop packets capture.
10. Look at the capture buffer; you should see BPDUs with the ‘Topology Change’ bit set propagated by the switch “8:B”over its port “8:1”.
11. Verify that TCN messages are being propagated no longer than twice of the Hello Time interval.
12. Start packets capture.
13. Reconnect port “8:1”of the switch “8:A” and wait ~10 seconds.
14. Stop packets capture.
15. Look at the capture buffer; you should see BPDUs with the ‘Topology Change’ bit set propagated by the switch “8:B”over its port “8:1”.
16. Verify that TCN messages are being propagated no longer than twice of the Hello Time interval.

####2.18 Topology Changes Detection - MSTI
#### Setup
##### Topology diagram

```ditaa
                              +----------------------+
                              |End station running a |
                              |“packet analyzer”     |
                              +--------^-------------+
+--------------------------------------|-----------------------------------------------------+
|                                      |                  MST Region One: MSTI 1,2,3,4,5 6,7 |
|   +-----------------+            +---+----+                 +------------------+           |
|   |           8:1  [D]-----------+  HUB   +----------------[R] 8:1             |           |
|   |   8:A           |            +--------+                 |       8:B        |           |
|   |   **            |                                       |                  |           |
|   |                 |                                       |                  |           |
|   |           8:2  [D]------------------------------------+X|  8:2             |           |
|   +-----------------+                                       +------------------+           |
|                                                                                            |
+--------------------------------------------------------------------------------------------+

```
#### Configuration
On all switches, starting from default configuration, protocol version is set to “mstp”, protocol administrative status is “enabled”, participating ports are configured to be non-Edge, both switches are in the same region, few MSTIs are configured on both switches, path-cost for all participating ports is manually configured to 2000000 for the IST and all MSTIs.
#### Description
1. Configure and connect switches as shown in the topology example.
2. Verify that resulting active topology match to that shown on the picture.
3. Disconnect port “8:1” of the switch “8:A” and wait >=30 seconds.
4. Start packets capture.
5. Connect port “8:1” of the switch “8:A”. Looking at the capture buffer wait until switch starts to send BPDUs with the ‘Topology Change’ bit set in the ‘flags’ field.
6. Stop packets capture.
7. Look at all BPDUs transmitted by the switch “8:A” on port “8:1” and verify that the ‘Topology Change’ bit was first set when the ‘Forwarding’ bit was set, i.e. switch detected the topology change when its port “8:1” went to the forwarding state.
8. Verify that the TCN BPDU has the ‘Topology Change’ bit set in the ‘flags’ field in each MSTI Configuration Message carried in the BPDU.
NOTE: The particular TCN BPDU may not contain ‘Topology Change’ bit for every MSTI encoded in that BPDU. The important thing is that in all BPDUs following the first TCN BPDU within 2 * Hello Time interval the ‘Topology Change’ bit should be present for each MSTI at least one time.
9. Repeat steps 3-8 with the port “8:1” of the switch “8:B”. Verify that the result is the same.
10. Disable spanning tree protocol on the “8:A”switch.
11. Start packets capture.
12. Enable spanning tree protocol on the “8:A”switch.
13. Looking at the capture buffer wait until switch “8:A” starts to send BPDUs with the ‘Topology Change’ bit set in the ‘flags’ field.
14. Stop packets capture.
15. Look at all BPDUs transmitted by the switch “8:A” on port “8:1” and verify that the ‘Topology Change’ bit was first set when the ‘Forwarding’ bit was set, i.e. switch detected the topology change when its port “8:1” went to the forwarding state.
16. Verify that the TCN BPDU has the ‘Topology Change’ bit set in the ‘flags’ field in each MSTI Configuration Message carried in the BPDU.
NOTE: The particular TCN BPDU may not contain ‘Topology Change’ bit for every MSTI encoded in that BPDU. The important thing is that in all BPDUs following the first TCN BPDU within 2 * Hello Time interval the ‘Topology Change’ bit should be present for each MSTI at least one time.
17. Verify that TCN messages are being sent no longer than twice of the Hello Time interval.
18. Repeat steps 11-17 with “8:B” switch. Verify that the result is the same.
19. Configure and connect switches as shown in the original topology example. Verify that resulting active topology match to that shown on the picture.
20. Start packets capture.
21. Disconnect any side of the link that connects “8:2” ports of the switches.
22. Wait >=30 seconds.
23. Stop packets capture.
24. In the capture buffer you should not see any BPDU with the ‘Topology Change’ bit set.

####2.19 Topology Changes Propagation - MSTI
#### Setup
##### Topology diagram

```ditaa
+--------------------------------------------------------------------------------------------+
|                                                        MST Region One: MSTI 1,2,3,4,5,6,7  |
| +-------------------+       +-------------------+                    +------------------+  |
| |      8:A          |       |       8:B         |     +--------+     |      8:C         |  |
| |      **       8:1[D]-----[R]8:2           8:1[D]----|  HUB   |----[R]8:1              |  |
| |                   |       |                   |     +----^---+     |                  |  |
| |  8:2         8:3  |       |                   |          |         |                  |  |
| +--[D]--------------+       +-------------------+          |         +------------------+  |
|     +          XXX                                         |                               |
|     |           |                                          |                               |
|     |           |                                          |                               |
|     +-----------+                                   +-------------------+                  |
|                                                     |End station running|                  |
|                                                     |“packet analyzer”” |                  |
|                                                     +-------------------+                  |
+--------------------------------------------------------------------------------------------+

```
#### Configuration
On all switches, starting from default configuration, protocol version is set to “mstp”, protocol administrative status is “enabled”, participating ports are configured to be non-Edge, both switches are in the same region, few MSTIs are configured on both switches, path-cost for all participating ports is manually configured to 2000000 for the IST and MSTIs.
#### Description
1. Configure and connect switches as shown in the topology example.
2. Verify that resulting active topology match to that shown on the picture.
3. Start packets capture.
4. Disconnect port “8:3”of the switch “8:A” and wait >= 30seconds.
5. Stop packets capture.
6. In the capture buffer you should not see any BPDU with the ‘Topology Change’ bit set.
7. Start packets capture.
8. Connect port “8:3”of the switch “8:A” and wait for about 30 seconds to let the port  “8:2” become forwarding and initiate the switch “8:A”to start generate TCN messages on its port “8:1”.
9. Stop packets capture.
10. Look at the capture buffer; you should see BPDUs with the ‘Topology Change’ bit set propagated by the switch “8:B”over its port “8:1”. Verify that in BPDUs following the first transmitted TCN BPDU within 2 * Hello Time interval the ‘Topology Change’ bit is set in the ‘flags’ field in each MSTI Configuration Message carried in that BPDU.
NOTE: The particular TCN BPDU may not contain ‘Topology Change’ bit for every MSTI encoded in that BPDU. The important thing is that in all BPDUs transmitted after the first TCN BPDU within 2 x Hello Time interval the ‘Topology Change’ bit should be present for each MSTI at least one time.
11. Verify that TCN BPDUs are being propagated no longer than approximately twice of the Hello Time interval.
12. Start packets capture.
13. Reconnect port “8:1”of the switch “8:A” and wait ~10 seconds.
14. Stop packets capture.
15. Look at the capture buffer; you should see BPDUs with the ‘Topology Change’ bit set propagated by the switch “8:B”over its port “8:1”. Verify that in BPDUs following the first transmitted TCN BPDU within 2 x Hello Time interval the ‘Topology Change’ bit is set in the ‘flags’ field in each MSTI Configuration Message carried in that BPDU.
NOTE: The particular TCN BPDU may not contain ‘Topology Change’ bit for every MSTI encoded in that BPDU. The important thing is that in all BPDUs transmitted after the first TCN BPDU within 2 x Hello Time interval the ‘Topology Change’ bit should be present for each MSTI at least one time.
16. Verify that TCN messages are being propagated no longer than approximately twice of the Hello Time interval.
