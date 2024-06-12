#include <float.h>
#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "postgres.h"

#if PG_VERSION_NUM >= 150000
#include "common/pg_prng.h"
#else
#include "pg_prng_compat.h"
#endif

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
#include "utils/inet.h"
#include "utils/pg_crc.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(random_string);
PG_FUNCTION_INFO_V1(random_bytea);
PG_FUNCTION_INFO_V1(random_int);
PG_FUNCTION_INFO_V1(random_bigint);
PG_FUNCTION_INFO_V1(random_real);
PG_FUNCTION_INFO_V1(random_double_precision);
PG_FUNCTION_INFO_V1(random_numeric_ext);
PG_FUNCTION_INFO_V1(random_macaddr);
PG_FUNCTION_INFO_V1(random_macaddr8);
PG_FUNCTION_INFO_V1(random_inet);
PG_FUNCTION_INFO_V1(random_cidr);
PG_FUNCTION_INFO_V1(random_cidr2);


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

/*
 * random_macaddr
 *		generate random MACADDR value
 */
Datum
random_macaddr(PG_FUNCTION_ARGS)
{
	uint32	seed = PG_GETARG_INT32(0);
	uint32	nvalues = PG_GETARG_INT32(1);
	uint64	value;

	macaddr    *result;
	char *ptr;

	maybe_init_prng();

	/* initialize the 'value' PRNG with one of the distinct seeds */
	value = pg_prng_uint64(&main_state) % nvalues;

	pg_prng_seed(&value_state, ((uint64) seed << 32) | value);

	result = (macaddr *) palloc(sizeof(macaddr));

	value = pg_prng_uint64(&value_state);
	ptr = (char *) &value;

	memcpy(&result->a, &ptr[0], 1);
	memcpy(&result->b, &ptr[1], 1);
	memcpy(&result->c, &ptr[2], 1);
	memcpy(&result->d, &ptr[3], 1);
	memcpy(&result->e, &ptr[4], 1);
	memcpy(&result->f, &ptr[5], 1);

	PG_RETURN_MACADDR_P(result);
}

/*
 * random_macaddr8
 *		generate random MACADDR8 value
 */
Datum
random_macaddr8(PG_FUNCTION_ARGS)
{
	uint32	seed = PG_GETARG_INT32(0);
	uint32	nvalues = PG_GETARG_INT32(1);
	uint64	value;

	macaddr8   *result;
	char *ptr;

	maybe_init_prng();

	/* initialize the 'value' PRNG with one of the distinct seeds */
	value = pg_prng_uint64(&main_state) % nvalues;

	pg_prng_seed(&value_state, ((uint64) seed << 32) | value);

	result = (macaddr8 *) palloc(sizeof(macaddr8));

	value = pg_prng_uint64(&value_state);
	ptr = (char *) &value;

	memcpy(&result->a, &ptr[0], 1);
	memcpy(&result->b, &ptr[1], 1);
	memcpy(&result->c, &ptr[2], 1);
	memcpy(&result->d, &ptr[3], 1);
	memcpy(&result->e, &ptr[4], 1);
	memcpy(&result->f, &ptr[5], 1);
	memcpy(&result->g, &ptr[6], 1);
	memcpy(&result->h, &ptr[7], 1);

	PG_RETURN_MACADDR8_P(result);
}

/*
 * random_inet
 *		generate random inet value
 */
Datum
random_inet(PG_FUNCTION_ARGS)
{
	uint32	seed = PG_GETARG_INT32(0);
	uint32	nvalues = PG_GETARG_INT32(1);

	inet   *result;
	int		nbits;
	int		nbytes;
	unsigned char   *data;
	uint64	value;
	unsigned char   *ptr;

	maybe_init_prng();

	/* initialize the 'value' PRNG with one of the distinct seeds */
	value = pg_prng_uint64(&main_state) % nvalues;

	pg_prng_seed(&value_state, ((uint64) seed << 32) | value);

	result = (inet *) palloc0(sizeof(inet));

	/* ipv4 */
	ip_family(result) = PGSQL_AF_INET;

	nbits = ip_maxbits(result);
	nbytes = (nbits / 8);

	data = ip_addr(result);

	ip_bits(result) = 32;

	value = pg_prng_uint64(&value_state);
	ptr = (unsigned char *) &value;

	for (int i = 0; i < ip_maxbits(result) / 8; i++)
		data[i % nbytes] ^= ptr[i];

	SET_INET_VARSIZE(result);

	PG_RETURN_INET_P(result);
}

