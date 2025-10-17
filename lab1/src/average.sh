#!/usr/bin/env bash

awk 'BEGIN {
  for (i = 1; i < ARGC; i++) s += ARGV[i];
  c = ARGC - 1;
  printf "count: %d\naverage: %.6f\n", c, s / c
}' "$@"