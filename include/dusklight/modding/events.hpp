#pragma once

#include <cstdint>

namespace dusklight::events {

struct Frame {
    float delta_seconds = 0.0f;
    uint64_t frame_index = 0;
};

struct ActorCreated {
    uint32_t id = 0;
};

struct ActorDestroyed {
    uint32_t id = 0;
};

}  // namespace dusklight::events

