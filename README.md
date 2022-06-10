# APSU
A c++ library about asymmetric PSU is based on the [APSI](https://github.com/microsoft/APSI) and [libOTe](https://github.com/osu-crypto/libOTe).(It works only in "x64 Release" mod now.)
## Build
```
git clone https://github.com/lqvir/APSU.git
mkdir build
cd build
cmake ..
cmake --build . --config release 
``` # uPSU
## Introduction
uPSU is an unbalanced PSU protocol which is described in [eprint.iacr.org/2022/653](https://eprint.iacr.org/2022/653). The larger difference betweend the sizeof of two sets, the better our protocol performs.   


## How to build

Our `uPSU` is amended form [APSI](https://github.com/microsoft/APSI), so you may use for reference the method of build `APSI` to compile `uPSU`. 


### vcpkg
[vcpkg](https://github.com/microsoft/vcpkg) can help you manange C and C++ libraries on Windows, Linux and MacOS. We recommend to build and install dependencies for `uPSU ` with vcpkg.  
Some Dependencies which are needed by building uPSU are as follows. 

| Dependency                                                | vcpkg name                                           |
|-----------------------------------------------------------|------------------------------------------------------|
| [Microsoft SEAL](https://github.com/microsoft/SEAL)       | `seal[no-throw-tran]`                                |
| [Microsoft Kuku](https://github.com/microsoft/Kuku)       | `kuku`                                               |
| [Log4cplus](https://github.com/log4cplus/log4cplus)       | `log4cplus`                                          |
| [cppzmq](https://github.com/zeromq/cppzmq)                | `cppzmq` (needed only for ZeroMQ networking support) |
| [FlatBuffers](https://github.com/google/flatbuffers)      | `flatbuffers`                                        |
| [jsoncpp](https://github.com/open-source-parsers/jsoncpp) | `jsoncpp`                                            |
| [TCLAP](https://sourceforge.net/projects/tclap/)          | `tclap` (needed only for building CLI)               |



First follow this [Quick Start on Unix](https://github.com/microsoft/vcpkg#quick-start-unix), and then run:
```powershell
./vcpkg install [package name]:x64-linux
```


To build your CMake project with dependency on uPSU, follow [this guide](https://github.com/microsoft/vcpkg#using-vcpkg-with-cmake).

### libOTe
[libOTe](https://github.com/osu-crypto/libOTe) is a  fast and portable C++17 library for Oblivious Transfer extension (OTe). We can build it by following commands.
```
git clone --recursive https://github.com/osu-crypto/libOTe.git
cd libOTe
python build.py --setup --boost --relic --install=/your/path
python build.py -D ENABLE_RELIC=ON -D ENABLE_ALL_OT=ON
```
We recommend that you install `libOTe` in the folder `thirdparty\`. If not, please make sure the following arguments are passed to CMake configuration:
- `-DLIBOTE_PATH=/your/path/libOTe`

### Kunlun
[Kunlun](https://github.com/yuchen1024/Kunlun) is an efficient and modular crypto library. We use the implement of `MP-OPRF` and `PEQT` in Kunlun, and adjust some paramaters used in `MP-OPRF`. So we recommend that you employ Kunlun in the folder `thirdparty/` directly.
**TIPS** Temporarily, we have to adjust the number of threads used in Kunlun manually. 
### uPSU
When you have all dependencies ready, you can build uPSU by the following commands. 
```
git clone https://github.com/lqvir/uPSU.git
cd uPSU
mkdir build
cd build
make 
```
## Command-Line Interface (CLI)
Same as `APSI`, out uPSU comes with example command-line programs implementing a sender and a receiver.
In this section we describe how to run these programs.

### Common Arguments

The following optional arguments are common both to the sender and the receiver applications.

| Parameter | Explanation |
|-----------|-------------|
| `-t` \| `--threads` | Number of threads to use |
| `-f` \| `--logFile` | Log file path |
| `-s` \| `--silent` | Do not write output to console |
| `-l` \| `--logLevel` | One of `all`, `debug`, `info` (default), `warning`, `error`, `off` |

### Sender

The following arguments specify the receiver's behavior.

| Parameter | Explanation |
|-----------|-------------|
| `-q` \| `--queryFile` | Path to a text file containing query data (one per line) |
| `-a` \| `--ipAddr` | IP address for a sender endpoint |
| `--port` | TCP port to connect to (default is 1212) |

### Receiver

The following arguments specify the sender's behavior and determine the parameters for the protocol.
In our CLI implementation the sender always chooses the parameters and the receiver obtains them through a parameter request.
Note that in other applications the receiver may already know the parameters, and the parameter request may not be necessary.

| <div style="width:190px">Parameter</div> | Explanation |
|-----------|-------------|
| `-d` \| `--dbFile` | Path to a CSV file describing the sender's dataset (an item-label pair on each row) or a file containing a serialized `SenderDB`; the CLI will first attempt to load the data as a serialized `SenderDB`, and &ndash; upon failure &ndash; will proceed to attempt to read it as a CSV file |
| `-p` \| `--paramsFile` | Path to a JSON file [describing the parameters](#loading-from-json) to be used by the sender
| `--port` | TCP port to bind to (default is 1212) |


### AutoTest
We have prapared an automated testing tool in the folder `\tools`. You could test whether uPSU protocol works successlfully.
