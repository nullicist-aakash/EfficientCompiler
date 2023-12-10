module;

#include <iostream>

export module parser.lexer;
export import parser.dfa;

import helpers.reflection;
import <string_view>;
import <memory>;

export template <typename TokenType> requires is_token_type<TokenType>
struct Token
{
    TokenType type = TokenType::UNINITIALISED;
    int line_number = 0;
    std::string_view lexeme{};
};

export template <typename TokenType> requires is_token_type<TokenType>
std::ostream& operator<<(std::ostream& out, const Token<TokenType>& tk)
{
    out << tk.type << " " << tk.line_number << " " << tk.lexeme << std::endl;
    return out;
}

using State = int;
struct Status
{
    State final_dfa_state;
    int final_state_code_pos;

    State cur_dfa_state;
    int cur_code_position;
};

template <typename TokenType, int num_states, int num_keywords> requires is_token_type<TokenType>
class Lexer;

struct sentinel {};

template <typename TokenType, int num_states, int num_keywords> requires is_token_type<TokenType>
class iterator
{
    const Lexer<TokenType, num_states, num_keywords>& lexer;
    std::unique_ptr<Token<TokenType>> token;
    int line_number = 1;
    int cur_position;

    constexpr auto get_token_from_dfa()
    {
        Status status{ .final_dfa_state = -1, .final_state_code_pos = -1, .cur_dfa_state = 0, .cur_code_position = cur_position };

        // We reached End of input
        if (status.cur_code_position >= lexer.source_code.size())
            return std::make_unique<Token<TokenType>>(Token
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
            return std::make_unique<Token<TokenType>>(Token
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
            return std::make_unique<Token<TokenType>>(Token
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
        auto ptr = std::make_unique<Token<TokenType>>(Token
            {
                .type = lexer.dfa.final_states[status.final_dfa_state],
                .line_number = line_number,
                .lexeme = lexer.source_code.substr(start, len)
            });

        // Error checks
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

public:
    constexpr iterator(Lexer<TokenType, num_states, num_keywords>& l) : lexer{ l }, cur_position{ 0 }
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

export template <typename TokenType, int num_states, int num_keywords> requires is_token_type<TokenType>
class Lexer
{
    const DFA<TokenType, num_states, num_keywords>& dfa;
    const std::string_view source_code;
    const int symbol_length_limit;

    friend class iterator<TokenType, num_states, num_keywords>;
public:
    constexpr iterator<TokenType, num_states, num_keywords> begin() { return iterator{ *this }; }
    constexpr sentinel end() { return {}; }

    constexpr Lexer(DFA<TokenType, num_states, num_keywords>& dfa, std::string_view sc, int symbol_length_limit = 50)
        : dfa{ dfa }, source_code{ sc }, symbol_length_limit{ symbol_length_limit } {}
};