#!/bin/bash
set -e

: ${TEST_DIR:="tests"}
mkdir -p "$TEST_DIR"

python ded_genrandom.py > "$TEST_DIR/test_set.in"

echo "Done"
