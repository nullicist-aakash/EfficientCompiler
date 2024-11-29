#pragma once
#include <variant>

namespace helpers::internal
{
    template <typename T, typename... Ts>
    struct unique { using type = T; };

    template <typename... Ts, typename U, typename... Us>
    struct unique<std::variant<Ts...>, U, Us...>
            : std::conditional_t<(std::is_same_v<U, Ts> || ...),
                    unique<std::variant<Ts...>, Us...>,
                    unique<std::variant<Ts..., U>, Us...>> {};
}

namespace helpers
{
    using namespace helpers::internal;

    template <typename... Ts>
    using variant_unique = typename unique<
            std::variant<>,
            std::conditional_t<std::is_same_v<Ts, void>, std::monostate, Ts>...>::type;


    template <typename ostream, typename A, typename... Types>
    constexpr ostream& operator<<(ostream& out, const std::variant<A, Types...>& vr)
    {
        std::visit([&](auto&& arg) { out << arg; }, vr);
        return out;
    }

    template <typename Type, typename... Types>
    constexpr bool operator==(const std::variant<Types...>& lhs, const Type& rhs)
    {
        return std::holds_alternative<Type>(lhs) && std::get<Type>(lhs) == rhs;
    }

    template <typename Type, typename... Types>
    constexpr bool operator==(const Type& lhs, const std::variant<Types...>& rhs)
    {
        return rhs == lhs;
    }

    template <typename Type, typename... Types>
    constexpr bool operator!=(const std::variant<Types...>& lhs, const Type& rhs)
    {
        return !(lhs == rhs);
    }

    template <typename Type, typename... Types>
    constexpr bool operator!=(const Type& lhs, const std::variant<Types...>& rhs)
    {
        return !(rhs == lhs);
    }
}