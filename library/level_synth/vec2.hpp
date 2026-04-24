#pragma once

namespace ls {

struct vec2 {
    float x = 0.0f;
    float y = 0.0f;

    bool operator==(const vec2&) const = default;
};

}
