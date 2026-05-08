#pragma once
#include <cassert>
#include <cstdint>

namespace ls {

enum class tag_type { symbolic, numeric };

// Tags can be symbolic (by hashing the levels.of.tags and
// numeric by providing a number.
// Bit layout (64 bits total):
//   Bit 63:      mode flag (0 = symbolic, 1 = numeric)
//   Bits 62-42:  L0 (21 bits) when symbolic
//   Bits 41-21:  L1 (21 bits) when symbolic
//   Bits 20-0:   L2 (21 bits) when symbolic
//   Bits 62-0:   signed 63-bit value when numeric
//
// Symbolic levels are segment hashes assigned by the tag_registry. A level
// value of 0 acts as a wildcard for matching. The all-zero tag (raw == 0)
// is a valid sentinel meaning "empty symbolic tag" / matches anything.

struct tag {
    static constexpr int      k_level_bits = 21;
    static constexpr uint64_t k_level_mask = (uint64_t(1) << k_level_bits) - 1;
    static constexpr uint64_t k_mode_bit   = uint64_t(1) << 63;

    constexpr tag() noexcept : m_raw(0) {}
    constexpr explicit tag(uint64_t raw) noexcept : m_raw(raw) {}

    static constexpr tag numeric(int64_t value) noexcept {
        // Truncate to 63 bits silently; set mode bit.
        return tag(k_mode_bit | (uint64_t(value) & ~k_mode_bit));
    }

    static constexpr tag symbolic(uint32_t l0, uint32_t l1, uint32_t l2) noexcept {
        assert((l0 & ~k_level_mask) == 0 && "L0 exceeds 21 bits");
        assert((l1 & ~k_level_mask) == 0 && "L1 exceeds 21 bits");
        assert((l2 & ~k_level_mask) == 0 && "L2 exceeds 21 bits");
        return tag((uint64_t(l0) << 42) | (uint64_t(l1) << 21) | uint64_t(l2));
    }

    constexpr tag_type type() const noexcept {
        return (m_raw & k_mode_bit) ? tag_type::numeric : tag_type::symbolic;
    }

    constexpr uint32_t l0() const noexcept { return uint32_t((m_raw >> 42) & k_level_mask); }
    constexpr uint32_t l1() const noexcept { return uint32_t((m_raw >> 21) & k_level_mask); }
    constexpr uint32_t l2() const noexcept { return uint32_t(m_raw & k_level_mask); }

    constexpr int64_t value() const noexcept {
        // Sign-extend from bit 62. Shift left by 1 puts the sign bit at the
        // top, then arithmetic right-shift sign-extends across all 64 bits.
        return int64_t(m_raw << 1) >> 1;
    }

    constexpr uint64_t raw() const noexcept { return m_raw; }

    // Match semantics:
    // - Both symbolic: bidirectional subset on bits, with 0 at any level
    //   acting as a wildcard. Wall matches Wall.Damaged, etc.
    // - Both numeric: exact equality on the value bits.
    // - Mixed mode: always false.
    //
    // Numerics use equality rather than the subset rule because for ordinary
    // integer values (e.g., 42 vs 43) bit-subset is almost never the intended
    // semantics. Use a range predicate elsewhere if you need it.
    friend constexpr bool match(const tag& a, const tag& b) noexcept {
        const uint64_t l = a.m_raw;
        const uint64_t r = b.m_raw;
        if ((l ^ r) & k_mode_bit) return false; // different modes
        if (l & k_mode_bit) return l == r;      // both numeric: exact
        const uint64_t and_ = l & r;            // both symbolic: subset
        return and_ == l || and_ == r;
    }

    friend constexpr bool operator==(const tag& a, const tag& b) noexcept {
        return a.m_raw == b.m_raw;
    }

private:
    uint64_t m_raw;
};

static_assert(sizeof(tag) == 8);

} // namespace ls