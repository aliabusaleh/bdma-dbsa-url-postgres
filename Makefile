PG_CONFIG = pg_config
PKG_CONFIG = pkg-config

EXTENSION = url
MODULE_big = url
OBJS = url.o
DATA = # to be filled such as  uri--0.sql 

ifeq (no,$(shell $(PKG_CONFIG) liburiparser || echo no))
$(warning liburiparser not registed with pkg-config, build might fail)
endif

PG_CPPFLAGS += $(shell $(PKG_CONFIG) --cflags-only-I liburiparser)
SHLIB_LINK += $(shell $(PKG_CONFIG) --libs liburiparser)

REGRESS = init test escape
REGRESS_OPTS = --inputdir=test

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
