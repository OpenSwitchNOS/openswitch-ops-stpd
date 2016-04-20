# MSTP Test Cases
## Contents

- [CIST Root Bridge Election](#cist-root-bridge-election)
- [CIST Root Bridge Election in multiple region](#cist-root-bridge-election-in-multiple-region)
- [MSTI Regional Root Bridge Election](#msti-regional-root-bridge-election)
- [MSTI Regional Root Bridge Election in multi region](#msti-regional-root-bridge-election-in-multi-region)
- [Fault Tolerance in CIST](#fault-tolerance-in-cist)


## CIST Root Bridge Election
#### Objective
This test case confirms that the CIST root is selected correctly in single region topology and port states and role update accordingly.
#### Requirements
- Physical Switch/Switch Test setup
- **FT File**:

#### Setup
##### Topology diagram
```ditaa
                         +----------------+
                         |                |
       +---------------INT1     S1       INT2--------------+
       |                 |                |                |
       |                 +----------------+                |
       |                                                  ---
       |                                                   |
+-----INT1-------+                                +------INT1-------+
|                |                                |                 |
|     S2        INT2----------------------------INT2       S3       |
|                |                                |                 |
+----------------+                                +-----------------+


---  Blocking  link
```

#### Description
1. Setup the topology as show and enable the interfaces and make it L2 interface.
2. Active spanning tree on all the switch and wait till the convergence time for loop solving. Topology should be built and become stable in about “2xHello Time” period.
3. Use CLI commands for configuration changes and for displaying spanning tree status information. Every topology should be built and become stable in about “2xHello Time” period.
4. In this case switch with lowest mac becomes the CIST root.
5. One of the port in switch with highest mac becomes blocking and remaining ports of all the switch become forwarding.

### CIST Root Bridge Election in multiple region
#### Objective
This test case confirms that the CIST root and regional root is selected correctly in multi region topology and port states and role update.
#### Requirements
- Physical Switch/Switch Test setup
- **FT File**:

#### Setup
##### Topology diagram
```ditaa
+------------------------+  +-------------------------------------------------+
|                        |  | +----------------+                              |
|                        |  | |                |                              |
|           +-------------- -INT1     S1      INT2--------------+             |
|           |            |  | |                |                |             |
|           |            |  | +----------------+                |             |
|           |            |  |                                  ---            |
|           |            |  |                                   |             |
|    +----INT1--------+  |  |                          +------INT1-------+    |
|    |                |  |  |                          |                 |    |
|    |     S2       INT2------------------------------INT2      S3       |    |
|    |                |  |  |                          |                 |    |
|    +----------------+  |  |                          +-----------------+    |
|                        |  |                                                 |
|                        |  |                                                 |
|   Region One           |  |                 Region Two                      |
+------------------------+  +-------------------------------------------------+

---  Blocking  link

```

#### Description
1. Setup the topology as show and enable the interfaces and make it L2 interface.
2. Configure S1 and S3 in mstp region One and configure S2 in mstp region 2.
3. Active spanning tree on all the switch and wait till the convergence time for loop solving. Topology should be built and become stable in about “2xHello Time” period.
4. Consider S1 with lowest mac, and S1 becomes the CIST root of all the switchs.
5. Now S2 becomes the regional root of Region One and s1 becomes the regional root of region Two.

### MSTI Regional Root Bridge Election
#### Objective
This test case confirms that the CIST root and MSTI root and regional root are elcted correctly and port state and role update correctly.
#### Requirements
- Physical Switch/Switch Test setup
- **FT File**:

#### Setup
##### Topology diagram
```ditaa
                         +----------------+
                         |                |
       +--------------+INT1     S1       INT2+-------------+
       |                 |  Priority:8    |                |
       |                 +----------------+                |          MST Region One: CIST
       |                                                   |
       +                                                   +
+----+INT1+------+                                +-----+INT1+------+
|                |                                |                 |
|     S2        INT2+--|------------------------+INT2       S3      |
| Priority:9     |                                |   Priority:8    |
+----------------+                                +-----------------+

                         +----------------+
                         |                |
       +--------------+INT1     S1       INT2+-------------+
       |                 |   Priority:8   |                |           MST Region One: MST1
       |                 +----------------+                |
       |                                                  ---
       +                                                   +
+----+INT1+------+                                +-----+INT1+------+
|                +                                +                 |
|     S2        INT2+--------------------------+INT2       S3       |
| Priority:7     +                                +   Priority:8    |
+----------------+                                +-----------------+

                         +----------------+
                         |                |
       +--------------+INT1     S1       INT2+-------------+
       |                 |  Priority:8    |                |           MST Region One: MST2
      ---                +----------------+                |
       |                                                   |
       +                                                   +
+----+INT1+------+                                +-----+INT1+------+
|                +                                +                 |
|     S2        INT2+--------------------------+INT2       S3       |
|  Priority:8    +                                +   Priority:7    |
+----------------+                                +-----------------+

---  Blocking  link
```

#### Description
1. Setup the topology as show and enable the interfaces and make it L2 interface.
2. Configure Bridge Priority value for each MSTI as shown in topology.
3. Verify that within MST Region One the active topology built by each spanning tree instance (the IST and MSTIs) matches to what is shown in the topology, i.e. each instance has different Regional Root switch.
4. In each instance the switch configured with lowest priority becomes the reional root.

### MSTI Regional Root Bridge Election in multi region
#### Objective
This test case confirms that the CIST root and MSTI root and regional root are elected correctly with switches in multi region and port state and role update correctly.
#### Requirements
- Physical Switch/Switch Test setup
- **FT File**:

#### Setup
##### Topology diagram
```ditaa
 +----------------------------------------------------------------------+
 |  MST Region One: MSTI 1                                              |
 |                                                                      |
 |                         +----------------+                           |
 |                         |                |                           |
 |       +--------------+INT1     S1       INT2+-------------+          |
 |       |                 |  Priority:8    |                |          |
 |       |                 +----------------+                |          |
 +----------------------------------------------------------------------+
         |                                                   |
         |                                                  ---
         |                                                   |
+--------+---------------------------------------------------+-----------+
| +----+INT1+------+                                +-----+INT1+------+  |
| |                |                                |                 |  |
| |     S2        INT2+--------------------------+INT2       S3       |  |
| |  Priority:8    |                                |   Priority:8    |  |
| +----------------+                                +-----------------+  |
|                                                                        |
|   MST Region Two: MSTI 1                                               |
+------------------------------------------------------------------------+

---  Blocking  link
```

#### Description
1. Setup the topology as show and enable the interfaces and make it L2 interface.
2. Create a instance with switch that are in different region and configure Bridge Priority value for MSTI as shown in topology.
3. Verify that within MST Region One and Two the active topology built by each spanning tree instance (the IST and MSTIs) matches to what is shown in the topology, i.e. MSTI instance has one Root and each region has its regional root.
4. Consider S1 has the lowest mac, S1 becomes the root of MST1 and regional root of region One.
5. Consider S2 has the lowest mac in region Two, S2 becomes the regional root of region Two.

### Fault Tolerance in CIST
#### Objective
This test case confirms if the active link goes down, the spanning tree topology recovers and continue data traffic.
#### Requirements
- Physical Switch/Switch Test setup
- **FT File**:

#### Setup
##### Topology diagram
```ditaa
+-------------------------------------------+
|                                           |
|                     Region One            |
|                                           |
|  +-----------------------+                |     +-----------+
|  |      S1               |                |     |           |
|  |                       |                |     |           |
|  |                      6<---------------------->   host1   |
|  |                       |                |     |           |
|  |  1   2  3  4  5       |                |     +-----------+
|  +--^---^--^--^--^-------+                |
|     |   |  |  |  |                        |
|     |   |  |  |  |                        |
+-------------------------------------------+
      |   |  |  |  |
      |   |  |  |  |
      |   |  |  |  |
      |   |  |  |  |
      |  ------------
      |   |  |  |  |
      |   |  |  |  |
+---------------------------------------------+
|     |   |  |  |  |                          |
|     |   |  |  |  |      Region Two          |
|     |   |  |  |  |                          |
|   +-v---v--v--v--v--------+                 |    +------------+
|   | 1   2  3  4  5        |                 |    |            |
|   |                       |                 |    |            |
|   |                      6<---------------------->   host2    |
|   |                       |                 |    |            |
|   |      s2               |                 |    +------------+
|   +-----------------------+                 |
|                                             |
|                                             |
+---------------------------------------------+

---  Blocking  link
```

#### Description
1. Setup the topology as show and enable the interfaces and make it L2 interface.
2. Configure switches and connect the switches ports as shown in the topology . Verify that port roles and states match to that shown in topology.
3. Start “ping” Host 1 <-> Host 2. Verify that on both hosts each ICMP Echo Request packet is echoed back via an ICMP Echo Response packet, i.e. connectivity between hosts is established
4. Disconnect links between port 1 os S1 and port 1 of S2. Verify that “ping” still succeeds and connectivity recovery time does not exceeds the 2 * Hello Time interval. During links disconnection check that all ports states and roles matches to what is expected.
