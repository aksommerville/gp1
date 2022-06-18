# gp1

There's a few pieces:
- gp1: Runtime. Basically libgp1vm, libgp1io, and some command-line utilities.
- libgp1vm: Library for the VM core.
- libgp1io: Library for host I/O.
- Facilities for starting a new project: Build both GP1 images and native executables.
- Client-side library, eg video and audio buffer assembly.
- Web host. Use the browser's facilities for Wasm, and implement audio, video, and input with other browser APIs.

Prereqs:
- Wasm Micro Runtime: https://github.com/bytecodealliance/wasm-micro-runtime
- WASI SDK 14.0: https://github.com/WebAssembly/wasi-sdk/releases

## TODO

- [ ] VM
- - [x] ROM file
- - [x] Execution
- - [x] Linkage
- - [ ] Renderer
- - [ ] Software renderer
- - [ ] Vulkan?
- - [ ] Metal?
- - [ ] Direct3d?
- - [ ] Synthesizer
- - [ ] Input manager (how much of this belongs in VM, and how much in host?)
- [ ] I/O library
- - [x] GLX driver
- - - [ ] icon
- - [x] DRM driver
- - - [ ] Is it kosher as is, or do we need a separate "begin frame" hook?
- - [ ] MacOS video driver
- - [ ] Windows video driver
- - [x] ALSA driver
- - [x] PulseAudio driver
- - [ ] MacOS audio driver
- - [ ] Windows audio driver
- - [x] evdev
- - [ ] MacOS HID
- - [ ] Windows HID
- - [ ] User languages
- [ ] Lights-On validator ROM
- - [ ] All image formats
- - [ ] Display inputs
- - [ ] Multiple languages
- - [ ] Use strings
- - [ ] Use data
- - [ ] Use HTTP
- - [ ] Use WebSocket
- - [ ] Use storage
- - [ ] Use every rendering command
- - [ ] Use every audio command
- - [ ] Every audio config type
- - [ ] Play a song
- [ ] Native host
- - [ ] General GUI
- - [x] --help, show available drivers
- [ ] Web host
- - [ ] Execution
- - [ ] Renderer
- - [ ] Synthesizer
- [ ] Tooling
- - [x] Pack ROM
- - [x] Convert chunk
- - [x] Dump ROM info
- - [ ] Build pipeline for Wasm
- - [ ] Spawn project
- [ ] Documentation
- [x] Let imAG be empty, as a declaration of an intermediate framebuffer. No API for creating framebuffers live.
