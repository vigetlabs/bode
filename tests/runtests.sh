#!/bin/sh
#
# Run every compiled test binary in tests/ (named *_tests). Honors $VALGRIND:
# set it to a valgrind invocation to run each test under it, e.g.
#
#     VALGRIND="valgrind --error-exitcode=1 --leak-check=full" make test
#
# A test fails the run by exiting non-zero; full output is appended to
# tests/tests.log for inspection.

echo "Running unit tests:"

: > tests/tests.log

for test in tests/*_tests
do
    if [ -f "$test" ]
    then
        if $VALGRIND ./"$test" 2>> tests/tests.log
        then
            echo "PASS $test"
        else
            echo "FAIL $test — see tests/tests.log"
            echo "------"
            tail -n 20 tests/tests.log
            exit 1
        fi
    fi
done

echo ""
