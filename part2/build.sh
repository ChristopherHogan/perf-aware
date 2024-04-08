#!/bin/bash

set -e

decode_files=(
)

release_flags="-O3 -g"
debug_flags="-O0 -ggdb3"
common_flags="-Wall -Wextra --std=c++20"

echo -n "Compilation Time:"
time {
    g++ ${release_flags} ${common_flags} -o genPoints point_generator.cpp &
    g++ ${debug_flags} ${common_flags} -o genPoints_db point_generator.cpp &
    # g++ ${release_flags} ${common_flags} -o test_sim86 test_sim86.cpp &
    # g++ ${debug_flags} ${common_flags} -o test_sim86db test_sim86.cpp &
    wait
}
echo ""

# if [[ ! -z "${TEST}" ]]; then
#     exe_name=sim86
#     test_name=test_${exe_name}

#     for f in "${decode_files[@]}"; do
#         echo ${f}
#         ./${exe_name} ${f}
#         decoded_name=$(echo ${f} | grep -E -o '[^_]*_[0-9]{4}')_decoded
#         nasm ${decoded_name}.asm
#         diff ${f} ${decoded_name}
#     done
# fi
# echo ""

# if [[ ! -z "${EXEC}" ]]; then
#     ./${test_name}
# fi
