package com.bodega.gta2

import android.Manifest
import android.app.Activity
import android.content.pm.PackageManager
import android.graphics.PixelFormat
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.ViewGroup
import android.widget.FrameLayout
import android.widget.Toast
import java.io.File

class MainActivity : Activity() {

    private lateinit var surfaceView: SurfaceView
    private lateinit var overlay: InputOverlay
    private var started = false
    private var gameLoaded = false
    private var surfaceReady = false

    // Deteccion de vehiculo para el overlay: joystick al conducir, flechas a pie.
    private val pollHandler = Handler(Looper.getMainLooper())
    private val pollTask = object : Runnable {
        override fun run() {
            if (started) {
                val car = nativeInVehicle()
                if (car != overlay.inCar) overlay.applyVehicle(car)
                pollHandler.postDelayed(this, 300)
            }
        }
    }

    override fun onResume() {
        super.onResume()
        if (started) pollHandler.post(pollTask)
    }

    override fun onPause() {
        super.onPause()
        pollHandler.removeCallbacks(pollTask)
    }

    override fun onDestroy() {
        super.onDestroy()
        pollHandler.removeCallbacks(pollTask)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        surfaceView = SurfaceView(this).apply {
            holder.setFormat(PixelFormat.RGBA_8888)
            holder.addCallback(object : SurfaceHolder.Callback {
                override fun surfaceCreated(holder: SurfaceHolder) {
                    nativeSetSurface(holder.surface)
                    surfaceReady = true
                    maybeStart()
                }

                override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {}

                override fun surfaceDestroyed(holder: SurfaceHolder) {
                    surfaceReady = false
                    nativeSetSurface(null)
                }
            })
        }

        overlay = InputOverlay(this, ::nativeKey)

        val root = FrameLayout(this)
        root.addView(surfaceView, FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
        root.addView(overlay, FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
        setContentView(root)

        if (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE)
            != PackageManager.PERMISSION_GRANTED
        ) {
            requestPermissions(arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE), 1)
        } else {
            bootIfReady()
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        bootIfReady()
    }

    private fun findGameData(): String? {
        val candidates = listOf(
            "/storage/emulated/0/Download/GTA2",
            "/storage/emulated/0/Download/GTA2/gta2.exe",
            "${filesDir}/game/GTA2/gta2.exe",
            getExternalFilesDir(null)?.absolutePath
        )
        for (c in candidates) {
            if (c == null) continue
            val f = File(c)
            if (f.isFile) {
                if (f.length() > 0) return f.parentFile?.absolutePath ?: c
            } else if (f.isDirectory) {
                if (File(f, "data").isDirectory) return f.absolutePath
                if (File(f, "gta2.exe").isFile) return f.absolutePath
            }
        }
        return null
    }

    private fun bootIfReady() {
        if (gameLoaded) return
        if (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE)
            != PackageManager.PERMISSION_GRANTED
        ) {
            Log.i(TAG, "Sin permiso de almacenamiento.")
            return
        }

        val dir = findGameData()
        if (dir == null) {
            Log.i(TAG, "No se encontraron los datos del juego.")
            Toast.makeText(this, "Datos de GTA2 no encontrados", Toast.LENGTH_LONG).show()
            return
        }

        Log.i(TAG, "Datos del juego: $dir")
        val exe = if (dir.endsWith("gta2.exe")) dir else "$dir/gta2.exe"
        if (nativeLoadGame(exe)) {
            gameLoaded = true
            maybeStart()
        }
    }

    private fun maybeStart() {
        if (!started && gameLoaded && surfaceReady) {
            started = true
            nativeStart()
        }
    }

    external fun nativeLoadGame(path: String): Boolean
    external fun nativeStart(): Boolean
    external fun nativeStop()
    external fun nativeSetSurface(surface: android.view.Surface?)
    external fun nativeKey(scan: Int, down: Int)
    external fun nativeInVehicle(): Boolean

    private companion object {
        const val TAG = "GTA2-port"

        init {
            System.loadLibrary("gta2native")
        }
    }
}