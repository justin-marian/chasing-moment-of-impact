@echo off
setlocal

:: Ensure the script is running from the project root
cd /d "%~dp0"

echo ===================================================
echo  Chasing Moment of Impact - Viewer Launcher
echo ===================================================
echo.

:: ---------------------------------------------------------
:: Choose your run mode by uncommenting ONE of the lines below
:: ---------------------------------------------------------

:: Default Launch (CPU, Demo Scene)
:: .\exports\builds\windows-x64\physics_viewer.exe

:: Heavy Load (CUDA, Matrix A - 850 bodies) - RECOMMENDED
.\exports\builds\windows-x64\physics_viewer.exe --backend cuda --scenario A

:: Impact Strength Test (No gravity, focused on collisions)
:: .\exports\builds\windows-x64\physics_viewer.exe --scenario impact

:: Launch Paused (To inspect the initial spawn state)
:: .\exports\builds\windows-x64\physics_viewer.exe --paused

:: Headless Capture (Saves a PNG without opening a window)
:: .\exports\builds\windows-x64\physics_viewer.exe --hidden --frames 60 --capture media/capture_A.png --scenario A

echo.
echo Viewer closed. If it crashed immediately, check the error above.
pause
