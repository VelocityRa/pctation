@echo off
pushd "src"
for /f %%f in ('dir *.cpp *.hpp /b/s') do clang-format -i %%f
popd
