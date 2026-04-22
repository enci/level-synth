#pragma once

namespace ls {

template<std::integral T>
class grid {
public:
    grid(int width, int height)
        : m_width(width), m_height(height), m_data(width * height) {}

    grid(int width, int height, T fill_value)
        : m_width(width), m_height(height), m_data(width * height, fill_value) {}

    ~grid() = default;

    grid(const grid&) = default;
    grid& operator=(const grid&) = default;

    grid(grid&&) = default;
    grid& operator=(grid&&) = default;

    T get(int x, int y) const { return m_data[y * m_width + x]; }
    void set(int x, int y, T value) { m_data[y * m_width + x] = value; }

    bool in_bounds(int x, int y) const { return x >= 0 && x < m_width && y >= 0 && y < m_height; }

    T& operator()(int x, int y) { return m_data[y * m_width + x]; }
    T operator()(int x, int y) const { return m_data[y * m_width + x]; }

    void fill(T value) { std::fill(m_data.begin(), m_data.end(), value); }

    int width() const { return m_width; }
    int height() const { return m_height; }

private:
    int m_width = -1;
    int m_height = -1;
    std::vector<T> m_data;
};

}


