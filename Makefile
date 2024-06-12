MODULE_big = random
OBJS = random.o

EXTENSION = random
DATA = random--1.0.0.sql random--1.0.0--2.0.0-dev.sql
MODULES = random

CFLAGS=`pg_config --includedir-server`

TESTS        = $(wildcard test/sql/*.sql)
REGRESS      = $(patsubst test/sql/%.sql,%,$(TESTS))
REGRESS_OPTS = --inputdir=test

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
