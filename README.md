# BBServer
## Dependencies
Purely dependent on cmake, POSIX and C++17

## Installation
Install the dependencies and devDependencies and start the server.
```sh
clone the repository
cd bbserver
mkdir build
cd build
cmake ..
make
```

## How to run
```sh
cp BBServer ./test/test_01
cp BBServer ./test/test_02
cd ./test/test_01
./BBServer
cd ../test_02
./BBServer
cd test
g++ client.cpp -o client -std=c++17
./client
```
