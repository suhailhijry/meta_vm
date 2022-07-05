# g++ ./src/*.cpp -o ./build/metavm -fno-exceptions -fno-rtti -I./inc -I./inc/achilles -lbfd -ldl -W -Wall -D BACKWARD_HAS_BFD=1 -g3
g++ ./src/main.cpp -o ./build/metavm -fno-exceptions -fno-rtti -I./inc -I./inc/achilles -W -Wall -O3 -g3

