AM_CFLAGS = -Wall -D_GNU_SOURCE -DMAX_CITY_DB=\"$(MAX_CITY_DB)\"
AM_CPPFLAGS = -I$(VARNISHSRC)/include -I$(VARNISHSRC)

vmoddir = $(VMODDIR)
vmod_LTLIBRARIES = libvmod_geo.la

libvmod_geo_la_LDFLAGS = -module -export-dynamic -avoid-version -shared -lm

libvmod_geo_la_SOURCES = \
	vcc_if.c \
	vcc_if.h \
	vmod_geo.h \
	geo_functions.c \
	vmod_geo.c

bin_PROGRAMS = maxmind
maxmind_SOURCES = \
	maxmind.c

maxmind_LDADD = geo_functions.o
maxmind_LDFLAGS = -lmaxminddb -lm

vcc_if.c vcc_if.h: $(VARNISHSRC)/lib/libvmod_std/vmod.py $(top_srcdir)/src/vmod_geo.vcc
	@PYTHON@ $(VARNISHSRC)/lib/libvmod_std/vmod.py $(top_srcdir)/src/vmod_geo.vcc


VMOD_TESTS = tests/*.vtc
.PHONY: $(VMOD_TESTS)

tests/*.vtc:
	$(VARNISHSRC)/bin/varnishtest/varnishtest -Dvarnish_source=$(VARNISHSRC) -Dvarnishd=$(VARNISHSRC)/bin/varnishd/varnishd -Dvmod_topbuild=$(abs_top_builddir) $@

CLEANFILES = $(builddir)/vcc_if.c $(builddir)/vcc_if.h

EXTRA_DIST = \
	vmod_geo.vcc \
	$(VMOD_TESTS)

test:
	$(MAKE) -C tests
