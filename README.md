Collectd plugin "intel_cpu_energy"
==================================

This [collectd][collectd] plugin measures and reports the power usage of 2nd
Generation (or later) Intel® Core™ processors.

It will report up to four values for each physical processor: the accumulated
energy usage (in Joules) for the "package", "core", "uncore", and "dram"
domains, as reported by the CPU. Not all domains are supported by all CPU
models, so some of them might be missing on your system.

The code for these measurements is based on Intel's [Power Gadget 2.5 for
Linux][powergadget].

Requirements
------------

To read the CPU's internal energy measurements, the plugin requires write
access to the CPU's model-specific registers (MSRs). For this, **the `msr`
kernel module has to be loaded**.
In addition, write access to /dev/cpu/\*/msr, along with the SYS_RAWIO
capability, is required, but since collectd runs its plugins with root
privileges anyway, those permissions should be available by default.

To build the module, you will need to have the `collectd-dev` package installed
(the package name might differ on non-Debian-based distributions).

The module has been successfully tested with version 5.8-3ubuntu2 of
`collectd-dev` (on Ubuntu 18.04), but collectd's plugin API can be highly
unstable, even between minor versions, so it might not work out of the box with
other versions.

Building and installing
-----------------------

Collectd's plugin API was moved to a different directory in commit 216c624 and
changed to use a different time representation in commit cce1369.
These changes will be included in Collectd versions 5.5.0 and higher.

### Collectd version >= 5.5.0

For those new version, simply run the following:

    make clean all
    sudo make install
    service collectd restart


Usage
-----

The plugin will produce data sets named according to the pattern

    ${host}/intel_cpu_energy-cpu{0..}/energy-{package,core,uncore,dram}

Each data point will contain the accumulated energy consumption in Joules (=
Watt seconds) since the last (re-)start of `collectd`.

The value is internally processed as a double-precision floating point number,
so you shouldn't encounter any problems with overflows.
Because of potential overflow issues the update interval should not be longer than 60 seconds.


[collectd]: https://github.com/collectd/collectd/
[powergadget]: https://software.intel.com/en-us/articles/intel-power-gadget-20
