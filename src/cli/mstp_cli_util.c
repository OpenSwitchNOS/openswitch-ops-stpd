/*
 * Copyright (C) 1997, 98 Kunihiro Ishiguro
 * Copyright (C) 2015-2016 Hewlett Packard Enterprise Development LP
 *
 * GNU Zebra is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
/****************************************************************************
 *    File               : mstp_cli_util.c
 *    Description        : MSTP Protocol CLI Utilities
 ******************************************************************************/

#include "mstp_inlines.h"
#include "vtysh/vty.h"
#include "vtysh/command.h"
#include "vtysh/vtysh.h"

extern struct ovsdb_idl *idl;
#define MAX_VID_STR_LEN 10 /* length of "xxxx-xxxx" + '\0' */
#define MAX_INDENT      15
#define MAX_LINE_LEN    80

/**PROC+**********************************************************************
* Name:      print_vidmap_multiline
*
* Purpose:   Prints a vidmap in multiple lines
*
* Returns:   Number of characters printed.
*
* Params:    vidMap            -> VIDMAP to be printed
*            lineLength        -> Length for printing VLAN list
*            lineIndent        -> Offset to start printing VLAN list
**PROC-**********************************************************************/
void print_vidmap_multiline(VID_MAP * vidMap, uint32_t lineLength,
                       uint32_t lineIndent) {
   int32_t   vid;
   int32_t   vidFound = 0;
   bool     findRange  = FALSE;
   int32_t   printedLineLen = 0;
   char      vidStr[MAX_VID_STR_LEN];
   char      *tmp;
   char      *vidDelimiter = ",";
   char      *rangeDelimiter = "-";
   int       l = 0;

   STP_ASSERT(vidMap);

   /* Print VIDs that are set in the VID MAP
    * NOTE: We loop one extra time, so that we can print the final VID
    *       before we exit */
   tmp = vidStr;
   for(vid = MIN_VLAN_ID; vid <= MAX_VLAN_ID + 1; vid++)
   {
      if(is_vid_set(vidMap, vid))
      {/* VID is set */
         if(findRange == FALSE)
         {/* print the VID found and start looking for a range */
            STP_ASSERT((tmp - vidStr) < (int)sizeof(vidStr));
            sprintf(tmp, "%d%n", vid, &l);
            tmp += l;
            findRange = TRUE;
            vidFound = vid;
         }
         /* clear VID from map to keep track on how many others left */
         clear_vid(vidMap, vid);
      }
      else
      {/* VID is not set */
         if(findRange == TRUE)
         {/* we tried to find a VID range and the first VID in range has been
           * already printed */
            int rangeSize = (vid - 1) - vidFound;

            if(rangeSize == 0)
            {/* no range detected (i.e. no next adjacent VID found), if
              * there are still other VIDs follow in the map then print
              * 'vidDelimiter' */
               if(are_any_vids_set(vidMap))
               {
                  STP_ASSERT((tmp - vidStr) < (int)sizeof(vidStr));
                  sprintf(tmp, "%s%n", vidDelimiter, &l);
                  tmp += l;
               }
            }
            else
            {/* the VID range is detected; print last VID in the range, if
              * range size is greater than 1 then print 'rangeDelimiter',
              * otherwise use 'vidDelimiter' */
               STP_ASSERT((tmp - vidStr) < (int)sizeof(vidStr));
               sprintf(tmp, "%s%d%n",
                       (rangeSize > 1) ? rangeDelimiter : vidDelimiter,
                       vid - 1, &l);
               tmp += l;
               if(are_any_vids_set(vidMap))
               {
                  STP_ASSERT((tmp - vidStr) < (int)sizeof(vidStr));
                  sprintf(tmp, "%s%n", vidDelimiter, &l);
                  tmp += l;
               }
            }
            findRange = FALSE;

            /* Format output lines if necessary */
            if((printedLineLen + strlen(vidStr)) > lineLength)
            {
               vty_out(vty, "%s", VTY_NEWLINE);
               vty_out(vty, "%*s", lineIndent, "");
               printedLineLen = 0;
            }
            vty_out(vty, "%s%n", vidStr, &l);
            printedLineLen += l;
            tmp = vidStr;
         }
      }
   }
   return;
}

void
print_vid_for_instance(int inst_id) {

    const struct ovsrec_mstp_instance *mstp_row = NULL;
    const struct ovsrec_bridge *bridge_row = NULL;
    int mstid = 0, vid = 0;

    VID_MAP vidMap;
    clear_vid_map(&vidMap);

    bridge_row = ovsrec_bridge_first(idl);
    if (!bridge_row) {
        vty_out(vty, "No bridge record found%s:%d%s", __FILE__, __LINE__, VTY_NEWLINE);
        assert(0);
        return;
    }

    /* Print VLANS mapped to CIST*/
    if (inst_id == MSTP_CISTID) {
        /* Setting the VID to the bitmap*/
        for (mstid=0; mstid < bridge_row->n_mstp_instances; mstid++) {
            mstp_row = bridge_row->value_mstp_instances[mstid];
            if(mstp_row == NULL) {
                vty_out(vty, "No MSTP record found%s:%d%s", __FILE__, __LINE__, VTY_NEWLINE);
                assert(0);
                return;
            }
            for (vid=0; vid<mstp_row->n_vlans; vid++) {
                set_vid(&vidMap, mstp_row->vlans[vid]->id);
            }
        }
        bit_inverse_vid_map(&vidMap);
        print_vidmap_multiline(&vidMap, MAX_LINE_LEN, MAX_INDENT);
    }

    /* Print VLANS mapped to specific instance*/
    else if (MSTP_VALID_MSTID(inst_id)) {
        /* find the instance id*/
        for (mstid=0; mstid < bridge_row->n_mstp_instances; mstid++) {
            if (bridge_row->key_mstp_instances[mstid] == inst_id) {
                mstp_row = bridge_row->value_mstp_instances[mstid];
                break;
            }
        }

        if(mstp_row == NULL) {
            vty_out(vty, "No MSTP record found%s:%d%s", __FILE__, __LINE__, VTY_NEWLINE);
            return;
        }

        for (vid=0; vid<mstp_row->n_vlans; vid++) {
            set_vid(&vidMap, mstp_row->vlans[vid]->id);
        }
        print_vidmap_multiline(&vidMap, MAX_LINE_LEN, MAX_INDENT);
    }
}
