## Sending and receiving logic

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
ResQPi/cpp/
        ├── models/
        │   ├── YOLOv10n_gestures.onnx
        │   └── yolov10n_gestures_int8.onnx
        ├── src/sender.cpp
        ├── src/receiver.cpp
        ├── CMakeLists.txt
        └── vcpkg.json
```
 
Edit `model_path` in `receiver.cpp` to point at whichever one you want to run.
 
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



# Redpanda 

## Run Instructions

```bash
docker run -d --name redpanda \
  -p 9092:9092 \
  -p 9644:9644 \
  docker.redpanda.com/redpandadata/redpanda:latest \
  redpanda start --mode dev-container
  ```

## Adding topic

```bash
docker exec -it redpanda rpk topic create detection-events
```