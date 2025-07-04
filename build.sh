#!/bin/bash

echo "Building Air Traffic Control System..."
gcc -Wall -std=c99 -o traffic main.c -lraylib -lm -ldl -lpthread -lGL -lrt -lX11

if [ $? -eq 0 ]; then
    echo "Build successful! Run with ./traffic"
else
    echo "Build failed."
fi
