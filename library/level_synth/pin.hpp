#pragma once

#include <memory>
#include <string>
#include <variant>

namespace ls {

class grid;

enum class pin_type {
    number,
    grid
};

enum class pin_direction {
    input,
    output
};

struct pin_descriptor {
    std::string name;
    pin_direction direction;
    pin_type type;
    bool required = true;
};

using pin_value = std::variant<
    double,
    std::shared_ptr<grid>
>;

} // namespace ls