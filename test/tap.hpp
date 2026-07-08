// Copyright (C) 2020-2026 by Rob Menke.  All rights reserved.  See
// accompanying LICENSE.txt file for details.

#ifndef _tap_hpp_
#define _tap_hpp_

#include <cxxabi.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <iterator>
#include <regex>
#include <sstream>

/**
 * @brief The namespace enclosing the TAP objects.
 */
namespace tap {

/**
 * @brief Namespace for internal details of displaying values.
 */
namespace __detail {

/**
 * @brief Decode a mangled symbol.
 *
 * Use the C++ ABI to demangle a symbol.  Most C++ compilers store the
 * mangled type in the @c name() method of the @c std::type_info
 * structure.
 *
 * @param mangled the mangled symbol
 * @returns the unmangled name or type
 * @throws std::bad_alloc if a memory allocation error occurs
 * @throws std::invalid_argument if the name cannot be decoded
 * @throws std::runtime_error if an unexpected error occurs
 */
inline std::string demangle(const std::string &mangled) {
    // NOLINTBEGIN: use of manual memory management is req'd by C++ ABI

    int status = 0;

    std::size_t len = 255;
    char *buf = static_cast<char *>(malloc(len));
    char *demangled =
        __cxxabiv1::__cxa_demangle(mangled.c_str(), buf, &len, &status);

    if (demangled == nullptr) {
        free(buf);

        switch (status) {
            case -1:
                throw std::bad_alloc{};
            case -2:
                throw std::invalid_argument{"not a mangled name"};
            case -3:
                throw std::invalid_argument{"invalid argument"};
            default: {
                auto msg =
                    "__cxa_demangle: status = " + std::to_string(status);
                throw std::runtime_error{std::move(msg)};
            }
        }
    }

    std::string result{demangled};
    free(demangled);
    return result;

    // NOLINTEND
}

/**
 * @brief Display a type garnered from a @c typeid() expression.
 *
 * @param os the output stream
 * @param ti the @c std::type_info object
 */
inline void sprint_one(std::ostream &os, const std::type_info &ti) {
    os << demangle(ti.name());
}

/**
 * @brief Display an exception stored in a @c std::exception_ptr object.
 *
 * Display the type of the exception.  If the exception belongs to a
 * subclass of @c std::exception, also display the exception message
 * using the what() method of the exception object.
 *
 * @param os the output stream
 * @param ptr the @c std::exception_ptr object
 */
inline void sprint_one(std::ostream &os, std::exception_ptr ptr) {
    try {
        std::rethrow_exception(ptr);
    }
    catch (const std::exception &ex) {
        os << "exception " << demangle(typeid(ex).name()) << ": "
           << ex.what();
    }
    catch (...) {
        os << "exception";
        auto *ti = __cxxabiv1::__cxa_current_exception_type();
        if (ti) os << ' ' << demangle(ti->name());
    }
}

/**
 * @brief Display an object.
 *
 * By default, @c operator<<() is used to display the value on the
 * given output stream.  This method can be overridden via ADL so that
 * a custom message is displayed instead.
 *
 * @tparam Arg the type of the object to display
 * @param os the output stream
 * @param arg the object to display
 */
template <class Arg>
void sprint_one(std::ostream &os, const Arg &arg) {
    os << arg;
}

/**
 * Class implementing the @c sprint_one niebloid.
 * @internal
 */
struct __sprint_one_fn {
    /**
     * @brief Call the niebloid.
     *
     * @param os the output stream
     * @param arg the object to display
     * @internal
     */
    template <class Arg>
    void operator()(std::ostream &os, const Arg &arg) const {
        sprint_one(os, arg);
    }
};

} // namespace __detail

// Niebloid for value displaying.  All TAP functions eventually use
// this for output.  Use ADL for custom types.

static inline constexpr __detail::__sprint_one_fn sprint_one;

/**
 * @brief Convenience function for calling sprint_one on each argument
 * in sequence.
 *
 * @param os the output stream
 * @param args the arguments
 */
template <class... Args>
void sprint(std::ostream &os, Args &&...args) {
    (sprint_one(os, std::forward<Args>(args)), ...);
}

/**
 * @brief Display the objects on the standard output stream.
 *
 * Same as <code>sprint(std::cout, args...)</code>.
 *
 * @param args the arguments
 */
template <class... Args>
void print(Args &&...args) {
    sprint(std::cout, std::forward<Args>(args)...);
}

/**
 * @brief Display the objects on the standard output stream, followed
 * by a newline.
 *
 * Same as <code>print(args..., '\\n')</code>.
 *
 * @param args the arguments
 */
template <class... Args>
void println(Args &&...args) {
    print(std::forward<Args>(args)..., '\n');
}

/**
 * @brief Display the object on the standard output stream, prefixed
 * by a hashmark and followed by a newline.
 *
 * Same as <code>print("# ", args..., '\\n')</code>.
 *
 * @param args the arguments
 */
template <class... Args>
void diag(Args &&...args) {
    println("# ", std::forward<Args>(args)...);
}

struct skip_all_t {};

static constexpr skip_all_t skip_all{};

class test_plan {
    unsigned _plan;
    unsigned _done = 0;
    bool _failed = false;

