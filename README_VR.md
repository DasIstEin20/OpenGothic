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
- `--vr-grab=on|off` – enable grabbing with controllers (default `on`)
- `--vr-grab-distance=<m>` – distance for grabbing items (default `3.0`)
- `--vr-grab-radius=<m>` – hit-test sphere radius (default `0.10`)
- `--vr-throw-scale=<float>` – throw velocity multiplier (default `1.0`)
- `--vr-teleport-grounded=on|off` – snap teleport to ground surface (default `on`)
- `--vr-teleport-max-slope=<deg>` – reject teleport to steep surfaces (default `45`)
- `--vr-walk-step=<meters>` – step offset for collision (default `0.30`)
- `--vr-walk-slope=<deg>` – maximum walkable slope (default `45`)
- `--vr-walk-accel=<m/s^2>` – walk acceleration (default `10`)
- `--vr-walk-maxspeed=<m/s>` – walk speed clamp (default `3`)
- `--vr-keep-heading=on|off` – keep controller yaw on teleport (default `on`)
- `--vr-ui-scroll-scale=<float>` – scroll wheel scale for HUD laser (default `1.0`)
- `--vr-ui-longpress=<sec>` – hold duration for context click (default `0.45`)
- `--vr-haptics=on|off` – enable controller vibration feedback (default `on`)

## HUD/UI in VR

The 2D interface can be rendered onto a floating quad in front of the player.
Available options:

- `--vr-hud-distance=<meters>` – distance from the head to the HUD plane (default `1.4`)
- `--vr-hud-width=<meters>` – physical width of the HUD quad (default `1.0`)
- `--vr-hud-scale=<0.5..2.0>` – additional HUD scale factor (default `1.0`)
- `--vr-hud-pitch-deg=<deg>` – pitch offset of the quad in degrees (default `-10`)
- `--vr-hud-follow=on|off` – make the HUD follow the head (default `on`)
- `--vr-hud-res-scale=<float>` – resolution scale for the offscreen HUD texture (default `1.0`)

Additional tweaks:

- `--vr-render-scale=<float>` – resolution scale for the 3D scene (default `1.0`)
- `--vr-vignette-strength=<0..1>` – comfort vignette strength for the scene only (default `0`)
- `--vr-recenter-hotkey=<key>` – recenter view to current head pose (default `R`)
- `--vr-seated=on|off` – lower height for seated play (default `off`)
- `--vr-dominant-hand=right|left` – choose hand for laser pointer and haptics (default `right`)
- `--vr-log=<off|basic|verbose>` – control OpenXR logging verbosity (default `basic`)

Vignette affects only the 3D world rendering, the HUD layer remains untouched. Recommended values for Quest 2/3: render-scale `1.0–1.2`, vignette-strength `0.15–0.3`, dominant-hand `right`, recenter hotkey `R`.

### Laser pointer & Follow

Aim with the right-hand controller; the trigger (interact) maps to a mouse click on the HUD quad. The quad position can be tuned via `--vr-hud-distance`, `--vr-hud-width`, `--vr-hud-scale`, `--vr-hud-pitch-deg`, `--vr-hud-follow` and `--vr-hud-res-scale`. When follow is enabled, the quad smoothly tethers to head movement with gentle smoothing.
Recommended values for Quest 2: distance `1.4m`, width `1.0m`, pitch `-10°`, res-scale `1.0`.

## VR Hands/Controllers

Simple unlit proxies for your controllers/hands can be rendered in front of the player.

Flags:

- `--vr-show-hands=on|off` – toggle rendering (default `on`)
- `--vr-hands-mode=controller|ghost` – choose proxy style (default `controller`)
- `--vr-laser=on|off` – draw laser from dominant hand aim pose (default `on`)
- `--vr-hand-scale=<float>` – scale of the hand/controller models (default `1.0`)
- `--vr-hand-color-left=<r,g,b>` – RGB color for left hand (default `0.2,0.7,1.0`)
- `--vr-hand-color-right=<r,g,b>` – RGB color for right hand (default `1.0,0.5,0.2`)

The dominant hand is selected with `--vr-dominant-hand`. Laser and reticle appear only when a valid aim pose is available.

## Grabbing & Grounded Teleport

Squeeze a controller to grab nearby objects tagged for VR (name prefix `vr_pickup_`).
Held objects follow the controller grip and can be dropped or thrown by releasing the squeeze.
Teleport rays snap to the first walkable surface when `--vr-teleport-grounded=on`.
Too-steep or invalid targets are rejected with a brief haptic tick.

## Locomotion internals

All movement is routed through the original Gothic character controller via the `VRLocomotion` module. The left stick builds a
desired velocity which is limited by `--vr-walk-accel` (default `10 m/s²`) and `--vr-walk-maxspeed` (default `3 m/s`). The
`VRCharacter` adapter performs collision checks and passes the corrected motion to the game physics. Teleports are validated by
`VRNav` before warping the player. Turning supports both snap and smooth modes and is applied through the player controller so
non-VR behaviour is preserved.
