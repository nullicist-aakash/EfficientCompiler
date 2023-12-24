import <string_view>;
import <array>;
import <variant>;
import parser.lexer;
import parser.structures;
import helpers.reflection;

using std::string_view;
using std::array;

#include <numeric>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

enum class MyTokenType
{
    TK_OB,
    TK_CB,
    TK_ASSIGN,
    TK_PLUS,
    TK_MINUS,
    TK_AND,
    TK_OR,
    TK_NOT,
    TK_AT,
    TK_SEMICOLON,
    TK_COMMENT,
    TK_WHITESPACE,
    TK_NEWLINE,
    TK_NUM,
    TK_SYMBOL,
    TK_M,
    TK_D,
    TK_MD,
    TK_A,
    TK_AM,
    TK_AD,
    TK_AMD,
    TK_JGT,
    TK_JEQ,
    TK_JGE,
    TK_JLT,
    TK_JNE,
    TK_JLE,
    TK_JMP,
    TK_SP,
    TK_LCL,
    TK_ARG,
    TK_THIS,
    TK_THAT,
    TK_REG,
    TK_SCREEN,
    TK_KBD
};

struct MyLexerToken
{
    variant<MyTokenType, TokenErrors> type = TokenErrors::UNINITIALISED;
    std::string_view lexeme{};
    int line_num{1};

    constexpr void afterConstruction(const MyLexerToken& previous_token)
    {
        if (holds_alternative<TokenErrors>(previous_token.type))
            return;

        if (get<MyTokenType>(previous_token.type) == MyTokenType::TK_NEWLINE)
            line_num = previous_token.line_num + 1;
        else
            line_num = previous_token.line_num;
    }
};

template <typename T>
constexpr T& operator<<(T& out, const MyLexerToken& tk)
{
    out << tk.line_num << " " << tk.lexeme;
    return out;
}

static consteval auto get_lexer()
{
    using enum MyTokenType;

    constexpr auto transitions = []()
        {
            return array
            {
                TransitionInfo{0, 1, "("},
                TransitionInfo{0, 2, ")"},
                TransitionInfo{0, 3, "="},
                TransitionInfo{0, 4, "+"},
                TransitionInfo{0, 5, "-"},
                TransitionInfo{0, 6, "&"},
                TransitionInfo{0, 7, "|"},
                TransitionInfo{0, 8, "!"},
                TransitionInfo{0, 9, "@"},
                TransitionInfo{0, 10, ";"},
                TransitionInfo{0, 11, "/"},
                TransitionInfo{0, 13, " \t\r"},
                TransitionInfo{0, 14, "0123456789"},
                TransitionInfo{0, 15, "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM_.$:"},
                TransitionInfo{11, 12, "/"},
                TransitionInfo{13, 13, " \t\r"},
                TransitionInfo{14, 14, "0123456789"},
                TransitionInfo{15, 15, "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM_.$:0123456789"},
                TransitionInfo{0, 16, "\n"},
                TransitionInfo{.from = 12, .to = -1, .pattern = "\n", .default_transition_state = 12}
            };
        };

    constexpr auto final_states = []()
        {
            return array
            {
                FinalStateInfo{1, TK_OB},
                FinalStateInfo{2, TK_CB},
                FinalStateInfo{3, TK_ASSIGN},
                FinalStateInfo{4, TK_PLUS},
                FinalStateInfo{5, TK_MINUS},
                FinalStateInfo{6, TK_AND},
                FinalStateInfo{7, TK_OR},
                FinalStateInfo{8, TK_NOT},
                FinalStateInfo{9, TK_AT},
                FinalStateInfo{10, TK_SEMICOLON},
                FinalStateInfo{12, TK_COMMENT},
                FinalStateInfo{13, TK_WHITESPACE},
                FinalStateInfo{14, TK_NUM},
                FinalStateInfo{15, TK_SYMBOL},
                FinalStateInfo{16, TK_NEWLINE}
            };
        };

    constexpr auto keywords = []()
        {
            return array
            {
                KeywordInfo{ "M", TK_M },
                KeywordInfo{"D", TK_D},
                KeywordInfo{ "MD", TK_MD },
                KeywordInfo{ "A", TK_A },
                KeywordInfo{ "AM", TK_AM },
                KeywordInfo{ "AD", TK_AD },
                KeywordInfo{ "AMD", TK_AMD },
                KeywordInfo{ "JGT", TK_JGT },
                KeywordInfo{ "JEQ", TK_JEQ },
                KeywordInfo{ "JGE", TK_JGE },
                KeywordInfo{ "JLT", TK_JLT },
                KeywordInfo{ "JNE", TK_JNE },
                KeywordInfo{ "JLE", TK_JLE },
                KeywordInfo{ "JMP", TK_JMP },
                KeywordInfo{ "SP", TK_SP },
                KeywordInfo{ "LCL", TK_LCL },
                KeywordInfo{ "ARG", TK_ARG },
                KeywordInfo{ "THIS", TK_THIS },
                KeywordInfo{ "THAT", TK_THAT },
                KeywordInfo{ "R0", TK_REG },
                KeywordInfo{ "R1", TK_REG },
                KeywordInfo{ "R2", TK_REG },
                KeywordInfo{ "R3", TK_REG },
                KeywordInfo{ "R4", TK_REG },
                KeywordInfo{ "R5", TK_REG },
                KeywordInfo{ "R6", TK_REG },
                KeywordInfo{ "R7", TK_REG },
                KeywordInfo{ "R8", TK_REG },
                KeywordInfo{ "R9", TK_REG },
                KeywordInfo{ "R10", TK_REG },
                KeywordInfo{ "R11", TK_REG },
                KeywordInfo{ "R12", TK_REG },
                KeywordInfo{ "R13", TK_REG },
                KeywordInfo{ "R14", TK_REG },
                KeywordInfo{ "R15", TK_REG },
                KeywordInfo{ "SCREEN", TK_SCREEN },
                KeywordInfo{ "KBD", TK_KBD }
            };
        };

    return build_lexer<MyLexerToken>(transitions, final_states, keywords);
}

static auto read_file(string_view filename)
{
	ifstream file{ filename.data() };
	ostringstream ss;
	ss << file.rdbuf(); // reading data
	return ss.str();
}

int main()
{
    constexpr auto lexer = get_lexer();
    auto contents = read_file("source.jack");

    for (auto& x : lexer("Hello"))
        cout << x << endl;

    cout << lexer << endl;
}