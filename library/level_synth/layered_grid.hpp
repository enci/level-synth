#pragma once

#include <unordered_map>
#include <string>

#include "grid.hpp"

namespace ls {

class layered_grid {

public:
    layered_grid(int width, int height);
    grid<int>& attr(std::string_view name);          // creates if missing
    const grid<int>& attr(std::string_view name) const;
    bool has(std::string_view name) const;
    auto names() const;
    int width() const;
    int height() const;

private:
    int m_width, m_height;
    std::unordered_map<std::string, grid<int>> m_attrs;
};

}