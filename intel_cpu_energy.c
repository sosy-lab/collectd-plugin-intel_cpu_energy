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

#if ! HAVE_CONFIG_H
#include <stdlib.h>
#include <string.h>
#ifndef __USE_ISOC99 /* required for NAN */
# define DISABLE_ISOC99 1
# define __USE_ISOC99 1
#endif /* !defined(__USE_ISOC99) */
#include <math.h>
#if DISABLE_ISOC99
# undef DISABLE_ISOC99
# undef __USE_ISOC99
#endif /* DISABLE_ISOC99 */
#include <time.h>
#endif /* ! HAVE_CONFIG */

#include <collectd/liboconfig/oconfig.h>
#include <collectd/core/daemon/collectd.h>
#include <collectd/core/daemon/common.h>
#include <collectd/core/daemon/plugin.h>

#include "rapl.h"

#include <unistd.h>
#include <inttypes.h>


uint64_t rapl_node_count = 0;
double **prev_sample = NULL;
double **cum_energy_J = NULL;


int get_rapl_energy_info(uint64_t power_domain, uint64_t node, double *total_energy_consumed)
{
    int err;
    
    switch (power_domain) {
        case PKG:
            err = get_pkg_total_energy_consumed(node, total_energy_consumed);
            break;
        case PP0:
            err = get_pp0_total_energy_consumed(node, total_energy_consumed);
            break;
        case PP1:
            err = get_pp1_total_energy_consumed(node, total_energy_consumed);
            break;
        case DRAM:
            err = get_dram_total_energy_consumed(node, total_energy_consumed);
            break;
        case PSYS:
            err = get_psys_total_energy_consumed(node, total_energy_consumed);
            break;
        default:
            err = MY_ERROR;
            break;
    }
    
    return err;
}

static int energy_submit (unsigned int cpu_id, unsigned int domain, double measurement)
{
    /*
     * an id is of the form host/plugin-instance/type-instance with
     * both instance-parts being optional.
     * in our case: [host]/intel_cpu_energy-[e.g. cpu0]/energy-[e.g. package]
     */
    
    value_list_t vl = VALUE_LIST_INIT;
    
    value_t values[1];
    values[0].gauge = measurement;
    vl.values = values;
    vl.values_len = STATIC_ARRAY_SIZE (values);
    
    sstrncpy (vl.host, hostname_g, sizeof (vl.host));
    sstrncpy (vl.plugin, "intel_cpu_energy", sizeof (vl.plugin));
    snprintf (vl.plugin_instance, sizeof (vl.plugin_instance), "cpu%u", cpu_id);
    sstrncpy (vl.type, "energy", sizeof (vl.type));
    sstrncpy (vl.type_instance, RAPL_DOMAIN_STRINGS[domain], sizeof (vl.type_instance));
    
    return plugin_dispatch_values (&vl);
}

static int energy_read (void)
{
    int err;
    int domain = 0;
    uint64_t node = 0;
    double new_sample, delta;
    
    /* add new measured energy consumption to cum_energy */
    for (uint64_t i = node; i < rapl_node_count; i++) {
        for (domain = 0; domain < RAPL_NR_DOMAIN; ++domain) {
            if (is_supported_domain(domain)) {
                get_rapl_energy_info(domain, i, &new_sample);
                delta = new_sample - prev_sample[i][domain];
                
                /* handle wraparound */
                if (delta < 0) {
                    delta += MAX_ENERGY_STATUS_JOULES;
                }
                
                prev_sample[i][domain] = new_sample;
                cum_energy_J[i][domain] += delta;
                
                err = energy_submit(i, domain, cum_energy_J[i][domain]);
                if (err) {
                    ERROR ("intel_cpu_energy plugin: Failed to submit energy information for node %" PRIu64 ", domain %d (%s): Return value %d", i, domain, RAPL_DOMAIN_STRINGS[domain], err);
                    return err;
                }
            }
        }
    }
    
    return (0);
}

static int energy_init (void)
{
    int domain = 0;
    uint64_t node = 0;
    
    /* initialize rapl */
    int err = init_rapl();
    if (0 != err) {
        ERROR ("intel_cpu_energy plugin: RAPL initialisation failed with return value %d", err);
        terminate_rapl();
        return MY_ERROR;
    }
    rapl_node_count = get_num_rapl_nodes();
    INFO ("intel_cpu_energy plugin: found %lu nodes (physical CPUs)", rapl_node_count);
    
    /* allocate pointers */
    cum_energy_J = calloc(rapl_node_count, sizeof(double *));
    prev_sample = calloc(rapl_node_count, sizeof(double *));
    for (uint64_t i = 0; i < rapl_node_count; i++) {
        cum_energy_J[i] = calloc(RAPL_NR_DOMAIN, sizeof(double));
        prev_sample[i] = calloc(RAPL_NR_DOMAIN, sizeof(double));
    }
    
    /* don't buffer if piped */
    setbuf(stdout, NULL);
    
    /* read initial values */
    for (uint64_t i = node; i < rapl_node_count; i++) {
        for (domain = 0; domain < RAPL_NR_DOMAIN; ++domain) {
            if (is_supported_domain(domain)) {
                get_rapl_energy_info(domain, i, &prev_sample[i][domain]);
                get_rapl_energy_info(domain, i, &cum_energy_J[i][domain]);
            }
        }
    }
    
    return 0;
}

static int energy_shutdown (void)
{
    terminate_rapl();
    
    return 0;
}

void module_register (void)
{
    plugin_register_init ("intel_cpu_energy", energy_init);
    plugin_register_shutdown ("intel_cpu_energy", energy_shutdown);
    plugin_register_read ("intel_cpu_energy", energy_read);
}
