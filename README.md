# HL-Rally

This repository contains the source code for the HL-Rally modification of
Half-Life.

## Building with Make

The original Makefile in `dlls/` has been updated to use a maintained
compiler. Ensure `g++` or `clang++` is available and run:

```bash
cd dlls
make
```

The resulting server library `hlr_i386.so` will be created in the `dlls`
directory.

## Building with CMake

A CMake build system is provided for cross-platform compilation and
optional WebAssembly output via Emscripten.

### Native build

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### WebAssembly build

Install the Emscripten SDK and use `emcmake`:

```bash
mkdir build
cd build
emcmake cmake ..
cmake --build .
```

This produces a `hlr.wasm` module inside `dlls/`.
