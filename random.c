#include <float.h>
#include <math.h>

#include "postgres.h"

#include "common/pg_prng.h"
#include "utils/datum.h"
#include "utils/array.h"
#include "utils/lsyscache.h"

#include "utils/memutils.h"
#include "utils/numeric.h"
#include "utils/builtins.h"
#include "utils/palloc.h"
#include "utils/elog.h"
#include "catalog/pg_type.h"
#include "nodes/execnodes.h"
#include "access/tupmacs.h"
#include "utils/pg_crc.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(random_string);
PG_FUNCTION_INFO_V1(random_bytea);
PG_FUNCTION_INFO_V1(random_int);
PG_FUNCTION_INFO_V1(random_bigint);
PG_FUNCTION_INFO_V1(random_real);
PG_FUNCTION_INFO_V1(random_double_precision);

/* main PRNG generator */
static	bool			main_initialized = false;
static	pg_prng_state	main_state;

/* value PRNG generator */
static	pg_prng_state	value_state;

/* initialize the main generater, if not initialized yet */
static void
maybe_init_prng(void)
{
	if (main_initialized)
		return;

	pg_prng_seed(&main_state, rand());
	main_initialized = true;
}

/*
 * random_string
 *		generate random string with ASCII characters and symbols
 */
Datum
random_string(PG_FUNCTION_ARGS)
{
	uint32	seed = PG_GETARG_INT32(0);
	uint32	nvalues = PG_GETARG_INT32(1);
	int32	min_len = PG_GETARG_INT32(2);
	int32	max_len = PG_GETARG_INT32(3);

	int		i;
	char   *str;
	char   *chars = " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_-+={}[];:'\"\\|/?.>,<~`\r\n\t";
	int		nchars;
	uint64	value;
	int		len;

	maybe_init_prng();

	/* initialize the 'value' PRNG with one of the distinct seeds */
	value = pg_prng_uint64(&main_state) % nvalues;

	pg_prng_seed(&value_state, ((uint64) seed << 32) | value);
 
	/* some basic sanity checks */
	if (min_len <= 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("minimal length must be a non-negative integer")));

	if (max_len < min_len)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("maximal length must be a smaller than minimal length")));

	len = min_len;
	if (min_len < max_len)
		len += pg_prng_uint32(&value_state) % (max_len - min_len);

	str = palloc(len + 1);
	nchars = strlen(chars);

	for (i = 0; i < len; i++)
		str[i] = chars[pg_prng_uint32(&value_state) % nchars];

	str[len] = '\0';

	PG_RETURN_TEXT_P(cstring_to_text(str));
}

/*
 * random_bytea
 *		generate random bytea value
 */
Datum
random_bytea(PG_FUNCTION_ARGS)
{
	uint32	seed = PG_GETARG_INT32(0);
	uint32	nvalues = PG_GETARG_INT32(1);
	int32	min_len = PG_GETARG_INT32(2);
	int32	max_len = PG_GETARG_INT32(3);

	int				i;
	int32			len;
	bytea		   *val;
	uint64			value;
	unsigned char  *ptr;
 
	maybe_init_prng();

	/* initialize the 'value' PRNG with one of the distinct seeds */
	value = pg_prng_uint64(&main_state) % nvalues;

	pg_prng_seed(&value_state, ((uint64) seed << 32) | value);

	/* some basic sanity checks */
	if (min_len <= 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("minimal length must be a non-negative integer")));

	if (max_len < min_len)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("maximal length must be a smaller than minimal length")));

	len = min_len;
	if (min_len < max_len)
		len += pg_prng_uint32(&value_state) % (max_len - min_len);

	/* some basic sanity checks */
	if (len <= 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("length must be a non-negative integer")));

	val = palloc(VARHDRSZ + len);
	SET_VARSIZE(val, VARHDRSZ + len);
	ptr = (unsigned char *) VARDATA(val);

	for (i = 0; i < len; i += sizeof(uint64))
	{
		int		l = Min(len - i, sizeof(uint64));
		uint64	v = pg_prng_uint64(&value_state);

		memcpy(&ptr[i], &v, l);
	}

	PG_RETURN_BYTEA_P(val);
}

