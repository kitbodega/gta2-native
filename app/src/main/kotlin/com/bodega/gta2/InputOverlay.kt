package com.bodega.gta2

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.view.MotionEvent
import android.view.View
import kotlin.math.abs
import kotlin.math.hypot
import kotlin.math.min
import kotlin.math.sqrt

/**
 * Overlay tactil: a la izquierda un joystick analogico cuando se conduce
 * (auto-detectado via [inCar]) o cuatro flechas (d-pad) al ir a pie; a la
 * derecha los botones de accion en rejilla grande. Multi-touch, animacion de
 * presionado e histeresis en el joystick. No depende de MainActivity: recibe
 * los keys a traves de [onKey] (scan usado + estado down/up).
 */
class InputOverlay(context: Context, private val onKey: (Int, Int) -> Unit) : View(context) {

    /** true = modo vehiculo (joystick), false = a pie (flechas d-pad). */
    var inCar: Boolean = false

    fun applyVehicle(car: Boolean) {
        if (inCar == car) return
        inCar = car
        resetAll()
        invalidate()
    }

    enum class Zone(val scan: Int) {
        UP(0xC8), DOWN(0xD0), LEFT(0xCB), RIGHT(0xCD),
        A(0x1C), B(0x01), ATK(0x1D), JUMP(0x39)
    }

    private data class Button(val cx: Float, val cy: Float, val side: Float, val hitR: Float)

    // ---- Escala relativa al lado corto de la pantalla ----
    private object Layout {
        const val STICK_X = 0.34f            // centro x del stick (frac. de s)
        const val STICK_OFFSET_Y = 0.34f     // distancia al borde inferior
        const val STICK_DIAM_R = 0.15f       // radio visual de la base
        const val THUMB_R = 0.10f            // radio del pulgar
        const val GRAB_MULT = 2.8f           // area de agarre del stick
        const val CLUSTER_RIGHT = 0.30f      // ancla del cluster derecho (frac. s)
    }

    // Botones: rejilla 2x2 en la esquina inferior derecha.
    private object ButtonLayout {
        const val A_SIDE = 0.22f
        const val A_HIT = 0.115f
        const val OTHER_SIDE = 0.20f
        const val OTHER_HIT = 0.105f
        const val STACK_DX = 0.22f
        const val STACK_DY = 0.24f
        const val STACK_OFFSET_X = 0.26f    // borde derecho: x = w - s * OFFSET
        const val STACK_OFFSET_Y = 0.26f    // borde inferior: y = h - s * OFFSET
    }

    // D-pad (a pie): cruz de 4 flechas centrada donde va el joystick.
    private object DpadLayout {
        const val RADIUS = 0.155f           // distancia centro -> boton (frac. s)
        const val SIDE = 0.16f              // lado del boton dibujado
        const val HIT = 0.115f              // radio de tacto
        const val GLYPH = 0.30f             // apertura de la flecha (frac. del lado)
        const val STROKE = 0.10f            // grosor de la flecha (frac. del lado)
    }

    // Color de acento por direccion (para que cada boton se distinga).
    private fun accentColor(z: Zone): Int = when (z) {
        Zone.UP -> 0xFF7FA8FF.toInt()
        Zone.DOWN -> 0xFF7FE8B0.toInt()
        Zone.LEFT -> 0xFFFFC07F.toInt()
        Zone.RIGHT -> 0xFFFF8A8A.toInt()
        else -> 0xFFFFFFFF.toInt()
    }

    private val dpad = HashMap<Zone, Button>()
    private val dpadOrder = listOf(Zone.UP, Zone.DOWN, Zone.LEFT, Zone.RIGHT)
    private var dpadN: Bitmap? = null
    private var dpadP: Bitmap? = null

    private val PUSH = 0.32f
    private val RELEASE = 0.18f

    private val atlas: Bitmap? = try {
        BitmapFactory.decodeStream(context.assets.open("virtualpad/virtual-input.png"))
    } catch (e: Exception) {
        null
    }

    private val normal = HashMap<Zone, Bitmap?>()
    private val pressed = HashMap<Zone, Bitmap?>()
    private val buttons = HashMap<Zone, Button>()
    private val pressAnim = HashMap<Zone, Float>()
    private val pointers = HashMap<Int, Zone>()
    private val active = HashSet<Zone>()
    private val stickKeys = HashSet<Zone>()

