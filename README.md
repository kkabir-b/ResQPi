# ResQPi
Build instructions: 
sudo apt install libopencv-dev 
vcpkg install
build: 
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build