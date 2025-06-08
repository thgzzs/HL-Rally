# HL-Rally

HL-Rally is a racing modification for the original **Half-Life** engine. This repository contains the C++ source for the mod, including server and client components.

## Building on Linux

The server game library can be built using `make`:

```bash
cd dlls
make
```

The resulting `hlr_i386.so` should be placed in your Half-Life `hlrally` mod directory.

## Building on Windows

Windows project files are provided for Visual Studio. Open `cl_dll/cl_dll.sln` to compile the client library and `dlls/hl.vcproj` to build the server game DLL.

## Optional WebAssembly build

An experimental WebAssembly build can be attempted with [Emscripten](https://emscripten.org/). After installing Emscripten and configuring the environment, run `emmake make` in the `dlls` directory or adapt the Visual Studio project for Emscripten to produce a `.wasm` output.

## License

This project is released under the MIT License. See the [LICENSE](LICENSE) file for details.
