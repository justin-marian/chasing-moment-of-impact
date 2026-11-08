@echo off
setlocal

:: Go up one level from scripts/ to the project root
cd /d "%~dp0.."

echo ===================================================
echo Chasing the Moment of Impact - Build and Launch
echo ===================================================
echo.

:: ---------------------------------------------------------
:: BUILD THE PROJECT
:: ---------------------------------------------------------
echo [1/2] Building the simulator...

:: NOTE: If you do not have an NVIDIA GPU, remove the -Cuda flag below!
powershell -ExecutionPolicy Bypass -File ".\scripts\deployment\build-windows.ps1" -Cuda -Viewer -Architecture 89

:: Check if the build succeeded
if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Build failed. Aborting launch.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo [2/2] Build successful. Launching viewer...
echo.

:: ---------------------------------------------------------
:: CHOOSE YOUR RUN MODE
:: ---------------------------------------------------------
:: Uncomment ONE of the lines below to choose your launch mode.

:: Default Launch (CPU, Demo Scene)
:: .\exports\builds\windows-x64\physics_viewer.exe

:: Heavy Load (CUDA, Matrix A - 850 bodies) - RECOMMENDED
.\exports\builds\windows-x64\physics_viewer.exe --backend cuda --scenario A

:: Impact Strength Test (No gravity, focused on collisions)
:: .\exports\builds\windows-x64\physics_viewer.exe --scenario impact

:: Launch Paused (To inspect the initial spawn state)
:: .\exports\builds\windows-x64\physics_viewer.exe --paused

:: Headless Capture (Saves a PNG without opening a window)
:: .\exports\builds\windows-x64\physics_viewer.exe --hidden --frames 60 --capture media\capture_A.png --scenario A

echo.
echo Viewer closed. If it crashed immediately, check the error above.
pause
