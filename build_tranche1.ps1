param([string]$Tranche = "all")

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

# Modulos excluidos del tranche 1 (graficos/audio/input/network/build)
$Exclude = @(
 'Bink','dma_video','Draw','Frontend','gbh_graphics','gtx_0x106C','Hud','MapRenderer',
 'magical_germain_0x8EC','sharp_pare_0x15D8','sprite','text_0x14',
 'cSampleManager','root_sound','sound_obj','jolly_poitras_0x2BC0',
 'input','keybrd_0x204',
 'NetPlay','Network_20324',
 'ImGuiDebug','HookLoader','WinMainProxy','ExePatcher','cl_proxy','debug','Function',
 'winmain'
)

$Files = Get-ChildItem -File -Filter *.cpp $SrcDir | Where-Object { $Exclude -notcontains $_.BaseName }
Write-Host "Compilando $($Files.Count) fuentes del motor (tranche 1)..."

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
        $errs = $txt | Where-Object { $_ -match "error:" } | Select-Object -First 6
        Write-Host "==== FAIL $($f.BaseName) ===="
        if ($errs) {
            $errs | ForEach-Object { $_.Trim() }
        } else {
            ($txt | Select-Object -Last 4) | ForEach-Object { $_.Trim() }
        }
    } else {
        $ok++
    }
}

Write-Host ""
Write-Host "OK=$ok  FAIL=$($fail.Count) / $($Files.Count)"
if ($fail.Count) { Write-Host "Fails: $($fail -join ', ')" }
exit 0