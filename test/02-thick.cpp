#include "tap.hpp"

#include "channel.hpp"
#include "raster.hpp"

#include <filesystem>
#include <numbers>
#include <ranges>

using namespace tap;

namespace std {

void sprint_tuple(ostream &, auto &&, std::index_sequence<>) {}

template <std::size_t Ix, std::size_t... Ir>
void sprint_tuple(
    ostream &os, auto &&tuple, std::index_sequence<Ix, Ir...>
) {
    if constexpr (Ix > 0) tap::sprint(os, ", ");
    tap::sprint(os, get<Ix>(tuple));
    sprint_tuple(os, tuple, std::index_sequence<Ir...>());
}

template <typename F, typename S>
void sprint_one(ostream &os, std::pair<F, S> const &p) {
    tap::sprint(os, "(");
    sprint_tuple(os, p, std::make_index_sequence<2>());
    tap::sprint(os, ")");
}

template <typename T>
void sprint_one(ostream &os, std::vector<T> const &v) {
    tap::sprint(os, "[");

    auto b = v.begin();
    auto e = v.end();

    if (b != e) {
        tap::sprint(os, *b);
        while (++b != e) tap::sprint(os, ", ", *b);
    }

    tap::sprint(os, "]");
}

} // namespace std

int main(int argc, char **argv) {
    if (argc < 1) throw std::runtime_error{"launch"};
    auto progname =
        std::filesystem::weakly_canonical(argv[0]).replace_extension();

    test_plan plan;

    try {
        using namespace channel::views;

        static constexpr int lines = 24;

        raster pixmap{320, 320};

        auto output = progname;

        auto semiturns = std::ranges::views::iota(0, lines) |
                         std::views::transform([](int angle) {
                             return static_cast<double>(2 * angle) / lines;
                         });

        std::size_t overlaps = 0;

        raster::pixel colors[] = {
            raster::RED, raster::YELLOW, raster::GREEN, raster::CYAN, raster::BLUE, raster::MAGENTA
        };

        for (auto &&semiturn : semiturns) {
            double const s = sin(std::numbers::pi * semiturn);
            double const c = cos(std::numbers::pi * semiturn);

            std::pair<int, int> p0, p1;

            auto &&[x0, y0] = p0;
            auto &&[x1, y1] = p1;

            x0 = (int)std::lround(50.0 * c + 160.0);
            y0 = (int)std::lround(50.0 * s + 160.0);
            x1 = (int)std::lround(150.0 * c + 160.0);
            y1 = (int)std::lround(150.0 * s + 160.0);

            for (auto &&pts : line(p0, p1, 5)) {
                std::size_t cindex = 0;
                for (auto &&[x, y] : pts) {
                    if (pixmap[y][x] != raster::WHITE) ++overlaps;
                    pixmap[y][x] = colors[cindex];
                    cindex = (cindex + 1) % std::ranges::size(colors);
                }

                auto middle = std::ranges::distance(pts) / 2;
                auto &&[x, y] = pts[middle];
                pixmap[y][x] = raster::BLACK;
            }
        }

        eq(0, overlaps, "no overlaps");

        pixmap.save(output.replace_extension("ppm"));
        diag(output);
    }
    catch (...) {
        bail_out(std::current_exception());
    }
}
