#ifndef _channel_hpp_
#define _channel_hpp_

#include <cassert>
#include <cmath>
#include <functional>
#include <iterator>
#include <ostream>
#include <ranges>
#include <utility>
#include <vector>

/// @brief Classes for the generation of pixelated lines.

namespace channel {

/// @brief A 2-D coordinate pair.
///
/// While it would have been possible for point to be implemented as a
/// @c std::pair<int,int> class, it made the introduction of operators
/// working with the values difficult.

struct point {
    int x; ///< The x-coordinate.
    int y; ///< The y-coordinate.

    /// @brief Default constructor.
    constexpr point() noexcept {}

    /// @brief Construct a point from two coordinates.
    constexpr point(int x, int y) noexcept
        : x(x)
        , y(y) {}

    /// @brief Construct a point from any type that is bindable.
    constexpr point(auto &&pt) {
        auto &&[x0, y0] = std::forward<decltype(pt)>(pt);
        x = x0, y = y0;
    }

    /// @brief Convert a @c point object into a @c std::pair object.
    constexpr operator std::pair<int, int>() const noexcept {
        return {x, y};
    }

    /// @brief Equality operator.
    constexpr bool operator==(point const &) const noexcept = default;

    /// @brief Vector addition.
    constexpr point operator+(point const &rhs) const noexcept {
        return {x + rhs.x, y + rhs.y};
    }

    /// @brief Vector subtraction.
    constexpr point operator-(point const &rhs) const noexcept {
        return {x - rhs.x, y - rhs.y};
    }

    /// @brief Vector negation.
    constexpr point operator-() const noexcept {
        return {-x, -y};
    }

    /// @brief Multiply the vector by an integral value.
    constexpr point &operator*=(std::integral auto &&t) noexcept {
        x *= t, y *= t;
        return *this;
    }

    /// @brief Calculate the square of the euclidean length.
    constexpr std::size_t length_squared() const noexcept {
        return x * x + y * y;
    }

    /// @brief Output the point as an ordered pair.
    friend std::ostream &operator<<(std::ostream &os, point const &pt) {
        return os << "(" << pt.x << ", " << pt.y << ")";
    }
};

constexpr point abs(point const &p) noexcept {
    return {std::abs(p.x), std::abs(p.y)};
}

constexpr int sgn(int x) noexcept {
    return x > 0 ? +1 : x < 0 ? -1 : 0;
}
constexpr point sgn(point const &p) noexcept {
    return {sgn(p.x), sgn(p.y)};
}

/// @brief The Bresenham error accumulator.
///
/// A simple object that maintains the accumulated error from
/// the ideal line.  Depending on the value of @c _error, the update()
/// member function may indicate that motion is needed along the
/// x-axis, the y-axis, or both.
///
/// The operation is deceptively simple.  When the error accumulator
/// exceeds @f$-\frac{\mathop{\mathit{dy}}}{2}@f$, it emits an
/// instruction to move horizontally.  When the error accumulator
/// falls below @f$\frac{\mathop{\mathit{dx}}}{2}@f$, it emits an
/// instruction to move vertically.  It then updates the error
/// accumulator according to the move: a vertical move increases the
/// accumulator by @f$\mathop{\mathit{dx}}@f$ and an horizontal move
/// decreases it by @f$\mathop{\mathit{dy}}@f$; both changes may
/// happen but at least one is guaranteed to occur.  For the first
/// octant, this means that a vertical move is emitted for roughly
/// @f$\frac{\mathop{\mathit{dx}}} {\mathop{\mathit{dy}}}@f$
/// horizontal moves.
///
/// @note This is actually Zingl's modification of Bresenham's
/// algorithm.  It requires one more comparison per update() call than
/// traditional Bresenham, but it is octant-independent while
/// Bresenham is restricted to the first octant.
///
/// @sa https://en.wikipedia.org/wiki/Bresenham's_line_algorithm
/// @sa https://zingl.github.io/Bresenham.pdf

class error_accumulator {
    int _dx; ///< The distance that will be moved along the x-axis.
    int _dy; ///< The distance that will be moved along the y-axis.

    int _error; ///< The accumulated error.

  public:
    /// @brief Default constructor.
    constexpr error_accumulator() noexcept = default;

    /// @brief Copy constructor.
    constexpr error_accumulator(error_accumulator const &) noexcept =
        default;

    /// @brief Move constructor.
    constexpr error_accumulator(error_accumulator &&) noexcept = default;

