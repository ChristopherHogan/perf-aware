#!/bin/bash

set -e
# set -x

if [[ ! -d ./build ]]; then
    mkdir -p build
fi

pushd build

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     machine=Linux;;
    MINGW*)     machine=MinGw;;
    *)          machine="UNKNOWN:${unameOut}"
esac

if [ "$machine" = "Linux" ]; then
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
elif [ "$machine" = "MinGw" ]; then
    profile_flag="-DPERFAWARE_PROFILE"
    release_flags="-arch:AVX2 -O2 ${profile_flag}"
    debug_flags="${profile_flag}"
    common_flags="-Zi -W4 -EHsc -nologo -std:c++20"

    programs=("point_generator" "haversine_processor" "read_repetition_tester")

    echo -n "Compilation Time:"
    time {
        for prog in "${programs[@]}"; do
            cl ${release_flags} ${common_flags} -Fd${prog}.pdb ../${prog}.cpp -Fe${prog}.exe &
            cl ${debug_flags} ${common_flags} -Fo: ${prog}_db.obj -Fd${prog}_db.pdb ../${prog}.cpp -Fe${prog}_db.exe &
        done
        wait
    }
    echo ""
fi

popd
