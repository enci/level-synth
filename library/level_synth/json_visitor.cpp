#include "json_visitor.hpp"

#include <string>

namespace ls {

void json_writer::visit(std::string_view name, double& v) {
    m_out[std::string(name)] = v;
}

void json_writer::visit(std::string_view name, int& v) {
    m_out[std::string(name)] = v;
}

void json_writer::visit(std::string_view name, vec2& v) {
    m_out[std::string(name)] = { {"x", v.x}, {"y", v.y} };
}

void json_writer::visit(std::string_view name, std::string& v) {
    m_out[std::string(name)] = v;
}

/*
void json_writer::visit(std::string_view name, bool& v) {
    m_out[std::string(name)] = v;
}
*/


namespace {

template <typename T>
void read_into(const nlohmann::json& src, std::string_view name, T& dst) {
    auto it = src.find(std::string(name));
    if (it == src.end()) return;           // missing key — keep current value
    if (!it->is_null()) dst = it->get<T>();
}

} // anonymous namespace

void json_reader::visit(std::string_view name, double& v) {
    read_into(m_in, name, v);
}

void json_reader::visit(std::string_view name, int& v) {
    read_into(m_in, name, v);
}

void json_reader::visit(std::string_view name, vec2& v) {
    auto it = m_in.find(std::string(name));
    if (it == m_in.end() || it->is_null()) return;
    v.x = it->value("x", v.x);
    v.y = it->value("y", v.y);
}

void json_reader::visit(std::string_view name, std::string& v) {
    read_into(m_in, name, v);
}

/*
void json_reader::visit(std::string_view name, bool& v) {
    read_into(m_in, name, v);
}
*/

}