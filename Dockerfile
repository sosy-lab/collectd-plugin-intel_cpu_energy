# This Dockerfile provides a build environment for the collectd plugin
# based on Ubuntu 16.04 and collectd 5.8.
# It can be used as follows:
# docker build -t collectd-plugin-intel_cpu_energy - < Dockerfile
# docker run --volume "$(pwd):$(pwd)" -w "$(pwd)" collectd-plugin-intel_cpu_energy make clean all

FROM ubuntu:xenial

RUN echo "deb http://pkg.ci.collectd.org/deb xenial collectd-5.8" >> /etc/apt/sources.list

ADD https://pkg.ci.collectd.org/pubkey.asc /etc/apt/collectd.asc

RUN apt-key add /etc/apt/collectd.asc

RUN apt-get update && apt-get install -y \
  build-essential \
  collectd-core \
  libcap-dev

# The current collectd-dev package has a wrong dependency version
RUN apt-get update && apt-get download -y collectd-dev && dpkg -i --force-depends-version collectd*.deb
