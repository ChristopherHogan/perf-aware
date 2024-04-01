#!/bin/bash

set -e

time g++ -O3 -g -Wall -Wextra --std=c++20 -o sim86 sim86.cpp
time g++ -ggdb3 -O0 -Wall -Wextra --std=c++20 -o sim86db sim86.cpp

files=(
    "listing_0037_single_register_mov"
    "listing_0038_many_register_mov"
    "listing_0039_more_movs"
    "listing_0040_challenge_movs"
    "listing_0041_add_sub_cmp_jnz"
)

if [[ ! -z "${TEST}" ]]; then
    for f in "${files[@]}"; do
        echo ${f}
        nasm ${f}.asm
        ./sim86 ${f}
        decoded_name=$(echo ${f} | grep -E -o '[^_]*_[0-9]{4}')_decoded
        nasm ${decoded_name}.asm
        diff ${f} ${decoded_name}
    done
fi

