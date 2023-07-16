## broccolini
A lightweight web browser for video game consoles and other low-dependency devices. Written in SDL2 using [Chesto](https://github.com/fortheusers/chesto), it supports viewing simple websites.

Broccolini uses [litehtml](https://github.com/litehtml/litehtml) as its rendering engine. litehtml is not intended to be used in a "full-fledged" browser, but it is usable!

### What Works
- renders HTML and CSS!
- inertia scrolling and touch-based navigation
- async image downloading
- history and bookmarks
- tabs and tab image previews
- private browsing mode
- restore previous tabs on re-launch
- full qwerty on-screen keyboard
- support for a few different font families
- base64 data uris and SVGs work

### To Do:
- pinch to zoom! and pan left/right
- add cursor to be controlled with the joystick
- fix images drawing on top of everything
- detailed history and managing bookmarks
- store and use cookies, local storage, etc
- handle POSTs and other non-GET requests
    - handle different error codes
- video and audio embed support
- simple javascript within pages
- improve HTML/CSS compatibility
    - no flexbox support
    - form input elements
    - iframes and frames
- probably many more things

### Download
There are no stable releases available yet, however there are in-development builds for each platform under [GH Actions](https://github.com/vgmoose/broccolini/actions).

Nightly links: [Console builds](https://nightly.link/fortheusers/hb-appstore/workflows/main/main) - [PC builds](https://nightly.link/fortheusers/hb-appstore/workflows/pc-builds/main)

### Building for PC
Requires SDL2 development libraries for your operating system, and a C++ toolchain.

```
git clone --recursive git@github.com:vgmoose/broccolini.git
cd broccolini
make pc
```

After building, `broccolini.bin` will be present in the current directory.

### Building for Consoles (using Docker)
Support for the Wii U and Switch consoles is provided using [homebrew](https://en.wikipedia.org/wiki/Homebrew_(video_games)) libraries, thanks to community maintained SDL2 ports and toolchains. The [Sealeo](https://github.com/fortheusers/sealeo) docker image contains pinned versions of these dependencies.

```
git clone --recursive git@github.com:vgmoose/broccolini.git
cd broccolini
export PLATFORM=wiiu    # or switch
docker run -v $(pwd):/code -it ghcr.io/fortheusers/sealeo "make $PLATFORM"
```

If successful, `broccolini.wuhb` or `broccolini.nro` will be built.

## License
This software is licensed under the GPLv3.