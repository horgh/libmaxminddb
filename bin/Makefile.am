include $(top_srcdir)/common.mk

AM_LDFLAGS = $(top_builddir)/src/libmaxminddb.la

bin_PROGRAMS = mmdblookup

if !WINDOWS
mmdblookup_CFLAGS = $(CFLAGS) -pthread
endif
