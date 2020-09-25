@echo off
rmdir /s /q .vs_proj

mkdir .vs_proj

pushd .vs_proj

cmake .. -DCMAKE_BUILD_TYPE=Debug

popd

@echo Project make done!
goto :eof
