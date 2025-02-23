# scx-manager
Simple GUI for managing sched-ext schedulers via scx_loader.

Requirements
------------
* C++23 feature required (tested with GCC 14.1.1 and Clang 18)
Any compiler which support C++23 standard should work.

######
## Installing from source

This is tested on Arch Linux, but *any* recent Arch Linux based system with latest C++23 compiler should do:

```sh
sudo pacman -S \
    base-devel cmake make qt6-base qt6-tools polkit-qt6 python
```

### Cloning the source code
```sh
git clone https://github.com/cachyos/scx-manager.git
cd scx-manager
```

### Building and Configuring
To build, first, configure it(if you intend to install it globally, you
might also want `--prefix=/usr`):
```sh
./configure.sh --prefix=/usr/local
```
Second, build it:
```sh
./build.sh
```


### Libraries used in this project

* [Qt](https://www.qt.io) used for GUI.
* [A modern formatting library](https://github.com/fmtlib/fmt) used for formatting strings, output and logging.
