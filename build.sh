# 1. 创建构建目录
mkdir -p build && cd build

# 2. 生成 Makefile
cmake ..

# 3. 编译
make -j

# 4. 运行
./sim
