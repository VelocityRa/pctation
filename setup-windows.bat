git submodule update --init

REM Setting up vcpkg...
pushd "external/vcpkg"
set "VCPKG_DEFAULT_TRIPLET=x64-windows"
if not exist "vcpkg.exe" (
    bootstrap-vcpkg.bat
)

REM Installing dependencies...
vcpkg install fmt spdlog gsl-lite sdl2 imgui glbinding

REM Done.
popd
