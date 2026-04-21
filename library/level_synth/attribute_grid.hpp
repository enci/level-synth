#pragma once

#include <string>
#include <vector>
#include <span>
#include <stdexcept>
#include <unordered_map>

#include "grid.hpp"

namespace ls {

/*
class attribute_grid {
public:
    attribute_grid() = default;
    attribute_grid(int width, int height);

    int width() const { return m_width; }
    int height() const { return m_height; }
    bool in_bounds(int x, int y) const { return x >= 0 && x < m_width && y >= 0 && y < m_height; }

    void add_attribute(const std::string& name, int default_val = 0);
    bool has_attribute(const std::string& name) const;

    int get(const std::string& name, int x, int y) const;
    void set(const std::string& name, int x, int y, int value);

    std::vector<std::string> attribute_names() const;

    std::span<int> data(const std::string& name);
    std::span<const int> data(const std::string& name) const;

private:
    struct attribute {
        std::string name;
        int default_value = 0;
        std::vector<int> data;
    };

    int m_width = 0;
    int m_height = 0;
    std::vector<attribute> m_attrs;

    attribute* find_attr(const std::string& name);
    const attribute* find_attr(const std::string& name) const;
};
*/


    class attributed_grid
    {
    public:
        attributed_grid(int width, int height);

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