    test_plan *_parent;

    static inline test_plan *_current_plan = nullptr; // NOLINT

    std::string _todo;

  public:
    /**
     * @brief Create a regular test plan.
     *
     * Creates a test plan with an optional expected count.  If there
     * is an existing test plan active, this instance is a subtest.
     */
    test_plan(unsigned plan = 0)
        : _plan(plan)
        , _parent(_current_plan) {
        _current_plan = this;

        if (_parent) {
            if (_plan > (_parent->_plan - _parent->_done)) {
                throw std::logic_error{"too many subtests"};
            }
        }

        if (plan) println("1..", plan);
    }

    /**
     * @brief Create a skip test plan.
     *
     * Creates a test plan with a count.  The tests of the plan will
     * be marked as skipped.  If this is the root test, all tests will
     * be skipped and the test suite will exit.
     *
     * @param plan the number of tests to skip
     * @param args the arguments to use for the SKIP message
     */
    template <class... Args>
    test_plan(skip_all_t, unsigned plan, Args &&...args)
        : _plan(plan)
        , _done(plan)
        , _parent(_current_plan) {
        _current_plan = this;

        if (!_parent) {
            println("1..0 # SKIP ", std::forward<Args>(args)...);
            std::exit(0);
        }

        for (unsigned i = 1; i <= _plan; ++i) {
            _parent->skip(args...);
        }
    }

    /**
     * Test plans cannot be copied.
     */
    test_plan(const test_plan &) = delete;

    /**
     * Move-construct a test plan.
     */
    test_plan(test_plan &&r) noexcept
        : _plan(std::exchange(r._plan, 0))
        , _done(std::exchange(r._done, 0))
        , _parent(std::exchange(r._parent, nullptr)) {
        _current_plan = this;
    }

    /**
     * Test plans cannot be copied.
     */
    test_plan &operator=(const test_plan &) = delete;

    /**
     * Move-assign a test plan.
     */
    test_plan &operator=(test_plan &&r) noexcept {
        _plan = std::exchange(r._plan, 0);
        _done = std::exchange(r._done, 0);
        _parent = std::exchange(r._parent, nullptr);

        return *this;
    }

    ~test_plan() noexcept(false) {
        if (_current_plan != this) {
            throw std::logic_error{"incorrect nesting of plans"};
        }

        _current_plan = _parent;

        if (_plan == 0 && !_parent) {
            println("1..", _done);
        }
        else if (_plan != _done) {
            diag("Looks like you planned ", _plan, ' ',
                 (_plan == 1 ? "test" : "tests"), " but ran ", _done);
        }
    }

    /**
     * @brief Get the current plan.
     *
     * Get a pointer the current plan.
     *
     * @returns a pointer to the current plan instance
     * @throws std::logic_error if no instance exists
     */
    static test_plan *current_plan() {
        auto plan = _current_plan;
        if (!plan) throw std::logic_error{"no plan"};
        return plan;
    }

