/* random--1.0.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION random" to load this file. \quit

CREATE FUNCTION random_string(len int) RETURNS text
AS 'MODULE_PATHNAME', 'random_string'
LANGUAGE C STRICT;

CREATE FUNCTION random_bytea(len int) RETURNS bytea
AS 'MODULE_PATHNAME', 'random_bytea'
LANGUAGE C STRICT;

CREATE FUNCTION random_int(min_value int, max_value int) RETURNS int
AS 'MODULE_PATHNAME', 'random_int'
LANGUAGE C STRICT;

CREATE FUNCTION random_bigint(min_value bigint, max_value bigint) RETURNS bigint
AS 'MODULE_PATHNAME', 'random_bigint'
LANGUAGE C STRICT;

CREATE FUNCTION random_real(min_value real, max_value real) RETURNS real
AS 'MODULE_PATHNAME', 'random_real'
LANGUAGE C STRICT;

CREATE FUNCTION random_double_precision(min_value double precision, max_value double precision) RETURNS double precision
AS 'MODULE_PATHNAME', 'random_double_precision'
LANGUAGE C STRICT;
