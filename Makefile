CC = gcc
CFLAGS := -Wall -DHAVE_CONFIG_H -shared -fPIC -g $(CFLAGS)
INCLUDES = -I vendor/cpu-energy-meter
LFLAGS = -L.
LIBS = -lm
SRCS = intel_cpu_energy.c $(wildcard vendor/cpu-energy-meter/*.c)
OBJS = $(SRCS:.c=.o)
PLUGIN_NAME = intel_cpu_energy
MAIN = $(PLUGIN_NAME).so
TYPE_DB = energy-type.db

all:    $(MAIN)

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)
	strip $(MAIN)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) $(OBJS) *~ $(MAIN)

install: $(MAIN) $(TYPE_DB)
	cp $(MAIN) /usr/lib/collectd/
	cp $(TYPE_DB) /etc/collectd/
	echo 'LoadPlugin $(PLUGIN_NAME)' > /etc/collectd/collectd.conf.d/intel_cpu_energy.conf
	echo 'TypesDB "/usr/share/collectd/types.db" "/etc/collectd/$(TYPE_DB)"' >> /etc/collectd/collectd.conf.d/intel_cpu_energy.conf

uninstall:
	rm -f /usr/lib/collectd/$(MAIN)
	rm -f /etc/collectd/$(TYPE_DB)
	rm -f /etc/collectd/collectd.conf.d/intel_cpu_energy.conf

depend: $(SRCS)
	makedepend $(INCLUDES) $^

.PHONY: format-source
format-source:
	clang-format -i *.c
# DO NOT DELETE THIS LINE -- make depend needs it
