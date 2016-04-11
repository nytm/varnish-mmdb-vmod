AM_CFLAGS = -Werror -D_GNU_SOURCE -DMAX_CITY_DB=\"$(MAX_CITY_DB)\"
AM_CPPFLAGS = -I/usr/include/varnish

vmoddir = $(VMODDIR)
vmod_LTLIBRARIES = libvmod_geo.la

libvmod_geo_la_LDFLAGS = -module -export-dynamic -avoid-version -shared

libvmod_geo_la_SOURCES = \
	vcc_if.c \
	vcc_if.h \
	vmod_geo.h \
	geo_functions.c \
	vmod_geo.c


vcc_if.c vcc_if.h: $(top_srcdir)/src/vmod_geo.vcc
	@VMODTOOL@ $(top_srcdir)/src/vmod_geo.vcc


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
