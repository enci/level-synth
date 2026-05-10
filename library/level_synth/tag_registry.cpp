#include "tag_registry.hpp"

#include <algorithm>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace ls {

namespace {

std::vector<std::string> split_segments(std::string_view identifier) {
    std::vector<std::string> out;
    for (auto seg : std::views::split(identifier, '.')) {
        if (seg.empty()) return {};
        out.emplace_back(seg.begin(), seg.end());
    }
    if (out.empty() || out.size() > 3) return {};
    return out;
}

} // anonymous namespace

uint32_t tag_registry::hash_segment(std::string_view segment) noexcept {
    uint64_t h = 0xcbf29ce484222325ull;
    for (char c : segment) {
        h ^= static_cast<uint8_t>(c);
        h *= 0x100000001b3ull;
    }
    uint32_t r = static_cast<uint32_t>(h & tag::k_level_mask);
    return r == 0 ? 1u : r;
}

bool tag_registry::add(std::string_view identifier) {
    auto segs = split_segments(identifier);
    if (segs.empty()) return false;

    uint32_t ids[3] = {0, 0, 0};
    for (size_t i = 0; i < segs.size(); i++) {
        ids[i] = hash_segment(segs[i]);
        auto it = m_segments.find(ids[i]);
        if (it != m_segments.end() && it->second != segs[i])
            return false;
        for (size_t j = 0; j < i; j++)
            if (ids[j] == ids[i] && segs[j] != segs[i])
                return false;
    }

    for (size_t i = 0; i < segs.size(); i++)
        m_segments.try_emplace(ids[i], segs[i]);

    // Register every ancestor prefix so "Geometry.Wall.Damaged" also
    // ensures "Geometry" and "Geometry.Wall" exist as real tags.
    for (size_t depth = 1; depth <= segs.size(); ++depth) {
        uint32_t a[3] = {0, 0, 0};
        for (size_t i = 0; i < depth; ++i) a[i] = ids[i];
        const tag ancestor = tag::symbolic(a[0], a[1], a[2]);
        if (m_tags.find(ancestor.raw()) == m_tags.end()) {
            std::string prefix = segs[0];
            for (size_t i = 1; i < depth; ++i) prefix += '.' + segs[i];
            m_tags[ancestor.raw()] = std::move(prefix);
        }
    }
    return true;
}

void tag_registry::remove(std::string_view identifier) {
    auto t = parse(identifier);
    if (t) {
        m_tags.erase(t->raw());
        m_colors.erase(std::string(identifier));
    }
}

bool tag_registry::rename(std::string_view old_id, std::string_view new_id) {
    std::string old_str(old_id);
    std::string new_str(new_id);

    if (!find(old_id))         return false;  // old not registered
    if (find(new_id))          return false;  // new already exists
    if (!add(new_str))         return false;  // new fails validation

    // Migrate color before removing old entry.
    auto it = m_colors.find(old_str);
    if (it != m_colors.end()) {
        m_colors[new_str] = it->second;
        m_colors.erase(it);
    }

    remove(old_str);
    return true;
}

std::optional<tag> tag_registry::parse(std::string_view identifier) const {
    auto segs = split_segments(identifier);
    if (segs.empty()) return std::nullopt;
    uint32_t ids[3] = {0, 0, 0};
    for (size_t i = 0; i < segs.size(); i++)
        ids[i] = hash_segment(segs[i]);
    return tag::symbolic(ids[0], ids[1], ids[2]);
}

std::optional<tag> tag_registry::find(std::string_view identifier) const {
    auto t = parse(identifier);
    if (!t) return std::nullopt;
    if (m_tags.find(t->raw()) == m_tags.end()) return std::nullopt;
    return t;
}

std::string_view tag_registry::identifier(const tag& t) const {
    if (t.type() != tag_type::symbolic) return {};
    auto it = m_tags.find(t.raw());
    if (it == m_tags.end()) return {};
    return it->second;
}

std::vector<std::string> tag_registry::all_identifiers() const {
    std::vector<std::string> result;
    result.reserve(m_tags.size());
    for (const auto& [_, id_str] : m_tags)
        result.push_back(id_str);
    std::sort(result.begin(), result.end());
    return result;
}

bool tag_registry::valid(const tag& t) const {
    if (t.type() == tag_type::numeric) return true;
    if (t.raw() == 0) return false;
    return m_tags.find(t.raw()) != m_tags.end();
}

void tag_registry::set_color(std::string_view identifier, uint32_t color) {
    // Only store colors for registered tags.
    if (find(identifier))
        m_colors[std::string(identifier)] = color;
}

std::optional<uint32_t> tag_registry::get_color(std::string_view identifier) const {
    auto it = m_colors.find(std::string(identifier));
    if (it != m_colors.end()) return it->second;
    return std::nullopt;
}

void tag_registry::clear_color(std::string_view identifier) {
    m_colors.erase(std::string(identifier));
}

}