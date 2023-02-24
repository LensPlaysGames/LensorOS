:: ASSUMES NINJA BUILD SYSTEM AND WORKING DIRECTORY OF ROOT OF REPOSITORY

set PROC=
for /f "delims=" %%a in ('echo %NUMBER_OF_PROCESSORS%') do @set PROC=%%a

:: SYSTEM LIBRARIES
cmake -G Ninja -S user/libc -B user/libc/winja
if %errorlevel% neq 0 exit /b %errorlevel%
cmake --build user/libc/winja --clean-first -- -j %PROC%
if %errorlevel% neq 0 exit /b %errorlevel%
:: USERSPACE PROGRAMS
cmake -G Ninja -S user/blazeit -B user/blazeit/winja
if %errorlevel% neq 0 exit /b %errorlevel%
cmake --build user/blazeit/winja --clean-first -- -j %PROC%
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /y user\blazeit\winja\blazeit user\blazeit\blazeit
if %errorlevel% neq 0 exit /b %errorlevel%
cmake -G Ninja -S user/stdout -B user/stdout/winja
if %errorlevel% neq 0 exit /b %errorlevel%
cmake --build user/stdout/winja --clean-first -- -j %PROC%
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /y user\stdout\winja\stdout user\stdout\stdout
if %errorlevel% neq 0 exit /b %errorlevel%
:: KERNEL
cmake -G Ninja -S kernel -B kernel/winja -DHIDE_UART_COLOR_CODES=OFF
if %errorlevel% neq 0 exit /b %errorlevel%
cmake --build kernel/winja --target runhda_qemu --clean-first -- -j %PROC%
if %errorlevel% neq 0 exit /b %errorlevel%
