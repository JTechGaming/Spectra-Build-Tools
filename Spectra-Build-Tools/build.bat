@echo off
setlocal

:: Compile TestClass.cpp into a DLL with debug info and no optimization
ccache\ccache.exe clang\bin\clang-cl.exe /LD /Fe:runtime\TestClass.dll TestClass.cpp /MD /nologo


endlocal