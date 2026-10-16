# GTA2 Native

Android native port of Grand Theft Auto 2 (PSX/PC), built on top of the [gta2_re](https://github.com/CriminalRETeam/gta2_re) decompilation project.

## Overview

This project compiles the decompiled GTA2 C/C++ source code into a native Android library (ARM) and wraps it with a Kotlin-based Android application. It uses OpenGL ES for rendering via a compatibility layer that translates the original DirectX/Windows API calls.

## Architecture

```
app/src/main/
├── kotlin/com/bodega/gta2/
│   ├── MainActivity.kt          # Android entry point, GLSurfaceView setup
│   └── InputOverlay.kt          # Virtual gamepad overlay
├── cpp/
│   ├── jni_native.cpp           # JNI bridge (Java ↔ C++)
│   ├── CMakeLists.txt           # NDK build config
│   └── platform/
│       ├── engine_backend.cpp    # Engine lifecycle management
│       ├── gles_glue.cpp/.hpp   # OpenGL ES wrapper (replaces D3D)
│       ├── wincompat.cpp        # Windows API shims for Android
│       ├── wincompat_registry.cpp
│       ├── mss_stubs.cpp        # Miles Sound System stubs
│       ├── windows.h / wintypes.hpp  # Minimal Windows type definitions
│       └── VC98/Include/         # Legacy VC98 headers (DPLAY, etc.)
├── res/                         # Android resources (icons, layouts)
└── assets/
    └── virtualpad/
        └── virtual-input.png     # Gamepad texture
```

The core game logic lives in the [gta2_re](https://github.com/CriminalRETeam/gta2_re) repository (included as a sibling directory). The native build compiles ~230 C++ source files from `gta2_re/Source/` along with the platform compatibility layer.

## Requirements

- **Android Studio** (or Android SDK + NDK)
- **JDK 17** (Android Studio bundled JBR recommended)
- **Android SDK** with API 26+ and NDK
- **Gradle** (wrapper included)

## Building

### Debug APK

```bash
./gradlew assembleDebug
```

Output: `app/build/outputs/apk/debug/app-debug.apk`

### Release APK (signed)

```bash
./gradlew assembleRelease
```

The release build is signed with the keystore at `gta2.keystore` (alias: `gta2`).

## Configuration

| Setting | Value |
|---------|-------|
| `applicationId` | `com.bodega.gta2` |
| `minSdk` | 26 (Android 8.0) |
| `targetSdk` | 28 (Android 9) |
| ABI | `armeabi-v7a` |
| C++ Standard | C++17 |

## Project Structure

| Path | Description |
|------|-------------|
| `app/src/main/cpp/` | JNI bridge + platform compatibility layer |
| `app/src/main/kotlin/` | Android app (Activity, InputOverlay) |
| `app/src/main/cpp/platform/` | Windows→Android shims (GL, sound, registry) |
| `tools/bink_movie/` | Bink video decoder utilities |
| `build_tranche1.ps1` | Windows build script (tranche 1) |
| `build_tranche2.ps1` | Windows build script (tranche 2) |
| `link_tranche1.ps1` | Link helper for tranche 1 |

## Related Projects

- [gta2_re](https://github.com/CriminalRETeam/gta2_re) — GTA2 decompilation (C/C++)
- [GTA2Hax](https://github.com/CriminalRETeam/gta2_re/tree/master/3rdParty/GTA2Hax) — DirectX hooking framework

## License

This project is for educational/research purposes. Grand Theft Auto 2 is a trademark of Rockstar Games.
