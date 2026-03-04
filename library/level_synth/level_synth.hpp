#pragma once
#include <string>
#include <vector>

namespace ls
{

    class grid {
    public:
        grid(int width, int height)
            : m_width(width), m_height(height), m_data(width * height) {}

        ~grid() = default;

        grid(const grid&) = default;
        grid& operator=(const grid&) = default;

        int get(int x, int y) const { return m_data[y * m_width + x]; }
        void set(int x, int y, int value) { m_data[y * m_width + x] = value; }

        bool in_bounds(int x, int y) const { return x >= 0 && x < m_width && y >= 0 && y < m_height; }

        int& operator()(int x, int y) { return m_data[y * m_width + x]; }
        int operator()(int x, int y) const { return m_data[y * m_width + x]; }

        int width() const { return m_width; }
        int height() const { return m_height; }

    private:
        int m_width;
        int m_height;
        std::vector<int> m_data;
    };

    class transformation
    {
    public:



    };

    enum class pin_type
    {
        integer,
        real,
        grid
    };

    enum class node_type
    {
        input,
        output,
        process
    };

    class pin
    {

    };

    class node
    {
    public:
        node_type type;
        std::string name;

    };

    class level_synth
    {
    public:
        level_synth(const std::string& file);
        level_synth() = default;
        level_synth(const level_synth&) = delete;
        level_synth& operator=(const level_synth&) = delete;
        level_synth(level_synth&&) = delete;
        ~level_synth() = default;

        void set_parameter(const std::string& name, int value);
        void set_parameter(const std::string& name, float value);

        grid get_grid_output(const std::string& name) const;


    };

}
