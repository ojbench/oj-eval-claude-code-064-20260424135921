#pragma once
#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <exception>
#include <iostream>
#include <ostream>
#include <span>
#include <string_view>
#include <string>
#include <vector>
#include <type_traits>
#include <cstdint>

namespace sjtu {

using sv_t = std::string_view;

struct format_error : std::exception {
public:
    format_error(const char *msg = "invalid format") : msg(msg) {}
    auto what() const noexcept -> const char * override {
        return msg;
    }

private:
    const char *msg;
};

template <typename Tp>
struct formatter;

struct format_info {
    inline static constexpr auto npos = static_cast<std::size_t>(-1);
    std::size_t position; // where is the specifier
    std::size_t consumed; // how many characters consumed
};

template <typename... Args>
struct format_string {
public:
    // must be constructed at compile time, to ensure the format string is valid
    consteval format_string(const char *fmt);
    constexpr auto get_format() const -> std::string_view {
        return fmt_str;
    }
    constexpr auto get_index() const -> std::span<const format_info> {
        return fmt_idx;
    }

private:
    inline static constexpr auto Nm = sizeof...(Args);
    std::string_view fmt_str;            // the format string
    std::array<format_info, Nm> fmt_idx; // where are the specifiers
};

constexpr auto find_specifier_impl(sv_t &fmt) -> bool {
    do {
        if (const auto next = fmt.find('%'); next == sv_t::npos) {
            return false;
        } else if (next + 1 == fmt.size()) {
            throw format_error{"missing specifier after '%'"};
        } else if (fmt[next + 1] == '%') {
            // escape the specifier
            fmt.remove_prefix(next + 2);
        } else {
            fmt.remove_prefix(next + 1);
            return true;
        }
    } while (true);
};

template <typename... Args>
constexpr auto compile_time_format_check(sv_t fmt, std::span<format_info> idx) -> void {
    std::size_t n = 0;
    auto parse_fn = [&](auto parser) {
        sv_t current_fmt = fmt;
        if (!find_specifier_impl(current_fmt)) {
            throw format_error{"too few specifiers"};
        }

        const auto position = static_cast<std::size_t>(current_fmt.data() - fmt.data() - 1);
        const auto consumed = parser.parse(current_fmt);

        idx[n++] = {
            .position = position,
            .consumed = consumed,
        };

        fmt = current_fmt;
        fmt.remove_prefix(consumed);
    };

    (parse_fn(formatter<Args>{}), ...);
    if (find_specifier_impl(fmt)) // extra specifier found
        throw format_error{"too many specifiers"};
}

template <typename... Args>
consteval format_string<Args...>::format_string(const char *fmt) :
    fmt_str(fmt), fmt_idx() {
    compile_time_format_check<Args...>(fmt_str, fmt_idx);
}

// Specialization for String-like types
template <typename StrLike>
    requires(
        std::same_as<StrLike, std::string> ||
        std::same_as<StrLike, std::string_view> ||
        std::same_as<StrLike, char *> ||
        std::same_as<StrLike, const char *> ||
        std::is_array_v<StrLike>
    )
struct formatter<StrLike> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        if (fmt.starts_with("s")) return 1;
        if (fmt.starts_with("_")) return 1;
        throw format_error{"invalid specifier for string"};
    }
    static auto format_to(std::ostream &os, const StrLike &val, sv_t spec) -> void {
        os << val;
    }
};

// Specialization for Signed Integers
template <typename Int>
    requires(std::integral<Int> && std::is_signed_v<Int> && !std::same_as<Int, bool>)
struct formatter<Int> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        if (fmt.starts_with("d")) return 1;
        if (fmt.starts_with("_")) return 1;
        throw format_error{"invalid specifier for signed integer"};
    }
    static auto format_to(std::ostream &os, const Int &val, sv_t spec) -> void {
        os << static_cast<int64_t>(val);
    }
};

// Specialization for Unsigned Integers
template <typename UInt>
    requires(std::integral<UInt> && std::is_unsigned_v<UInt> && !std::same_as<UInt, bool>)
struct formatter<UInt> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        if (fmt.starts_with("u")) return 1;
        if (fmt.starts_with("_")) return 1;
        throw format_error{"invalid specifier for unsigned integer"};
    }
    static auto format_to(std::ostream &os, const UInt &val, sv_t spec) -> void {
        os << static_cast<uint64_t>(val);
    }
};

// Specialization for Vector
template <typename T>
struct formatter<std::vector<T>> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        if (fmt.starts_with("_")) return 1;
        throw format_error{"invalid specifier for vector"};
    }
    static auto format_to(std::ostream &os, const std::vector<T> &val, sv_t spec) -> void {
        os << "[";
        for (std::size_t i = 0; i < val.size(); ++i) {
            formatter<std::decay_t<T>>::format_to(os, val[i], "_");
            if (i + 1 < val.size()) os << ",";
        }
        os << "]";
    }
};

template <typename... Args>
using format_string_t = format_string<std::decay_t<Args>...>;

template <typename... Args>
inline auto printf(format_string_t<Args...> fmt, const Args &...args) -> void {
    sv_t fmt_str = fmt.get_format();
    std::size_t current = 0;
    auto process_arg = [&](const auto &arg) {
        std::size_t next_percent = fmt_str.find('%', current);
        while (next_percent != sv_t::npos && next_percent + 1 < fmt_str.size() && fmt_str[next_percent + 1] == '%') {
            std::cout << fmt_str.substr(current, next_percent - current + 1);
            current = next_percent + 2;
            next_percent = fmt_str.find('%', current);
        }

        std::cout << fmt_str.substr(current, next_percent - current);
        current = next_percent + 1;

        sv_t spec = fmt_str.substr(current);
        std::size_t consumed = formatter<std::decay_t<decltype(arg)>>::parse(spec);
        formatter<std::decay_t<decltype(arg)>>::format_to(std::cout, arg, spec.substr(0, consumed));
        current += consumed;
    };

    (process_arg(args), ...);

    while (true) {
        std::size_t next_percent = fmt_str.find('%', current);
        if (next_percent == sv_t::npos) {
            std::cout << fmt_str.substr(current);
            break;
        }
        std::cout << fmt_str.substr(current, next_percent - current);
        if (next_percent + 1 < fmt_str.size() && fmt_str[next_percent + 1] == '%') {
            std::cout << "%";
            current = next_percent + 2;
        } else {
            current = next_percent + 1;
        }
    }
}

} // namespace sjtu
