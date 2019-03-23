git submodule update --init
pushd "external/vcpkg"
set "VCPKG_DEFAULT_TRIPLET=x64-windows"
if not exist "vcpkg.exe" (
    bootstrap-vcpkg.bat
)
vcpkg install fmt spdlog
popd
