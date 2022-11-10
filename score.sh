#!/usr/bin/env bash

answers=(
  "10 : plus"

  "22 : plus"

  "24 : plus, minus"

  "27 : plus, minus"

  "10 : plus, minus
26 : foo
33 : foo"

"33 : plus, minus"

"10 : plus, minus
26 : clever"

"10 : plus, minus
28 : clever
30 : clever"

"10 : plus, minus
26 : clever"

"10 : plus, minus
14 : foo
30 : clever"

"15 : plus, minus
19 : foo
35 : clever"

"15 : foo
16 : plus
32 : clever"

"15 : foo
16 : plus, minus
32 : clever"

"30 : foo, clever
31 : plus, minus"

"24 : foo
31 : clever, foo
32 : plus, minus"

"14 : plus, minus
24 : foo
27 : foo"

"14 : foo
17 : clever
24 : clever1
25 : minus"

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
37 : plus"
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
  else
    echo "error"
  fi
  echo "---------------"

  echo ""
done

