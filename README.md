Serialization for the MobilityDB temporal types
===============================================

This directory contains serialization methods for MobilityDB temporal types.

Dependencies
------------
- [PostgreSQL 16+](https://www.postgresql.org/)
- [MobilityDB 1.1](https://github.com/MobilityDB/MobilityDB)
- [MEOS 1.1](https://www.libmeos.org/)

You should also set the following in postgresql.conf depending on the version of PostGIS and MobilityDB you have installed (below we use PostGIS 3, MobilityDB 1.1):

```
shared_preload_libraries = 'postgis-3,libMobilityDB-1.1'
```

Installation
------------
Compiling and installing the extension
```
make
sudo make install
```

```sql
CREATE EXTENSION mobilitydb_tserialized CASCADE;
```

Contact:
  Maxime Schoemans  <maxime.schoemans@ulb.be>