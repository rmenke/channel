#include "tap.hpp"

#include "channel.hpp"
#include "raster.hpp"

#include <math.h>

#include <filesystem>
#include <iterator>
#include <numbers>
#include <random>
#include <ranges>
#include <stdexcept>

using namespace tap;

namespace std {

void sprint_tuple(ostream &, auto const &, index_sequence<>) {}

template <std::size_t Ix, std::size_t... Ir>
void sprint_tuple(ostream &os, auto const &t, index_sequence<Ix, Ir...>) {
    if constexpr (Ix > 0) tap::sprint(os, ", ");
    tap::sprint(os, get<Ix>(t));
    sprint_tuple(os, t, index_sequence<Ir...>());
}

template <typename T>
    requires(std::tuple_size<T>::value >= 0)
void sprint_one(ostream &os, T const &t) {
    os << '[';
    sprint_tuple(os, t, std::make_index_sequence<std::tuple_size_v<T>>{});
    os << ']';
}

} // namespace std

void test_random(std::filesystem::path output, auto &&urbg) {
    using namespace channel::ranges;

    output.concat("-random");

    raster pixmap{240, 240};

    raster::pixel colors[] = {{255, 0, 0},   {255, 255, 0}, {0, 255, 0},
                              {0, 255, 255}, {0, 0, 255},   {255, 0, 255}};

    for (auto i = 0; i < 5; ++i) {
        for (auto &&color : colors) {
            std::pair<int, int> p0, p1, delta;

            std::uniform_int_distribution<int> dist(20, 220);

            while (get<0>(p0) == get<0>(p1) && get<1>(p0) == get<1>(p1)) {
                p0 = {dist(urbg), dist(urbg)};
                p1 = {dist(urbg), dist(urbg)};
            }

            get<0>(delta) = std::abs(get<0>(p1) - get<0>(p0));
            get<1>(delta) = std::abs(get<1>(p1) - get<1>(p0));

            thin_line_view c(p0, p1);

            auto expected_count =
                std::max(get<0>(delta), get<1>(delta));

            // ensure that the point set is not infinite, or the distance()
            // operator won't halt.
            auto points = c | std::views::take(2 * expected_count);

            eq(expected_count, std::ranges::distance(points), "expected distance");

            auto b = c.begin(), e = c.end();

            eq(*b, p0, "first");
            eq(*e, p1, "last");

            bool monotonic = true;

            for (auto p = b++; b != e; p = b++) {
                auto &&[x, y] = *b;
                auto &&[px, py] = *p;

                auto dx = x - px, dy = y - py;

                monotonic = monotonic && (-1 <= dx && dx <= 1); // 0 or ±1
                monotonic = monotonic && (-1 <= dy && dy <= 1); // 0 or ±1
                monotonic =
                    monotonic && (dx != 0 || dy != 0); // not both zero
            }

            ok(monotonic, "monotonic");

            for (auto [x, y] : c) {
                pixmap[y][x] = color;
            }
        }
    }

    pixmap.save(output.replace_extension("ppm"));
    diag(output);
}

void test_radial(std::filesystem::path output) {
    using namespace channel::views;

    output.concat("-radial");

    static constexpr int lines = 24;

    raster pixmap{240, 240};

    auto endpoints =
        std::views::iota(0, lines) | std::views::transform([](int t) {
            auto angle = (t * 2.0 / lines);
            double const s = sin(std::numbers::pi * angle);
            double const c = cos(std::numbers::pi * angle);

            int x0 = static_cast<int>(std::lround(10.0 * c + 120.0));
            int y0 = static_cast<int>(std::lround(10.0 * s + 120.0));
            int x1 = static_cast<int>(std::lround(100.0 * c + 120.0));
            int y1 = static_cast<int>(std::lround(100.0 * s + 120.0));

            return std::pair{std::pair{x0, y0}, std::pair{x1, y1}};
        });

    auto overlap = 0;

    for (auto &&[p0, p1] : endpoints) {
        for (auto &&[x, y] : line(p0, p1)) {
            if (pixmap[y][x] != raster::pixel{255, 255, 255}) {
                pixmap[y][x] = raster::pixel{255, 0, 0};
                ++overlap;
            }
            else {
                pixmap[y][x] = raster::pixel{0, 0, 0};
            }
        }
    }

    ok(overlap == 0, "no overlaps");

    pixmap.save(output.replace_extension("ppm"));
    diag(output);
}

void test_degenerate() {
    channel::ranges::thin_line_view t(std::pair{25, 73}, std::pair{25, 73});

    ok(std::ranges::empty(t), "degenerate range empty");

    auto b = t.begin();
    auto e = t.end();

    eq(b, e, "begin() == end()");
    eq(*e, channel::point{25, 73}, "iterator valid");
    eq(e, ++b, "increment does not change iterator");
}

int main(int argc, char **argv) {
    if (argc < 1) throw std::runtime_error{"launch"};

    auto progname =
        std::filesystem::weakly_canonical(argv[0]).replace_extension();

    test_plan plan;

    try {
        std::default_random_engine urbg{std::random_device{}()};

        test_random(progname, urbg);
        test_radial(progname);
        test_degenerate();
    }
    catch (...) {
        bail_out(std::current_exception());
    }
}
