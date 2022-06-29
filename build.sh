g++ ./src/*.cpp -o ./build/metavm -fno-exceptions -fno-rtti -I./inc -I./inc/achilles -lbfd -ldl -Wall -D BACKWARD_HAS_BFD=1 -g3
# g++ ./src/*.cpp -o ./build/metavm -fno-exceptions -fno-rtti -I./inc -I./inc/achilles -Wall -O3

