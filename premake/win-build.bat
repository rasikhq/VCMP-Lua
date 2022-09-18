@echo off
pushd %~dp0\..\
call premake\premake5.exe clean
call premake\premake5.exe vs2022
popd
PAUSE