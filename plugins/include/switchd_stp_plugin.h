/* Copyright (C) 2016 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*  @file switchd_stp_plugin.h
    @brief This file contains all of the functions owned of
    switchd_stp_plugin
*/

#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "dynamic-string.h"
#include "reconfigure-blocks.h"

#ifndef __switchd_stp_plugin_H__
#define __switchd_stp_plugin_H__

/** @def switchd_stp_plugin_NAME
    @brief Plugin plugin_name version definition
*/
/* Do not change this number */
#define switchd_stp_plugin_NAME    "STP"

/** @def switchd_stp_plugin_MAJOR
    @brief Plugin major version definition
*/
#define switchd_stp_plugin_MAJOR   1

/** @def switchd_stp_plugin_MINOR
    @brief Plugin plugin_name version definition
*/
#define switchd_stp_plugin_MINOR    1

/* Plugin Functions
 * Add the functions of the plugin here following the next example.
 *
 */

void stp_reconfigure(struct blk_params*);
void stp_plugin_dump_data(struct ds *ds, int argc, const char *argv[]);
#endif /*__switchd_stp_plugin_H__*/
