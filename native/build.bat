@echo off
rem Compila native\build\tbfg_cli.exe con g++ (p. ej. w64devkit) o cl (MSVC).
setlocal
cd /d "%~dp0"
if not exist build mkdir build
set SRC=tbfg_core\fixed16.cpp tbfg_cli\main.cpp

where g++ >nul 2>&1
if %errorlevel%==0 (
	g++ -std=c++17 -O2 -Wall -Wextra -pedantic -static -o build\tbfg_cli.exe %SRC%
	exit /b %errorlevel%
)
where cl >nul 2>&1
if %errorlevel%==0 (
	cl /nologo /std:c++17 /O2 /EHsc /W4 /Fobuild\ /Fe:build\tbfg_cli.exe %SRC%
	exit /b %errorlevel%
)
echo No se encontro g++ ni cl. Instala w64devkit o MSVC Build Tools y anadelo al PATH.
exit /b 1
