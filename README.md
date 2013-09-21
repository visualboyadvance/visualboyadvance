VBA - a GameBoy Advance emulator
====================================

VBA Development
---------------
VBA is a GBA emulator written in C. VBA is an open source project, licensed under the GPL 2.0 (or later). Anyone is welcome to contribute their improvements to the code.

### Simplicity
VBA has a strong emphasis on keeping the code as simple as possible, but no simpler. That is, no abstractions, not lots of whitespace fluff but reasonably tight and flexible (where appropriate only!).

### Dependencies
VBA has few dependencies: basically GLib and libarchive. This is intended and should remain this way. Frontends can be added and come up with more dependencies to make the user experience richer.

### Contributing
Pick from the list of tasks below, go look at the open issues on GitHub, or just implement whatever missing feature you feel like. If you send a couple of good pull requests, you'll be added as a contributor and get your own commit access.

### Task list
- Clean up core code, converting it to clean C modules.
- Port fixes from VBA-M and libretro.
- Make use of the game database to provide rich features.
- Port to SDL2
- Add more features to the SDL frontend
- Add more frontends
- Port to more platforms

Basic build instructions
------------------------

VBA currently uses CMake for its build system. In order
to build for most systems, create a `build` directory and
run:

    cmake path/to/vba
    make
