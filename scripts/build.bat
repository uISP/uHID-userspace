rmdir /S /Q build
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DUHID_HIDAPI_DIR=C:\libs\hidapi -DCMAKE_BUILD_TYPE=StaticRelease
mingw32-make
mingw32-make package
