#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ls {

class node;
struct node_descriptor;

class node_registry {
public:
    using factory_fn = std::function<std::unique_ptr<node>()>;
    void register_node(const std::string& type_name, factory_fn factory);
    std::unique_ptr<node> create(const std::string& type_name) const;
    std::vector<std::string> registered_types() const;
    const node_descriptor& descriptor(const std::string& type_name) const;
    static node_registry& instance();

private:
    struct entry {
        factory_fn factory;
        mutable std::unique_ptr<node> prototype;
    };

    std::unordered_map<std::string, entry> m_entries;
};

#define LS_REGISTER_NODE(TypeName)                                       \
    static bool _reg_##TypeName = [] {                                   \
        ls::node_registry::instance().register_node(                     \
            #TypeName, [] { return std::make_unique<TypeName>(); });     \
        return true;                                                     \
    }()

} // namespace ls