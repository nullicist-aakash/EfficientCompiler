import <string_view>;
import <array>;
import <format>;

import parser.dfa;
import helpers.reflection;
using std::string_view;
using std::array;

#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
using namespace std;

enum class TokenType
{
    UNINITIALISED,
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
    TK_KBD,
    TK_EOF,
    TK_ERROR_SYMBOL,
    TK_ERROR_PATTERN,
    TK_ERROR_LENGTH
};

struct Token
{
    TokenType type = TokenType::UNINITIALISED;
    int line_number = 0;
    std::string_view lexeme{};

    friend std::ostream& operator<<(std::ostream& out, const Token& tk)
    {
        out << tk.type << " " << tk.line_number << " " << tk.lexeme << std::endl;
        return out;
    }
};

template <typename TokenType, int num_states, int num_keywords>
class Lexer
{
    const DFA<TokenType, num_states, num_keywords>& dfa;
    const string_view source_code;
    const int symbol_length_limit;
public:
    struct sentinel {};

    struct iterator
    {
        const Lexer& lexer;
        std::unique_ptr<Token> token;
        int line_number = 1;
        int cur_position;

        constexpr auto get_token_from_dfa()
        {
            using State = int;
            struct Status
            {
                State final_dfa_state;
                int final_state_code_pos;

                State cur_dfa_state;
                int cur_code_position;
            };

            Status status{ .final_dfa_state = -1, .final_state_code_pos = -1, .cur_dfa_state = 0, .cur_code_position = cur_position };

            // We reached End of input
            if (status.cur_code_position >= lexer.source_code.size())
				return std::make_unique<Token>(Token
                    {
						.type = TokenType::TK_EOF,
						.line_number = line_number,
						.lexeme = ""
					});

            while (status.cur_code_position < lexer.source_code.size())
            {
                auto cur_symbol = lexer.source_code[status.cur_code_position];
                State next_dfa_state = lexer.dfa.productions[status.cur_dfa_state][cur_symbol];

                if (next_dfa_state == -1)
                    break;

                if (lexer.dfa.final_states[next_dfa_state] != TokenType::UNINITIALISED)
                {
                    status.final_dfa_state = next_dfa_state;
                    status.final_state_code_pos = status.cur_code_position;
                }

                status.cur_dfa_state = next_dfa_state;
                status.cur_code_position++;
            }

            // We didn't move at all
            if (status.cur_code_position == this->cur_position)
                return std::make_unique<Token>(Token
                    {
                        .type = TokenType::TK_ERROR_SYMBOL,
                        .line_number = this->line_number,
                        .lexeme = this->lexer.source_code.substr(this->cur_position++, 1)
                    });

            // We moved somewhere but didn't reach any final state
            if (status.final_dfa_state == -1)
            {
                auto start = this->cur_position++;
                auto len = status.cur_code_position - start + 1;
                return std::make_unique<Token>(Token
                    {
                        .type = TokenType::TK_ERROR_PATTERN,
                        .line_number = this->line_number,
                        .lexeme = this->lexer.source_code.substr(start, len)
                    });
            }

            // We return for the last seen final state
            int start = cur_position;
            int len = status.final_state_code_pos - start + 1;
            this->cur_position += len;
            auto ptr = std::make_unique<Token>(Token
                {
                    .type = lexer.dfa.final_states[status.final_dfa_state],
                    .line_number = line_number,
                    .lexeme = lexer.source_code.substr(start, len)
                });

            if (ptr->type == TokenType::TK_NEWLINE)
				line_number++;
            else if (ptr->type == TokenType::TK_SYMBOL)
            {
                if (lexer.dfa.keyword_to_token.exists(ptr->lexeme))
                    ptr->type = lexer.dfa.keyword_to_token.at(ptr->lexeme);

                if (ptr->lexeme.size() > lexer.symbol_length_limit)
                    ptr->type = TokenType::TK_ERROR_LENGTH;
            }

            return ptr;
        }

        constexpr iterator(Lexer& l) : lexer{ l }, cur_position{0}
        {
            this->token = get_token_from_dfa();
        }
        constexpr bool operator!=(sentinel) const { return this->token->type != TokenType::TK_EOF; }
        constexpr iterator& operator++() { this->token = get_token_from_dfa(); return *this; }
        constexpr auto operator*() 
        {
            return std::move(token);
        }
    };

    constexpr iterator begin() { return iterator{ *this }; }
    constexpr sentinel end() { return {}; }

    constexpr Lexer(DFA<TokenType, num_states, num_keywords>& dfa, string_view sc, int symbol_size_limit = 50) 
        : dfa{ dfa }, source_code{ sc }, symbol_length_limit{ symbol_length_limit } {}
};

consteval auto fetch_dfa()
{
    struct TransitionInfo
    {
        int from{};
        int to{};
        string_view pattern{};
        int default_transition_state{ -1 };
    };

    struct FinalStateInfo
    {
        int state_no;
        TokenType token_type;
    };

    struct KeywordInfo
    {
        string_view keyword;
        TokenType token_type;
    };

    using enum TokenType;

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
                KeywordInfo{ "D", TK_D },
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

    return get_dfa(transitions, final_states, keywords);
}

int main()
{
    auto dfa = fetch_dfa();
    cout << dfa << endl;

    ifstream file{ "source.jack" };
    ostringstream ss;
    ss << file.rdbuf(); // reading data
    string str = ss.str();

    Lexer l{ dfa, str };
    for (auto x = l.begin(); x != l.end(); ++x)
    {
        auto ptr = std::move(*x);
        cout << *ptr << endl;
    }
}