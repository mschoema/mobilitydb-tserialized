-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION mobilitydb_tserialized" to load this file. \quit

/******************************************************************************
 * Serialized data type for MobilityDB temporal types
 ******************************************************************************/

CREATE TYPE temporal;

CREATE FUNCTION temporal_in(cstring, oid, integer)
  RETURNS temporal
  AS 'MODULE_PATHNAME', 'TSerialized_in'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION temporal_out(temporal)
  RETURNS cstring
  AS 'MODULE_PATHNAME', 'TSerialized_out'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE TYPE temporal (
  internallength = variable,
  input = temporal_in,
  output = temporal_out,
  storage = extended,
  alignment = double
);

/******************************************************************************/

CREATE FUNCTION temporal(tint)
  RETURNS temporal
  AS 'MODULE_PATHNAME', 'TSerialized_from_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION temporal(tfloat)
  RETURNS temporal
  AS 'MODULE_PATHNAME', 'TSerialized_from_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION temporal(tbool)
  RETURNS temporal
  AS 'MODULE_PATHNAME', 'TSerialized_from_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION temporal(ttext)
  RETURNS temporal
  AS 'MODULE_PATHNAME', 'TSerialized_from_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION temporal(tgeompoint)
  RETURNS temporal
  AS 'MODULE_PATHNAME', 'TSerialized_from_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION temporal(tgeogpoint)
  RETURNS temporal
  AS 'MODULE_PATHNAME', 'TSerialized_from_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST (tint AS temporal) WITH FUNCTION temporal(tint);
CREATE CAST (tfloat AS temporal) WITH FUNCTION temporal(tfloat);
CREATE CAST (tbool AS temporal) WITH FUNCTION temporal(tbool);
CREATE CAST (ttext AS temporal) WITH FUNCTION temporal(ttext);
CREATE CAST (tgeompoint AS temporal) WITH FUNCTION temporal(tgeompoint);
CREATE CAST (tgeogpoint AS temporal) WITH FUNCTION temporal(tgeogpoint);

CREATE FUNCTION tint(temporal)
  RETURNS tint
  AS 'MODULE_PATHNAME', 'TSerialized_to_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tfloat(temporal)
  RETURNS tfloat
  AS 'MODULE_PATHNAME', 'TSerialized_to_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tbool(temporal)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'TSerialized_to_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION ttext(temporal)
  RETURNS ttext
  AS 'MODULE_PATHNAME', 'TSerialized_to_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tgeompoint(temporal)
  RETURNS tgeompoint
  AS 'MODULE_PATHNAME', 'TSerialized_to_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION tgeogpoint(temporal)
  RETURNS tgeogpoint
  AS 'MODULE_PATHNAME', 'TSerialized_to_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST (temporal AS tint) WITH FUNCTION tint(temporal);
CREATE CAST (temporal AS tfloat) WITH FUNCTION tfloat(temporal);
CREATE CAST (temporal AS tbool) WITH FUNCTION tbool(temporal);
CREATE CAST (temporal AS ttext) WITH FUNCTION ttext(temporal);
CREATE CAST (temporal AS tgeompoint) WITH FUNCTION tgeompoint(temporal);
CREATE CAST (temporal AS tgeogpoint) WITH FUNCTION tgeogpoint(temporal);

/******************************************************************************/

CREATE FUNCTION memSize(temporal)
  RETURNS integer
  AS 'MODULE_PATHNAME', 'TSerialized_mem_size'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION numInstants(temporal)
  RETURNS integer
  AS 'MODULE_PATHNAME', 'TSerialized_num_instants'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/******************************************************************************/