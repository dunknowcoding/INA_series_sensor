<#
.SYNOPSIS
  Cross-platform compile matrix for INA_Series_Sensor library.
  Tests representative examples on multiple Arduino board targets.
.USAGE
  powershell -ExecutionPolicy Bypass -File scripts/compile-matrix.ps1
#>
param(
  [string]$LibPath = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = "Continue"

# ---- Board matrix (FQBN → short name) ----
$boards = [ordered]@{
  "arduino:avr:uno"                         = "AVR-Uno"
  "esp32:esp32:esp32c3"                     = "ESP32-C3"
  "esp8266:esp8266:generic"                 = "ESP8266"
  "rp2040:rp2040:rpipico"                   = "RP2040-Pico"
  "arduino:samd:arduino_zero_native"        = "SAMD-Zero"
  "STMicroelectronics:stm32:Nucleo_64"      = "STM32-Nucleo64"
  "arduino:megaavr:nona4809"                = "megaAVR-NanoEvery"
  "arduino:renesas_uno:minima"              = "Renesas-UNOR4"
}

# ---- Representative examples ----
$i2cExamples = @(
  "ina228_basic"
  "ina228_advanced"
  "ina228_alert_interrupt"
  "ina226_basic"
  "ina219_basic"
  "ina3221_basic"
  "ina3221_advanced"
)

$spiExamples = @(
  "ina229_basic"
  "ina229_advanced"
)

$allExamples = $i2cExamples + $spiExamples

# ---- Results table ----
$results = @()
$passCount = 0
$failCount = 0
$skipCount = 0

foreach ($fqbn in $boards.Keys) {
  $boardName = $boards[$fqbn]

  foreach ($ex in $allExamples) {
    $inoDir  = Join-Path (Join-Path $LibPath "examples") $ex
    $inoFile = Join-Path $inoDir "$ex.ino"

    if (-not (Test-Path $inoFile)) {
      $results += [PSCustomObject]@{Board=$boardName; Example=$ex; Status="SKIP"; Flash=""; RAM=""; Note="ino not found"}
      $skipCount++
      continue
    }

    Write-Host -NoNewline "  $boardName / $ex ... "

    $output = & arduino-cli compile --fqbn $fqbn --library $LibPath $inoFile 2>&1 | Out-String

    if ($LASTEXITCODE -eq 0 -and $output -match "Sketch uses (\d+) bytes.*Maximum is (\d+)") {
      $flash = $Matches[1]
      $flashMax = $Matches[2]
      $pct = [math]::Round(100 * [int]$flash / [int]$flashMax)

      $ramInfo = ""
      if ($output -match "Global variables use (\d+) bytes.*Maximum is (\d+)") {
        $ram = $Matches[1]
        $ramMax = $Matches[2]
        $ramPct = [math]::Round(100 * [int]$ram / [int]$ramMax)
        $ramInfo = "${ramPct}%"
      }

      Write-Host "PASS (${pct}% flash, ${ramInfo} RAM)" -ForegroundColor Green
      $results += [PSCustomObject]@{Board=$boardName; Example=$ex; Status="PASS"; Flash="${pct}%"; RAM=$ramInfo; Note=""}
      $passCount++
    } else {
      # Extract first error line
      $errLine = ""
      $lines = $output -split "`n"
      foreach ($l in $lines) {
        if ($l -match "error:") {
          $errLine = ($l -replace ".*error:\s*", "").Trim().Substring(0, [Math]::Min(80, ($l -replace ".*error:\s*", "").Trim().Length))
          break
        }
      }
      Write-Host "FAIL" -ForegroundColor Red
      if ($errLine) { Write-Host "    $errLine" -ForegroundColor DarkRed }
      $results += [PSCustomObject]@{Board=$boardName; Example=$ex; Status="FAIL"; Flash=""; RAM=""; Note=$errLine}
      $failCount++
    }
  }
}

# ---- Summary ----
Write-Host ""
Write-Host "============================================"
Write-Host " Compile Matrix Summary"
Write-Host "============================================"
Write-Host "  PASS: $passCount   FAIL: $failCount   SKIP: $skipCount"
Write-Host ""

$results | Format-Table -AutoSize

if ($failCount -gt 0) {
  Write-Host "FAILURES:" -ForegroundColor Red
  $results | Where-Object { $_.Status -eq "FAIL" } | Format-Table -AutoSize
  exit 1
}

Write-Host "All compilations passed!" -ForegroundColor Green
exit 0
