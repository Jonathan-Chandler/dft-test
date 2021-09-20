@ECHO OFF
call mingw32-make clean
echo.
call mingw32-make
echo.
if errorlevel 1 goto build_failed

call bin\mic.exe
goto exit_make

:build_failed
echo.
echo Build failed
goto exit_make

:unit_test_failed
echo.
echo Unit test failed
goto exit_make

:exit_make
