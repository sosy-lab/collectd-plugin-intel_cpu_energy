/**
 * collectd - intel_cpu_energy.c
 * Copyright (C) 2019  Maximilian Hailer
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; only version 2 of the License is applicable.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author:
 *   Maximilian Hailer <git at * dot de>
 * Based on the load plugin developed by:
 *   Nils Steinger <git at n-st dot de>
 *   Florian octo Forster <octo at collectd.org>
 *   Manuel Sanmartin
 *   Vedran Bartonicek <vbartoni at gmail.com>
 **/

#if !HAVE_CONFIG_H
#include <stdlib.h>
#include <string.h>
#ifndef __USE_ISOC99 /* required for NAN */
#define DISABLE_ISOC99 1
#define __USE_ISOC99 1
#endif /* !defined(__USE_ISOC99) */
#include <math.h>
#if DISABLE_ISOC99
#undef DISABLE_ISOC99
#undef __USE_ISOC99
#endif /* DISABLE_ISOC99 */
#include <time.h>
#endif /* ! HAVE_CONFIG */

// clang-format off
#include <collectd/liboconfig/oconfig.h> // needs to come first
// clang-format on
#include <collectd/core/daemon/collectd.h>
#include <collectd/core/daemon/common.h>
#include <collectd/core/daemon/plugin.h>

#include "rapl.h"

#include <inttypes.h>
#include <unistd.h>

static int rapl_node_count = 0;
static double (*prev_sample)[][RAPL_NR_DOMAIN] = NULL;
static double (*cum_energy_J)[][RAPL_NR_DOMAIN] = NULL;

static int energy_submit(int cpu_id, int domain, double measurement) {
  /*
   * an id is of the form host/plugin-instance/type-instance with
   * both instance-parts being optional.
   * in our case: [host]/intel_cpu_energy-[e.g. cpu0]/energy-[e.g. package]
   */

  value_list_t vl = VALUE_LIST_INIT;

  value_t values[1];
  values[0].gauge = measurement;
  vl.values = values;
  vl.values_len = STATIC_ARRAY_SIZE(values);

  sstrncpy(vl.host, hostname_g, sizeof(vl.host));
  sstrncpy(vl.plugin, "intel_cpu_energy", sizeof(vl.plugin));
  snprintf(vl.plugin_instance, sizeof(vl.plugin_instance), "cpu%d", cpu_id);
  sstrncpy(vl.type, "energy", sizeof(vl.type));
  sstrncpy(vl.type_instance, RAPL_DOMAIN_STRINGS[domain], sizeof(vl.type_instance));

  return plugin_dispatch_values(&vl);
}

static int energy_read(void) {
  if (get_total_energy_consumed_for_nodes(rapl_node_count, *prev_sample, *cum_energy_J) != 0) {
    ERROR("intel_cpu_energy plugin: Failed to read energy information");
    return -1;
  }

  for (int i = 0; i < rapl_node_count; i++) {
    for (int domain = 0; domain < RAPL_NR_DOMAIN; domain++) {
      if (is_supported_domain(domain)) {
        int err = energy_submit(i, domain, (*cum_energy_J)[i][domain]);
        if (err != 0) {
          ERROR(
              "intel_cpu_energy plugin: Failed to submit energy information "
              "for node %d, domain %d (%s): Return value %d",
              i,
              domain,
              RAPL_DOMAIN_STRINGS[domain],
              err);
          return -1;
        }
      }
    }
  }

  return 0;
}

static int energy_init(void) {
  /* initialize rapl */
  if (init_rapl() != 0) {
    ERROR("intel_cpu_energy plugin: RAPL initialisation failed");
    terminate_rapl();
    return -1;
  }
  rapl_node_count = get_num_rapl_nodes();
  INFO("intel_cpu_energy plugin: found %d nodes (physical CPUs)", rapl_node_count);

  /* allocate pointers */
  cum_energy_J = calloc(rapl_node_count, sizeof(double[RAPL_NR_DOMAIN]));
  prev_sample = calloc(rapl_node_count, sizeof(double[RAPL_NR_DOMAIN]));

  /* read initial values */
  if (get_total_energy_consumed_for_nodes(rapl_node_count, *prev_sample, NULL) != 0) {
    ERROR("intel_cpu_energy plugin: Failed to read energy information");
    return -1;
  }

  return 0;
}

static int energy_shutdown(void) {
  terminate_rapl();

  return 0;
}

void module_register(void) {
  plugin_register_init("intel_cpu_energy", energy_init);
  plugin_register_shutdown("intel_cpu_energy", energy_shutdown);
  plugin_register_read("intel_cpu_energy", energy_read);
}
