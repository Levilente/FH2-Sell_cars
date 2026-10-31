@echo off
setlocal EnableExtensions
cd /d "%~dp0"
rem Builds dist\SellCar.dll with Zig 0.15.2 (https://ziglang.org/download/).
rem zig.exe: first argument if given (drop zig.exe onto this file)else zig from PATH.
set "ZIG=%~1"
if not defined ZIG set "ZIG=zig"
if not "%ZIG%"=="zig" if not exist "%ZIG%" (
 echo ERROR: zig.exe not found: %ZIG%
 echo Drop Zig 0.15.2 zig.exe onto this batch fileor put zig on PATH.
 pause
 exit /b 1
)
if not exist dist mkdir dist
"%ZIG%" cc -target x86_64-windows-gnu -std=c11 -O2 -shared -ffreestanding -fno-builtin -fno-stack-protector -Iinclude -Isrc src\SellCar.c -luser32 -lkernel32 -o dist\SellCar.dll
if errorlevel 1 (
 echo BUILD FAILED.
 pause
 exit /b 1
)
echo.
echo BUILD COMPLETE: dist\SellCar.dll
echo Copy it to ^<Azimuth folder^>\fh2-mods\ (game closed).
pause
endlocal