    private var baseImg: Bitmap? = null
    private var thumbImg: Bitmap? = null

    private var baseCx = 0f
    private var baseCy = 0f
    private var baseR = 0f
    private var thumbR = 0f
    private var grabR = 0f

    private var stickPtr: Int? = null
    private var thumbX = 0f
    private var thumbY = 0f
    private var targetTX = 0f
    private var targetTY = 0f
    private var animRunning = false

    private val paint = Paint()
    private val dstRect = RectF()
    private val arrowPaint = Paint().apply {
        isAntiAlias = true
        style = Paint.Style.STROKE
        strokeCap = Paint.Cap.ROUND
        strokeJoin = Paint.Join.ROUND
    }
    private val arrowPath = Path()

    init {
        // tira de 396x135 -> 3 tiles de 132x135: normal / over / press
        fun tile(stripX: Int, stripY: Int, state: Int): Bitmap? =
            atlas?.let { a ->
                val h = minOf(135, a.height - stripY)
                Bitmap.createBitmap(a, stripX + 132 * state, stripY, 132, h)
            }

        fun bind(z: Zone, sx: Int, sy: Int) {
            normal[z] = tile(sx, sy, 0)
            pressed[z] = tile(sx, sy, 2)
        }

        bind(Zone.A, 0, 0)
        bind(Zone.B, 0, 140)
        bind(Zone.ATK, 798, 0)
        bind(Zone.JUMP, 399, 416)
        // D-pad: mismo estilo de boton redondo del atlas (tira gris col1 row0),
        // con la flecha pintada encima.
        dpadN = tile(399, 0, 0)
        dpadP = tile(399, 0, 2)
        baseImg = atlas?.let { Bitmap.createBitmap(it, 798, 138, 252, 252) }
        thumbImg = atlas?.let { Bitmap.createBitmap(it, 798, 393, 156, 156) }

        isClickable = true
        setOnTouchListener { _, event ->
            when (event.actionMasked) {
                MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> touchDown(event)
                MotionEvent.ACTION_MOVE -> touchMove(event)
                MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> touchUp(event)
                MotionEvent.ACTION_CANCEL -> resetAll()
            }
            true
        }
    }

    override fun onSizeChanged(w: Int, h: Int, oldW: Int, oldH: Int) {
        super.onSizeChanged(w, h, oldW, oldH)
        val s = min(w.toFloat(), h.toFloat())
        baseCx = s * Layout.STICK_X
        baseCy = h - s * Layout.STICK_OFFSET_Y
        baseR = s * Layout.STICK_DIAM_R
        thumbR = s * Layout.THUMB_R
        grabR = baseR * Layout.GRAB_MULT
        layoutButtons(w.toFloat(), h.toFloat(), s)
        layoutDpad(s)
    }

    private fun layoutDpad(s: Float) {
        val r = s * DpadLayout.RADIUS
        val side = s * DpadLayout.SIDE
        val hit = s * DpadLayout.HIT
        dpad[Zone.UP] = Button(baseCx, baseCy - r, side, hit)
        dpad[Zone.DOWN] = Button(baseCx, baseCy + r, side, hit)
        dpad[Zone.LEFT] = Button(baseCx - r, baseCy, side, hit)
        dpad[Zone.RIGHT] = Button(baseCx + r, baseCy, side, hit)
    }

    private fun layoutButtons(w: Float, h: Float, s: Float) {
        val ax = w - s * ButtonLayout.STACK_OFFSET_X
        val ay = h - s * ButtonLayout.STACK_OFFSET_Y
        buttons[Zone.A] = Button(ax, ay, s * ButtonLayout.A_SIDE, s * ButtonLayout.A_HIT)
        buttons[Zone.B] = Button(ax - s * ButtonLayout.STACK_DX, ay,
            s * ButtonLayout.OTHER_SIDE, s * ButtonLayout.OTHER_HIT)
        buttons[Zone.ATK] = Button(ax, ay - s * ButtonLayout.STACK_DY,
            s * ButtonLayout.OTHER_SIDE, s * ButtonLayout.OTHER_HIT)
        buttons[Zone.JUMP] = Button(ax - s * ButtonLayout.STACK_DX, ay - s * ButtonLayout.STACK_DY,
            s * ButtonLayout.OTHER_SIDE, s * ButtonLayout.OTHER_HIT)
    }

