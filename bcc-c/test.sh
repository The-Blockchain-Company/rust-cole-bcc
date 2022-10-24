#!/bin/sh
# Run tests
# Set the VALGRIND variable to true to check for memory leaks
if [ -d target/debug ]; then
	PROJECT_ROOT="./"
elif [ -d ../target/debug ]; then
	PROJECT_ROOT="../"
else
	echo "no target debug directory"
	exit 1
fi

C_ROOT="${PROJECT_ROOT}bcc-c/"
C_LIB_A="${PROJECT_ROOT}target/debug/libbcc_c.a"

if [ ! -f "${C_LIB_A}" ]; then
	echo "no library file found. compile bcc-c first"
	exit 2
fi

if [ -t 1 ]; then
	UNITY_COLOR="-D UNITY_OUTPUT_COLOR"
fi

: "${VALGRIND:-false}"

OS_OPTIONS=""
if [ $(uname) == "Darwin" ]; then
    OS_OPTIONS="-framework Security"
fi

for FILENAME in ${C_ROOT}test/*.c; do
    [ -e "$FILENAME" ] || continue
	gcc -std=c99 -o test-bcc-c.$$ -I "${C_ROOT}" "${FILENAME}" "${C_ROOT}test/unity/unity.c" "${PROJECT_ROOT}target/debug/libbcc_c.a" -lpthread -lm -ldl ${UNITY_COLOR} ${OS_OPTIONS}
	echo "######################################################################"
	if [ "$VALGRIND" = true ] ; then
		valgrind ./test-bcc-c.$$
		else
		./test-bcc-c.$$
	fi
	echo ""
	echo "######################################################################"
	rm test-bcc-c.$$
done

