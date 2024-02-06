export module JackParser;

import std;
import compiler;
import helpers;

using namespace std;

namespace JackParser
{
    enum class Terminal
    {
        eps,
        NUM,
        PARENO,
        PARENC,
        PLUS,
        MINUS,
        MULT,
        DIV,
        WHITESPACE,
        TK_EOF
    };

    enum class NonTerminal
    {
        start,
        _class,
        expression,
        expression_suffix,
        term,
        term_sub_iden,
        op
    };

    struct LexerToken
    {
        std::variant<Terminal, ELexerError> type = ELexerError::UNINITIALISED;
        std::string_view lexeme{};
        int line_num{ 1 };

        constexpr void after_construction(const LexerToken& previous_token)
        {
            line_num = previous_token.line_num +
                (int)std::count(previous_token.lexeme.begin(), previous_token.lexeme.end(), '\n');
        }

        constexpr bool discard() const
        {
            return type == Terminal::WHITESPACE;
        }

        template <typename T>
        friend constexpr T& operator<<(T& out, const LexerToken& tk)
        {
            return out << "{ line_number: " << tk.line_num << ", type: " << tk.type << ", lexeme: '" << tk.lexeme << "' }";
        }
    };

    export constexpr auto get_lexer()
    {
        using enum Terminal;

        constexpr auto transitions = []()
            {
                return array
                {
                    TransitionInfo{0, 1, "("},
                    TransitionInfo{0, 2, ")"},
                    TransitionInfo{0, 3, "+"},
                    TransitionInfo{0, 4, "-"},
                    TransitionInfo{0, 5, "*"},
                    TransitionInfo{0, 6, "/"},
                    TransitionInfo{0, 7, "0123456789"},
                    TransitionInfo{7, 7, "0123456789"},
                    TransitionInfo{0, 8, " \r\t\n"},
                    TransitionInfo{8, 8, " \r\t\n"},
                };
            };

        constexpr auto final_states = []()
            {
                return array
                {
                    FinalStateInfo{1, PARENO},
                    FinalStateInfo{2, PARENC},
                    FinalStateInfo{3, PLUS},
                    FinalStateInfo{4, MINUS},
                    FinalStateInfo{5, MULT},
                    FinalStateInfo{6, DIV},
                    FinalStateInfo{7, NUM},
                    FinalStateInfo{8, WHITESPACE}
                };
            };

        return build_lexer<LexerTypes<LexerToken>>(transitions, final_states);
    }

    export constexpr auto get_parser()
    {
        using enum NonTerminal;
        using enum Terminal;
        using PI = ProductionInfo<LexerTypes<LexerToken>, NonTerminal, 30>;
        return build_parser([]()
            {
                return std::array
                {
                    PI(start, expression, TK_EOF),
                    PI(expression, term, expression_suffix),
                    PI(expression_suffix, op, term, expression_suffix),
                    PI(expression_suffix, eps),
                    PI(term, PARENO, expression, PARENC),
                    PI(term, MINUS, term),
                    PI(op, PLUS),
                    PI(op, MINUS),
                    PI(op, MULT),
                    PI(op, DIV)
                };
            }, []() { return JackParser::get_lexer(); });
    }
}