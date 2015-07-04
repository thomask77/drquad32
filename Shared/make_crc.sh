#!/bin/bash
#
GEN="python ../../Tools/pycrc-0.8.1/pycrc.py"


# Generate files
#
$GEN	--model crc-16				\
		--algorithm table-driven	\
		--table-idx-width 8			\
		--symbol-prefix crc16_		\
		--generate c				\
		-o crc16.c

$GEN	--model crc-16				\
		--algorithm table-driven	\
		--table-idx-width 8			\
		--symbol-prefix crc16_		\
		--generate h				\
		-o crc16.h

$GEN	--model crc-32				\
		--algorithm table-driven	\
		--table-idx-width 8			\
		--symbol-prefix crc32_		\
		--generate c				\
		-o crc32.c

$GEN	--model crc-32				\
		--algorithm table-driven	\
		--table-idx-width 8			\
		--symbol-prefix crc32_		\
		--generate h				\
		-o crc32.h

