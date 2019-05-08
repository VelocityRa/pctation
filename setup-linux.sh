git submodule update --init
pushd "external/vcpkg"
VCPKG_DEFAULT_TRIPLET=x64-linux
#./bootstrap-vcpkg.sh
./vcpkg install fmt spdlog gsl-lite sdl2 imgui glbinding glm
popd
