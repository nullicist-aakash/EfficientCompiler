export module RegexParser:lexer;
import :structures;

import <array>;

import compiler;

namespace RegexParser
{
    export constexpr auto get_lexer()
    {
        using enum Terminal;

        constexpr auto transitions = []()
            {
                return std::array
                {
                    TransitionInfo{.from = 0, .to = -1, .pattern = R"(|()*+[]-^?.\)", .default_transition_state = 1 },
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
                    TransitionInfo{0, 13, R"(\)"},
                    TransitionInfo{13, 14, R"(|()*+[]-^?.\nrt)"},
                    TransitionInfo{13, 15, "e"},
                };
            };

        constexpr auto final_states = []()
            {
                return std::array
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