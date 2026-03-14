#/usr/bin/bash

set -e

flags="\
 -g \
 -Wall \
 -Werror \
 -Wextra \
 -Wpedantic \
 -Wconversion \
 -Wformat=2 \
 -Wshadow \
 -Wno-overlength-strings \
"

# gcc $flags asset_pipeline.c
# ./a.out

gcc $flags page.c -lm
# ./a.out
