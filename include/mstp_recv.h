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

#ifndef __MSTP_RECV_H__
#define __MSTP_RECV_H__

#define MIN_MSTP_BPDU_PKT_SIZE 102
#define MAX_MSTP_BPDU_PKT_SIZE 1126  /* 102 +(16 *64 MSTI config mesages) */

typedef struct mlacp__rxPdu {
    int  pktLen;
    char data[MAX_MSTP_BPDU_PKT_SIZE];
}MSTP_RX_PDU;

#endif  /* __MSTP_RECV_H__ */