/*
 * random_int
 *		generate random integer
 */
Datum
random_int(PG_FUNCTION_ARGS)
{
	uint32	seed = PG_GETARG_INT32(0);
	uint32	nvalues = PG_GETARG_INT32(1);
	int32	min_value = PG_GETARG_INT32(2),
			max_value = PG_GETARG_INT32(3);
	int32	val;
	uint64	value;

	maybe_init_prng();

	/* initialize the 'value' PRNG with one of the distinct seeds */
	value = pg_prng_uint64(&main_state) % nvalues;

	pg_prng_seed(&value_state, ((uint64) seed << 32) | value);

	/* some basic sanity checks */
	if (min_value > max_value)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid combination of min/max values (%d/%d)",
						min_value, max_value)));

	val = min_value + pg_prng_uint64(&value_state) % (max_value - min_value);

	PG_RETURN_INT32(val);
}

/*
 * random_bigint
 *		generate random bigint value
 */
Datum
random_bigint(PG_FUNCTION_ARGS)
{
	uint32	seed = PG_GETARG_INT32(0);
	uint32	nvalues = PG_GETARG_INT32(1);
	int64	min_value = PG_GETARG_INT32(2),
			max_value = PG_GETARG_INT32(3);
	int64	val;
	uint64	value;

	maybe_init_prng();

	/* initialize the 'value' PRNG with one of the distinct seeds */
	value = pg_prng_uint64(&main_state) % nvalues;

	pg_prng_seed(&value_state, ((uint64) seed << 32) | value);

	/* some basic sanity checks */
	if (min_value > max_value)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid combination of min/max values (%ld/%ld)",
						min_value, max_value)));

	val = min_value + pg_prng_uint64(&value_state) % (max_value - min_value);

	PG_RETURN_INT64(val);
}

/*
 * random_real
 *		generate random 32-bit float
 */
Datum
random_real(PG_FUNCTION_ARGS)
{
	uint32	seed = PG_GETARG_INT32(0);
	uint32	nvalues = PG_GETARG_INT32(1);
	float4	min_value = PG_GETARG_FLOAT4(2),
			max_value = PG_GETARG_FLOAT4(3);
	float4	val;
	uint64	value;

	maybe_init_prng();

	/* initialize the 'value' PRNG with one of the distinct seeds */
	value = pg_prng_uint64(&main_state) % nvalues;

	pg_prng_seed(&value_state, ((uint64) seed << 32) | value);

	/* some basic sanity checks */
	if (min_value > max_value)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid combination of min/max values (%f/%f)",
						min_value, max_value)));

	val = pg_prng_double(&value_state);

	val = min_value + val * (max_value - min_value);

	PG_RETURN_FLOAT4(val);
}

/*
 * random_double_precision
 *		generate random 64-bit float value
 */
Datum
random_double_precision(PG_FUNCTION_ARGS)
{
	uint32	seed = PG_GETARG_INT32(0);
	uint32	nvalues = PG_GETARG_INT32(1);
	float8	min_value = PG_GETARG_FLOAT8(2),
			max_value = PG_GETARG_FLOAT8(3);
	float8	val;
	uint64	value;

	maybe_init_prng();
 
	/* initialize the 'value' PRNG with one of the distinct seeds */
	value = pg_prng_uint64(&main_state) % nvalues;

	pg_prng_seed(&value_state, ((uint64) seed << 32) | value);

	/* some basic sanity checks */
	if (min_value > max_value)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid combination of min/max values (%f/%f)",
						min_value, max_value)));

	val = pg_prng_double(&value_state);

	val = min_value + val * (max_value - min_value);

	PG_RETURN_FLOAT8(val);
}
