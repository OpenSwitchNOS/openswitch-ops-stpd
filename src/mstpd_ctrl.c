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

/***************************************************************************
 *    File               : mstpd_ctrl.c
 *    Description        : MSTP Protocol thread main entry point
 ***************************************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/filter.h>

#include <util.h>
#include <openvswitch/vlog.h>

#include <mqueue.h>
#include "mstp.h"
#include "mstp_cmn.h"
#include "mstp_recv.h"
#include "mstp_ovsdb_if.h"

VLOG_DEFINE_THIS_MODULE(mstpd_ctrl);

/************************************************************************
 * Global Variables
 ************************************************************************/
static int mstp_init_done = false;
int mstpd_shutdown = 0;

/* Message Queue for MSTPD main protocol thread */
mqueue_t mstpd_main_rcvq;

/* epoll FD for MSTP PDU RX. */
int epfd = -1;

/* Max number of events returned by epoll_wait().
 * This number is arbitrary.  It's only used for
 * sizing the epoll events data structure. */
#define MAX_EVENTS 64

/* MSTP filter
 *
 * Berkeley Packet Filter to receive MSTP BPDU from interfaces.
 *
 * MSTP: "ether dst 01:80:c2:00:00:00"
 *
 *    tcpdump -dd "(ether dst 01:80:c2:00:00:00)"
 *
 * low-level BPF filter code:
 * (000) ld       [2]          ; load 4 bytes from Dst MAC offset 2
 * (001) jeq      #0xc2000000  ; compare 4 bytes, move to next instruction if equal
 * (002) ldh      [0]          ; load 4 bytes from Dst MAC offset 0
 * (003) jeq      #0x0180      ; compare 2 bytes, move to next instruction if equal
 * (004) ret      #65535       ; return 65535 bytes of packet
 * (005) ret      #0           ; return 0
 *
 * { 0x20, 0, 0, 0x00000002 },
 * { 0x15, 0, 3, 0xc2000000 },
 * { 0x28, 0, 0, 0x00000000 },
 * { 0x15, 0, 1, 0x00000180 },
 * { 0x6, 0,  0, 0x0000ffff },
 * { 0x6, 0,  0, 0x00000000 }
 */


#define MSTPD_FILTER_F \
    { 0x20, 0, 0, 0x00000002 }, \
    { 0x15, 0, 3, 0xc2000000 }, \
    { 0x28, 0, 0, 0x00000000 }, \
    { 0x15, 0, 1, 0x00000180 }, \
    { 0x6, 0, 0, 0x0000ffff }, \
    { 0x6, 0, 0, 0x00000000 }

static struct sock_filter mstpd_filter_f[] = { MSTPD_FILTER_F };
static struct sock_fprog mstpd_fprog = {
    .filter = mstpd_filter_f,
    .len = sizeof(mstpd_filter_f) / sizeof(struct sock_filter)
};

/************************************************************************
 * Event Receiver Functions
 ************************************************************************/
int
mstp_init_event_rcvr(void)
{
    int rc;

    rc = mqueue_init(&mstpd_main_rcvq);
    if (rc) {
        VLOG_ERR("Failed MSTP main receive queue init: %s",
                 strerror(rc));
    }

    return rc;
} /* mstp_init_event_rcvr */

int
mstpd_send_event(mstpd_message *pmsg)
{
    int rc;

    rc = mqueue_send(&mstpd_main_rcvq, pmsg);
    if (rc) {
        VLOG_ERR("Failed to send to MSTP main receive queue: %s",
                 strerror(rc));
    }

    return rc;
} /* mstpd_send_event */

mstpd_message *
mstpd_wait_for_next_event(void)
{
    int rc;
    mstpd_message *pmsg = NULL;

    rc = mqueue_wait(&mstpd_main_rcvq, (void **)(void *)&pmsg);
    if (!rc) {
        pmsg->msg = (void *)(pmsg+1);
    } else {
        VLOG_ERR("MSTP main receive queue wait error, rc=%s",
                 strerror(rc));
    }

    return pmsg;
} /* mstpd_wait_for_next_event */

