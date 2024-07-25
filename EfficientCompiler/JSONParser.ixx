export module JSONParser;

import std;
import compiler;
import helpers;

using namespace std;

namespace JSONParser
{
    enum class Terminal
    {
        eps,
        TRUE,
        FALSE,
        NULL,
        NUMBER,
        STRING,
        OBJ_START,
        OBJ_END,
        ARR_START,
        ARR_END,
        COLON,
        COMMA,
        WHITESPACE,
        TK_EOF
    };

    enum class NonTerminal
    {
        start,
        _json,
        json_object_start,
        json_object_member_start,
        json_object_member_end,
        json_array_start,
        json_array_member_start,
        json_array_member_end
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
                    TransitionInfo{0, 1, " \n\r\t"},
                    TransitionInfo{0, 2, "t"},
                    TransitionInfo{2, 3, "r"},
                    TransitionInfo{3, 4, "u"},
                    TransitionInfo{4, 5, "e"},
                    TransitionInfo{0, 6, "f"},
                    TransitionInfo{6, 7, "a"},
                    TransitionInfo{7, 8, "l"},
                    TransitionInfo{8, 9, "s"},
                    TransitionInfo{9, 10, "e"},
                    TransitionInfo{0, 11, "n"},
                    TransitionInfo{11, 12, "u"},
                    TransitionInfo{12, 13, "l"},
                    TransitionInfo{13, 14, "l"},
                    TransitionInfo{0, 15, "{"},
                    TransitionInfo{0, 16, "}"},
                    TransitionInfo{0, 17, "["},
                    TransitionInfo{0, 18, "]"},
                    TransitionInfo{0, 19, ":"},
                    TransitionInfo{0, 20, ","},
                    TransitionInfo{0, 21, "\""},
                    TransitionInfo{21, -1, "\\\"", 21},
                    TransitionInfo{21, 22, "\\"},
                    TransitionInfo{22, 21, "\"\\/bfnrt"},
                    TransitionInfo{21, 23, "\""},
                    TransitionInfo{0, 24, "-"},
                    TransitionInfo{0, 25, "0123456789"},
                    TransitionInfo{24, 25, "0123456789"},
                    TransitionInfo{25, 25, "0123456789"},
                    TransitionInfo{25, 26, "."},
                    TransitionInfo{26, 27, "0123456789"},
                    TransitionInfo{27, 27, "0123456789"},
                    TransitionInfo{25, 28, "eE"},
                    TransitionInfo{27, 28, "eE"},
                    TransitionInfo{28, 29, "0123456789"},
                    TransitionInfo{28, 30, "+-"},
                    TransitionInfo{29, 29, "0123456789"},
                    TransitionInfo{29, 30, "+-"},
                    TransitionInfo{30, 31, "0123456789"},
                    TransitionInfo{31, 31, "0123456789"}
                };
            };

        constexpr auto final_states = []()
            {
                return array
                {
                    FinalStateInfo{1, WHITESPACE},
                    FinalStateInfo{5, TRUE},
                    FinalStateInfo{10, FALSE},
                    FinalStateInfo{14, NULL},
                    FinalStateInfo{15, OBJ_START},
                    FinalStateInfo{16, OBJ_END},
                    FinalStateInfo{17, ARR_START},
                    FinalStateInfo{18, ARR_END},
                    FinalStateInfo{19, COLON},
                    FinalStateInfo{20, COMMA},
                    FinalStateInfo{23, STRING},
                    FinalStateInfo{25, NUMBER},
                    FinalStateInfo{27, NUMBER},
                    FinalStateInfo{29, NUMBER},
                    FinalStateInfo{31, NUMBER},
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
                    PI(start, _json, TK_EOF),
                    PI(_json, TRUE),
                    PI(_json, FALSE),
                    PI(_json, NULL),
                    PI(_json, STRING),
                    PI(_json, NUMBER),
                    PI(_json, OBJ_START, json_object_start),
                    PI(_json, ARR_START, json_array_start),
                    PI(json_object_start, OBJ_START, json_object_member_start),
                    PI(json_object_member_start, STRING, COLON, _json, json_object_member_end),
                    PI(json_object_member_end, COMMA, json_object_member_start),
                    PI(json_object_member_end, OBJ_END),
                    PI(json_array_start, ARR_START, json_array_member_start),
                    PI(json_array_member_start, _json, json_array_member_end),
                    PI(json_array_member_end, COMMA, json_array_member_start),
                    PI(json_array_member_end, ARR_END)
                };
            }, []() { return JSONParser::get_lexer(); });
    }
}