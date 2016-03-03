/** @file switchd_stp_plugin.h
    @brief This file contains all of the functions owned of
    switchd_stp_plugin
*/

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

struct bridge; /*forward declaration */
/** @def switchd_stp_plugin_NAME
    @brief Plugin plugin_name version definition
*/
/* Do not change this number */
#define switchd_stp_plugin_NAME    "stp"

/** @def switchd_stp_plugin_MAJOR
    @brief Plugin major version definition
*/
#define switchd_stp_plugin_MAJOR    0

/** @def switchd_stp_plugin_MINOR
    @brief Plugin plugin_name version definition
*/
#define switchd_stp_plugin_MINOR    1

/*
 * Interface structure that is going to be exposed. Contains the pointers to the
 * plugin functions and the control version values as the plugin_name, major and
 * minor.
 */

/** @struct switchd_stp_plugin_interface
 *  @brief Plugin interface structure
 *
 *  @param plugin_name plugin name
 *  @param major Plugin major version
 *  @param minor Plugin minor version
*/
struct switchd_stp_plugin_interface {
   void (* stp_reconfigure)(struct blk_params*);
   void (* stp_plugin_dump_data)(struct ds *ds, int argc, const char *argv[]);

};

/* Plugin Functions
 * Add the functions of the plugin here following the next example.
 *
 */

/** @fn int function_test()
    @brief Description of testing function
    @return 0 if success, errno value otherwise
*/
int function_test();
void stp_reconfigure(struct blk_params*);
void stp_plugin_dump_data(struct ds *ds, int argc, const char *argv[]);
#endif /*__switchd_stp_plugin_H__*/
