#!/bin/bash
ERRORS=0

function assert() {
  local EXPECTED=$1
  local ACTUAL=$2
  if [[ $EXPECTED != $ACTUAL ]]; then
    echo "ERROR: Expected '$EXPECTED', got: '$ACTUAL'"
    ERRORS=1
  fi
}

echo "Testing default behavior."
EXPECTED="Hello"
ACTUAL=$(echo $EXPECTED | ./ssss-split -t 3 -n 5 2>/dev/null | head -3 | ./ssss-combine -t 3 2>/dev/null)
assert $EXPECTED $ACTUAL

echo "Testing maximum security."
EXPECTED="Hello"
ACTUAL=$(echo $EXPECTED | ./ssss-split -t 3 -n 5 -s 1024 2>/dev/null | head -3 | ./ssss-combine -t 3 2>/dev/null)
assert $EXPECTED $ACTUAL

echo "Testing minimum security."
EXPECTED="Hello"
ACTUAL=$(echo $EXPECTED | ./ssss-split -t 3 -n 5 -s 40 2>/dev/null | head -3 | ./ssss-combine -t 3 2>/dev/null)
assert $EXPECTED $ACTUAL

echo "Testing excessively long secret."
EXPECTED=1
ACTUAL=$(echo "AnExceedinglyLongValueThatShouldResultInAnError" | ./ssss-split -t 3 -n 5 -s 40 2>&1 | grep 'FATAL: input string too long.' | wc -l)
assert $EXPECTED $ACTUAL

echo "Testing hex encoding."
EXPECTED="00000048656c6c6f"
ACTUAL=$(echo $EXPECTED | ./ssss-split -t 3 -n 5 -x 2>/dev/null | head -3 | ./ssss-combine -t 3 -x 2>/dev/null)
assert $EXPECTED $ACTUAL


if [[ $ERRORS -gt 0 ]]; then
  exit 1
fi
