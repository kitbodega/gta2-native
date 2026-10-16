# link_tranche1.ps1 - Link test del motor GTA2 hacia libGTA2.so (ARM Android).
# Objetivo: inventario de simbolos undefined reference (link adn no enlaza del todo).
$ErrorActionPreference = "Continue"

$RepoRoot   = "C:\Users\Admin\Downloads\gta2-build\gta2_re"
$ProjectDir = "C:\Users\Admin\Downloads\gta2-build\gta2-native\app\src\main\cpp"
$Ndk        = "C:\Android\sdk\ndk\26.1.10909125"
$ClangCC    = Join-Path $Ndk "toolchains\llvm\prebuilt\windows-x86_64\bin\armv7a-linux-androideabi24-clang++.cmd"
$ObjDir     = "C:\Users\Admin\AppData\Local\Temp\opencode\gta2_t1"
New-Item -ItemType Directory -Path $ObjDir -Force | Out-Null

$SrcDir = Join-Path $RepoRoot "Source"
$IncDirs = @(
    (Join-Path $ProjectDir "platform"),
    $SrcDir,
    (Join-Path $SrcDir "3rdParty"),
    $RepoRoot
)
$IncStr = ($IncDirs | ForEach-Object { "-I`"$_`"" }) -join " "
$BaseFlags = "-std=c++17 -O0 -g -fPIC -fdeclspec -fms-extensions -fshort-wchar -D_CRT_SECURE_NO_WARNINGS -Dswprintf=GTA2_swprintf"

# --- 1. Compilar wincompat + registry del port (si hace falta) ---
$plat = @(
    (Join-Path $ProjectDir "platform\wincompat.cpp"),
    (Join-Path $ProjectDir "platform\wincompat_registry.cpp")
)
$needRebuild = $false
foreach ($src in $plat) {
    $o = Join-Path $ObjDir ([IO.Path]::GetFileNameWithoutExtension($src) + "_port.o")
    if (-not (Test-Path $o) -or (Get-Item $src).LastWriteTime -gt (Get-Item $o).LastWriteTime) {
        $needRebuild = $true
        Invoke-Expression "& `"$ClangCC`" $BaseFlags $IncStr -c `"$src`" -o `"$o`" 2>&1"
        if ($LASTEXITCODE -ne 0) { Write-Host "FALLO compilando $([IO.Path]::GetFileName($src))"; exit 1 }
        Write-Host "compilado $([IO.Path]::GetFileName($src))"
    }
}
if (-not $needRebuild) { Write-Host "wincompat/registry: .o frescos (sin recompilar)" }

# --- 2. Link del motor ---
$objs = Get-ChildItem -File -Filter *.o $ObjDir | Select-Object -ExpandProperty FullName
$outSo = Join-Path $ObjDir "libGTA2.so"
Write-Host "Enlazando $($objs.Count) objetos -> libGTA2.so ..."

$linkArgs = @(
    "-shared",
    "-Wl,-soname,libGTA2.so",
    "-Wl,--no-dependent-libraries",
    "-Wl,--unresolved-symbols=ignore-in-object-files",
    "-Wl,--warn-unresolved-symbols"
)
$linkArgs += $objs
$linkArgs += @("-o", $outSo, "-llog", "-ldl", "-lm")

$errOut = & $ClangCC $linkArgs 2>&1
$errLines = $errOut | ForEach-Object { $_.ToString() }
$errLines | Set-Content -Encoding utf8 -LiteralPath (Join-Path $ObjDir "link.log")

if ($LASTEXITCODE -eq 0) {
    Write-Host "LINK OK: $outSo"
} else {
    Write-Host "LINK con errores (parcial). Ver link.log"
    $errLines | Select-Object -First 20 | ForEach-Object { $_.Trim() }
}

# --- 3. Inventario de undefined (via nm del .so, canonico) ---
$nm = Join-Path $Ndk "toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-nm.exe"
$nmOut = & $nm -D -u $outSo
$symbols = @()
foreach ($line in $nmOut) {
    $t = $line.ToString().Trim()
    if ($t -match "^U\s+(.+)$") { $symbols += $Matches[1].Trim() }
}
$unique = $symbols | Sort-Object -Unique
Write-Host ""
Write-Host "=== UNDEFINED EN .so ($($unique.Count)) ==="
$unique | Set-Content -Encoding utf8 -LiteralPath (Join-Path $ObjDir "undef_symbols.txt")

# Clasificacion rough por prefijo
function Classify($s) {
    if ($s -match "^(gbh_|pVid_|D3D|CreateSurface|DirectDraw|CreateWindow|wgl|SDLGTA|G2_)") { return "Graphics" }
    if ($s -match "^(AIL_|mss|VMSS|DigDrv|MIX_)") { return "Audio" }
    if ($s -match "^Bink") { return "Bink" }
    if ($s -match "^(Reg|Win32_|GetTickCount|timeGetTime|Sleep|LocalAlloc|CreateMutex|GetModule|FindFirst|OpenProcess|OutputDebugString|_stricmp|_strnicmp|_strdup|_vsnprintf|_snprintf|_splitpath|_makepath|_ltoa|_itoa|_sopen|_dos|_pipe|_strlwr|_strupr)")
    {
        return "CRT/win32"
    }
    if ($s -match "^(DirectPlay|Lobby|DP|DPLOBBY|IDirectPlay)") { return "DirectPlay" }
    if ($s -match "SDL|EGL|JNI|ANativeWindow") { return "ShellAndroid" }
    return "Motor/otro"
}

$groups = @{}
foreach ($s in $unique) {
    $c = Classify $s
    if (-not $groups.ContainsKey($c)) { $groups[$c] = @() }
    $groups[$c] += $s
}
foreach ($k in ($groups.Keys | Sort-Object)) {
    Write-Host ""
    Write-Host "----- $k ($($groups[$k].Count)) -----"
    $groups[$k] | ForEach-Object { Write-Host "  $_" }
}
Write-Host ""
Write-Host "FIN. Log: $logFile"
exit 0