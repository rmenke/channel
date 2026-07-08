#ifndef _raster_hpp_
#define _raster_hpp_

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <ostream>
#include <span>

class raster {
  public:
    using pixel = std::array<std::uint8_t, 3>;
    using point = std::array<std::size_t, 2>;

    static constexpr pixel BLACK = {0, 0, 0};
    static constexpr pixel WHITE = {255, 255, 255};

    static constexpr pixel RED = {255, 0, 0};
    static constexpr pixel GREEN = {0, 255, 0};
    static constexpr pixel BLUE = {0, 0, 255};

    static constexpr pixel CYAN = {0, 255, 255};
    static constexpr pixel MAGENTA = {255, 0, 255};
    static constexpr pixel YELLOW = {255, 255, 0};

  private:
    std::unique_ptr<pixel[]> _data;
    std::size_t _width, _height;

  public:
    raster(std::size_t width, std::size_t height)
        : _data(std::make_unique_for_overwrite<pixel[]>(width * height))
        , _width(width)
        , _height(height) {
        auto begin = _data.get();
        auto end = _data.get() + width * height;
        std::ranges::uninitialized_fill(begin, end, WHITE);
    }

    auto operator[](std::size_t row) const {
        return std::span(_data.get() + row * _width, _width);
    }

    auto operator[](std::size_t row) {
        return std::span(_data.get() + row * _width, _width);
    }

    auto width() const {
        return _width;
    }
    auto height() const {
        return _height;
    }
    auto size() const {
        return _width * _height;
    }

    friend std::ostream &operator<<(std::ostream &os, const raster &r) {
        os << "P3\n";
        os << r.width() << ' ' << r.height() << '\n';
        os << static_cast<int>(
                  std::numeric_limits<pixel::value_type>::max()
              )
           << '\n';

        auto b = r._data.get();
        auto e = b + r.size();

        while (b != e) {
            auto [red, green, blue] = *b;

            os << static_cast<int>(red) << ' ';
            os << static_cast<int>(green) << ' ';
            os << static_cast<int>(blue) << ' ';

            ++b;
        }

        return os << std::endl;
    }

    void save(std::filesystem::path path) {
        std::ofstream{path} << *this;
    }
};

#endif
