**Disclaimer:**
This repository is a mirror of the original project hosted at (https://hg.sr.ht/~scoopta/wlrobs).
I am not the original author and claim no ownership of the code.

# wlrobs
wlrobs is an obs-studio plugin that allows you to screen capture on wlroots based wayland compositors

[![builds.sr.ht status](https://builds.sr.ht/~scoopta/wlrobs.svg)](https://builds.sr.ht/~scoopta/wlrobs?)

## Dependencies
	libwayland-dev
	libobs-dev
	pkg-config
	meson
## Building
	git clone https://github.com/OmarKrypton/wlrobs.git
	cd wlrobs
	meson setup build
	ninja -C build
The screencopy backend can be disabled with the `-Duse_scpy=false` meson option, likewise dmabuf can be disabled with `-Duse_dmabuf=false`

## Installing
	mkdir -p ~/.config/obs-studio/plugins/wlrobs/bin/64bit
	cp build/libwlrobs.so ~/.config/obs-studio/plugins/wlrobs/bin/64bit
## Uninstalling
	rm -rf ~/.config/obs-studio/plugins/wlrobs
## Bug Reports
Please file bug reports at https://todo.sr.ht/~scoopta/wlrobs
## Contributing
Please submit patches to https://lists.sr.ht/~scoopta/wlrobs

You can find documentation here https://man.sr.ht/hg.sr.ht/email.md