    // ---- Entrada ----
    private fun touchDown(event: MotionEvent) {
        val id = event.getPointerId(event.actionIndex)
        val x = event.getX(event.actionIndex)
        val y = event.getY(event.actionIndex)
        if (stickPtr == null && inCar && inStickGrab(x, y)) {
            stickPtr = id
            updateStick(x, y)
        } else {
            zoneAt(x, y)?.let { z ->
                pointers[id] = z
                setZone(z, true)
            }
        }
    }

    private fun touchMove(event: MotionEvent) {
        val ptr = stickPtr
        for (i in 0 until event.pointerCount) {
            if (ptr != null && event.getPointerId(i) == ptr) {
                updateStick(event.getX(i), event.getY(i))
            }
        }
    }

    private fun touchUp(event: MotionEvent) {
        val id = event.getPointerId(event.actionIndex)
        if (id == stickPtr) releaseStick()
        pointers.remove(id)?.let { setZone(it, false) }
    }

    private fun resetAll() {
        pointers.clear()
        for (z in active.toList()) setZone(z, false)
        releaseStick()
    }

    private fun inStickGrab(x: Float, y: Float): Boolean {
        val dx = x - baseCx
        val dy = y - baseCy
        return dx * dx + dy * dy <= grabR * grabR
    }

    private fun updateStick(x: Float, y: Float) {
        var dx = (x - baseCx).coerceIn(-(baseR - thumbR), baseR - thumbR)
        var dy = (y - baseCy).coerceIn(-(baseR - thumbR), baseR - thumbR)
        if (hypot(dx, dy) > baseR - thumbR) {
            val k = (baseR - thumbR) / sqrt(dx * dx + dy * dy)
            dx *= k
            dy *= k
        }
        thumbX = dx
        thumbY = dy
        targetTX = dx
        targetTY = dy

        val maxOff = baseR - thumbR
        val nx = dx / maxOff
        val ny = dy / maxOff

        val horiz = when {
            nx > PUSH || (stickKeys.contains(Zone.RIGHT) && nx > RELEASE) -> Zone.RIGHT
            nx < -PUSH || (stickKeys.contains(Zone.LEFT) && nx < -RELEASE) -> Zone.LEFT
            else -> null
        }
        val vert = when {
            ny > PUSH || (stickKeys.contains(Zone.DOWN) && ny > RELEASE) -> Zone.DOWN
            ny < -PUSH || (stickKeys.contains(Zone.UP) && ny < -RELEASE) -> Zone.UP
            else -> null
        }

        fun settle(z: Zone?, axis: List<Zone>) {
            if (z == null) {
                axis.filter(stickKeys::contains).forEach {
                    stickKeys.remove(it)
                    onKey(it.scan, 0)
                }
            } else {
                axis.filter { it != z && stickKeys.contains(it) }.forEach {
                    stickKeys.remove(it)
                    onKey(it.scan, 0)
                }
                if (stickKeys.add(z)) onKey(z.scan, 1)
            }
        }
        settle(horiz, listOf(Zone.LEFT, Zone.RIGHT))
        settle(vert, listOf(Zone.UP, Zone.DOWN))
        invalidate()
    }

    private fun releaseStick() {
        stickPtr = null
        stickKeys.toList().forEach {
            stickKeys.remove(it)
            onKey(it.scan, 0)
        }
        targetTX = 0f
        targetTY = 0f
        wake()
    }

    private fun zoneAt(x: Float, y: Float): Zone? {
        for ((z, b) in buttons) {
            val dx = x - b.cx
            val dy = y - b.cy
            if (dx * dx + dy * dy <= b.hitR * b.hitR) return z
        }
        if (!inCar) {
            for ((z, b) in dpad) {
                val dx = x - b.cx
                val dy = y - b.cy
                if (dx * dx + dy * dy <= b.hitR * b.hitR) return z
            }
        }
        return null
    }

    private fun setZone(z: Zone, down: Boolean) {
        if (down == active.contains(z)) return
        if (down) active.add(z) else active.remove(z)
        onKey(z.scan, if (down) 1 else 0)
        wake()
    }

    // ---- Animacion ----
    private fun wake() {
        if (!animRunning) {
            animRunning = true
            postOnAnimation(::step)
        }
    }

