git submodule update --init

REM Setting up vcpkg...
pushd "external/vcpkg"
set "VCPKG_DEFAULT_TRIPLET=x64-windows"
if not exist "vcpkg.exe" (
    bootstrap-vcpkg.bat
)
vcpkg install fmt spdlog gsl-lite
popd

REM Setting up bigg...
pushd "external/bigg"
git submodule update --init --recursive
popd
