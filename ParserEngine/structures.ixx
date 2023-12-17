export module parser.structures;

import helpers.flatmap;
import <string_view>;
import <type_traits>;
import <array>;
import <concepts>;

export template<typename T>
concept token_type = requires(T t)
{
    std::is_enum_v<T>;
    { T::UNINITIALISED } -> std::convertible_to<T>;
    { T::TK_NEWLINE } -> std::convertible_to<T>;
    { T::TK_SYMBOL } -> std::convertible_to<T>;
    { T::TK_EOF } -> std::convertible_to<T>;
    { T::TK_ERROR_SYMBOL } -> std::convertible_to<T>;
    { T::TK_ERROR_PATTERN } -> std::convertible_to<T>;
    { T::TK_ERROR_LENGTH } -> std::convertible_to<T>;
};

export template<class T>
concept lexer_token = requires(T t, const T& u)
{
    requires token_type<decltype(t.type)>;
    { t.lexeme } -> std::convertible_to<std::string_view>;
    { t.afterConstruction(u) } -> std::same_as<void>;
};

export struct TransitionInfo
{
    int from{};
    int to{};
    std::string_view pattern{};
    int default_transition_state{ -1 };
};

export template <token_type TokenType>
struct FinalStateInfo
{
    int state_no;
    TokenType token_type;
};

export template <token_type TokenType>
struct KeywordInfo
{
    std::string_view keyword;
    TokenType token_type;
};