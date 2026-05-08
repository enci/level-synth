#include "tag_registry.hpp"

#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace ls {

namespace {

// Split an identifier on '.' into 1-3 non-empty segments.
// Returns an empty vector on any structural error (empty input, empty
// segment, more than 3 segments).
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
    // FNV-1a, 64-bit, masked to 21 bits.
    uint64_t h = 0xcbf29ce484222325ull;
    for (char c : segment) {
        h ^= static_cast<uint8_t>(c);
        h *= 0x100000001b3ull;
    }
    // Reserve 0 as "wildcard / unused level"; remap a hash of 0 to something
    // non-zero. With 21 bits and any reasonable input this is exceedingly rare.
    uint32_t r = static_cast<uint32_t>(h & tag::k_level_mask);
    return r == 0 ? 1u : r;
}

bool tag_registry::add(std::string_view identifier) {
    auto segs = split_segments(identifier);
    if (segs.empty()) return false;

    // Pass 1: hash all segments and check for collisions.
    uint32_t ids[3] = {0, 0, 0};
    for (size_t i = 0; i < segs.size(); i++) {
        ids[i] = hash_segment(segs[i]);

        // Collision against an existing entry?
        auto it = m_segments.find(ids[i]);
        if (it != m_segments.end() && it->second != segs[i])
            return false;

        // Collision against an earlier segment within this identifier?
        for (size_t j = 0; j < i; j++)
            if (ids[j] == ids[i] && segs[j] != segs[i])
                return false;
    }

    // Pass 2: commit. try_emplace is a no-op when the key exists with the
    // same string; it can never overwrite because pass 1 already verified
    // any existing entry has a matching string.
    for (size_t i = 0; i < segs.size(); i++)
        m_segments.try_emplace(ids[i], segs[i]);

    const tag t = tag::symbolic(ids[0], ids[1], ids[2]);
    m_tags[t.raw()] = std::string(identifier);
    return true;
}

void tag_registry::remove(std::string_view identifier) {
    auto t = parse(identifier);
    if (t) m_tags.erase(t->raw());
    // We do NOT remove segments from m_segments. A segment string may still
    // be in use by other tags, and even if not, keeping the hash->string
    // mapping is harmless and preserves collision detection consistency.
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

bool tag_registry::valid(const tag& t) const {
    if (t.type() == tag_type::numeric) return true;
    if (t.raw() == 0) return false; // sentinel/wildcard, not a real tag
    return m_tags.find(t.raw()) != m_tags.end();
}

}