    /// @brief Accumulator constructor.
    constexpr error_accumulator(
        auto &&dx, ///< the absolute change in the x value
        auto &&dy  ///< the absolute change in the y value
    )
        : _dx(dx)
        , _dy(dy)
        , _error(dx - dy) {}

    /// @brief Accumulator constructor.
    constexpr error_accumulator(
        point delta ///< the magnitude of change in both directions
    )
        : error_accumulator(delta.x, delta.y) {}

    constexpr ~error_accumulator() = default;

    /// @brief Copy assignment operator.
    constexpr error_accumulator &
    operator=(error_accumulator const &) noexcept = default;

    /// @brief Move assignment operator.
    constexpr error_accumulator &
    operator=(error_accumulator &&) noexcept = default;

    /// @brief A pair indicating the required motions.
    ///
    /// The only situation when neither field is set after a call to
    /// update() is when the delta was originally zero.

    struct update_result {
        bool move_x : 1; ///< Take a step along the x-axis.
        bool move_y : 1; ///< Take a step along the y-axis.
    };

    /// @brief Update the error, returning the previous status.
    ///
    /// The range of values for @c _error is partitioned into three
    /// subranges, indicating that movement is required along the
    /// x-axis, the y-axis, or both.
    ///
    /// @returns an update_result structure

    constexpr update_result update() noexcept {
        update_result result{
            .move_x = 2 * _error > -_dy, .move_y = 2 * _error < _dx
        };

        if (result.move_x) _error -= _dy;
        if (result.move_y) _error += _dx;

        return result;
    }

    friend std::ostream &
    operator<<(std::ostream &os, error_accumulator const &e) {
        return os << "{(" << e._dx << ", " << e._dy
                  << "), error=" << e._error << "}";
    }
};

/// @brief Range factories.
namespace ranges {

/// @brief The classic Bresenham algorithm as a range factory.
///
/// Bresenham's algorithm maintains an error term that indicates how
/// far the nearest integral pixel is to the ideal line.

class thin_line_view : public std::ranges::view_interface<thin_line_view> {
    point _p0;    ///< The starting point.
    point _p1;    ///< The finishing point.
    point _delta; ///< The absolute difference between the endpoints.
    point _move;  ///< The direction of movement along the axes.

  public:
    /// @brief Construct a thin line between two endpoints.
    ///
    /// The result will contain only the first endpoint.

    constexpr thin_line_view(
        auto &&p0, ///< the starting point
        auto &&p1  ///< the ending point
    )
        : _p0(std::forward<decltype(p0)>(p0))
        , _p1(std::forward<decltype(p1)>(p1))
        , _delta(abs(_p1 - _p0))
        , _move(sgn(_p1 - _p0)) {}

    /// @brief An iterator that generates the integral points nearest to the
    /// ideal ray.
    ///
    /// The ideal ray is defined by its initial coordinates, the delta
    /// vector (magnitude), and the movement vector (direction).
    /// Because it is a ray, it does not have a built-in terminal
    /// point.

    class iterator {
        point _current;           ///< Current position of the iterator.
        error_accumulator _error; ///< Error accrued reaching this state.
        point _move;              ///< Direction of movement.

      public:
        using value_type = decltype(_current);
        using reference = value_type const &;
        using pointer = value_type const *;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        constexpr iterator() = default;

        /// @brief Construct an iterator with an initial error.
        constexpr iterator(
            auto &&current,          ///< the starting point
            error_accumulator error, ///< the initial error
            auto &&move              ///< the direction in which to advance
        )
            : _current(std::forward<decltype(current)>(current))
            , _error(std::move(error))
            , _move(std::forward<decltype(move)>(move)) {}

        /// @brief Construct an iterator with a delta.
        constexpr iterator(
            auto &&current, ///< the starting point
            auto &&delta,   ///< the absolute differences
            auto &&move     ///< the direction in which to advance
        )
            : iterator(
                  std::forward<decltype(current)>(current),
                  error_accumulator(std::forward<decltype(delta)>(delta)),
                  std::forward<decltype(move)>(move)
              ) {}

        /// @brief Extract the current value from the iterator.
        constexpr reference operator*() const noexcept {
            return _current;
        }

        /// @brief Extract the current value from the iterator.
        constexpr pointer operator->() const noexcept {
            return &_current;
        }

        /// @brief Preincrement operator.
        constexpr iterator &operator++() {
            auto &&[move_x, move_y] = _error.update();

            if (move_x) _current.x += _move.x;
            if (move_y) _current.y += _move.y;

            return *this;
        }

