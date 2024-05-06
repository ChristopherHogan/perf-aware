@echo off

IF NOT EXIST build mkdir build
pushd build

call "C:\program files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64 

setlocal

set profile_flag=/DPERFAWARE_PROFILE

call cl %profile_flag% -Zi -W4 -EHsc -nologo -std:c++20 ..\%1 -Fe%~n1_dm.exe
call cl %profile_flag% -arch:AVX2 -O2 -Zi -W4 -EHsc -nologo -std:c++20 ..\%1 -Fe%~n1_rm.exe

popd