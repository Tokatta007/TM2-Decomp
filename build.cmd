@echo off
setlocal
"%~dp0toolchain\python\python.exe" -I -B "%~dp0build.py" %*
exit /b %errorlevel%