    private fun step() {
        var busy = false
        for ((z, v) in pressAnim.toList()) {
            val target = if (active.contains(z)) 1f else 0f
            val next = v + (target - v) * 0.4f
            if (abs(next - target) > 0.004f) busy = true else pressAnim.remove(z)
            pressAnim[z] = next
        }
        if (stickPtr != null) {
            thumbX = targetTX
            thumbY = targetTY
        } else {
            thumbX += (targetTX - thumbX) * 0.30f
            thumbY += (targetTY - thumbY) * 0.30f
            if (abs(thumbX - targetTX) > 0.5f || abs(thumbY - targetTY) > 0.5f) busy = true
        }
        invalidate()
        if (busy) postOnAnimation(::step) else animRunning = false
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)

        for ((z, b) in buttons) {
            val p = pressAnim[z] ?: 0f
            val bmp = (if (p > 0.55f) pressed[z] else normal[z]) ?: continue
            val side = b.side * (1f - 0.10f * p)
            val yOff = b.side * 0.06f * p
            paint.alpha = (200 + 55 * p).toInt()
            dstRect.set(b.cx - side / 2, b.cy - side / 2 + yOff,
                b.cx + side / 2, b.cy + side / 2 + yOff)
            canvas.drawBitmap(bmp, null, dstRect, paint)
        }

        if (inCar) drawStick(canvas) else drawDpad(canvas)
    }

    private fun drawStick(canvas: Canvas) {
        val grabbed = stickPtr != null
        paint.alpha = if (grabbed) 255 else 205
        baseImg?.let {
            dstRect.set(baseCx - baseR, baseCy - baseR, baseCx + baseR, baseCy + baseR)
            canvas.drawBitmap(it, null, dstRect, paint)
        }
        thumbImg?.let {
            dstRect.set(baseCx + thumbX - thumbR, baseCy + thumbY - thumbR,
                baseCx + thumbX + thumbR, baseCy + thumbY + thumbR)
            canvas.drawBitmap(it, null, dstRect, paint)
        }
    }

    private fun drawDpad(canvas: Canvas) {
    for (z in dpadOrder) {
        val b = dpad[z] ?: continue
        val p = pressAnim[z] ?: 0f
        val side = b.side * (1f - 0.10f * p)
        val half = side / 2f
        val yOff = b.side * 0.06f * p
        val bmp = (if (p > 0.55f) dpadP else dpadN) ?: continue
        paint.alpha = (200 + 55 * p).toInt()
        dstRect.set(b.cx - half, b.cy - half + yOff, b.cx + half, b.cy + half + yOff)
        canvas.drawBitmap(bmp, null, dstRect, paint)

        // Anillo de acento: identifica la direccion y brilla al presionar.
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = b.side * 0.045f
        paint.color = accentColor(z)
        paint.alpha = (70 + 150 * p).toInt()
        canvas.drawCircle(b.cx, b.cy + yOff, half * (1f + 0.06f * p), paint)
        paint.style = Paint.Style.FILL

        // Flecha chevron redondeada (con AA y sombra suave).
        val g = b.side * DpadLayout.GLYPH
        val cy = b.cy + yOff
        canvas.save()
        canvas.rotate(rotationFor(z), b.cx, cy)
        arrowPaint.color = 0x40000000.toInt()
        arrowPaint.alpha = (60 + 60 * p).toInt()
        arrowPaint.strokeWidth = b.side * DpadLayout.STROKE * (1f + 0.05f * p)
        arrowPath.reset()
        arrowPath.moveTo(b.cx - g, cy + g + b.side * 0.02f)
        arrowPath.lineTo(b.cx, cy - g * 0.7f + b.side * 0.02f)
        arrowPath.lineTo(b.cx + g, cy + g + b.side * 0.02f)
        canvas.drawPath(arrowPath, arrowPaint)
        arrowPaint.color = 0xFFEFF4FC.toInt()
        arrowPaint.alpha = (200 + 55 * p).toInt()
        arrowPaint.strokeWidth = b.side * DpadLayout.STROKE
        arrowPath.reset()
        arrowPath.moveTo(b.cx - g, cy + g)
        arrowPath.lineTo(b.cx, cy - g * 0.7f)
        arrowPath.lineTo(b.cx + g, cy + g)
        canvas.drawPath(arrowPath, arrowPaint)
        canvas.restore()
    }
}

    private fun rotationFor(z: Zone): Float = when (z) {
        Zone.UP -> 0f
        Zone.RIGHT -> 90f
        Zone.DOWN -> 180f
        Zone.LEFT -> 270f
        else -> 0f
    }
}