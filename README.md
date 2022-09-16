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
cp BBServer ./test/test_03
cd ./test/test_01
./BBServer -b bbfile1 -f -T 5 -p 3200 -s 3300 localhost:3301 localhost:3302
cd ../test_02 
./BBServer -b bbfile1 -f -T 5 -p 3201 -s 3301 localhost:3300 localhost:3302
cd ../test_03
./BBServer -b bbfile1 -f -T 5 -p 3202 -s 3302 localhost:3300 localhost:3301

cd ..
g++ TCPClient.cpp -o TCPClient -lpthread -std=c++17
./TCPClient ip-address 3200
```
