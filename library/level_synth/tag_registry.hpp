#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "tag.hpp"

namespace ls {

// The registry owns the canonical list of declared symbolic tags.
// - Identifiers are dotted paths: "Level.Wall.Damaged" (1-3 segments).
// - Each segment hashes to a 21-bit ID; the three IDs form the tag's bits.
// - A second map detects hash collisions between distinct segment strings;
//   collisions are rejected at insertion time.
//
// Numeric tags are NOT registered. They carry their value directly in the
// bit pattern and are always considered valid.
//
// Parsing string -> tag is on-demand (no cached forward map). The reverse
// lookup tag -> string uses the m_tags map.
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

    // A tag is valid if:
    //   - it is numeric (any 63-bit value), or
    //   - it is symbolic and registered in this registry.
    // The all-zero tag (default-constructed) is NOT valid; it's a sentinel.
    bool valid(const tag& t) const;

private:
    static uint32_t hash_segment(std::string_view segment) noexcept;

    // Registered tags: raw tag bits -> canonical identifier string.
    std::unordered_map<uint64_t, std::string> m_tags;

    // Collision detection: segment hash -> canonical segment string.
    // Populated as segments are seen during add().
    std::unordered_map<uint32_t, std::string> m_segments;
};

}