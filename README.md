# real-time-vector-demo

`real-time-vector-demo` (`DEMO`) is a real-time vector engine prototype built for technical validation. It is not intended to be an end-user product. The project focuses on validating:

- A processing pipeline that converts local images and videos into stable vector paths
- Coordinate mapping from scene paths to multiple zones and devices
- Real-time UDP distribution to multiple local DEMO instances
- A core engine architecture that can run independently from the GUI

## Current MVP Scope

- A minimal Qt Widgets technical interface
- Local image and video loading
- OpenCV-based contour extraction, path simplification, smoothing, and cross-frame stabilization
- JSON-driven zone and device configuration
- Device frame encoding, UDP fragmentation, transmission, and reassembly
- Automatic receiver preview mode when no local media is loaded
- CLI sender and receiver entry points alongside the GUI

## Dependencies

- CMake 3.21+
- A C++17 compiler
- Qt 5.15+ or Qt 6
- OpenCV 4.x

Notes:

- OpenCV is a required dependency for the sender-side media processing pipeline.
- If OpenCV is not found during configuration, the project still builds in receiver-only / stub mode, but local image and video processing will be unavailable.

## Build

```bash
make build
```

## Run

Start the default three-window GUI demo:

```bash
make run
```

This launches:

- `configs/demo.receiver.50010.json`
- `configs/demo.receiver.50011.json`
- `configs/demo.sender.json`

The three GUI windows are labeled as `DEMO Receiver 50010`, `DEMO Receiver 50011`, and `DEMO Sender`.

Important:

- Only the window started with `configs/demo.sender.json` is configured to forward frames to the other two windows.
- Load media and click `Start` in the `DEMO Sender` window to drive the two receiver windows.
- If you load media in a receiver window, it will only preview locally and will not forward frames, because the receiver configs do not define any `network.targets`.

Start a CLI receiver instance on a specific config:

```bash
make run MODE=receiver CONFIG=configs/demo.receiver.50011.json
```

Start a CLI sender instance:

```bash
make run MODE=sender CONFIG=configs/demo.sender.json MEDIA=/path/to/video.mp4
```

Clean the build output:

```bash
make clean
```

You can still run the binaries directly if needed:

```bash
./build/demo_gui --config configs/demo.receiver.50010.json
./build/demo_gui --config configs/demo.receiver.50011.json
./build/demo_gui --config configs/demo.sender.json
./build/demo_cli --mode sender --config configs/demo.sender.json --media /path/to/video.mp4
```

## Configuration

Configuration files are stored in `configs/` and are currently JSON-driven.

- `network.listenPort`: UDP port used by the current instance for receiving
- `network.sourceId`: sender source identifier
- `network.targets[]`: sender-side fan-out target list
- `zones[]`: zone definitions in normalized scene coordinates
- `processing`: contour extraction, path stabilization, and real-time processing parameters

## Documentation

- Architecture and MVP decisions are documented in `docs/adr/ADR-0001-20260322-mvp-architecture.md`
