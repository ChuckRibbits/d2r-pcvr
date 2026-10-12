param([switch]$CheckOnly, [switch]$ExperimentalBattleNet, [switch]$Uninstall)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
try {
    if (![Environment]::Is64BitOperatingSystem) { throw 'D2R PCVR requires 64-bit Windows.' }
    $modDirectory = Split-Path $PSScriptRoot -Parent
    $runtimeDirectory = Join-Path $modDirectory '.runtime'
    $pythonDirectory = Join-Path $runtimeDirectory 'python'
    $pythonExecutable = Join-Path $pythonDirectory 'python.exe'
    if (!(Test-Path -LiteralPath $pythonExecutable)) {
        Write-Host 'D2R PCVR Windows setup'
        Write-Host 'Preparing a private Python runtime from python.org (11 MB). No system Python installation is needed.'
        New-Item -ItemType Directory -Force -Path $runtimeDirectory | Out-Null
        $archive = Join-Path $runtimeDirectory 'python-3.12.10-embed-amd64.zip'
        $digest = '4acbed6dd1c744b0376e3b1cf57ce906f9dc9e95e68824584c8099a63025a3c3'
        if (!(Test-Path -LiteralPath $archive) -or (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $digest) {
            [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
            Invoke-WebRequest -UseBasicParsing -Uri 'https://www.python.org/ftp/python/3.12.10/python-3.12.10-embed-amd64.zip' -OutFile $archive
        }
        if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $digest) { throw 'Python download checksum mismatch. Setup stopped.' }
        Expand-Archive -LiteralPath $archive -DestinationPath $pythonDirectory -Force
    }
    $signature = Get-AuthenticodeSignature -LiteralPath $pythonExecutable
    if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'Python Software Foundation') {
        throw 'Python publisher signature could not be verified. Setup stopped.'
    }
    # Keep this interpreter private to the extracted release, including imports.
    Set-Content -LiteralPath (Join-Path $pythonDirectory 'python312._pth') -Encoding ASCII -Value @('python312.zip', '.', '../../steamvr', '../..')
    $setupArguments = @()
    if ($CheckOnly) { $setupArguments += '--check' }
    if ($ExperimentalBattleNet) { $setupArguments += '--experimental-battlenet' }
    $setupScript = if ($Uninstall) { 'uninstall_windows.py' } else { 'setup_windows.py' }
    & $pythonExecutable (Join-Path $PSScriptRoot $setupScript) @setupArguments
    exit $LASTEXITCODE
} catch {
    Write-Host "Setup could not finish: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host 'Keep the extracted folder and consult INSTALL-WINDOWS.md. No security settings were changed.'
    exit 1
}
