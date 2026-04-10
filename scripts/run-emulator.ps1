# run-emulator.ps1
# This script builds the firmware and launches the web emulator

$ProjectRoot = Get-Location
$EmulatorDir = Join-Path $ProjectRoot "emulator"
$PioBuildDir = Join-Path $ProjectRoot ".pio\build\esp32dev"
$FirmwareBin = Join-Path $PioBuildDir "firmware.bin"

Write-Host "--- ToasterFirmware Emulator Runner ---" -ForegroundColor Cyan

# 1. Build Firmware if requested
if ($args -contains "--build") {
    Write-Host "[1/3] Building firmware with PlatformIO..." -ForegroundColor Yellow
    pio run
    if ($LASTEXITCODE -ne 0) {
        Write-Error "PlatformIO build failed."
        exit $LASTEXITCODE
    }
}

# 2. Check if firmware exists
if (-not (Test-Path $FirmwareBin)) {
    Write-Warning "Firmware binary not found at $FirmwareBin"
    Write-Host "Please run with --build flag or build via PlatformIO IDE."
} else {
    Write-Host "[2/3] Found firmware binary: $(Get-Item $FirmwareBin | Select-Object -ExpandProperty Name)" -ForegroundColor Green
    # Copy firmware to public folder for easy access by web app (optional, or we can use a server)
    $PublicDir = Join-Path $EmulatorDir "public\firmware"
    if (-not (Test-Path $PublicDir)) { New-Item -ItemType Directory -Path $PublicDir -Force }
    Copy-Item $FirmwareBin -Destination (Join-Path $PublicDir "firmware.bin") -Force
}

# 3. Launch Emulator
Write-Host "[3/3] Launching web emulator..." -ForegroundColor Yellow
Set-Location $EmulatorDir
npm run dev
