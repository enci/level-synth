#include "tag_registry.hpp"

#include <ranges>

using namespace std;

namespace ls {
namespace detail {

// Split "Terrain.Wall.Damaged" into segments, up to max 3.
// Returns a small struct describing what was found.
struct parsed_path
{
    std::string_view segments[3];
    int              depth;          // 1, 2, or 3
    bool             too_deep;       // path had more than 3 segments
    bool             empty_segment;  // path had ".." or started/ended with "."
};

parsed_path parse_path(std::string_view path)
{
    parsed_path result {};

    size_t pos = 0;
    int    seg_count = 0;

    while (pos <= path.size()) {
        size_t dot = path.find('.', pos);
        if (dot == std::string_view::npos)
            dot = path.size();

        std::string_view segment = path.substr(pos, dot - pos);

        if (segment.empty()) {
            result.empty_segment = true;
            return result;
        }

        if (seg_count >= 3) {
            result.too_deep = true;
            return result;
        }

        result.segments[seg_count++] = segment;

        if (dot == path.size())
            break;
        pos = dot + 1;
    }

    result.depth = seg_count;
    return result;
}

}

std::optional<tag> tag_registry::find(const std::string& identifier) const {

    /*auto it = m_tags.find(identifier);
    if (it != m_tags.end())
        return it->second;
    return std::nullopt;
    */
}

std::string_view tag_registry::identifier(const tag &t) const {
    return {};
}

void tag_registry::add(std::string_view identifier) {
    auto t = parse(identifier);
    if(t) m_tags[t.value().raw] = identifier;
}

void tag_registry::remove(std::string_view identifier) {
    auto t = parse(identifier);
    if(t) m_tags.erase(t.value().raw);
}

std::optional<tag> tag_registry::parse(std::string_view identifier) const {
    vector<string_view> segments;
    for (auto seg : views::split(identifier, '.'))
        segments.push_back(string_view(&*seg.begin(), distance(seg.begin(), seg.end())));

    if (segments.size() == 1)
        return tag::symbolic(1,0, 0);
    if (segments.size() == 2)
        return  tag::symbolic(1, 1, 0);
    if (segments.size() == 3)
        return tag::symbolic(1, 1, 1);
    return std::nullopt;
}

}
