#ifndef _raster_hpp_
#define _raster_hpp_

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mdspan>
#include <memory>
#include <ostream>
#include <utility>

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
    using extent_t = std::dextents<std::size_t, 2>;

    std::unique_ptr<pixel[]> _data;
    std::mdspan<pixel, extent_t, std::layout_left> _mdspan;

  public:
    raster(std::size_t width, std::size_t height)
        : _data(std::make_unique_for_overwrite<pixel[]>(width * height))
        , _mdspan(_data.get(), width, height) {
        auto begin = _data.get();
        auto end = _data.get() + width * height;
        std::ranges::uninitialized_fill(begin, end, WHITE);
    }

    template <typename... Args>
    decltype(auto) operator[](Args &&...args) {
        return _mdspan[std::forward<Args>(args)...];
    }

    auto width() const {
        return _mdspan.extent(0);
    }
    auto height() const {
        return _mdspan.extent(0);
    }
    auto size() const {
        return _mdspan.size();
    }

    friend std::ostream &operator<<(std::ostream &os, const raster &r) {
        os << "P3\n";
        os << r.width() << ' ' << r.height() << '\n';
        os << static_cast<int>(std::numeric_limits<pixel::value_type>::max()) << '\n';

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
