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

## HUD/UI in VR

The 2D interface can be rendered onto a floating quad in front of the player.
Available options:

- `--vr-hud-distance=<meters>` – distance from the head to the HUD plane (default `1.4`)
- `--vr-hud-width=<meters>` – physical width of the HUD quad (default `1.0`)
- `--vr-hud-scale=<0.5..2.0>` – additional HUD scale factor (default `1.0`)
- `--vr-hud-pitch-deg=<deg>` – pitch offset of the quad in degrees (default `-10`)
- `--vr-hud-follow=on|off` – make the HUD follow the head (default `on`)
- `--vr-hud-res-scale=<float>` – resolution scale for the offscreen HUD texture (default `1.0`)

### Laser pointer & Follow

Aim with the right-hand controller; the trigger (interact) maps to a mouse click on the HUD quad. The quad position can be tuned via `--vr-hud-distance`, `--vr-hud-width`, `--vr-hud-scale`, `--vr-hud-pitch-deg`, `--vr-hud-follow` and `--vr-hud-res-scale`. When follow is enabled, the quad smoothly tethers to head movement with gentle smoothing.
Recommended values for Quest 2: distance `1.4m`, width `1.0m`, pitch `-10°`, res-scale `1.0`.
