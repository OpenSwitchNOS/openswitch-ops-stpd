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

#ifndef __MSTP_CMN_H__
#define __MSTP_CMN_H__

typedef enum mstpd_message_type_enum {
    e_mstpd_timer=1,
    e_mstpd_lport_up,
    e_mstpd_lport_down,
    e_mstpd_rx_bpdu
} mstpd_message_type;

typedef struct mstpd_message_struct
{
    mstpd_message_type msg_type;
    void *msg;

} mstpd_message;

extern int mstpd_send_event(mstpd_message *pmsg);
extern mstpd_message* mstpd_wait_for_next_event(void);
extern void mstpd_event_free(mstpd_message *pmsg);
#endif  // __MSTP_CMN_H__
