EXTENSION   = mobilitydb_tserialized
MODULE_big 	= mobilitydb_tserialized
DATA        = mobilitydb_tserialized--1.0.sql
OBJS				= mobilitydb_tserialized.o
SHLIB_LINK	= -lmeos

PG_CONFIG ?= pg_config
PGXS = $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
