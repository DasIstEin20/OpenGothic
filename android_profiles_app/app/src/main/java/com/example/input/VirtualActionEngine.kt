package com.example.input

/**\n * Engine responsible for translating raw input events into high level actions.\n */
class VirtualActionEngine(private val profile: Map<String, VirtualAction>) {

    fun process(event: InputEvent): VirtualActionEvent? {
        val key = when(event.type) {
            EventType.KEY -> "KEY_${'$'}{event.code}"
            EventType.MOTION -> "AXIS_${'$'}{event.code}"
        }
        val action = profile[key] ?: return null
        val time = event.timestamp
        return when(action.type) {
            ActionType.BOOLEAN -> {
                val pressed = if(event.type == EventType.KEY) {
                    event.value != 0f
                } else {
                    if (kotlin.math.abs(event.value) <= action.deadzone) return null
                    event.value > 0
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
                var value = event.value
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

data class VirtualAction(
    val name: String,
    val type: ActionType,
    val deadzone: Float = 0f,
    val min: Float = -1f,
    val max: Float = 1f
)

data class VirtualActionEvent(
    val name: String,
    val type: ActionType,
    val booleanValue: Boolean? = null,
    val analogValue: Float? = null,
    val inputId: Int? = null,
    val timestamp: Long = System.currentTimeMillis()
)
