#!/usr/bin/env bash

mkdir -p bc
(
  cd tests
  cfiles=$(ls .)
  for file in $cfiles; do
    prefix=$(basename -s .c "$file")
    bc_file="$prefix.bc"
    clang -emit-llvm -c -O0 -g3 "$file" -o "$bc_file"
    ../cmake-build-debug/dis "$bc_file" > "$prefix.ll" 2>&1
  done
)
mv tests/*.bc bc
mv tests/*.ll bc
