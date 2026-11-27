# Qt 客户端打包脚本（Windows）
# 用法（在 Windows PowerShell 中）：
#   powershell -ExecutionPolicy Bypass -File .\client\package.ps1
# 前提：已用 Qt 6.9.3 MinGW 以 Release 模式构建出 client\bin\wechat.exe
param(
    [string]$QtBin = "D:\Qt\6.9.3\mingw_64\bin",
    [string]$Exe = "$PSScriptRoot\bin\wechat.exe"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $Exe)) {
    throw "未找到 $Exe，请先用 Qt Creator 以 Release 模式构建客户端"
}

$windeployqt = Join-Path $QtBin "windeployqt.exe"
if (-not (Test-Path $windeployqt)) {
    throw "未找到 $windeployqt，请用 -QtBin 指定 Qt 的 bin 目录"
}

$dist = Join-Path $PSScriptRoot "dist\wechat"
if (Test-Path $dist) { Remove-Item -Recurse -Force $dist }
New-Item -ItemType Directory -Force $dist | Out-Null
Copy-Item $Exe $dist

& $windeployqt --release --compiler-runtime --no-translations (Join-Path $dist "wechat.exe")

$zip = Join-Path $PSScriptRoot "dist\wechat-windows-x64.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path "$dist\*" -DestinationPath $zip

Write-Host "打包完成: $zip"
