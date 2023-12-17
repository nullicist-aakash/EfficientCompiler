export module parser.structures;

import helpers.flatmap;
import <string_view>;
import <type_traits>;
import <array>;

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

export template <token_type TokenType>
struct LexerToken
{
    TokenType type = TokenType::UNINITIALISED;
    std::string_view lexeme{};
    std::size_t line_number = 0;
};

export template <
    token_type TokenType,
    int num_states
>
struct DFA
{
    static const int state_count = num_states;
    std::array<std::array<int, 128>, num_states> productions{};
    std::array<TokenType, num_states> final_states{};
};

export template <token_type TokenType, int num_states, int num_keywords>
struct Lexer
{
    DFA<TokenType, num_states> dfa{};
    flatmap<std::string_view, TokenType, num_keywords> keyword_to_token{};
};