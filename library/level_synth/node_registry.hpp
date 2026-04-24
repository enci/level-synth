// node_registry.hpp
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace ls
{

class node;

template <typename T> struct node_type_info;   // specialised by macro

struct node_registration {
    std::string type_name;
    std::string display_name;
    std::string category;
    std::function<std::unique_ptr<node>()> factory;
    std::type_index type_idx;
};

class node_registry {
public:
    /// Gets the instance (Meyer's singleton)
    static node_registry& instance();

    /// Register a new node
    void register_node(node_registration&& entry);

    /// Create a new node from its type name
    std::unique_ptr<node> create(const std::string& type_name) const;

    /// Get a node registration for a given node
    const node_registration* find(const node& n) const;

    /// A way to iterate over all the entries
    const std::unordered_map<std::string, node_registration>& entries() const {
        return m_entries;
    }

private:
    std::unordered_map<std::string, node_registration> m_entries;
};


}


#define LS_REGISTER_NODE(TypeName, DisplayName, Category)            \
template <> struct node_type_info<TypeName> {                        \
    static constexpr std::string_view name = #TypeName;              \
};                                                                   \
static bool _reg_##TypeName = [] {                                   \
    node_registry::instance().register_node(                         \
        node_registration{                                           \
            std::string(node_type_info<TypeName>::name),             \
            DisplayName,                                             \
            Category,                                                \
            [] { return std::make_unique<TypeName>(); },             \
            std::type_index(typeid(TypeName))                        \
        });                                                          \
    return true;                                                     \
}()
