cd build
cmake .. -G "Visual Studio 17 2022" -A X64 -DBBL_RELEASE_TO_PUBLIC=1 -DBBL_INTERNAL_TESTING=0 -DCMAKE_PREFIX_PATH="C:\Subject\CAD\BambuStudio\deps\build\BambuStudio_dep\usr\local" -DCMAKE_INSTALL_PREFIX="../install-dir" -DCMAKE_BUILD_TYPE=Release
cmake --build . --target install --config Release -- -m