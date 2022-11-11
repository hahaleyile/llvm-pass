#!/usr/bin/env bash

answers=(
"10 : plus"

"22 : plus"

"24 : minus, plus"

"27 : minus, plus"

"10 : minus, plus
26 : foo
33 : foo"

"33 : minus, plus"

"10 : minus, plus
26 : clever"

"10 : minus, plus
28 : clever
30 : clever"

"10 : minus, plus
26 : clever"

"10 : minus, plus
14 : foo
30 : clever"

"15 : minus, plus
19 : foo
35 : clever"

"15 : foo
16 : plus
32 : clever"

"15 : foo
16 : minus, plus
32 : clever"

"30 : clever, foo
31 : minus, plus"

"24 : foo
31 : clever, foo
32 : minus, plus"

"14 : minus, plus
24 : foo
27 : foo"

"14 : foo
17 : clever
24 : clever1
25 : plus"

"14 : foo
17 : clever
24 : clever1
25 : plus"

"14 : foo
18 : clever, foo
30 : clever1
31 : plus"

"14 : foo
18 : clever, foo
24 : clever, foo
36 : clever1
37 : minus, plus"
)


for i in {0..19} ; do
  echo "$i"
  echo "---------------"
  echo -e "${answers[$i]}"
  echo "---------------"

  str=""
  if [ "$i" -lt 10 ]; then
    str="0$i"
  else
    str="$i"
  fi
  if output="$(./cmake-build-debug/llvmassignment "./bc/test$str.bc" 2>&1)"; then
    echo -e "$output"
    echo "---------------"
    if [[ "$output" == "${answers[$i]}" ]]; then
      echo "true"
    else
      echo "false"
    fi
  else
    echo "error"
    echo "---------------"
    echo "false"
  fi
  echo "---------------"

  echo ""
done
