#pragma once
#include <assert.h>
#include <cstdint>

namespace ls {

enum class tag_type { numeric, symbolic };

struct tag {
    constexpr tag() : raw(0) {}
    constexpr explicit tag(int64_t raw) : raw(raw) {}

    static constexpr tag numeric(int32_t value) noexcept {
        tag t;
        t.fields.value = value;
        t.fields.category = 0;
        t.fields.subcategory = 0;
        return t;
    }

    static constexpr tag symbolic(int32_t value, uint16_t category, uint16_t subcategory) noexcept {
        tag t;
        assert(category != 0 || subcategory != 0); // enforce symbolic tags to have non-zero category or subcategory
        t.fields.category = category;
        t.fields.subcategory = subcategory;
        t.fields.value = value;
        return t;
    }

    tag_type type() const {
        return (fields.category == 0 && fields.subcategory == 0)
            ? tag_type::numeric : tag_type::symbolic;
    }

    friend bool match(const tag& left, const tag& right) noexcept {
        // Used for pattern matching (use bit mask)
        // Wall.Damaged should match Wall.* and Wall.Damaged.Left should match Wall.* and Wall.Damaged.*
        auto res = left.raw & right.raw;
        return res == right.raw || res == left.raw;
    }

    friend bool operator==(const tag& a, const tag& b) noexcept { return a.raw == b.raw; }
    friend bool operator!=(const tag& a, const tag& b) noexcept { return a.raw != b.raw; }

    union {
        int64_t raw;
        struct {
            int32_t  value;
            uint16_t subcategory;
            uint16_t category;
        } fields;
    };
};

}
