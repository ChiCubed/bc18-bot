#!/bin/sh

# thanks to Orbitary Graph for giving us this compilation / upload workflow

# build the program!

if [ "$BC_PLATFORM" = 'LINUX' ]; then
    LIBRARIES="-lbattlecode-linux -lutil -ldl -lrt -pthread -lgcc_s -lc -lm -L../battlecode/c/lib"
    INCLUDES="-I../battlecode/c/include -I."
elif [ "$BC_PLATFORM" = 'DARWIN' ]; then
    LIBRARIES="-lbattlecode-darwin -lSystem -lresolv -lc -lm -L../battlecode/c/lib"
    INCLUDES="-I../battlecode/c/include -I."
else
    echo "Unknown platform '$BC_PLATFORM' or platform not set"
    echo "Make sure the BC_PLATFORM environment variable is set"
    exit 1
fi
g++ -std=c++14 -O2 main.cpp -o main $LIBRARIES $INCLUDES 

# run the program!
./main