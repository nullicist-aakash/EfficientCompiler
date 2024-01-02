export module helpers:extensions;
import <string_view>;
import <array>;
import <source_location>;
import <type_traits>;
import <variant>;

template <typename T>
concept CEnum = requires(T t)
{
    requires std::is_enum_v<T>;
};

template <CEnum Enum, Enum e>
static consteval auto get_enum_full_name()
{
    return std::source_location::current().function_name();
}

template <CEnum Enum, Enum e>
static consteval std::string_view get_enum_filtered_name()
{
    constexpr std::string_view str = get_enum_full_name<Enum, e>();
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

template <CEnum Enum, int index = 0>
consteval auto get_enum_array()
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

export template <CEnum Enum>
constexpr auto get_enum_string(Enum en)
{
    constexpr auto mapping = get_enum_array<Enum>();
    return mapping[(int)en];
}

export template <typename ostream, CEnum Enum>
constexpr ostream& operator<<(ostream& out, const Enum& en)
{
    out << get_enum_string(en);
    return out;
}

export template <CEnum Enum>
consteval auto get_enum_size()
{
	return get_enum_array<Enum>().size();
}

template <typename T, typename... Ts>
struct unique { using type = T; };

template <typename... Ts, typename U, typename... Us>
struct unique<std::variant<Ts...>, U, Us...>
    : std::conditional_t<(std::is_same_v<U, Ts> || ...),
    unique<std::variant<Ts...>, Us...>,
    unique<std::variant<Ts..., U>, Us...>> {};

export template <typename... Ts>
using variant_unique = typename unique<
    std::variant<>,
    std::conditional_t<std::is_same_v<Ts, void>, std::monostate, Ts>...>::type;



export template <typename ostream, typename A, typename... Types>
constexpr ostream& operator<<(ostream& out, const std::variant<A, Types...>& vr)
{
    std::visit([&](auto&& arg) { out << arg; }, vr);
    return out;
}

export template <typename Type, typename... Types>
constexpr bool operator==(const std::variant<Types...>& lhs, const Type& rhs)
{
    return std::holds_alternative<Type>(lhs) && std::get<Type>(lhs) == rhs;
}

export template <typename Type, typename... Types>
constexpr bool operator==(const Type& lhs, const std::variant<Types...>& rhs)
{
    return rhs == lhs;
}

export template <typename Type, typename... Types>
constexpr bool operator!=(const std::variant<Types...>& lhs, const Type& rhs)
{
    return !(lhs == rhs);
}

export template <typename Type, typename... Types>
constexpr bool operator!=(const Type& lhs, const std::variant<Types...>& rhs)
{
    return !(rhs == lhs);
}