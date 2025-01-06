Clear-Host
$path_root = git rev-parse --show-toplevel
$build_dir = Join-Path $path_root 'build'

if (Test-Path $build_dir) {
    Get-ChildItem -Path $build_dir -Recurse | Remove-Item -Force -Recurse
    Write-Host "Build directory cleaned."
} else {
    Write-Host "Build directory does not exist."
}
