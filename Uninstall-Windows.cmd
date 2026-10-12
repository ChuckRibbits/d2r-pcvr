@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0steamvr\setup_windows.ps1" -Uninstall
set "setup_result=%errorlevel%"
echo.
pause
exit /b %setup_result%
