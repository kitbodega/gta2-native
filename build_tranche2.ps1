# build_tranche2.ps1 - Compila los modulos del motor excluidos del tranche 1.
$ErrorActionPreference = "Continue"

$RepoRoot   = "C:\Users\Admin\Downloads\gta2-build\gta2_re"
$ProjectDir = "C:\Users\Admin\Downloads\gta2-build\gta2-native\app\src\main\cpp"
$Ndk        = "C:\Android\sdk\ndk\26.1.10909125"
$Clang      = Join-Path $Ndk "toolchains\llvm\prebuilt\windows-x86_64\bin\armv7a-linux-androideabi24-clang++.cmd"
$OutDir     = "C:\Users\Admin\AppData\Local\Temp\opencode\gta2_t1"
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null

$SrcDir = Join-Path $RepoRoot "Source"
$IncDirs = @(
    (Join-Path $ProjectDir "platform"),
    $SrcDir,
    (Join-Path $SrcDir "3rdParty"),
    $RepoRoot
)
$IncStr = ($IncDirs | ForEach-Object { "-I`"$_`"" }) -join " "

$Include = @(
 'Function','debug','jolly_poitras_0x2BC0','text_0x14','gtx_0x106C','sharp_pare_0x15D8',
 'magical_germain_0x8EC','cSampleManager','root_sound','sound_obj','input','keybrd_0x204',
 'sprite','Draw','MapRenderer','Hud','Frontend','WinMainProxy'
)

$Files = Get-ChildItem -File -Filter *.cpp $SrcDir | Where-Object { $Include -contains $_.BaseName }
# Stubs del port que acompañan a estos modulos (audio mss, etc.)
$StubFiles = @(
    (Join-Path $ProjectDir "platform\mss_stubs.cpp"),
    (Join-Path $ProjectDir "platform\engine_backend.cpp")
)
Write-Host "Compilando $($Files.Count) fuentes (tranche 2) + $($StubFiles.Count) stubs..."

$fail = @()
$ok = 0
foreach ($f in $Files) {
    $o = Join-Path $OutDir ($f.BaseName + ".o")
    $cmd = "& `"$Clang`" -std=c++17 -O0 -g -fPIC -fdeclspec -fms-extensions -fshort-wchar -D_CRT_SECURE_NO_WARNINGS -Dswprintf=GTA2_swprintf $IncStr -c `"$($f.FullName)`" -o `"$o`" 2>&1"
    $outLines = Invoke-Expression $cmd
    if ($LASTEXITCODE -ne 0) {
        $fail += $f.BaseName
        $logFile = Join-Path $OutDir ($f.BaseName + ".err")
        $txt = $outLines | ForEach-Object { $_.ToString() }
        $txt | Set-Content -Encoding utf8 -LiteralPath $logFile
        $errs = $txt | Where-Object { $_ -match "error:" } | Select-Object -First 8
        Write-Host "==== FAIL $($f.BaseName) ===="
        if ($errs) { $errs | ForEach-Object { $_.Trim() } }
        else { ($txt | Select-Object -Last 4) | ForEach-Object { $_.Trim() } }
    } else {
        $ok++
    }
}
foreach ($s in $StubFiles) {
    $name = [IO.Path]::GetFileName($s) -replace "\.cpp$", ""
    $o = Join-Path $OutDir ($name + "_port.o")
    $cmd = "& `"$Clang`" -std=c++17 -O0 -g -fPIC -fdeclspec -fms-extensions -fshort-wchar -D_CRT_SECURE_NO_WARNINGS -Dswprintf=GTA2_swprintf $IncStr -c `"$s`" -o `"$o`" 2>&1"
    $outLines = Invoke-Expression $cmd
    if ($LASTEXITCODE -ne 0) {
        $fail += $name
        $txt = $outLines | ForEach-Object { $_.ToString() }
        $errs = $txt | Where-Object { $_ -match "error:" } | Select-Object -First 5
        Write-Host "==== FAIL $name ===="
        if ($errs) { $errs | ForEach-Object { $_.Trim() } }
    } else {
        $ok++
    }
}

Write-Host ""
Write-Host "OK=$ok  FAIL=$($fail.Count) / $($Files.Count)"
if ($fail.Count) { Write-Host "Fails: $($fail -join ', ')" }
exit 0