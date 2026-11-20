# 语法分析器

## 目录结构

```
syntax/
├── src/
│   ├── main.cpp      读取 .dyd，写出 .var、.pro、.err
│   ├── parser.h      Variable、Procedure 等数据结构，Parser 类声明
│   └── parser.cpp    递归下降分析和符号表
├── tests/
│   ├── nested.mini   嵌套函数的测试程序
│   ├── errors.mini   多处语法错误的测试程序
│   └── test_pipeline.py   端到端测试
└── CMakeLists.txt
```

## 运行

先用词法分析器生成 `.dyd` 文件（见 `../lexical/README.md`），然后在 `syntax/` 目录下运行：

```sh
cd syntax
cmake -S . -B build
cmake --build build
./build/mini_parser ../lexical/output/sample1.dyd
```

结果写到 `output/` 下，即 `sample1.var`、`sample1.pro` 和 `sample1.err`。没有语法错误时 `.err` 为空，有错误时程序返回非 0。

一定要在 `syntax/` 目录下运行。如果在别的目录运行，输出可能会覆盖输入的 `.dyd` 或词法分析的 `.err`，程序会拒绝写入。

## 测试

测试需要先把两个程序都编译好：

```sh
cmake -S ../lexical -B ../lexical/build
cmake --build ../lexical/build
cmake -S . -B build
cmake --build build
python3 tests/test_pipeline.py
```
