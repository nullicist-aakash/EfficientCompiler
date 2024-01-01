export module RegexParser;

import <algorithm>;
import <array>;
import <string_view>;
import <vector>;
import <variant>;
import <ranges>;
import <memory>;
import <cassert>;

import compiler;
import helpers;

using namespace std;

namespace RegexParser
{
    enum class Terminal
    {
        eps,
        TK_EOF,

        CHAR,
        EMPTY,

        OR,
        STAR,
        PLUS,
        DOT,

        BRACKET_OPEN,
        BRACKET_CLOSE,
        CLASS_OPEN,
        CLASS_CLOSE,

        QUESTION_MARK,
        CARET,
        MINUS,

        ESCAPED_CHAR,
    };

    enum class NonTerminal
    {
        start
    };

    struct LexerToken
    {
        std::variant<Terminal, ELexerError> type = ELexerError::UNINITIALISED;
        std::string_view lexeme{};

        constexpr void after_construction(const LexerToken& previous_token)
        {

        }

        constexpr bool discard() const
        {
            return false;
        }

        template <typename T>
        friend constexpr T& operator<<(T& out, const LexerToken& tk)
        {
            return out << "{ type: " << tk.type << ", lexeme: '" << tk.lexeme << "' }";
        }
    };

    export constexpr auto get_lexer()
    {
        using enum Terminal;

        constexpr auto transitions = []()
            {
                return array
                {
                    TransitionInfo{.from = 0, .to = -1, .pattern = "|()*+[]-^?.\\", .default_transition_state = 1 },
                    TransitionInfo{0, 2, "|"},
                    TransitionInfo{0, 3, "("},
                    TransitionInfo{0, 4, ")"},
                    TransitionInfo{0, 5, "*"},
                    TransitionInfo{0, 6, "+"},
                    TransitionInfo{0, 7, "["},
                    TransitionInfo{0, 8, "]"},
                    TransitionInfo{0, 9, "-"},
                    TransitionInfo{0, 10, "^"},
                    TransitionInfo{0, 11, "?"},
                    TransitionInfo{0, 12, "."},
                    TransitionInfo{0, 13, "\\"},
                    TransitionInfo{13, 14, "|()*+[]-^?.\\"},
                    TransitionInfo{13, 15, "e"},
                };
            };

        constexpr auto final_states = []()
            {
                return array
                {
                    FinalStateInfo{1, CHAR},
                    FinalStateInfo{2, OR},
                    FinalStateInfo{3, BRACKET_OPEN},
                    FinalStateInfo{4, BRACKET_CLOSE},
                    FinalStateInfo{5, STAR},
                    FinalStateInfo{6, PLUS},
                    FinalStateInfo{7, CLASS_OPEN},
                    FinalStateInfo{8, CLASS_CLOSE},
                    FinalStateInfo{9, MINUS},
                    FinalStateInfo{10, CARET},
                    FinalStateInfo{11, QUESTION_MARK},
                    FinalStateInfo{12, DOT},
                    FinalStateInfo{13, CHAR},
                    FinalStateInfo{14, ESCAPED_CHAR},
                    FinalStateInfo{15, EMPTY},
                };
            };

        return build_lexer<LexerTypes<LexerToken>>(transitions, final_states);
    }
}