void
mstpd_event_free(mstpd_message *pmsg)
{
    if (pmsg != NULL) {
        free(pmsg);
    }
} /* mstpd_event_free */

/************************************************************************
 * MSTP PDU Send and Receive Functions
 ************************************************************************/
void *
mstpd_rx_pdu_thread(void *data)
{
    int reg_sockfd;

    /* Detach thread to avoid memory leak upon exit. */
    pthread_detach(pthread_self());

    epfd = epoll_create1(0);
    if (epfd == -1) {
        VLOG_ERR("Failed to create epoll object.  rc=%d", errno);
        return NULL;
    }

    reg_sockfd = register_mcast_addr();
    if (reg_sockfd == -1) {
        VLOG_ERR("Failed to register MSTP BPDU mac address");
        return NULL;
     }

    for (;;) {
        int n;
        int nfds;
        struct epoll_event events[MAX_EVENTS];

        if (mstpd_shutdown) {
           break;
        }

        nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);

        if (nfds < 0) {
            VLOG_ERR("epoll_wait returned error %s", strerror(errno));
            break;
        } else {
            VLOG_DBG("epoll_wait returned, nfds=%d", nfds);
        }

        for (n = 0; n < nfds; n++) {
            int count;
            int clientlen;
            int pdu_sockfd;
            struct sockaddr_ll clientaddr;
            mstpd_message *pmsg;
            int total_msg_size;
            MSTP_RX_PDU  *pkt_event;

            pdu_sockfd = events[n].data.fd;
            if (pdu_sockfd != reg_sockfd) {
                VLOG_ERR("invalid sockfd for epoll event!");
                continue;
            } else {
                VLOG_DBG("epoll event #%d: events flags=0x%x, sock=%d",
                         n, events[n].events, pdu_sockfd);
            }


            total_msg_size = sizeof(mstpd_message) + sizeof(MSTP_RX_PDU);

            pmsg = xzalloc(total_msg_size);
            pmsg->msg_type = e_mstpd_rx_bpdu;
            pkt_event = (MSTP_RX_PDU *)(pmsg+1);

            clientlen = sizeof(clientaddr);
            count = recvfrom(pdu_sockfd, (void *)pkt_event->data,
                             MAX_MSTP_BPDU_PKT_SIZE, 0,
                             (struct sockaddr *)&clientaddr,
                             (unsigned int *)&clientlen);
            if (count < 0) {
                /* General socket error. */
                VLOG_ERR("Read failed, fd=%d: errno=%d",
                         pdu_sockfd, errno);
                free(pmsg);
                continue;

            } else if (!count) {
                /* Socket is closed.  Get out. */
                VLOG_ERR("socket=%d closed", pdu_sockfd);
                free(pmsg);
                continue;

            } else if (count <= MAX_MSTP_BPDU_PKT_SIZE) {
                pkt_event->pktLen = count;
                mstpd_send_event(pmsg);
            }
        } /* for nfds */
    } /* for(;;) */

    deregister_mcast_addr(reg_sockfd);

    return NULL;
} /* mstpd_rx_pdu_thread */

