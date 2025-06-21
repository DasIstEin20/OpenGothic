# Android Integration Guide

This guide describes how to embed the OpenGothic engine into a minimal
`NativeActivity` based launcher. The approach follows the backend design with
interfaces implemented in `backend/`.

1. **Build system**
   - The root `CMakeLists.txt` checks for `ANDROID` and includes the `android`
     directory. The `android/CMakeLists.txt` builds `libOpenGothic.so` from the
     backend sources and links against the NDK libraries.

2. **Native entry point**
   - `backend/android/android_main.cpp` defines `android_main` using the NDK
     glue. It creates backend objects (`AndroidInputBackend`, `AndroidAudioBackend`,
     `AndroidFileSystemBackend`, `AndroidNativeGlue`) and passes them to
     `GameMain` – an exported function expected to start the engine. Input events
     from the looper are forwarded via `onInputEvent` to the input backend.

3. **Manifest**
   - `android/AndroidManifest.xml` configures a landscape `NativeActivity` and
     requests external storage permissions.

4. **Extending the engine**
   - Game code should call `GameMain` instead of `main` when built for Android
     so that platform backends can be injected. Touch handling and asset loading
     can then be implemented on top of the provided interfaces.
  - Version 2 introduced basic key and motion callbacks. Input events are now
    classified via `InputEventType` into `KEY`, `MOTION`, `SPECIAL`, `SYSTEM`,
    `VIRTUAL` or `UNCLASSIFIED`. `AndroidInputBackend` normalizes data into a
    small `InputEventData` structure for logging and internal processing.
  - Version 4 adds duplicate filtering and switches logging to a JSON style
    record such as `{ "type": "MOTION", "x": 100, "y": 200 }`. Events are
    validated with `validCoords` and filtered with `sameEvent` before
    dispatch. Verbose output can be toggled with
    `AndroidInputBackend::setVerboseLogging`.
  - Version 5 records a timestamp (`eventTime`) and the event `source`
    (`TOUCHSCREEN`, `KEYBOARD`, or `JOYSTICK`). Logged entries now resemble
    `{ "type": "MOTION", "source": "TOUCHSCREEN", "dev": 2, "key": 0,
      "pressed": false, "x": 100, "y": 200, "eventTime": 12345 }`. When a
    duplicate is filtered a debug message like
    `Duplicate event skipped: { "type": "MOTION", ... }` is printed.

`eventTime` represents the original Android timestamp in milliseconds and
`source` indicates the origin device after translating the Android input source
flags. These fields help correlate events with engine behavior and diagnose
suppression of duplicates.

