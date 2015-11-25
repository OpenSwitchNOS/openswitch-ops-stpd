/*
 * (c) Copyright 2015 Hewlett Packard Enterprise Development LP
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
#ifndef __MSTP_OVSDB_IF__H__
#define __MSTP_OVSDB_IF__H__

extern bool exiting;

/**************************************************************************//**
 * mstpd daemon's main OVS interface function.
 *
 * @param arg pointer to ovs-appctl server struct.
 *
 *****************************************************************************/
extern void *mstpd_ovs_main_thread(void *arg);

#endif /* __MSTP_OVSDB_IF__H__ */