        /// @brief Postincrement operator.
        constexpr iterator operator++(int) {
            iterator copy{*this};
            operator++();
            return copy;
        }

        /// @brief Equality operator.
        constexpr bool operator==(iterator const &rhs) const noexcept {
            return _current == rhs._current;
        }

        friend std::ostream &
        operator<<(std::ostream &os, const iterator &i) {
            return os << "{current=" << i._current << ", error=" << i._error
                      << ", move=" << i._move << "}";
        }
    };

    /// @brief Create an iterator that is positioned at the starting
    /// coordinates.
    constexpr iterator begin() const {
        return iterator(_p0, _delta, _move);
    }

    /// @brief Create an iterator that is positioned one step past the
    /// ending coordinates.
    constexpr iterator end() const {
        return iterator(_p1, _delta, _move);
    }
};

/// @brief The Bresenham-Murphy algorithm as a range factory.
///
/// This is an implementation of Murphy's extension to the Bresenham
/// algorithm.  It works by generating a perpendicular line to the
/// baseline, then generating parallel lines using the perpendicular
/// as the starting point.  Each parallel is adjusted so that their
/// diagonal moves mesh together, leaving no gaps.
///
/// Murphy's original paper was impenetrable so this version of the
/// algorithm was derived from first principles rather than the
/// flowcharts.  It differs from the algorithm in the paper by the way
/// it generates the perpendicular and the way it calculates the
/// initial error values of the parallel lines.
///
/// The initial parallel is generated by taking a long line segment
/// and rejecting the points that fall outside the requested radius.
/// This is less efficient than Murphy's estimation of sine and
/// cosine, but doesn't involve floating-point calculations.

class thick_line_view
    : public std::ranges::view_interface<thick_line_view> {
    point _p0;        ///< The starting point.
    point _p1;        ///< The finishing point.
    unsigned int _r2; ///< The radius squared.
    point _delta;     ///< The absolute difference between the endpoints.
    point _move;      ///< The direction of movement along each axis.

  public:
    /// @brief Construct a thick line between two endpoints with a
    /// given thickness.
    thick_line_view(
        auto &&p0,    ///< the starting point
        auto &&p1,    ///< the ending point
        auto &&radius ///< the half-width of the thick line
    )
        : _p0(std::forward<decltype(p0)>(p0))
        , _p1(std::forward<decltype(p1)>(p1))
        , _r2(radius * radius)
        , _delta(abs(_p1 - _p0))
        , _move(sgn(_p1 - _p0)) {}

    /// @brief An iterator that manages multiple thin iterators in tandem.

    class iterator {
        std::vector<thin_line_view::iterator> _iters;
        std::vector<point> _value;

      public:
        using value_type = decltype(_value);
        using reference = value_type const &;
        using pointer = value_type const *;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        iterator() noexcept = default;

        /// @brief Construct a thick iterator from a set of thin iterators.

        template <std::ranges::range Range>
        iterator(
            Range &&iters ///< a collection of point generating iterators
        )
            : _iters(std::forward<Range>(iters)) {
            for (auto &&iter : _iters) {
                _value.push_back(*iter);
            }
        }

        /// @brief Extract the current value from the iterator.
        reference operator*() const {
            return _value;
        }

        /// @brief Extract the current value from the iterator.
        pointer operator->() const {
            return &_value;
        }

        /// @brief Preincrement operator.
        iterator &operator++() {
            _value.clear();

            for (auto &&iter : _iters) {
                _value.push_back(*(++iter));
            }

            return *this;
        }

        /// @brief Postincrement operator.
        iterator operator++(int) {
            iterator copy{*this};
            operator++();
            return copy;
        }

        /// @brief Equality operator.
        bool operator==(const iterator &rhs) const noexcept = default;

        /// @brief Factory method to produce an iterator.
        static iterator create(
            point const &current, ///< the current position of the iterator
            point const &delta,   ///< the absolute change of position
            point const &move,    ///< the direction of movement
            unsigned int r2       ///< the square of the radius
        );
    };

    /// @brief Create an iterator that is positioned at the starting
    /// coordinates.
    iterator begin() const {
        return iterator::create(_p0, _delta, _move, _r2);
    }

    /// @brief Create an iterator that is positioned one step past the
    /// ending coordinates.
    iterator end() const {
        return iterator::create(_p1, _delta, _move, _r2);
    }
};

inline thick_line_view::iterator thick_line_view::iterator::create(
    point const &current, point const &delta, point const &step,
    unsigned int r2
) {
    auto &&[dx, dy] = delta;
    auto &&[sx, sy] = step;

    // This gets complicated...
    //
    // Determine the axis along which the parallel lines have the
    // greatest change. Make some pointers to member fields for later
    // use.

    int point::*const major = delta.x >= delta.y ? &point::x : &point::y;
    int point::*const minor = delta.x >= delta.y ? &point::y : &point::x;

    // Generate a perpendicular to the baseline that passes
    // through the origin.  These points will be the offsets from the
    // baseline to the parallel lines.

    // Find a point that lies on the perpendicular and is sufficiently
    // far away from the origin.

    point perp_step = {-sy, +sx};

    // The orientation of the perpendicular is important.  The
    // movement component in the major direction for both the parallel
    // and perpendicular lines must be the same.  If it is not, rotate
    // the perpendicular an additional 180°.

    if (perp_step.*major != step.*major) {
        perp_step *= -1;
    }

    point perp_endpt = {
        perp_step.x * dy,
        perp_step.y * dx,
    };

    // Ensure that the starting point is sufficiently far away.  (It
    // should alredy be except in pathological cases.)

    while (perp_endpt.length_squared() < r2) {
        perp_endpt *= 2;
    }

    // A predicate that determines if the point is within the radius
    // from the origin.

    const auto within_radius = [&r2](auto &&pt) {
        return pt.length_squared() <= r2;
    };

    // Create a view over the perpendicular that only contains the
    // points within the radius.

    auto perpendicular =
        thin_line_view(-perp_endpt, perp_endpt) |
        std::views::drop_while(std::not_fn(within_radius)) |
        std::views::take_while(within_radius);

    auto begin = perpendicular.begin();
    auto end = perpendicular.end();
    auto next = *begin;

    std::vector<thin_line_view::iterator> iterators;
    error_accumulator parallel_error(delta);

    while (++begin != end) {
        auto offset = std::exchange(next, *begin);

        iterators.emplace_back(current + offset, parallel_error, step);

        auto const x_will_change = (next.x - offset.x) != 0;
        auto const y_will_change = (next.y - offset.y) != 0;

        // If the perpendicular is about to make a diagonal move, then
        // update the parallel error accumulator.

        if (x_will_change && y_will_change) {
            auto [move_x, move_y] = parallel_error.update();

            // If the parallel iterator had a diagonal move as well
            // then this will leave a pixel-wide gap between the
            // parallel lines.  Murphy solved this by drawing an
            // additional parallel, effectively turning a single
            // diagonal move into two square moves.

            if (move_x && move_y) {
                offset.*major += perp_step.*major;
                iterators.emplace_back(
                    current + offset, parallel_error, step
                );
                offset.*minor += perp_step.*minor;
            }
        }
    }

    iterators.emplace_back(current + next, parallel_error, step);

    return iterator{std::move(iterators)};
}

} // namespace ranges

namespace views {

struct __line_fn {
    constexpr auto operator()(auto &&p0, auto &&p1) const {
        return ranges::thin_line_view(
            std::forward<decltype(p0)>(p0), std::forward<decltype(p1)>(p1)
        );
    }

