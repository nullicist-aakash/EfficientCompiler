export module helpers.checks;

import <string_view>;
import <array>;
import <algorithm>;

struct oversized_array
{
    std::array<char, 1024> data{};
    std::size_t size{};
};

consteval auto to_oversized_array(std::string_view sv)
{
    oversized_array result{};
    std::copy(sv.begin(), sv.end(), result.data.begin());
    result.size = sv.size() + 1;
    return result;
}

consteval auto to_right_sized_array(auto callable)
{
    constexpr auto oversized = to_oversized_array(callable());
    std::array<char, oversized.size> result{};
    std::copy(oversized.data.begin(), oversized.data.begin() + oversized.size, result.begin());
    return result;
}

template <auto Data>
consteval void cc_check()
{
    constexpr auto res = std::string_view(Data.data(), Data.size());
    static_assert(res[0] == '\0', "Compile-time check failed");
}

export consteval auto ct_assert(auto callable)
{
    cc_check<to_right_sized_array(callable)>();
}