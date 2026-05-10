#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "tag.hpp"

namespace ls {

class tag_registry {
public:
    // Add a symbolic tag identifier. Returns false if:
    //   - identifier is empty, has empty segments, or > 3 segments
    //   - a segment hash collides with a previously registered segment
    //     whose string differs
    //   - two segments within the new identifier collide with each other
    // Re-adding an existing identifier is a no-op and returns true.
    bool add(std::string_view identifier);

    // Remove a previously registered identifier. No-op if not present.
    void remove(std::string_view identifier);

    // Rename an existing identifier to a new one.
    // Migrates any stored color. Returns false if old_id is not found,
    // new_id already exists, or new_id fails structural validation.
    bool rename(std::string_view old_id, std::string_view new_id);

    // Parse an identifier into a tag by hashing its segments. Does NOT
    // require the identifier to be registered. Returns nullopt only on
    // structural errors (empty segments, too many segments).
    std::optional<tag> parse(std::string_view identifier) const;

    // Look up a registered identifier by its parsed tag. Returns nullopt
    // if no such symbolic tag is registered. (Does not handle numeric tags.)
    std::optional<tag> find(std::string_view identifier) const;

    // Reverse lookup: get the identifier string for a registered tag.
    // Returns empty view if the tag is numeric or not registered.
    std::string_view identifier(const tag& t) const;

    // Returns every registered identifier string, sorted alphabetically.
    std::vector<std::string> all_identifiers() const;

    // A tag is valid if:
    //   - it is numeric (any 63-bit value), or
    //   - it is symbolic and registered in this registry.
    // The all-zero tag (default-constructed) is NOT valid; it's a sentinel.
    bool valid(const tag& t) const;

    // Per-tag display color, stored as ImU32 (packed ABGR, same as IM_COL32).
    // Purely display metadata — does not affect tag matching.
    // Missing color = inherit from parent level (resolved by the UI layer).
    void                    set_color(std::string_view identifier, uint32_t color);
    std::optional<uint32_t> get_color(std::string_view identifier) const;
    void                    clear_color(std::string_view identifier);

private:
    static uint32_t hash_segment(std::string_view segment) noexcept;

    // Registered tags: raw tag bits -> canonical identifier string.
    std::unordered_map<uint64_t, std::string> m_tags;

    // Collision detection: segment hash -> canonical segment string.
    std::unordered_map<uint32_t, std::string> m_segments;

    // Display colors: identifier string -> packed ABGR color.
    std::unordered_map<std::string, uint32_t> m_colors;
};

} // namespace ls