    constexpr auto operator()(auto &&p0, auto &&p1, auto &&radius) const {
        return ranges::thick_line_view(
            std::forward<decltype(p0)>(p0), std::forward<decltype(p1)>(p1),
            radius
        );
    }
};

inline constexpr auto line = __line_fn{};

} // namespace views

} // namespace channel

template <>
constexpr inline bool
    std::ranges::enable_borrowed_range<channel::ranges::thin_line_view> =
        true;

template <>
constexpr inline bool
    std::ranges::enable_borrowed_range<channel::ranges::thick_line_view> =
        true;

static_assert(
    std::forward_iterator<channel::ranges::thin_line_view::iterator>
);

static_assert(std::ranges::view<channel::ranges::thin_line_view>);
static_assert(std::ranges::forward_range<channel::ranges::thin_line_view>);
static_assert(std::ranges::common_range<channel::ranges::thin_line_view>);
static_assert(std::ranges::borrowed_range<channel::ranges::thin_line_view>);

static_assert(
    std::forward_iterator<channel::ranges::thick_line_view::iterator>
);

static_assert(std::ranges::view<channel::ranges::thick_line_view>);
static_assert(std::ranges::forward_range<channel::ranges::thick_line_view>);
static_assert(std::ranges::common_range<channel::ranges::thick_line_view>);
static_assert(
    std::ranges::borrowed_range<channel::ranges::thick_line_view>
);

#endif
