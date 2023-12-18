export module parser.structures;

import helpers.flatmap;
import <string_view>;
import <type_traits>;
import <array>;
import <concepts>;
import <variant>;

export enum class TokenErrors;

export template<typename T>
concept user_token_type = requires(T t)
{
    requires std::is_enum_v<T>;
    { T::TK_SYMBOL };
};

export template<typename T, typename UserTokenType = std::variant_alternative_t<0, T>>
concept token_type = requires(T t)
{
    requires user_token_type<UserTokenType>;
    requires !std::is_same_v<UserTokenType, TokenErrors>;
    requires std::is_same_v<std::variant_alternative_t<1, T>, TokenErrors>;
};

export template<class T>
concept lexer_token = requires(T t, const T& u)
{
    requires token_type<decltype(t.type)>;
    { t.lexeme } -> std::convertible_to<std::string_view>;
    { t.afterConstruction(u) } -> std::same_as<void>;
};

enum class TokenErrors
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

export template <user_token_type UserTokenType>
struct FinalStateInfo
{
    int state_no;
    UserTokenType token_type;
};

export template <user_token_type UserTokenType>
struct KeywordInfo
{
    std::string_view keyword;
    UserTokenType token_type;
};