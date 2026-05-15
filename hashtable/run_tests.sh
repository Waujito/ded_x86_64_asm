#!/bin/bash
set -e

REPORT_FILE="reports/hashtables_report.csv"
mkdir -p reports

echo "optim_strat,ith_test,time" > "$REPORT_FILE"

get_strat_flag() {
	STRAT_FLAG=""

	for arg in "$@"; do
		case "$arg" in
			"crc32")
				STRAT_FLAG+="-DCRC32_INTRIN "
				;;
			"asm_strcmp")
				STRAT_FLAG+="-DIS_K_EQ_ASM "
				;;
			"strlen")
				STRAT_FLAG+="-DUSE_HT_MY_STRLEN"
				;;
			*)
				echo "Invalid arg: $arg" >&2
				return 1
				;;
		esac
	done

	echo -n "$STRAT_FLAG"
	return 0
}

run_tests() {
	K_WARMUP=1
	K_RUNS=7
	OPTIM_STRATS=(
		""
		"crc32"
		"asm_strcmp"
		"crc32 asm_strcmp"
		"strlen"
		"asm_strcmp strlen"
		"crc32 strlen"
		"crc32 asm_strcmp strlen"
	)

	for strat in "${OPTIM_STRATS[@]}"; do
		read -r -a strat_args <<< "$strat"
		STRAT_FLAG=$(get_strat_flag "${strat_args[@]}")

		make clean >/dev/null
		make FLAGS="$STRAT_FLAG" >/dev/null

		echo "Test with strat \"$strat\""
		

		for i in $(seq "$K_WARMUP"); do
			echo "Warmup $i/$K_WARMUP: $(./build/tester) ms"
		done

		for i in $(seq "$K_RUNS"); do
			STRAT_TIME=$(./build/tester)
			echo "\"$strat\",$i,$STRAT_TIME" >> "$REPORT_FILE"
			echo "Run $i/$K_RUNS: $STRAT_TIME ms"
		done
	done

	echo "Done"
}

build_strat() {
	STRAT_FLAG=$(get_strat_flag "$@")
	make clean
	make FLAGS="$STRAT_FLAG"
}

if [[ "$@" != "" ]]; then
	echo "Building strategy $@"
	build_strat "$@"
else
	echo "Running all tests"
	run_tests
fi
