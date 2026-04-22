#pragma once

#include <unordered_map>
#include <string>

#include "grid.hpp"

namespace ls {

class layered_grid {

public:
    layered_grid() : m_width(0), m_height(0) {}
    layered_grid(int width, int height);
    grid<int>& operator[] (std::string_view name);
    const grid<int>& operator[] (std::string_view name) const;
    const bool has_layer(std::string_view name) const;
    void create_layer(std::string_view name, int fill);
    int get(std::string_view layer, int x, int y) const; // TODO: use multidimensional subscript operator once supported
    void set(std::string_view layer, int x, int y, int value); // TODO: use multidimensional subscript operator once supported
    int width() const;
    int height() const;

    // bool has(std::string_view name) const;
    // auto names() const;

private:
    int m_width, m_height;
    std::unordered_map<std::string, grid<int>> m_layers;
};

}