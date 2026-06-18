# Dusklight Modding SDK

## 1. Mental Model

The SDK exposes Dusklight as a native C++ modding runtime built around Flecs.

```txt
Dusklight engine/runtime
        |
        v
Shared flecs::world
        |
        v
Native C++ mods
```

Dusklight owns the game process, creates the Flecs world, mirrors selected game state into Flecs entities/components, emits events, and applies allowed component changes back to the game.

Mods do not own the world. A mod receives `flecs::world&`, then registers its own components, systems, observers, and optional UI/config. Flecs is part of the public API: mod code should use `flecs::world`, `flecs::entity`, systems, queries, and observers directly.

Target user code:

```cpp
#include <flecs.h>
#include <dusklight/sdk.hpp>

struct DoubleEnemyHealth {};

DUSK_MOD_INIT(world) {
    world.component<DoubleEnemyHealth>();

    world.observer<dusk::Enemy, dusk::Health>()
        .event(flecs::OnAdd)
        .each([](flecs::entity enemy, dusk::Health& health) {
            health.max *= 2;
            health.current = health.max;
        });
}
```

## 2. SDK Scope

The first SDK version should be small, stable, and useful. It should provide:

- a native mod loader;
- a shared `flecs::world`;
- public Dusklight components and tags;
- public Dusklight events;
- logging helpers;
- a per-mod metadata format;
- examples that compile outside the main game tree.

Initial public components/tags should focus on common gameplay work:

```cpp
dusk::Transform
dusk::Velocity
dusk::Actor
dusk::Health
dusk::Player
dusk::Enemy
dusk::Npc
dusk::Item
dusk::Scene
dusk::Room
```

Initial events:

```cpp
dusk::events::Frame
dusk::events::SceneLoaded
dusk::events::RoomLoaded
dusk::events::ActorSpawned
dusk::events::ActorDestroyed
dusk::events::SaveLoaded
```

Non-goals for the first version:

- no scripting language;
- no sandboxing for native mods;
- no full replacement for internal game classes;
- no guarantee that advanced native pointers remain stable.

## 3. ABI And Entrypoint

Mods are native dynamic libraries: `.dll`, `.so`, or `.dylib`. The loader should use a small C ABI for discovery and initialization, while the user-facing code remains C++ and Flecs-based.

Required exported entrypoint:

```cpp
extern "C" DUSK_MOD_EXPORT void dusk_mod_init(flecs::world* world);
```

Recommended user-facing macro:

```cpp
#define DUSK_MOD_INIT(world_name) \
    extern "C" DUSK_MOD_EXPORT void dusk_mod_init(flecs::world* world_name)
```

This keeps loading simple and avoids C++ symbol name issues. The SDK and examples should document that mods must be built against the same Dusklight SDK/Flecs version declared in their metadata.

Future optional exports may include:

```cpp
extern "C" DUSK_MOD_EXPORT void dusk_mod_shutdown();
extern "C" DUSK_MOD_EXPORT const dusk_mod_api* dusk_mod_get_api();
```

## 4. Mod Package Format

Each mod is a directory under `mods/`:

```txt
mods/
  double_enemy_health/
    mod.json
    double_enemy_health.dll
    assets/
    config.json
```

On Linux/macOS the library extension changes to `.so` or `.dylib`.

Minimal `mod.json`:

```json
{
  "id": "double_enemy_health",
  "name": "Double Enemy Health",
  "version": "0.1.0",
  "sdk_version": "0.1.0",
  "library": "double_enemy_health",
  "author": "Mod Author",
  "description": "Doubles enemy health when enemies spawn."
}
```

Field rules:

- `id`: stable lowercase identifier, using `a-z`, `0-9`, and `_`.
- `name`: display name.
- `version`: mod version, preferably SemVer.
- `sdk_version`: SDK version the mod was built for.
- `library`: dynamic library basename without extension.
- `assets`: optional directory for mod-owned files.
- `config.json`: optional user-editable config owned by the mod.

The loader should reject missing `id`, `version`, `sdk_version`, or `library`, and should skip mods built for incompatible SDK versions.
