# VR Notes

Run the game with `--vr` to enable the OpenXR backend.

### Options
* `--vr-first-person=on|off` – toggle first person camera (default `on`).
* `--vr-height-offset=<m>` – vertical eye offset in meters (default `1.75`).
* `--vr-allow-roll=on|off` – allow roll from the headset (default `off`).
* `--vr-snap-deg=<deg>` – snap turn angle per key press (default `30`).

Keyboard, mouse and gamepad remain for movement and turning.

## Controller bindings & locomotion

Default Oculus Touch bindings:

- Left thumbstick – move
- Right thumbstick X – turn
- X – jump
- A – interact
- B – menu
- Right trigger – attack
- Left trigger – teleport
- Right aim pose – teleport ray

Additional options:

- `--vr-teleport=on|off` – enable teleport locomotion (default `on`)
- `--vr-turn-mode=snap|smooth` – snap or smooth turning (default `snap`)
- `--vr-turn-speed=<float>` – smooth turn speed in deg/sec (default `120`)
- `--vr-turn-deadzone=<float>` – deadzone for snap turn sticks (default `0.25`)
- `--vr-snap-cooldown-ms=<int>` – cooldown between snap turns (default `250`)
- `--vr-move-speed-scale=<float>` – movement speed multiplier (default `1.0`)
- `--vr-vignette-strength=<0..1>` – comfort vignette strength (default `0`)
