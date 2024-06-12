/* random--1.0.0--2.0.0-dev.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION random" to load this file. \quit

DROP FUNCTION random_string(int);
DROP FUNCTION random_bytea(int);
DROP FUNCTION random_int(int, int);
DROP FUNCTION random_bigint(bigint, bigint);
DROP FUNCTION random_real(real, real);
DROP FUNCTION random_double_precision(double precision, double precision);

CREATE FUNCTION random_string(seed int, nvalues int, min_len int, max_len int) RETURNS text
AS 'MODULE_PATHNAME', 'random_string'
LANGUAGE C STRICT;

CREATE FUNCTION random_bytea(seed int, nvalues int, min_len int, max_len int) RETURNS bytea
AS 'MODULE_PATHNAME', 'random_bytea'
LANGUAGE C STRICT;

CREATE FUNCTION random_int(seed int, nvalues int, min_value int, max_value int) RETURNS int
AS 'MODULE_PATHNAME', 'random_int'
LANGUAGE C STRICT;

CREATE FUNCTION random_bigint(seed int, nvalues int, min_value bigint, max_value bigint) RETURNS bigint
AS 'MODULE_PATHNAME', 'random_bigint'
LANGUAGE C STRICT;

CREATE FUNCTION random_real(seed int, nvalues int, min_value real, max_value real) RETURNS real
AS 'MODULE_PATHNAME', 'random_real'
LANGUAGE C STRICT;

CREATE FUNCTION random_double_precision(seed int, nvalues int, min_value double precision, max_value double precision) RETURNS double precision
AS 'MODULE_PATHNAME', 'random_double_precision'
LANGUAGE C STRICT;

CREATE FUNCTION random_numeric(seed int, nvalues int, min_value double precision, max_value double precision) RETURNS numeric
AS 'MODULE_PATHNAME', 'random_numeric_ext'
LANGUAGE C STRICT;

CREATE FUNCTION random_macaddr(seed int, nvalues int) RETURNS macaddr
AS 'MODULE_PATHNAME', 'random_macaddr'
LANGUAGE C STRICT;

CREATE FUNCTION random_macaddr8(seed int, nvalues int) RETURNS macaddr8
AS 'MODULE_PATHNAME', 'random_macaddr8'
LANGUAGE C STRICT;

CREATE FUNCTION random_inet(seed int, nvalues int) RETURNS inet
AS 'MODULE_PATHNAME', 'random_inet'
LANGUAGE C STRICT;

CREATE FUNCTION random_cidr(seed int, nvalues int) RETURNS cidr
AS 'MODULE_PATHNAME', 'random_cidr'
LANGUAGE C STRICT;

CREATE FUNCTION random_cidr2(seed int, nvalues int) RETURNS cidr
AS 'MODULE_PATHNAME', 'random_cidr2'
LANGUAGE C STRICT;
