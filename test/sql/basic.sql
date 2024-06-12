CREATE EXTENSION random;

SELECT COUNT(*), COUNT(DISTINCT random_string(1, 100, 32, 64)) > 95 FROM generate_series(1,10000);

SELECT COUNT(*), COUNT(DISTINCT random_bytea(1, 100, 32, 64)) > 95 FROM generate_series(1,10000);

SELECT COUNT(*), COUNT(DISTINCT random_int(1, 100, 1, 1000000)) > 95 FROM generate_series(1,10000);

SELECT COUNT(*), COUNT(DISTINCT random_bigint(1, 100, 1, 1000000)) > 95 FROM generate_series(1,10000);

SELECT COUNT(*), COUNT(DISTINCT random_real(1, 100, 1, 1000000)) > 95 FROM generate_series(1,10000);

SELECT COUNT(*), COUNT(DISTINCT random_double_precision(1, 100, 1, 1000000)) > 95 FROM generate_series(1,10000);

SELECT COUNT(*), COUNT(DISTINCT random_numeric(1, 100, 1, 1000000)) > 95 FROM generate_series(1,10000);

SELECT COUNT(*), COUNT(DISTINCT random_macaddr(1, 100)) > 95 FROM generate_series(1,10000);

SELECT COUNT(*), COUNT(DISTINCT random_macaddr8(1, 100)) > 95 FROM generate_series(1,10000);

SELECT COUNT(*), COUNT(DISTINCT random_inet(1, 100)) > 95 FROM generate_series(1,10000);

SELECT COUNT(*), COUNT(DISTINCT random_cidr(1, 100)) > 95 FROM generate_series(1,10000);

SELECT COUNT(*), COUNT(DISTINCT random_cidr2(1, 100)) > 95 FROM generate_series(1,10000);
