#!/bin/bash
cd build && cmake .. && make && clear
cd .. && ./bin/gazc test.prog output &&
llc output -o output.s && clang -lm output.s -o out -L ./bin/ -lgazrt -Wl,-rpath,./bin/
