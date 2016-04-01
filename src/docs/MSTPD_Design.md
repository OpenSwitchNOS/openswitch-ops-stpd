High level design of ops-stpd
==============================

The ops-stpd process manages to avoid bridge loops (multiple paths linking one segment to another, resulting in an infinite loop situation).
(see [MSTP Daemon design](/documents/user/mstpd_design) for feature design information).

Responsibilities
----------------
The ops-stpd process is responsible for managing all MSTP Instances defined by the user.

Design choices
--------------
N/A

Relationships to external OpenSwitch entities
---------------------------------------------
+-------------+      +-------------+       +----+
|             +------>Interfaces   +       +    +
|             |      +			   +-------+PEER|
|             |      +-------------+       +----+
| ops-stpd    |
|             |
|             |
|             |
|             |       +--------------+
|             +------->              |
+-------------+       |              |
                      |   database   |
                      |              |
+-------------+       |              |
|             +------->              |
|             |       |              |
|             |       +--------------+
|  switchd    |
|             |       +--------+
|             +------->        |
|             |       | SWITCH |
+-------------+       |        |
                      +--------+

The ops-stpd process monitors the MSTP related Schema in the database. As the configuration and state information for the ports and interfaces are changed, ops-stpd examines the data to determine if there are new Instances being defined in the configuration and if state information for interfaces has changed. The ops-stpd process uses this information to manage MSTP Instances and update the MSTP protocol state machines.

The ops-stpd process registers for MSTP protocol packets on all L2 interfaces configured for MSTP, and sends and receives state information to the peer through the interfaces.

When the state information maintained by ops-stpd changes, it updates the information in the database. Some of this information is strictly status, but this also includes hardware configuration, which is used by switchd to configure the switch.

OVSDB-Schema
------------
The following OpenSwitch database schema elements are referenced or set by ops-stpd:
Bridge Table:
	mstp_instances : References to MSTP Instances
	mstp_common_instance : References to MSTP Common Instance
	mstp_enable : to determine MSTP enable or disable

MSTP_Instance:
	vlans : VLANS associated to the particular MSTP Instance
	priority : to determine MSTP Instance priority
	bridge_identifier : to set the bridge identifier for this instance.
	mstp_instance_ports : Reference to PORTs associated to this Instance.
	hardware_grp_id : to store Hardware Group ID to this instance
	designated_root : Designated root for this instance
	root_path_cost : Path cost to the root for this instance
	root_priority : Priority of the root in this instance
	root_port : Port which is connected to Root.
	time_since_top_change : To store the time since the topology has changed
	top_change_cnt : to keep track of the number of topology Changes.
	topology_change_disable: this is will be set when topology change happen.

MSTP_Common_Instance:
	vlans : VLANS associated to the particular MSTP Instance
	priority : to determine MSTP Instance priority
	bridge_identifier : to set the bridge identifier for this instance.
	hello_time : Hello time for Common instance
	forward_delay : Forward delay time for Common instance
	max_age : Max Age for Common instance
	tx_hold_count : TX hold count for Common instance
	mstp_common_instance_ports : References to ports associated with MSTP Common Instance
	hardware_grp_id : to store Hardware Group ID to this instance
	designated_root : Designated root for this instance
	root_path_cost : Path cost to the root for this instance
	root_priority : Priority of the root in this instance
	root_port : Port which is connected to Root.
	regional_root : To Store the Regional Root for the CST
	cist_path_cost : Path cost for the CIST
	remaining_hops : to keep track of the number of remaining hops
	oper_hello_time : To set the Oper Hello time based on the priority vectors
	oper_forward_delay : To set the Oper Forward Delay based on the priority vectors
	oper_max_age : to set the Max Age based on the priority vectors
	hello_expiry_time : to keep track of Hello Expiry time
	forward_delay_expiry_time : To keep track of Forward delay expiry time
	time_since_top_change : To Keep track of time since topology change
	oper_tx_hold_count : To set the Oper TX hold count based on the priority vectors
	top_change_cnt : to keep track of the count of changes in topology.

MSTP_Common_Instance_Port :
	port : Reference the port row
	port_priority : Priority of the port in CIST
	admin_path_cost : Path cost set by admin to the interface
	admin_edge_port_disable : to set the port as admin edge
	bpdus_rx_enable : to enable bpdu rx
	bpdus_tx_enable : to enable bpdu tx
	restricted_port_role_disable : to enable restricted port role
	restricted_port_tcn_disable : to enable restricted port tcn
	bpdu_guard_disable : to enable bpdu guard
	loop_guard_disable : to enable loop guard
	root_guard_disable : to enable root guard
	bpdu_filter_disable : to enable bpdu filter
	port_role : to set the port role for the CIST
	port_state : to set the port state for the CIST
	link_type : to set the link type for the CIST
	oper_edge_port : To check if the port is oper edge
	cist_regional_root_id : To store the Regional Root
	cist_path_cost : to store the path cost to the CIST Root
	port_path_cost : to store the path cost to the Root
	designated_path_cost : to store the path cost to the Designated Root
	designated_bridge : to store the mac address of designated Bridge
	designated_port : to store the port of the Designated root.
	port_hello_time : to store the operational Hello time
	protocol_migration_enable : To enable the protocol migration.
	mstp_statistics: to store MSTP statistics.

MSTP_Instance_Port :
	port : Reference the port row
	port_priority : Priority of the port in CIST
	admin_path_cost : Path cost set by admin to the interface
	port_role : to set the port role for the CIST
	port_state : to set the port state for the CIST
	designated_root : to store the mac address of Designated Root.
	designated_root_priority : to store the priority of the designated root
	designated_cost : to store the cost to the designated root.
	designated_bridge : to store the mac address of designated Bridge
	designated_port : to store the port of the Designated root.
	designated_bridge_priority : to store the priority of th designated bridge.


Internal structure
------------------
The ops-stpd process has three operational threads:
* ovsdb_thread
  This thread processes the typical OVSDB main loop, and handles any changes. Some changes are handled by passing messages to the mstpd_protocol_thread thread.
* mstpd_protocol_thread
  This thread processes messages sent to it by the other two threads. Processing of the messages includes operating the finite state machines.
* mstp_rx_pdu_thread
  This thread waits for MSTP packets on interfaces. When a packet is received, it sends a message (including the packet data) to the mstpd_protocol_thread thread for processing through the state machines.

The ops-stpd process the MSTP packets from the Peers, and compares with the config related parameters in the DB.
It calculates the priority vectors and takes a decision on the ports which has to be forwarded and blocked.
ops-stpd also updates DB on the state changes/statistics/status parameters in interfaces based on priority vectors.

References
----------
* [MSTP Daemon design](/documents/user/mstpd_design)
