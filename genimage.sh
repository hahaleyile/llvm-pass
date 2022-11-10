#!/usr/bin/env bash

cd bc
cfiles="$(ls *.ll)"
for file in $cfiles; do
  name=$(basename -s .ll "$file")
  opt -dot-cfg "$file" >/dev/null
  dots="$(ls .*.dot)"
  for dot in $dots; do
    funcname=$(basename -s .dot "$dot")
    dot -Tpng -o ./"$name$funcname".png $dot >/dev/null
  done
  opt -dot-callgraph "$file" >/dev/null
  dot -Tpng -o ./"$name".callgraph.png ./*callgraph.dot >/dev/null
  rm .*.dot
  rm ./*callgraph.dot
done
