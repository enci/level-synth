#pragma once

#include <string>
#include <vector>

#include <level_synth/node_graph.hpp>

// ---------------------------------------------------------------------------
// snapshot_command — an undoable edit stored as before/after graph JSON
// ---------------------------------------------------------------------------

struct snapshot_command {
    std::string description;
    std::string before_json;
    std::string after_json;

    void undo(ls::node_graph& graph) { graph.load(before_json); }
    void redo(ls::node_graph& graph) { graph.load(after_json); }
};

// ---------------------------------------------------------------------------
// command_history
// ---------------------------------------------------------------------------

class command_history {
public:
    void push(snapshot_command cmd) {
        m_stack.erase(m_stack.begin() + m_pos, m_stack.end());
        m_stack.push_back(std::move(cmd));
        ++m_pos;
        if (m_saved_pos > m_pos) m_saved_pos = -1;
    }

    void undo(ls::node_graph& graph) {
        if (can_undo()) m_stack[--m_pos].undo(graph);
    }

    void redo(ls::node_graph& graph) {
        if (can_redo()) m_stack[m_pos++].redo(graph);
    }

    bool can_undo() const { return m_pos > 0; }
    bool can_redo() const { return m_pos < static_cast<int>(m_stack.size()); }

    void mark_saved() { m_saved_pos = m_pos; }

    void clear() {
        m_stack.clear();
        m_pos       = 0;
        m_saved_pos = 0;
    }

    bool is_modified() const { return m_pos != m_saved_pos; }

    // Read-only access for the history panel
    int size()      const { return static_cast<int>(m_stack.size()); }
    int pos()       const { return m_pos; }
    int saved_pos() const { return m_saved_pos; }
    const snapshot_command& entry(int i) const { return m_stack[i]; }

private:
    std::vector<snapshot_command> m_stack;
    int m_pos       = 0;
    int m_saved_pos = 0;
};
