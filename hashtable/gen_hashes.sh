#!/bin/bash
set -e

: ${TESTS_DIR:="tests"}
: ${MODULO:="1009"}

mkdir -p "$TESTS_DIR"
mkdir -p reports

set -o xtrace

./uint_streamhash "$MODULO" raw_mod 		< "./$TESTS_DIR/random_uint.in" > reports/uint_raw_mod.out
./uint_streamhash "$MODULO" bithash 		< "./$TESTS_DIR/random_uint.in" > reports/uint_bithash.out
./uint_streamhash "$MODULO" knuth_const 	< "./$TESTS_DIR/random_uint.in" > reports/uint_knuth_const.out

./float_streamhash "$MODULO" int_cast 	< "./$TESTS_DIR/random_float.in" > reports/float_int_cast.out
./float_streamhash "$MODULO" bithash 	< "./$TESTS_DIR/random_float.in" > reports/float_bithash.out
./float_streamhash "$MODULO" mantissa 	< "./$TESTS_DIR/random_float.in" > reports/float_mantissa.out
./float_streamhash "$MODULO" exponent 	< "./$TESTS_DIR/random_float.in" > reports/float_exponent.out
./float_streamhash "$MODULO" mantissa_by_exponent < "./$TESTS_DIR/random_float.in" > reports/float_mantissa_by_exponent.out

./str_streamhash "$MODULO" strlen	< "./$TESTS_DIR/random_str.in" > reports/str_strlen.out
./str_streamhash "$MODULO" letters_sum 	< "./$TESTS_DIR/random_str.in" > reports/str_letters_sum.out
./str_streamhash "$MODULO" poly	 	< "./$TESTS_DIR/random_str.in" > reports/str_poly.out
./str_streamhash "$MODULO" crc32 	< "./$TESTS_DIR/random_str.in" > reports/str_crc32.out

set +o xtrace
