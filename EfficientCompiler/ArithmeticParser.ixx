export module ArithmeticParser;

import <algorithm>;
import <array>;
import <string_view>;
import <vector>;
import <variant>;
import <ranges>;

import compiler;
import helpers.extensions;

using namespace std;

enum class ATerminal
{
    eps,
    IDENTIFIER,
    TK_EOF,

    NUMBER,
    PLUS,
    WHITESPACE,
};

enum class ANonTerminal
{
    start,
    expression
};

struct ALexerToken
{
    std::variant<ATerminal, ELexerError> type = ELexerError::UNINITIALISED;
    std::string_view lexeme{};

    constexpr void after_construction(const ALexerToken& previous_token)
    {

    }

    constexpr bool discard() const
    {
        return type == ATerminal::WHITESPACE;
    }

    template <typename T>
    friend constexpr T& operator<<(T& out, const ALexerToken& tk)
    {
        return out << "{ type: " << tk.type << ", lexeme: '" << tk.lexeme << "' }";
    }
};

static consteval auto get_Alexer()
{
    using enum ATerminal;

    constexpr auto transitions = []()
        {
            return array
            {
                TransitionInfo{0, 1, "0123456789"},
                TransitionInfo{1, 1, "0123456789"},
                TransitionInfo{0, 2, "+"},
                TransitionInfo{0, 3, " \t\r\n"},
                TransitionInfo{3, 3, " \t\r\n"},
            };
        };

    constexpr auto final_states = []()
        {
            return array
            {
                FinalStateInfo{1, NUMBER},
                FinalStateInfo{2, PLUS},
                FinalStateInfo{3, WHITESPACE},
            };
        };

    constexpr auto keywords = []()
        {
            return array
            {
                KeywordInfo{ "ONE", NUMBER },
            };
        };

    return build_lexer<LexerTypes<ALexerToken>>(transitions, final_states, keywords);
}

export consteval auto get_arithmetic_parser()
{
    using enum ANonTerminal;
    using enum ATerminal;
    using PI = ProductionInfo<LexerTypes<ALexerToken>, ANonTerminal, 5>;
    return build_parser([]() { return array{
        PI(start, NUMBER, expression, TK_EOF),
        PI(expression, eps),
        PI(expression, PLUS, NUMBER, expression),
        }; }, get_Alexer());
}