# deltatunix

A portable clone of Toastworth's [DeltaTune](https://github.com/ToadsworthLP/deltatune) for Linux, an overlay that shows the title and artist of whatever song you're playing.
This uses the MPRIS DBus interface to check for running media players.

![Screenshot](screenshot.png)

Made with raylib and primarily made because I'm bored as hell.

# Compiling

You will need:

- CMake (atleast 3.5+)
- raylib [6.x]
- sdbus-c++
- A compiler that supports atleast C++20 (which is most nowadays...)

This uses the CMake build system for building.

```bash
cmake -B build
cmake --build build
./run.sh # This will run the program
```
