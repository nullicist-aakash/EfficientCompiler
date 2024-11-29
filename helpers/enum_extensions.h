#pragma once
#include <concepts>
#include <type_traits>
#include <source_location>
#include <string_view>
#include <array>

namespace helpers::internal
{
    template <typename T>
    concept CEnum = requires(T t)
    {
        requires std::is_enum_v<T>;
    };

    template <CEnum Enum, Enum e>
    consteval auto get_enum_full_name() noexcept
    {
        return std::source_location::current().function_name();
    }

    template <CEnum Enum, Enum e>
    consteval std::string_view get_enum_filtered_name() noexcept
    {
        constexpr std::string_view str = get_enum_full_name<Enum, e>();
        constexpr int new_start_index = str.find_last_of(':');

        if (new_start_index == std::string_view::npos)
            return "";

        constexpr auto window = str.substr(new_start_index + 1, str.size() - new_start_index - 2);

        if (window.contains(')'))
            return "";

        return window;
    }

    template <CEnum Enum, int index = 0>
    consteval auto get_enum_array() noexcept
    {
        constexpr auto result = get_enum_filtered_name<Enum, (Enum)index>();
        if constexpr (result == "")
            return std::array<std::string_view, 0>{};
        else
        {
            constexpr auto res = get_enum_array<Enum, index + 1>();
            std::array<std::string_view, res.size() + 1> ans;
            ans[0] = result;
            for (size_t i = 0; i < res.size(); ++i)
                ans[i + 1] = res[i];
            return ans;
        }
    }
}

namespace helpers
{
    using namespace helpers::internal;

    constexpr auto get_enum_string(CEnum auto en) noexcept -> std::string_view
    {
        constexpr auto mapping = helpers::internal::get_enum_array<decltype(en)>();
        return mapping[(int)en];
    }

    template <CEnum Enum>
    consteval auto get_enum_count() noexcept -> std::size_t
    {
        return helpers::internal::get_enum_array<Enum>().size();
    }

    template <typename ostream, CEnum Enum>
    constexpr ostream& operator<<(ostream& out, const Enum& en) noexcept
    {
        out << get_enum_string(en);
        return out;
    }
}

// Compile time test
namespace helpers::tests
{
    using namespace helpers;
    enum class TestEnum
    {
        Test1,
        Test2,
        Test3
    };

    static_assert(get_enum_count<TestEnum>() == 3);
    static_assert(get_enum_string(TestEnum::Test1) == "Test1");
    static_assert(get_enum_string(TestEnum::Test2) == "Test2");
    static_assert(get_enum_string(TestEnum::Test3) == "Test3");
}