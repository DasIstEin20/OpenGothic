# REGoth Legacy Android Components

This document lists Android specific pieces from `external/regoth_legacy` that are
useful when building an Android launcher around OpenGothic.

## Native glue
- `src/engine/PlatformAndroid.cpp` and `PlatformAndroid.h` implement
  `android_main`, input loop and JNI hooks for installer extraction.
- `REGoth-Android/app` contains a Java launcher using `NativeActivity` and a
  small bootstrap activity which loads the native library.

## Input handling
- Touch events are processed in `PlatformAndroid::onInputEvent` where two virtual
  thumb sticks are emulated. Mappings are translated into engine actions.
- Key bindings are configured via `bindKey` and processed each frame in
  `PlatformAndroid::run`.

## File system and installer helpers
- Asset and save data location is defined via `CONTENT_BASE_PATH` pointing to
  `/sdcard/REGoth`.
- Installer extraction is handled through JNI functions
  `Java_com_regothproject_regoth_InstallerExtract_*` which call
  `zTools::extractInstaller`.

## Audio backend
- REGoth relied on OpenAL (see `src/audio`); OpenSL is configured in
  `openal-soft` build scripts but not directly used from the engine code.

These components can be ported by implementing small backend classes that expose
input events, file access via `AAssetManager` and lifecycle hooks.