/*
 * random_cidr
 *		generate random cidr value
 */
Datum
random_cidr(PG_FUNCTION_ARGS)
{
	uint32	seed = PG_GETARG_INT32(0);
	uint32	nvalues = PG_GETARG_INT32(1);

	inet   *result;
	unsigned char   *data;
	int		masklen;
	uint64	value;
	unsigned char *ptr;

	/* total number of values we can generate */
	uint64	maxvalues = (256 - 1) + (256 * 256 - 1) + (256 * 256 * 256 - 1) + (256L * 256 * 256 * 256 - 1);

	maybe_init_prng();

	/* initialize the 'value' PRNG with one of the distinct seeds */
	value = pg_prng_uint64(&main_state) % nvalues;

	pg_prng_seed(&value_state, ((uint64) seed << 32) | value);

	result = (inet *) palloc0(sizeof(inet));

	/* ipv4 */
	ip_family(result) = PGSQL_AF_INET;

	data = ip_addr(result);

	value = pg_prng_uint64(&value_state) % maxvalues;

	if (value <= (256 - 1))
		masklen = 1;
	else if (value <= ((256 - 1) + (256 * 256 - 1)))
		masklen = 2;
	else if (value <= ((256 - 1) + (256 * 256 - 1) + (256 * 256 * 256 - 1)))
		masklen = 3;
	else
		masklen = 4;

	ip_bits(result) = masklen * 8;

	value = pg_prng_uint64(&value_state);
	ptr = (unsigned char *) &value;

	for (int i = 0; i < ip_maxbits(result) / 8; i++)
		data[i % masklen] ^= ptr[i];

	SET_INET_VARSIZE(result);

	PG_RETURN_INET_P(result);
}

/*
 * random_cidr2
 *		generate random cidr value
 */
Datum
random_cidr2(PG_FUNCTION_ARGS)
{
	uint32	seed = PG_GETARG_INT32(0);
	uint32	nvalues = PG_GETARG_INT32(1);
	uint32	ndistinct;

	inet   *result;
	unsigned char   *data;
	int		masklen;
	uint64	value;
	unsigned char *ptr;

	maybe_init_prng();

	/* initialize the 'value' PRNG with one of the distinct seeds */
	value = pg_prng_uint64(&main_state) % nvalues;

	pg_prng_seed(&value_state, ((uint64) seed << 32) | value);

	result = (inet *) palloc0(sizeof(inet));

	/* ipv4 */
	ip_family(result) = PGSQL_AF_INET;

	data = ip_addr(result);

	masklen = 1 + pg_prng_uint64(&value_state) % 4;

	/* calculate the number of distinct values for each masklen */
	for (int i = 1; i <= 4; i++)
	{
		/* fraction of values for each masklen */
		ndistinct = nvalues / (4 - i + 1);
		nvalues -= ndistinct;

		if (ndistinct >= pow(256, i))
		{
			nvalues += ndistinct - (pow(256, i) - 1);
			ndistinct = (pow(256, i) - 1);
		}

		if (masklen == i)
			break;
	}

	ip_bits(result) = masklen * 8;

	value = pg_prng_uint64(&main_state) % ndistinct;
	pg_prng_seed(&value_state, ((uint64) seed << 32) | value);

	value = pg_prng_uint64(&value_state);
	ptr = (unsigned char *) &value;

	for (int i = 0; i < ip_maxbits(result) / 8; i++)
		data[i % masklen] ^= ptr[i];

	SET_INET_VARSIZE(result);

	PG_RETURN_INET_P(result);
}

/*
 * random_numeric
 *		generate random numeric
 */
Datum
random_numeric_ext(PG_FUNCTION_ARGS)
{
	uint32	seed = PG_GETARG_INT32(0);
	uint32	nvalues = PG_GETARG_INT32(1);
	float8	min_value = PG_GETARG_FLOAT8(2),
			max_value = PG_GETARG_FLOAT8(3);
	float8	val;
	uint64	value;
	Numeric	num;

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

	num = DatumGetNumeric(DirectFunctionCall1(float8_numeric, Float8GetDatum(val)));

	PG_RETURN_NUMERIC(num);
}
