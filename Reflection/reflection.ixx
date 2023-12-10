module;
#include <iostream>

export module helpers.reflection;
import <string_view>;
import <array>;
import <source_location>;
import <type_traits>;

template <typename Enum, Enum e>
consteval auto get_enum_full_name()
{
    return std::source_location::current().function_name();
}

consteval std::string_view get_enum_filtered_name(auto callable)
{
    constexpr auto result = callable();
    constexpr std::string_view str = get_enum_full_name<decltype(result), result>();
    constexpr auto start_index = (str.find_last_of(",") == std::string_view::npos) ? str.find_last_of(";") : str.find_last_of(",");

    if (start_index == std::string_view::npos)
        return "";

    constexpr auto end_index = (str.find_last_of(">") == std::string_view::npos) ? str.find_last_of("]") : str.find_last_of(">");

    if (end_index == std::string_view::npos)
        return "";

    constexpr auto window = str.substr(start_index + 1, end_index - start_index - 1);
    if (window.find("(") != std::string_view::npos)
        return "";

    // return value from :: to end
    constexpr auto new_start_index = window.find_last_of("::") + 1;
    return window.substr(new_start_index);
}

template <typename T, int val = 0>
consteval auto get_enum_array()
{
    constexpr auto result = get_enum_filtered_name([]() constexpr { return (T)val; });
    if constexpr (result == "")
        return std::array<std::string_view, 0>{};
    else
    {
        constexpr auto res = get_enum_array<T, val + 1>();
        std::array<std::string_view, res.size() + 1> ans;
        ans[0] = result;
        for (int i = 0; i < res.size(); ++i)
            ans[i + 1] = res[i];
        return ans;
    }
}

template <typename T> requires std::is_enum_v<T>
constexpr std::string_view get_token_string(T token)
{
    static auto mapping = get_enum_array<T>();
    return mapping[(int)token];
}

export template <typename T> requires std::is_enum_v<T>
std::ostream& operator<<(std::ostream& out, const T& token)
{
    out << get_token_string(token);
    return out;
}

// TESTING
enum class TestEnum
{
    A, B, C
};

static_assert(get_enum_array<TestEnum>().size() == 3);