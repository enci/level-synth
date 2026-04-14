#include "attribute_grid.hpp"

namespace ls {

attribute_grid::attribute_grid(int width, int height)
    : m_width(width), m_height(height) {}

void attribute_grid::add_attribute(const std::string& name, int default_val) {
    if (has_attribute(name)) return;
    auto& attr = m_attrs.emplace_back();
    attr.name = name;
    attr.default_value = default_val;
    attr.data.assign(m_width * m_height, default_val);
}

bool attribute_grid::has_attribute(const std::string& name) const {
    return find_attr(name) != nullptr;
}

int attribute_grid::get(const std::string& name, int x, int y) const {
    auto* attr = find_attr(name);
    if (!attr) throw std::runtime_error("Unknown attribute: " + name);
    return attr->data[y * m_width + x];
}

void attribute_grid::set(const std::string& name, int x, int y, int value) {
    auto* attr = find_attr(name);
    if (!attr) throw std::runtime_error("Unknown attribute: " + name);
    attr->data[y * m_width + x] = value;
}

std::vector<std::string> attribute_grid::attribute_names() const {
    std::vector<std::string> names;
    names.reserve(m_attrs.size());
    for (const auto& a : m_attrs)
        names.push_back(a.name);
    return names;
}

std::span<int> attribute_grid::data(const std::string& name) {
    auto* attr = find_attr(name);
    if (!attr) throw std::runtime_error("Unknown attribute: " + name);
    return attr->data;
}

std::span<const int> attribute_grid::data(const std::string& name) const {
    auto* attr = find_attr(name);
    if (!attr) throw std::runtime_error("Unknown attribute: " + name);
    return attr->data;
}

attribute_grid::attribute* attribute_grid::find_attr(const std::string& name) {
    for (auto& a : m_attrs)
        if (a.name == name) return &a;
    return nullptr;
}

const attribute_grid::attribute* attribute_grid::find_attr(const std::string& name) const {
    for (auto& a : m_attrs)
        if (a.name == name) return &a;
    return nullptr;
}

} // namespace ls