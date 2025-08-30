@echo off
setlocal

clang\bin\clang-cl.exe /EHsc /std:c++20 main.cpp /Fe:runtime/main.exe

endlocal