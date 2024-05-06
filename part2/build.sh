#!/bin/bash

set -e
# set -x

profile_flag="-DPERFAWARE_PROFILE"
release_flags="-O3 -g ${profile_flag}"
debug_flags="-O0 -ggdb3 ${profile_flag}"
common_flags="-Wall -Wextra --std=c++20"

echo -n "Compilation Time:"
time {
    # g++ ${release_flags} ${common_flags} -o genPoints point_generator.cpp &
    # g++ ${debug_flags} ${common_flags} -o genPoints_db point_generator.cpp &
    g++ ${release_flags} ${common_flags} -o haversine haversine_processor.cpp &
    g++ ${debug_flags} ${common_flags} -o haversine_db haversine_processor.cpp &
    g++ ${debug_flags} ${common_flags} -o read_test read_repetition_tester.cpp &
    g++ ${release_flags} ${common_flags} -o read_test read_repetition_tester.cpp &
    wait
}
echo ""
