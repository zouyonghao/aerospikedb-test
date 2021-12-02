sudo apt install liblua5.1-dev

make clean

make USE_LUAJIT=0 \
    CC="clang-9 -fsanitize=address"

# make

# make USE_LUAJIT=0 CC=/home/zyh/distributed-system-test/build/fuzz/default_compiler