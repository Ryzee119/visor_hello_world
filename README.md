Minimilast OG Xbox Visor Exploit to Hello World - Just me tinkering learning about it.

```
mkdir build && cd build
cmake ..
cmake --build .
```

Creates a 256kB binary that you use as an Original Xbox BIOS image.

References:
* https://github.com/XboxDev/cromwell
* https://github.com/mborgerson/xqemu-kernel
* https://github.com/Ernegien/xdecode

sudo apt install git build-essential clang nasm cmake python3 python3-pip

pip install objutils lz4 pyelftools