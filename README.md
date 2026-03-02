# LSM Tree in C++

<img width="814" height="200" alt="image" src="https://github.com/user-attachments/assets/1960f46f-940a-4b4c-a70f-72114ba8b34b" />

Simple Log-Structured Merge tree with custom SkipList implementation in C++17.

## Build & Test

```bash
cmake -S . -B build
cmake --build build -j"$(nproc)"
```

```bash
ctest --test-dir build --output-on-failure
```

## Features

- Custom **SkipList** data structure
- **Multi-memtable** LSM storage with automatic freezing
- **Thread-safe** operations with proper locking
- **Comprehensive tests** (24 tests across 3 suites)
