#ifndef _channel_hpp_
#define _channel_hpp_

#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>
#include <vector>

/// @brief Classes for the generation of pixelated lines.

namespace channel {

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
};

/// @brief Range factories.

namespace ranges {

/// @brief The classic Bresenham algorithm as a range factory.
///
/// Bresenham's algorithm maintains an error term that indicates how
/// far the nearest integral pixel is to the ideal line.

class thin_line_view : public std::ranges::view_interface<thin_line_view> {
    int _x0, _y0; ///< The starting point.
    int _x1, _y1; ///< The finishing point.

    bool _closed;               ///< Include finishing point in range?

    int _dx = std::abs(_x1 - _x0); ///< Absolute change in x.
    int _dy = std::abs(_y1 - _y0); ///< Absolute change in y.
    int _sx = _x1 > _x0 ? +1 : -1; ///< Direction of x movement.
    int _sy = _y1 > _y0 ? +1 : -1; ///< Direction of y movement.

  public:
    /// @brief Construct a thin line between two endpoints.
    ///
    /// By default, the finishing point will not be included in the
    /// generated range.  If it should be, then the @p closed
    /// parameter should be set to @c true.

    constexpr thin_line_view(
        int x0,             ///< X coordinate of starting point.
        int y0,             ///< Y coordinate of starting point.
        int x1,             ///< X coordinate of finishing point.
        int y1,             ///< Y coordinate of finishing point.
        bool closed = false ///< Include the finishing point in range?
    ) noexcept
        : _x0(x0)
        , _y0(y0)
        , _x1(x1)
        , _y1(y1)
        , _closed(closed) {}

    /// @brief An iterator that generates the integral points nearest to the
    /// ideal ray.
    ///
    /// The ideal ray is defined by its initial coordinates, the delta
    /// vector (magnitude), and the movement vector (direction).
    /// Because it is a ray, it does not have a built-in terminal
    /// point.

    class iterator {
        using point = std::pair<int, int>;

        point _current; ///< Position of the iterator.

        int _step_x; ///< Direction of horizontal movement.
        int _step_y; ///< Direction of vertical movement.

        error_accumulator _error; ///< Error accrued reaching this state.

      public:
        using value_type = decltype(_current);
        using reference = value_type const &;
        using pointer = value_type const *;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        constexpr iterator() = default;

        /// @brief Construct an iterator with an initial error.
        constexpr iterator(
            int x,                  ///< the x position of the iterator
            int y,                  ///< the y position of the iterator
            int sx,                 ///< the movement in the x direction
            int sy,                 ///< the movement in the y direction
            error_accumulator error ///< the error accumulator
        )
            : _current(x, y)
            , _step_x(sx)
            , _step_y(sy)
            , _error(error) {}

        /// @brief Construct an iterator with a delta.
        constexpr iterator(
            int x,  ///< the x position of the iterator
            int y,  ///< the y position of the iterator
            int dx, ///< the absolute change in the x direction
            int dy, ///< the absolute change in the y direction
            int sx, ///< the movement in the x direction
            int sy  ///< the movement in the y direction
        )
            : iterator(x, y, sx, sy, error_accumulator(dx, dy)) {}

        /// @brief The current value from the iterator.
        constexpr reference operator*() const noexcept {
            return _current;
        }

        constexpr pointer operator->() const noexcept {
            return &_current;
        }

        /// @brief Preincrement operator.
        constexpr iterator &operator++() {
            auto &&[x, y] = _current;
            auto &&[move_x, move_y] = _error.update();

            if (move_x) x += _step_x;
            if (move_y) y += _step_y;

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
    };

    /// @brief Create an iterator that is positioned at the starting
    /// coordinates.

    constexpr iterator begin() const {
        return iterator(_x0, _y0, _dx, _dy, _sx, _sy);
    }

    /// @brief Create an iterator that is positioned one step past the
    /// finishing coordinates.
    ///
    /// If the @p _closed flag is true, this iterator will actually be
    /// one step beyond the finishing coordinates.

