export module helpers.checks;
import <string_view>;
import <array>;
import <algorithm>;

static consteval auto to_oversized_array(std::string_view sv)
{
    struct oversized_array
    {
        std::array<char, 1024> data;
        std::size_t size{};
    };

    oversized_array result{};
    std::copy(sv.begin(), sv.end(), result.data.begin());
    result.size = sv.size() + 1;
    return result;
}

template <auto Data>
static consteval auto cc_check()
{
    constexpr auto res = std::string_view(Data.data.data(), Data.size);
    static_assert(res[0] == '\0', "Compile-time check failed");
}

export constexpr void ct_assert(auto callable)
{
    cc_check<to_oversized_array(callable())>();
}