    /**
     * @brief Mark a test as passed.
     *
     * Increments the number of tests run in the current plan.
     * Outputs the string "ok" followed by the test number.  If the
     * todo() method has been called, outputs the TODO comment after
     * the test message; otherwise, concatenates the arguments as a
     * string.
     *
     * @param args the optional arguments to use as a message
     * @sa tap::test_plan::todo()
     * @sa tap::print()
     */
    template <class... Args>
    void pass(Args &&...args) {
        ++_done;

        if (_parent) return _parent->pass(std::forward<Args>(args)...);

        print("ok ", _done);

        if (!_todo.empty()) {
            print(" # TODO ", _todo);
        }
        else if constexpr (sizeof...(args)) {
            print(" - ", std::forward<Args>(args)...);
        }

        println();
    }

    template <class... Args>
    void fail(Args &&...args) {
        ++_done;

        if (_parent) return _parent->fail(std::forward<Args>(args)...);

        print("not ok ", _done);

        if (!_todo.empty()) {
            print(" # TODO ", _todo);
        }
        else if constexpr (sizeof...(args)) {
            print(" - ", std::forward<Args>(args)...);
        }

        println();

        _failed = true;
    }

    template <class... Args>
    void skip(Args &&...args) {
        if (_todo.empty()) {
            println("ok ", ++_done, " # SKIP ",
                    std::forward<Args>(args)...);
        }
        else {
            println("not ok ", ++_done, " # TODO & SKIP ", _todo);
        }
    }

    /**
     * @brief Mark the remainder of tests as works-in-progress.
     *
     * The TODO state can be cleared by calling todo() with no arguments.
     *
     * @param args the arguments used to generate the TODO message
     * @returns the previous TODO message.
     */
    template <class... Args>
    std::string todo(Args &&...args) {
        std::ostringstream os;
        sprint(os, std::forward<Args>(args)...);
        return std::exchange(_todo, std::move(os).str());
    }
};

/**
 * @brief Indicate the test passed.
 *
 * Unconditionally pass a test.
 *
 * @param args additional message parameters
 */
template <class... Args>
void pass(Args &&...args) {
    test_plan::current_plan()->pass(std::forward<Args>(args)...);
}

/**
 * @brief Indicate the test failed.
 *
 * Unconditionally fail a test.
 *
 * @param args additional message parameters
 */
template <class... Args>
void fail(Args &&...args) {
    test_plan::current_plan()->fail(std::forward<Args>(args)...);
}

/**
 * @brief Conditionally pass a test.
 *
 * Passes the test if and only if the first argument is @c true.
 *
 * @param result the status of the test
 * @param args additional message parameters
 * @returns the result boolean
 */
template <class... Args>
bool ok(bool result, Args &&...args) {
    auto plan = test_plan::current_plan();

    if (result)
        plan->pass(std::forward<Args>(args)...);
    else
        plan->fail(std::forward<Args>(args)...);

    return result;
}

/**
 * Convert a standard comparison function object into a string
 * describing the operation.
 *
 * @tparam T the comparison function object template.
 */
template <template <class> class T>
static inline const char *const cmp_op = nullptr;

template <>
inline const char *const cmp_op<std::equal_to> = "==";
template <>
inline const char *const cmp_op<std::not_equal_to> = "!=";
template <>
inline const char *const cmp_op<std::greater> = ">";
template <>
inline const char *const cmp_op<std::greater_equal> = ">=";
template <>
inline const char *const cmp_op<std::less> = "<";
template <>
inline const char *const cmp_op<std::less_equal> = "<=";

template <template <class> class Comp, class X, class Y, class... Args>
bool cmp(X &&x, Y &&y, Args &&...args) {
    if (!ok(Comp<void>{}(x, y), std::forward<Args>(args)...)) {
        diag(std::forward<X>(x), ' ', cmp_op<Comp>, ' ', std::forward<Y>(y),
             ": failed");
        return false;
    }
    return true;
}

template <class X, class Y, class... Args>
bool eq(X &&x, Y &&y, Args &&...args) {
    return cmp<std::equal_to>(std::forward<X>(x), std::forward<Y>(y),
                              std::forward<Args>(args)...);
}

template <class X, class Y, class... Args>
bool ne(X &&x, Y &&y, Args &&...args) {
    return cmp<std::not_equal_to>(std::forward<X>(x), std::forward<Y>(y),
                                  std::forward<Args>(args)...);
}

template <class X, class Y, class... Args>
bool gt(X &&x, Y &&y, Args &&...args) {
    return cmp<std::greater>(std::forward<X>(x), std::forward<Y>(y),
                             std::forward<Args>(args)...);
}

template <class X, class Y, class... Args>
bool ge(X &&x, Y &&y, Args &&...args) {
    return cmp<std::greater_equal>(std::forward<X>(x), std::forward<Y>(y),
                                   std::forward<Args>(args)...);
}

template <class X, class Y, class... Args>
bool lt(X &&x, Y &&y, Args &&...args) {
    return cmp<std::less>(std::forward<X>(x), std::forward<Y>(y),
                          std::forward<Args>(args)...);
}

template <class X, class Y, class... Args>
bool le(X &&x, Y &&y, Args &&...args) {
    return cmp<std::less_equal>(std::forward<X>(x), std::forward<Y>(y),
                                std::forward<Args>(args)...);
}

template <class X, class Y, class Z, class... Args>
bool within(X &&x, Y &&y, Z &&z, Args &&...args) {
    if (!ok(std::fabs(x - y) <= z, std::forward<Args>(args)...)) {
        diag(y, " = ", x, "±", z, ": failed");
        return false;
    }
    return true;
}

template <class Str, class RE, class... Args>
bool like(Str &&str, RE &&re, Args &&...args) {
    using CharT = typename std::remove_reference<Str>::type::value_type;

    if (!ok(std::regex_match(str, std::basic_regex<CharT>{re}),
            std::forward<Args>(args)...)) {
        diag(str, " is like ", re, ": failed");
        return false;
    }
    return true;
}

template <class T, class P, class... Args>
bool is_instanceof(P &&p, Args &&...args) {
    std::type_info const &expected = typeid(T);
    std::type_info const &actual = typeid(p);

    if (!ok(expected == actual, std::forward<Args>(args)...)) {
        diag("type of parameter is ", actual.name());
        diag("     but expected    ", expected.name());
        return false;
    }
    return true;
}

template <class... Args>
test_plan skip(unsigned count, Args &&...args) {
    return test_plan{skip_all, count, args...};
}

class todo {
    test_plan *_plan;
    std::string _old_todo;

