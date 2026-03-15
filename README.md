![CI](https://github.com/NicholasTerek/OarfishDB/actions/workflows/cmake-single-platform.yml/badge.svg)
![License](https://img.shields.io/github/license/NicholasTerek/OarfishDB)
![C++](https://img.shields.io/badge/language-C++-blue)
![Last Commit](https://img.shields.io/github/last-commit/NicholasTerek/OarfishDB)
![Issues](https://img.shields.io/github/issues/NicholasTerek/OarfishDB)


<img width="814" height="200" alt="image" src="https://github.com/user-attachments/assets/1960f46f-940a-4b4c-a70f-72114ba8b34b" />
<h4>OarfishDB is an experimental educational database management system written in C++</h4>

<p>&nbsp;</p>

> [!WARNING]
> OarfishDB is built for educational purposes only. Do **not** use in production environments.

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
