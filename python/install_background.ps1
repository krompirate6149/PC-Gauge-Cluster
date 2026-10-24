$ErrorActionPreference = 'Stop'

$sourceScript = Join-Path $PSScriptRoot 'kadran_arka_plan.py'
if (-not (Test-Path $sourceScript)) {
    throw "kadran_arka_plan.py bulunamadi. PS1 dosyasini Python dosyasiyla ayni klasorde calistirin."
}

# Python 3 kurulumunu bul
$pythonExe = (& py -3 -c "import sys; print(sys.executable)" 2>$null | Select-Object -First 1)
if (-not $pythonExe) {
    throw "Python 3 bulunamadi. Python 3 kurun ve kurulum sirasinda 'Add Python to PATH' secenegini etkinlestirin."
}
$pythonExe = $pythonExe.Trim()
$pythonwExe = Join-Path (Split-Path $pythonExe -Parent) 'pythonw.exe'
if (-not (Test-Path $pythonwExe)) {
    throw "pythonw.exe bulunamadi: $pythonwExe"
}

Write-Host 'Gerekli Python paketleri kuruluyor...'
& $pythonExe -m pip install psutil pyserial
if ($LASTEXITCODE -ne 0) {
    throw 'Paket kurulumu basarisiz. Internet baglantisini ve pip kurulumunu kontrol edin.'
}

# Scripti kalici bir konuma kopyala; ZIP klasorunun tasinmasi kisayolu bozmasin.
$appDir = Join-Path $env:LOCALAPPDATA 'KadranArkaPlan'
New-Item -ItemType Directory -Path $appDir -Force | Out-Null
$installedScript = Join-Path $appDir 'kadran_arka_plan.py'
Copy-Item $sourceScript $installedScript -Force

# Windows oturum acilis klasorune gorunmez pythonw kisayolu ekle.
$startupDir = [Environment]::GetFolderPath('Startup')
$shortcutPath = Join-Path $startupDir 'CPU-GPU-RAM Kadran.lnk'
$wsh = New-Object -ComObject WScript.Shell
$shortcut = $wsh.CreateShortcut($shortcutPath)
$shortcut.TargetPath = $pythonwExe
$shortcut.Arguments = '"' + $installedScript + '"'
$shortcut.WorkingDirectory = $appDir
$shortcut.WindowStyle = 7
$shortcut.Description = 'CPU-GPU-RAM fiziksel kadranlarini arka planda calistirir'
$shortcut.Save()

Write-Host ''
Write-Host 'Kurulum tamamlandi.' -ForegroundColor Green
Write-Host "Uygulama: $installedScript"
Write-Host "Baslangic kisayolu: $shortcutPath"
Write-Host "Log dosyasi: $(Join-Path $appDir 'kadran.log')"
Write-Host 'Simdi bir kez Windows oturumunu kapatip acin veya kisayola cift tiklayin.'
