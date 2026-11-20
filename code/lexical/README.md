# 词法分析器

## 目录结构

```
lexical/
├── src/
│   ├── main.cpp      读取源文件，写出 .dyd 和 .err
│   ├── lexer.h       Token、LexError 等数据结构，scan() 声明
│   └── lexer.cpp     扫描逻辑
├── tests/            测试程序（sample1、sample2 是老师给的，lexical_cases 是自己加的）
└── CMakeLists.txt
```

## 运行

```sh
cd lexical
cmake -S . -B build
cmake --build build
./build/mini_lexer tests/sample1.mini
```

结果会写到当前目录下的 `output/`，即 `output/sample1.dyd` 和 `output/sample1.err`。没有词法错误时 `.err` 为空，有错误时程序返回非 0。
