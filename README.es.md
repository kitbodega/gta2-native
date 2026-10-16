# GTA2 Native

Port nativo de Android de Grand Theft Auto 2, basado en el proyecto de decompilación [gta2_re](https://github.com/CriminalRETeam/gta2_re).

## Descripción

Compila el código C/C++ decompilado de GTA2 en una librería nativa para Android (ARM) con una app Kotlin que usa OpenGL ES para renderizar a través de una capa de compatibilidad que traduce las llamadas originales de DirectX/Windows.

## Arquitectura

```
app/src/main/
├── kotlin/com/bodega/gta2/
│   ├── MainActivity.kt          # Punto de entrada, GLSurfaceView
│   └── InputOverlay.kt          # Overlay de gamepad virtual
├── cpp/
│   ├── jni_native.cpp           # Puente JNI (Java ↔ C++)
│   ├── CMakeLists.txt           # Config NDK
│   └── platform/
│       ├── engine_backend.cpp    # Ciclo de vida del motor
│       ├── gles_glue.cpp/.hpp   # Wrapper OpenGL ES (reemplaza D3D)
│       ├── wincompat.cpp        # Shims de Windows API para Android
│       ├── wincompat_registry.cpp
│       ├── mss_stubs.cpp        # Stubs de Miles Sound System
│       ├── windows.h / wintypes.hpp  # Definiciones mínimas de tipos Windows
│       └── VC98/Include/         # Headers legacy VC98 (DPLAY, etc.)
├── res/                         # Recursos Android (iconos, layouts)
└── assets/
    └── virtualpad/
        └── virtual-input.png     # Textura del gamepad
```

El núcleo del juego vive en [gta2_re](https://github.com/CriminalRETeam/gta2_re) (directorio hermano). El build nativo compila ~230 archivos C++ de `gta2_re/Source/` junto con la capa de compatibilidad.

## Requisitos

- **Android Studio** (o SDK + NDK)
- **JDK 17** (JBR de Android Studio recomendado)
- **Android SDK** con API 26+ y NDK
- **Gradle** (wrapper incluido)

## Compilación

### Debug

```bash
./gradlew assembleDebug
```

Salida: `app/build/outputs/apk/debug/app-debug.apk`

### Release (firmado)

```bash
./gradlew assembleRelease
```

Firmado con el keystore en `gta2.keystore` (alias: `gta2`).

## Configuración

| Parámetro | Valor |
|-----------|-------|
| `applicationId` | `com.bodega.gta2` |
| `minSdk` | 26 (Android 8.0) |
| `targetSdk` | 28 (Android 9) |
| ABI | `armeabi-v7a` |
| C++ | C++17 |

## Estructura

| Ruta | Descripción |
|------|-------------|
| `app/src/main/cpp/` | Puente JNI + capa de compatibilidad |
| `app/src/main/kotlin/` | App Android (Activity, InputOverlay) |
| `app/src/main/cpp/platform/` | Shims Windows→Android (GL, sonido, registry) |
| `tools/bink_movie/` | Utilidades de video Bink |
| `build_tranche1.ps1` | Script de build Windows (tranche 1) |
| `build_tranche2.ps1` | Script de build Windows (tranche 2) |
| `link_tranche1.ps1` | Helper de link para tranche 1 |

## Proyectos relacionados

- [gta2_re](https://github.com/CriminalRETeam/gta2_re) — Decompilación de GTA2 (C/C++)
- [GTA2Hax](https://github.com/CriminalRETeam/gta2_re/tree/master/3rdParty/GTA2Hax) — Framework de hooks DirectX

## Licencia

Proyecto con fines educativos/investigación. Grand Theft Auto 2 es marca registrada de Rockstar Games.
