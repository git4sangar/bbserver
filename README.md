# BBServer
## Dependencies
Purely dependent on cmake, POSIX and C++17

## Installation
Install the dependencies and devDependencies and start the server.
```sh
clone the repository
cd bbserver
cmake .
make
```

## Bulding TCP Client
Build the client as follows
```sh
g++ TCPClient.cpp -o TCPClient -lpthread -std=c++17
./TCPClient ip-address port
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
