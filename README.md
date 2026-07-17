# ResQPi
## About

This project aims to create a detection system for distress signals and send a corresponding notification to authorities/people in charge.

ResQPi utilizes a quantized YOLOv10 model to detect the "call" gesture from the HaGRID dataset - interpreting it as the distress signal.

This project has two executables, sender and receiver. Sender is responsible for camera management and forwarding the frames through UDP while receiver reassembles the frames at the backend and does the inference/potential authority notification.
## Build instructions
### 1. System dependencies (Ubuntu/Debian)
 
```bash
sudo apt update
sudo apt install -y \
  build-essential pkg-config cmake git curl zip unzip tar \
  m4 bison flex gperf \
  autoconf autoconf-archive automake libtool \
  meson ninja-build \
  libx11-dev libxft-dev libxext-dev libxrandr-dev libxfixes-dev \
  libxi-dev libxtst-dev libxrender-dev libxcursor-dev libxinerama-dev \
  libgl1-mesa-dev libglu1-mesa-dev \
  libglib2.0-dev libdbus-1-dev libsystemd-dev \
  libcairo2-dev libpango1.0-dev \
  gettext
```
 
### 2. vcpkg
 
```bash
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
echo 'export VCPKG_ROOT=~/vcpkg' >> ~/.bashrc
source ~/.bashrc
```
 
### 3. ONNX Runtime (prebuilt binary, not vcpkg - had ONNX port issues w onnxruntime + opencv)
 
```bash
cd ~
curl -L -o onnxruntime.tgz https://github.com/microsoft/onnxruntime/releases/download/v1.23.2/onnxruntime-linux-x64-1.23.2.tgz
tar -xzf onnxruntime.tgz
```
 
Extracts to `~/onnxruntime-linux-x64-1.23.2/`. If you put it elsewhere, pass `-DONNXRUNTIME_ROOT=/your/path` when running cmake below.
 
### 4. Webcam permissions
 
```bash
sudo usermod -aG video $USER
```
 
Log out and back in.
 
### 5. Model files
 
```
ResQPi/
├── models/
│   ├── YOLOv10n_gestures.onnx
│   └── yolov10n_gestures_int8.onnx
├── src/sender.cpp
├── src/receiver.cpp
├── CMakeLists.txt
└── vcpkg.json
```
 
Edit `model_path` in `main.cpp` to point at whichever one you want to run.
 
### Build
 
```bash
rm -rf build vcpkg_installed
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build
```
 
### Run
 
```bash
./build/receiver
./build/sender
```


## Notes 
If you are running the quantized version change 
```cpp
padded.convertTo(floatImg, CV_32FC3,1.0f/255.0f);
``` 
to 
```cpp
padded.convertTo(floatImg, CV_32FC3);
``` 
in `src/inferrer/Inferrer.cpp`

## Citations
[HaGRID Dataset](https://github.com/hukenovs/hagrid)

## TODO
 - Add the notification system to notify clients 
   - Current considering email notification 
   - Have a dashboard of multiple rooms and ping if distress signal detect, used for example in an old age home