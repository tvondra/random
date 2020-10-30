#include <float.h>

#include "postgres.h"

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

/*
 * random_string
 *		generate random string with ASCII characters and symbols
 */
Datum
random_string(PG_FUNCTION_ARGS)
{
	int				i;
	int32			len = PG_GETARG_INT32(0);
	char		   *str;
	char		   *chars = " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_-+={}[];:'\"\\|/?.>,<~`\r\n\t\0";
	int			nchars;
 
	/* some basic sanity checks */
	if (len <= 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("length must be a non-negative integer")));

	str = palloc(len + 1);
	nchars = strlen(chars);

	for (i = 0; i < len; i++)
		str[i] = chars[random() % nchars];

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
	int				i;
	int32			len = PG_GETARG_INT32(0);
	bytea		   *val;
	unsigned char  *ptr;
 
	/* some basic sanity checks */
	if (len <= 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("length must be a non-negative integer")));

	val = palloc(VARHDRSZ + len);
	SET_VARSIZE(val, VARHDRSZ + len);
	ptr = (unsigned char *) VARDATA(val);

	for (i = 0; i < len; i++)
		ptr[i] = (unsigned char) (random() % 255);

	PG_RETURN_BYTEA_P(val);
}

/*
 * random_int
 *		generate random integer
 */
Datum
random_int(PG_FUNCTION_ARGS)
{
	int32			min_value = PG_GETARG_INT32(0),
					max_value = PG_GETARG_INT32(1);
	int32			val;

	/* some basic sanity checks */
	if (min_value > max_value)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid combination of min/max values (%d/%d)",
						min_value, max_value)));

	val = min_value + random() % (max_value - min_value);

	PG_RETURN_INT32(val);
}

/*
 * random_bigint
 *		generate random bigint value
 */
Datum
random_bigint(PG_FUNCTION_ARGS)
{
	int64			min_value = PG_GETARG_INT64(0),
					max_value = PG_GETARG_INT64(1);
	int64			val;

	/* some basic sanity checks */
	if (min_value > max_value)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid combination of min/max values (%ld/%ld)",
						min_value, max_value)));

	val = min_value + random() % (max_value - min_value);

	PG_RETURN_INT64(val);
}

/*
 * random_real
 *		generate random 32-bit float
 */
Datum
random_real(PG_FUNCTION_ARGS)
{
	float4			min_value = PG_GETARG_FLOAT4(0),
					max_value = PG_GETARG_FLOAT4(1);
	float4			val;

	/* some basic sanity checks */
	if (min_value > max_value)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid combination of min/max values (%f/%f)",
						min_value, max_value)));

	val = min_value + drand48() * (max_value - min_value);

	PG_RETURN_FLOAT4(val);
}

/*
 * random_double_precision
 *		generate random 64-bit float value
 */
Datum
random_double_precision(PG_FUNCTION_ARGS)
{
	float4			min_value = PG_GETARG_FLOAT8(0),
					max_value = PG_GETARG_FLOAT8(1);
	float8			val;

	if (PG_ARGISNULL(0))
		min_value = DBL_MIN;
	else
		min_value = PG_GETARG_FLOAT8(0);

	if (PG_ARGISNULL(1))
		max_value = DBL_MAX;
	else
		max_value = PG_GETARG_FLOAT8(1);

	/* some basic sanity checks */
	if (min_value > max_value)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid combination of min/max values (%f/%f)",
						min_value, max_value)));

	val = min_value + drand48() * (max_value - min_value);

	PG_RETURN_FLOAT8(val);
}
