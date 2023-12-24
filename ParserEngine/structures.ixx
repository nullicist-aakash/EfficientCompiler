export module parser.structures;

import helpers.flatmap;
import <string_view>;
import <type_traits>;
import <array>;
import <concepts>;
import <variant>;

export enum class ErrorTokenType;

export template<typename T>
concept is_user_token_type = requires(T t)
{
    requires std::is_enum_v<T>;
    { T::TK_SYMBOL };
};

export template<typename T, typename UserTokenType = std::variant_alternative_t<0, T>>
concept is_token_type = requires(T t)
{
    requires is_user_token_type<UserTokenType>;
    requires !std::is_same_v<UserTokenType, ErrorTokenType>;
    requires std::is_same_v<std::variant_alternative_t<1, T>, ErrorTokenType>;
};

export template<class T>
concept is_lexer_token = requires(T t, const T& u)
{
    requires is_token_type<decltype(t.type)>;
    { t.lexeme } -> std::convertible_to<std::string_view>;
    { t.afterConstruction(u) } -> std::same_as<void>;
};

enum class ErrorTokenType
{
    UNINITIALISED,
    TK_ERROR_SYMBOL,
    TK_ERROR_PATTERN,
    TK_EOF
};

export struct TransitionInfo
{
    int from{};
    int to{};
    std::string_view pattern{};
    int default_transition_state{ -1 };
};

export template <is_user_token_type UserTokenType>
struct FinalStateInfo
{
    int state_no;
    UserTokenType token_type;
};

export template <is_user_token_type UserTokenType>
struct KeywordInfo
{
    std::string_view keyword;
    UserTokenType token_type;
};