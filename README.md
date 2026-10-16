# GTA2 Native

Android native port of Grand Theft Auto 2, built on the [gta2_re](https://github.com/CriminalRETeam/gta2_re) decompilation.

## Overview

Compiles the decompiled GTA2 C/C++ source into a native Android library (ARM) with a Kotlin app that renders via OpenGL ES, using a compatibility layer that translates the original DirectX/Windows API calls.

## Architecture

```
app/src/main/
├── kotlin/com/bodega/gta2/
│   ├── MainActivity.kt          # Entry point, GLSurfaceView
│   └── InputOverlay.kt          # Virtual gamepad overlay
├── cpp/
│   ├── jni_native.cpp           # JNI bridge (Java ↔ C++)
│   ├── CMakeLists.txt           # NDK build config
│   └── platform/
│       ├── engine_backend.cpp    # Engine lifecycle
│       ├── gles_glue.cpp/.hpp   # OpenGL ES wrapper (replaces D3D)
│       ├── wincompat.cpp        # Windows API shims for Android
│       ├── wincompat_registry.cpp
│       ├── mss_stubs.cpp        # Miles Sound System stubs
│       ├── windows.h / wintypes.hpp  # Minimal Windows type defs
│       └── VC98/Include/         # Legacy VC98 headers (DPLAY, etc.)
├── res/                         # Android resources (icons, layouts)
└── assets/
    └── virtualpad/
        └── virtual-input.png     # Gamepad texture
```

The game core lives in [gta2_re](https://github.com/CriminalRETeam/gta2_re) (sibling directory). The native build compiles ~230 C++ source files from `gta2_re/Source/` plus the platform compatibility layer.

## Requirements

- **Android Studio** (or SDK + NDK)
- **JDK 17** (Android Studio bundled JBR recommended)
- **Android SDK** with API 26+ and NDK
- **Gradle** (wrapper included)

## Building

### Debug

```bash
./gradlew assembleDebug
```

Output: `app/build/outputs/apk/debug/app-debug.apk`

### Release (signed)

```bash
./gradlew assembleRelease
```

Signed with the keystore at `gta2.keystore` (alias: `gta2`).

## Configuration

| Setting | Value |
|---------|-------|
| `applicationId` | `com.bodega.gta2` |
| `minSdk` | 26 (Android 8.0) |
| `targetSdk` | 28 (Android 9) |
| ABI | `armeabi-v7a` |
| C++ | C++17 |

## Project Structure

| Path | Description |
|------|-------------|
| `app/src/main/cpp/` | JNI bridge + platform compatibility layer |
| `app/src/main/kotlin/` | Android app (Activity, InputOverlay) |
| `app/src/main/cpp/platform/` | Windows→Android shims (GL, sound, registry) |
| `tools/bink_movie/` | Bink video utilities |
| `build_tranche1.ps1` | Windows build script (tranche 1) |
| `build_tranche2.ps1` | Windows build script (tranche 2) |
| `link_tranche1.ps1` | Link helper for tranche 1 |

## Related Projects

- [gta2_re](https://github.com/CriminalRETeam/gta2_re) — GTA2 decompilation (C/C++)

## License

Educational/research purposes. Grand Theft Auto 2 is a trademark of Rockstar Games.