    constexpr iterator end() const {
        iterator iter(_x1, _y1, _dx, _dy, _sx, _sy);
        if (_closed) ++iter;
        return iter;
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
    int _x0, _y0;     ///< The starting point.
    int _x1, _y1;     ///< The finishing point.
    unsigned int _r2; ///< The radius squared.

    bool _closed; ///< Include the finishing point in the range?

    int _dx = std::abs(_x1 - _x0);
    int _dy = std::abs(_y1 - _y0);
    int _sx = _x1 > _x0 ? +1 : -1;
    int _sy = _y1 > _y0 ? +1 : -1;

  public:
    /// @brief Construct a thick line between two endpoints with a
    /// given thickness.

    thick_line_view(
        int x0, int y0, int x1, int y1, unsigned short radius,
        bool closed = false
    )
        : _x0(x0)
        , _y0(y0)
        , _x1(x1)
        , _y1(y1)
        , _r2(radius * radius)
        , _closed(closed) {}

    /// @brief An iterator that manages multiple thin iterators in tandem.

    class iterator {
        using point = std::pair<int, int>;

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
                auto &&[x, y] = *iter;
                _value.emplace_back(x, y);
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
                auto [x, y] = *(++iter);
                _value.emplace_back(x, y);
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
        bool operator==(const iterator &rhs) const noexcept {
            return std::ranges::equal(
                _value, rhs._value, [](auto &&a, auto &&b) {
                    auto &&[ax, ay] = a;
                    auto &&[bx, by] = b;
                    return ax == bx && ay == by;
                }
            );
        }

        /// @brief Factory method to produce an iterator.
        static iterator create(
            int x,          ///< the current x position of the iterator
            int y,          ///< the current y position of the iterator
            int dx,         ///< the absolute change in z
            int dy,         ///< the absolute change in y
            int sx,         ///< the direction of horizontal movement
            int sy,         ///< the direction of vertical movement
            unsigned int r2 ///< the square of the radius
        );
    };

    /// @brief Create an iterator that is positioned at the starting
    /// coordinates.

    iterator begin() const {
        return iterator::create(_x0, _y0, _dx, _dy, _sx, _sy, _r2);
    }

    /// @brief Create an iterator that is positioned at the finishing
    /// coordinates.

    iterator end() const {
        iterator iter = iterator::create(_x1, _y1, _dx, _dy, _sx, _sy, _r2);
        if (_closed) ++iter;
        return iter;
    }
};

inline thick_line_view::iterator thick_line_view::iterator::create(
    int cur_x, int cur_y, int dx, int dy, int sx, int sy, unsigned int r2
) {
    // Determine the axis along which the parallel lines have the
    // greatest change.  This is the major direction: create a
    // function pointer to access the corresponding component of any
    // point.

    // clang-format off
    int &(*const major)(int &, int &) noexcept = dx >= dy ?
        [](int &x, int &) noexcept -> int & { return x; } :
        [](int &, int &y) noexcept -> int & { return y; };
    // clang-format on

    // Generate a perpendicular to the baseline that passes
    // through the origin.  These points will be the offsets from the
    // baseline to the parallel lines.

    // Find a point that lies on the perpendicular and is sufficiently
    // far away from the origin.

    int px = -sy, py = +sx;

    // The orientation of the perpendicular is important.  The
    // movement component in the major direction for both the parallel
    // and perpendicular lines must be the same.  If it is not, rotate
    // the perpendicular an additional 180°.

    if (major(px, py) != major(sx, sy)) {
        px = +sy, py = -sx;
    }

    int perp_x = px * dy, perp_y = py * dx;

    // Ensure that the starting point is sufficiently far away.  (It
    // should alredy be except in pathological cases.)

    constexpr static auto length_squared =
        [](int x, int y) noexcept -> std::size_t {
        return x * x + y * y;
    };

    while (length_squared(perp_x, perp_y) < r2) {
        perp_x *= 2, perp_y *= 2;
    }

    // A predicate that determines if the point is within the radius
    // from the origin.

    const auto within_radius = [&r2](auto &&pt) {
        auto &&[x, y] = pt;
        return length_squared(x, y) <= r2;
    };

    // Create a view over the perpendicular that only contains the
    // points within the radius.

    auto perpendicular =
        thin_line_view(-perp_x, -perp_y, perp_x, perp_y) |
        std::views::drop_while(std::not_fn(within_radius)) |
        std::views::take_while(within_radius);

    auto begin = perpendicular.begin();
    auto end = perpendicular.end();

    int next_x = begin->first;
    int next_y = begin->second;

    std::vector<thin_line_view::iterator> iterators;
    error_accumulator parallel_error(dx, dy);

    while (++begin != end) {
        auto off_x = std::exchange(next_x, begin->first);
        auto off_y = std::exchange(next_y, begin->second);

        iterators.emplace_back(
            cur_x + off_x, cur_y + off_y, sx, sy, parallel_error
        );

        auto const x_will_change = (next_x - off_x) != 0;
        auto const y_will_change = (next_y - off_y) != 0;

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
                major(off_x, off_y) += major(px, py);
                iterators.emplace_back(
                    cur_x + off_x, cur_y + off_y, sx, sy, parallel_error
                );
            }
        }
    }

    iterators.emplace_back(
        cur_x + next_x, cur_y + next_y, sx, sy, parallel_error
    );

    return iterator{std::move(iterators)};
}

} // namespace ranges

namespace views {

struct __line_fn {
    constexpr auto operator()(auto &&p0, auto &&p1) const {
        auto &&[x0, y0] = p0;
        auto &&[x1, y1] = p1;

        return ranges::thin_line_view(x0, y0, x1, y1);
    }

    constexpr auto operator()(auto &&p0, auto &&p1, auto &&radius) const {
        auto &&[x0, y0] = std::forward<decltype(p0)>(p0);
        auto &&[x1, y1] = std::forward<decltype(p1)>(p1);

        return ranges::thick_line_view(x0, y0, x1, y1, radius);
    }
};

/// @brief Range adaptor closure object.
///
/// Constructs a thin or thick line depending if the radius parameter
/// is supplied.

inline constexpr auto line = __line_fn{};

} // namespace views

} // namespace channel

/// @cond
template <>
constexpr inline bool
    std::ranges::enable_borrowed_range<channel::ranges::thin_line_view> =
        true;

template <>
constexpr inline bool
    std::ranges::enable_borrowed_range<channel::ranges::thick_line_view> =
        true;
/// @endcond

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
