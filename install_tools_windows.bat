@echo off
REM PS2 x86 OS Tools Installation Script for Windows
REM Installs all missing tools for development and testing

echo === PS2 x86 OS Tools Installation Script for Windows ===
echo Installing all missing tools for PS2 x86 OS development...
echo.

REM Check if we're in WSL or native Windows
if exist /proc/version (
    echo [INFO] Running in WSL/Linux environment
    echo Please run the Linux installation script instead: bash install_tools.sh
    exit /b 1
)

echo [INFO] Running in native Windows environment
echo.

REM Check for Chocolatey
where choco >nul 2>&1
if %errorlevel% neq 0 (
    echo [WARNING] Chocolatey not found. Installing Chocolatey...
    echo Please run PowerShell as Administrator and execute:
    echo Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
    echo.
    echo After installing Chocolatey, run this script again.
    pause
    exit /b 1
)

echo [INFO] Chocolatey found. Installing tools...
echo.

REM Install essential development tools
echo [INFO] Installing essential development tools...
choco install -y make nasm mingw git

REM Install QEMU for emulation
echo [INFO] Installing QEMU for emulation...
choco install -y qemu

REM Install ISO creation tools
echo [INFO] Installing ISO creation tools...
choco install -y cdrtools

REM Install additional useful tools
echo [INFO] Installing additional useful tools...
choco install -y hexdump file

echo.
echo [INFO] Installation complete!
echo.
echo [INFO] Next steps:
echo 1. Open a new command prompt (to refresh PATH)
echo 2. Navigate to your PS2 x86 OS directory
echo 3. Run: make clean ^&^& make all
echo 4. Test with: make run
echo 5. Create ISO: make iso
echo.
echo [INFO] Note: For CD burning on Windows, you may need additional software
echo like ImgBurn or Nero Burning ROM.
echo.
echo Happy PS2 x86 OS development! 🎮
pause
