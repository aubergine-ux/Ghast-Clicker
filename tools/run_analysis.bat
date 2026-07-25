@echo off
setlocal enabledelayedexpansion

:: 1. Check if Python is even installed
python3 --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Python is not installed or not in your PATH.
    pause
    exit /b
)

echo [GHOSTTAP] Checking dependencies...

:: 2. Check for required libraries
python3 -c "import pandas, matplotlib, seaborn" >nul 2>&1

if %errorlevel% neq 0 (
    echo [GHOSTTAP] Missing libraries. Installing requirements...
    python3 -m pip install pandas matplotlib seaborn
    
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to install dependencies automatically. 
        echo Please run: pip install pandas matplotlib seaborn
        pause
        exit /b
    )
)

:: 3. Run the script
echo [GHOSTTAP] Launching Analysis...
python3 src/analyze.py

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] The Python script crashed. Check the logs above.
)