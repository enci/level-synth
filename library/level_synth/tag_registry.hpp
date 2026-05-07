#pragma once
#include <optional>
#include <string>
#include <unordered_map>

#include "tag.hpp"

namespace ls {

class tag_registry {
public:
    std::optional<tag> find(const std::string& identifier) const;
    std::string_view identifier(const tag& t) const;
    void add(std::string_view identifier);
    void remove(std::string_view identifier);
    std::optional<tag> parse(std::string_view identifier) const;

private:
    std::unordered_map<uint64_t, std::string> m_tags;
};

}
