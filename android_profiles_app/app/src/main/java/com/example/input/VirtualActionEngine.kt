package com.example.input

/**\n * Engine responsible for translating raw input events into high level actions.\n */
/**
 * Engine responsible for translating raw input events into high level actions.
 * The mapping profile can configure per-axis scaling, inversion and deadzone
 * handling. Raw input events are converted into [VirtualActionEvent] objects
 * that are easier for the UI layer to consume.
 */
class VirtualActionEngine(private val profile: Map<String, VirtualAction>) {

    /**
     * Accessor used by the UI to show current mappings.
     */
    fun mappings(): Map<String, VirtualAction> = profile

    companion object {
        /**
         * Parses a JSON string into a profile map understood by [VirtualActionEngine].
         * The JSON may use a simplified format where the value is a plain action
         * string or a full object describing the action parameters.
         */
        fun fromJson(json: String): Map<String, VirtualAction> {
            val result = mutableMapOf<String, VirtualAction>()
            val root = org.json.JSONObject(json)
            val it = root.keys()
            while (it.hasNext()) {
                val key = it.next()
                val value = root.get(key)
                if (value is String) {
                    result[key] = VirtualAction(name = value, type = ActionType.BOOLEAN)
                } else if (value is org.json.JSONObject) {
                    val name = value.optString("name")
                    val type = ActionType.valueOf(value.optString("type", "BOOLEAN"))
                    val dead = value.optDouble("deadzone", 0.0).toFloat()
                    val min = value.optDouble("min", -1.0).toFloat()
                    val max = value.optDouble("max", 1.0).toFloat()
                    val scale = value.optDouble("scale", 1.0).toFloat()
                    val inv = value.optBoolean("invert", false)
                    result[key] = VirtualAction(name, type, dead, min, max, scale, inv)
                }
            }
            return result
        }
    }

    fun process(event: InputEvent): VirtualActionEvent? {
        val key = when(event.type) {
            EventType.KEY -> "KEY_${'$'}{event.code}"
            EventType.MOTION -> "AXIS_${'$'}{event.code}"
        }
        val action = profile[key] ?: return null
        val time = event.timestamp
        return when(action.type) {
            ActionType.BOOLEAN -> {
                var value = event.value * action.scale
                if(action.invert) value = -value
                val pressed = if(event.type == EventType.KEY) {
                    value != 0f
                } else {
                    if (kotlin.math.abs(value) <= action.deadzone) return null
                    value > 0
                }
                VirtualActionEvent(
                    name = action.name,
                    type = ActionType.BOOLEAN,
                    booleanValue = pressed,
                    analogValue = null,
                    inputId = event.code,
                    timestamp = time
                )
            }
            ActionType.ANALOG -> {
                var value = event.value * action.scale
                if(action.invert) value = -value
                if(kotlin.math.abs(value) <= action.deadzone) return null
                val range = action.max - action.min
                if(range != 0f) {
                    value = ((value - action.min) / range).coerceIn(0f, 1f)
                }
                VirtualActionEvent(
                    name = action.name,
                    type = ActionType.ANALOG,
                    booleanValue = null,
                    analogValue = value,
                    inputId = event.code,
                    timestamp = time
                )
            }
        }
    }
}

enum class EventType { KEY, MOTION }

/** Source of the event if needed for debugging or filtering */
enum class EventSource { UNKNOWN, TOUCH, MULTITOUCH, KEYBOARD, GAMEPAD }

data class InputEvent(
    val type: EventType,
    val code: Int,
    val value: Float = 0f,
    val source: EventSource = EventSource.UNKNOWN,
    val timestamp: Long = System.currentTimeMillis()
)

enum class ActionType { BOOLEAN, ANALOG }

/**
 * Configuration describing how a particular input maps to an action.
 * @param scale    Scaling factor applied to axis value before processing.
 * @param invert   If true the axis value is inverted.
 */
data class VirtualAction(
    val name: String,
    val type: ActionType,
    val deadzone: Float = 0f,
    val min: Float = -1f,
    val max: Float = 1f,
    val scale: Float = 1f,
    val invert: Boolean = false
)

data class VirtualActionEvent(
    val name: String,
    val type: ActionType,
    val booleanValue: Boolean? = null,
    val analogValue: Float? = null,
    val inputId: Int? = null,
    val timestamp: Long = System.currentTimeMillis()
)