int
register_mcast_addr()
{
    int rc;
    int sockfd;
    struct sockaddr_ll addr;
    struct epoll_event event;


    if ((sockfd = socket(PF_PACKET, SOCK_RAW, 0)) < 0) {
        rc = errno;
        VLOG_ERR("Failed to open datagram socket rc=%s",
                 strerror(rc));
        return -1;
    }

    rc = setsockopt(sockfd, SOL_SOCKET, SO_ATTACH_FILTER,
                    &mstpd_fprog, sizeof(mstpd_fprog));
    if (rc < 0) {
        VLOG_ERR("Failed to attach socket filter rc=%s",
                  strerror(rc));
        close(sockfd);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = INADDR_ANY;
    addr.sll_protocol = htons(ETH_P_802_2); /* 802.2 frames */

    rc = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc < 0) {
        VLOG_ERR("Failed to bind socket to addr rc=%s",
                 strerror(rc));
        close(sockfd);
        return -1;
    }

    event.events = EPOLLIN;
    event.data.fd = sockfd;

    rc = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
    if (rc == 0) {
        VLOG_DBG("Registered sockfd %d with epoll loop.",
                 sockfd);
    } else {
        VLOG_ERR("Failed to register sockfd with epoll "
                 "loop.  err=%s", strerror(errno));
        close(sockfd);
        return -1;
    }

    return sockfd;
} /* register_mcast_addr */

void
deregister_mcast_addr(int reg_sockfd)
{
    int rc;

    rc = epoll_ctl(epfd, EPOLL_CTL_DEL, reg_sockfd, NULL);
    if (rc == 0) {
        VLOG_DBG("Deregistered sockfd %d with epoll loop.",
                 reg_sockfd);
    } else {
        VLOG_ERR("Failed to deregister sockfd with epoll "
                 "loop.  err=%s", strerror(errno));
    }

    close(reg_sockfd);

} /* deregister_mcast_addr */


/************************************************************************
 * MSTP Protocol Thread
 ************************************************************************/
void *
mstpd_protocol_thread(void *arg)
{
    mstpd_message *pmsg;

    /* Detach thread to avoid memory leak upon exit. */
    pthread_detach(pthread_self());

    VLOG_DBG("%s : waiting for events in the main loop", __FUNCTION__);

    /*******************************************************************
     * The main receive loop.
     *******************************************************************/
    while (1) {

        pmsg = mstpd_wait_for_next_event();

        if (mstpd_shutdown) {
            break;
        }

        if (!pmsg) {
            VLOG_ERR("MSTPD protocol: Received NULL event!");
            continue;
        }

        if ((pmsg->msg_type == e_mstpd_lport_up) ||
            (pmsg->msg_type == e_mstpd_lport_down)) {
            /***********************************************************
             * Msg from OVSDB interface for lports.
             ***********************************************************/
            VLOG_DBG("%s :Recieved lport event", __FUNCTION__);
            //TODO
        } else if (pmsg->msg_type == e_mstpd_timer) {
            /***********************************************************
             * Msg from MSTP timers.
             ***********************************************************/
            VLOG_INFO("%s : Recieved one sec timer tick event", __FUNCTION__);

        } else if (pmsg->msg_type == e_mstpd_rx_bpdu) {
            /***********************************************************
             * Packet has arrived through interface socket.
             ************************************************************/
            VLOG_DBG("%s : MSTP BPDU Packet arrived from interface socket",
                   __FUNCTION__);

            //TODO
        } else {
            /***********************************************************
             * Unknown/unregistered sender.
             ************************************************************/
            VLOG_ERR("%s : message from unknown sender",
                     __FUNCTION__);
        }

        mstpd_event_free(pmsg);

    } /* while loop */

    return NULL;
} /* mstpd_protocol_thread */

/************************************************************************
 * Initialization & main functions
 ************************************************************************/
int
mmstp_init(u_long  first_time)
{
    int status = 0;

    if (first_time != true) {
        VLOG_ERR("Cannot handle revival from dead");
        status = -1;
        goto end;
    }

    if (mstp_init_done == true) {
        VLOG_WARN("Already initialized");
        status = -1;
        goto end;
    }

    /* Initialize MSTP main task event receiver queue. */
    if (mstp_init_event_rcvr()) {
        VLOG_ERR("Failed to initialize event receiver.");
        status = -1;
        goto end;
    }

    mstp_init_done  = true;

end:
    return status;
} /* mmstp_init */
