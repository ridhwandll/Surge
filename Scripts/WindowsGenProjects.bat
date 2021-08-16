@echo off
pushd %~dp0\..\
echo [32m-----Generating project files for Surge [Windows x64]-----[0m
call mkdir build
call cd build
call cmake ..
echo [32m-----Done :)-----[0m
PAUSE