  public:
    template <class... Args>
    todo(Args &&...args)
        : _plan(test_plan::current_plan())
        , _old_todo(_plan->todo(std::forward<Args>(args)...)) {}
    todo(const todo &) = delete;
    todo(todo &&) = delete;

    todo &operator=(const todo &) = delete;
    todo &operator=(todo &&) = delete;

    ~todo() {
        _plan->todo(_old_todo);
    }
};

#define TODO(REASON) if (tap::todo __todo{REASON}; true) // NOLINT

template <class... Args>
[[noreturn]] void bail_out(Args &&...args) {
    println("Bail out! ", std::forward<Args>(args)...);
    exit(EXIT_FAILURE);
}

template <class... Args>
std::exception_ptr catch_exception(Args &&...args) {
    std::invoke(std::forward<Args>(args)...);
    return nullptr;
}

template <class Exception, class... Exceptions, class... Args>
std::exception_ptr catch_exception(Args &&...args) {
    try {
        return catch_exception<Exceptions...>(std::forward<Args>(args)...);
    }
    catch (const Exception &ex) {
        return std::current_exception();
    }
}

template <class... Exception, class... Args>
auto expected_exception(Args &&...args) {
    auto ex = catch_exception<Exception...>(std::forward<Args>(args)...);

    if (ex) {
        pass("caught ", ex);
    }
    else {
        fail("exception expected but not thrown");
    }

    return ex;
}

template <
    std::ranges::input_range R1, std::ranges::input_range R2, class... Args>
void compare_ranges(R1 &&r1, R2 &&r2, Args &&...args) { // NOLINT
    auto s1 = r1.begin();
    auto s2 = r2.begin();
    auto e1 = r1.end();
    auto e2 = r2.end();

    auto &&[i1, i2] = std::ranges::mismatch(s1, e1, s2, e2);

    if (!ok(i1 == e1 && i2 == e2, args...)) {
        const auto offset = std::ranges::distance(s1, i1);

        if (i1 == e1) {
            diag("<END> != ", *i2, "; offset ", offset);
        }
        else if (i2 == e2) {
            diag(*i1, " != <END>; offset ", offset);
        }
        else {
            diag(*i1, " != ", *i2, "; offset ", offset);
        }
    }
}

} // namespace tap

#endif
