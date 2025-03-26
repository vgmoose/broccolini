## broccolini
A lightweight web browser for video game consoles and other low-dependency devices. Written in SDL2 using [Chesto](https://github.com/fortheusers/chesto), it supports viewing simple websites.

Broccolini uses [litehtml](https://github.com/litehtml/litehtml) as its rendering engine. litehtml is not intended to be used in a "full-fledged" browser, but it is usable!

|       URL Webview        |      Tab Switcher        |
:-------------------------:|:-------------------------:
![Wikipedia demo](https://github.com/user-attachments/assets/060d91ab-3f61-4bd0-9279-10c28ba5f0a5) | ![Tab overview](https://github.com/user-attachments/assets/101fd92f-f5bc-43dc-9a55-069e9e68ec53)


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

Nightly links: [Console builds](https://nightly.link/vgmoose/broccolini/workflows/main/main) - [PC builds](https://nightly.link/vgmoose/broccolini/workflows/pc-builds/main)

### Building for PC
**As of this time, building must be done on a case-sensitive filesystem.** Requires SDL2 development libraries for your operating system, and a C++ toolchain.

```
git clone --recursive git@github.com:vgmoose/broccolini.git
cd broccolini
make pc
```

After building, `broccolini.bin` will be present in the current directory.

### Building for Consoles (using Docker)
Support for the Wii U and Switch consoles is provided using [homebrew](https://en.wikipedia.org/wiki/Homebrew_(video_games)) libraries, thanks to community maintained SDL2 ports and toolchains. The [Sealeo](https://github.com/fortheusers/sealeo) docker image contains pinned versions of these dependencies.

```
git clone --recursive https://github.com/vgmoose/broccolini.git
cd broccolini
export PLATFORM=wiiu    # or switch
docker run -v $(pwd):/code -it ghcr.io/fortheusers/sealeo "make $PLATFORM"
```

If successful, `broccolini.wuhb` or `broccolini.nro` will be built.

## License
This software is licensed under the GPLv3.
