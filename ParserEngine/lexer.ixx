module;

#include <iostream>

export module parser.lexer;
export import parser.dfa;

import helpers.reflection;
import <string_view>;
import <limits>;

export template <token_type TokenType>
struct Token
{
    TokenType type = TokenType::UNINITIALISED;
    std::string_view lexeme{};
    std::size_t line_number = 0;
};

export template <token_type TokenType>
std::ostream& operator<<(std::ostream& out, const Token<TokenType>& tk)
{
    out << tk.type << " " << tk.line_number << " " << tk.lexeme << std::endl;
    return out;
}

using State = int;
struct Status
{
    State final_dfa_state;
    std::size_t final_state_code_pos;

    State cur_dfa_state;
    std::size_t cur_code_position;
};

template <token_type TokenType, int num_states, int num_keywords>
class Lexer;

struct sentinel {};

template <token_type TokenType, int num_states, int num_keywords>
class iterator
{
    const Lexer<TokenType, num_states, num_keywords>& lexer;
    Token<TokenType> token;
    std::size_t line_number{ 1 };
    std::size_t cur_position{ 0 };

    constexpr Status get_status(const std::size_t& start) const
    {
        Status status =
        {
            .final_dfa_state = -1,
            .final_state_code_pos = std::numeric_limits<std::size_t>::max(),
            .cur_dfa_state = 0,
            .cur_code_position = start
        };

        while (status.cur_code_position < lexer.source_code.size())
        {
            auto cur_symbol = lexer.source_code[status.cur_code_position];
            auto next_dfa_state = lexer.dfa.productions[status.cur_dfa_state][cur_symbol];

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

        return status;
    }

    constexpr Token<TokenType> extract_token(Status status, const std::size_t& start) const
    {
        if (start >= lexer.source_code.size())
            return { TokenType::TK_EOF, "" };

        // We didn't move at all
        if (status.cur_code_position == start)
            return { TokenType::TK_ERROR_SYMBOL, lexer.source_code.substr(start, 1) };

        // We moved somewhere but didn't reach any final state
        if (status.final_dfa_state == -1)
        {
            auto len = status.cur_code_position - start + 1;
            return { TokenType::TK_ERROR_PATTERN, lexer.source_code.substr(start, len) };
        }

        // We return for the last seen final state
        std::size_t len = status.final_state_code_pos - start + 1;
        Token<TokenType> tk = {
            lexer.dfa.final_states[status.final_dfa_state],
            lexer.source_code.substr(start, len)
        };

        // Error checks
        if (tk.type == TokenType::TK_SYMBOL)
        {
            if (lexer.dfa.keyword_to_token.exists(tk.lexeme))
                tk.type = lexer.dfa.keyword_to_token.at(tk.lexeme);

            if (tk.lexeme.size() > lexer.symbol_length_limit)
                tk.type = TokenType::TK_ERROR_LENGTH;
        }

        return tk;
    }

    constexpr auto get_token_from_dfa()
    {
        auto tk = extract_token(get_status(cur_position), cur_position);
        tk.line_number = line_number;
        line_number += (tk.type == TokenType::TK_NEWLINE) ? 1 : 0;

        if (tk.type == TokenType::TK_ERROR_SYMBOL || tk.type == TokenType::TK_ERROR_PATTERN)
            cur_position += 1;
        else
            cur_position += tk.lexeme.size();

		return tk;
    }

public:
    constexpr iterator(Lexer<TokenType, num_states, num_keywords>& l) : lexer{l}, cur_position{0}
    {
        token = get_token_from_dfa();
    }
    constexpr bool operator!=(sentinel) const { return token.type != TokenType::TK_EOF; }
    constexpr iterator& operator++() { token = get_token_from_dfa(); return *this; }
    constexpr const auto& operator*()
    {
        return token;
    }
};

export template <token_type TokenType, int num_states, int num_keywords>
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