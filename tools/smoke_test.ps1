param(
    [string]$ProjectRoot = "C:\Users\22841\Desktop\MCStudio",
    [int]$WaitSeconds = 6
)

$ErrorActionPreference = "Stop"

$buildDir = Join-Path $ProjectRoot "build"
$exePath = Join-Path $buildDir "MCStudioIoTester.exe"
$compileBat = Join-Path $ProjectRoot "compile.bat"

Write-Host "[1/5] Stop existing process"
Get-Process MCStudioIoTester -ErrorAction SilentlyContinue | Stop-Process -Force

Write-Host "[2/5] Build"
Push-Location $ProjectRoot
try {
    cmd /c $compileBat | Out-Host
} finally {
    Pop-Location
}

Write-Host "[3/5] Launch"
Start-Process -FilePath $exePath -WorkingDirectory $buildDir

Write-Host "[4/5] Wait for app"
Start-Sleep -Seconds $WaitSeconds

Write-Host "[5/5] Check process and recent crash events"
$proc = Get-Process MCStudioIoTester -ErrorAction SilentlyContinue
if (-not $proc) {
    Write-Host "FAIL: process is not running" -ForegroundColor Red
    exit 1
}

$proc | Select-Object Id, ProcessName, MainWindowTitle, Responding, StartTime, Path | Format-List | Out-Host

try {
    $events = Get-WinEvent -FilterHashtable @{LogName='Application'; StartTime=(Get-Date).AddMinutes(-5)} |
        Where-Object { $_.ProviderName -in @('Application Error', 'Windows Error Reporting') -and $_.Message -like '*MCStudioIoTester*' } |
        Select-Object -First 5 TimeCreated, ProviderName, Id, Message

    if ($events) {
        Write-Host "WARN: recent crash-related events found" -ForegroundColor Yellow
        $events | Format-List | Out-Host
    } else {
        Write-Host "PASS: no recent crash-related events found" -ForegroundColor Green
    }
} catch {
    Write-Host "INFO: no matching Windows Application events were found"
}
