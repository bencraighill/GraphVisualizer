@echo off

REM Ensure Python is available
python --version
IF %ERRORLEVEL% NEQ 0 (
    echo Python not found.
    exit /b 1
)

REM Install Python dependencies
pip install osmnx pyyaml

REM Optional: ensure spatial libs (Windows wheels include them)
REM No extra system setup needed on Windows.

REM Build project
mkdir build 2>nul
cmake -S . -B build
cmake --build build

