/** @file switchd_stp_plugin.c
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "unixctl.h"
#include "openvswitch/vlog.h"
#include "plugin-extensions.h"
#include "switchd_stp_plugin.h"
#include "reconfigure-blocks.h"

VLOG_DEFINE_THIS_MODULE(switchd_stp_plugin);

/** @var stp_plugin_interface
    @brief new struct pointer
*/

struct switchd_stp_plugin_interface stp_plugin_interface = {
  .stp_reconfigure = &stp_reconfigure,
  .stp_plugin_dump_data = &stp_plugin_dump_data
};

void
stp_unixctl_show(struct unixctl_conn *conn, int argc,
                  const char *argv[], void *aux OVS_UNUSED)
{
    struct ds ds = DS_EMPTY_INITIALIZER;

    stp_plugin_dump_data(&ds, argc, argv);
    unixctl_command_reply(conn, ds_cstr(&ds));
    ds_destroy(&ds);
}

/** @fn int init(int phase_id)
    @brief Initialization of the plugin, needs to be run.
    @param[in] phase_id Indicates the number of times a plugin has been initialized.
    @param[out] ret Check if the plugin was correctly registered.
    @return 0 if success, errno value otherwise.
*/

/* Define plugin functions into a reconfigure block
 *
 * If a function of the plugin is considered as part of a global reconfigure
 * event, it is necesary to register a function into a specific reconfigure
 * block by call register_callback_block function
 * which has as parameters the pointer of the function, the priority and the ID
 * Block to be inserted.
 *
 * After its succesfully registration, you can call the function as following:
 *
 * int register_callback_block(void (*)(void*)stp_foo,
 *                              Block_ID blkId,
 *                              int priority);
 *
 * where:
 *   - stp_foo: is the function pointer of the plugin.
 *   - Block_ID: identify the block which the function will be registered.
 *   - priority: affects the plugin initialization ordering.
 *
 */

int init(int phase_id)
{
  struct plugin_extension_interface extension;

  int ret = 0;
  switch (phase_id) {
    case 0:

      extension.plugin_name = switchd_stp_plugin_NAME;
      extension.major = switchd_stp_plugin_MAJOR;
      extension.minor = switchd_stp_plugin_MINOR;
      extension.plugin_interface = (void *)&stp_plugin_interface;

      if (register_plugin_extension(&extension) != 0) {
        VLOG_ERR("Failed to register stp plugin interface");
        ret = EPERM;
      }
      VLOG_INFO("STP Plugin Registration ok");

      if (register_reconfigure_callback(stp_reconfigure,
                        BLK_BR_FEATURE_RECONFIG, NO_PRIORITY) != 0) {
         VLOG_ERR("Failed to register STP reconfigure in block %d",
                  BLK_BR_FEATURE_RECONFIG);
         ret = EPERM;
      }
      VLOG_INFO("STP Reconfgiure registerd in bridge reconfig block %d",
                 BLK_BR_FEATURE_RECONFIG);

      unixctl_command_register(switchd_stp_plugin_NAME, "[instance-id]", 0, 1,
                             stp_unixctl_show, NULL);
      break;
    case 1:
      VLOG_INFO("STP Plugin INIT_PHASE_2");
      break;
    default:
      VLOG_INFO("STP Plugin DEFAULT");
      ret = EPERM;
  }

  return ret;
}

/** @fn int run()
    @brief Run function plugin
    @return ret if success, errno value otherwise
*/
int run(void)
{
  int ret = 0;

  VLOG_DBG("STP Plugin is running...");

  /* Add a fail condition by the plugin as following
   *
   * if (fail condition)
   *    ret = errno;  // errno value must not be zero
  */
  goto error;

 error:
  return ret;
}

/** @fn int wait()
    @brief Wait function plugin
    @return ret if success, errno value otherwise
*/
int wait(void)
{
  int ret = 0;
  VLOG_DBG("STP Plugin is waiting for...");

  /* Add a fail condition by the plugin as following
   *
   * if (fail condition)
   *    ret = errno;  // errno value must not be zero
   */
  goto error;

 error:
  return ret;

}

/** @fn int destroy()
    @brief Destroy function plugin
    @return ret if success, errno value otherwise
*/
int destroy(void)
{
  int ret = 0;
  unregister_plugin_extension(switchd_stp_plugin_NAME);
  VLOG_DBG("STP Plugin was destroyed...");

  /* Add a fail condition by the plugin as following
   *
   * if (fail condition)
   *    ret = errno;  // errno value must not be zero
   */
  goto error;

 error:
  return ret;
}
