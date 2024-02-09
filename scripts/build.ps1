Clear-Host

$path_root = git rev-parse --show-toplevel

if ($IsWindows) {
    $devshell  = Join-Path $path_root 'scripts/helpers/devshell.ps1'
    # This HandmadeHero implementation is only designed for 64-bit systems
    & $devshell -arch amd64
}

Push-Location $path_root

$build_bat = Join-Path $path_root 'build.bat'
& $build_bat @args

Pop-Location
