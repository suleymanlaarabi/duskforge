#pragma once

#include <cstdint>

namespace dusklight {

struct Transform {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    int16_t pitch = 0;
    int16_t yaw = 0;
    int16_t roll = 0;
};

struct Velocity {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Actor {
    uint32_t id = 0;
    int16_t profile = 0;
    int8_t room = 0;
    uint8_t group = 0;
    uint32_t params = 0;
};

// Unstable dev-only escape hatch. Do not persist or use after ActorDestroyed.
struct NativeActor {
    uint32_t id = 0;
    void* ptr = nullptr;
};

struct Player {};
struct Enemy {};
struct Npc {};
struct Item {};
struct Scene {};
struct Room {};

}  // namespace dusklight

