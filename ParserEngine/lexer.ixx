export module parser.lexer;
import helpers.flatmap;
import parser.structures;

import helpers.checks;
import parser.dfa;
import helpers.reflection;
import <limits>;
import <string_view>;
import <numeric>;
import <map>;

using State = int;
struct Status
{
    State final_dfa_state;
    std::size_t final_state_code_pos;

    State cur_dfa_state;
    std::size_t cur_code_position;
};

struct sentinel {};

template <token_type TokenType, int num_states, int num_keywords>
class iterator
{
    template <token_type TokenType, int num_states, int num_keywords>
    class Lexer;

    const Lexer<TokenType, num_states, num_keywords>& lexer;
    LexerToken<TokenType> token;
    std::size_t line_number{ 1 };
    std::size_t cur_position{ 0 };

    constexpr Status get_status() const
    {
        Status status =
        {
            .final_dfa_state = -1,
            .final_state_code_pos = std::numeric_limits<std::size_t>::max(),
            .cur_dfa_state = 0,
            .cur_code_position = cur_position
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

    constexpr LexerToken<TokenType> get_next_token() const
    {
        const auto& status = get_status();
        const auto& start = cur_position;
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
        LexerToken<TokenType> tk = {
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
        auto tk = get_next_token();
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

export template <typename T, token_type TokenType>
constexpr T& operator<<(T& out, const LexerToken<TokenType>& tk)
{
    out << tk.type << " " << tk.line_number << " " << tk.lexeme;
    return out;
}

export template <typename T, token_type TokenType, int num_states, int num_keywords>
constexpr T& operator<<(T& out, const Lexer<TokenType, num_states, num_keywords>& lexer)
{
    out << lexer.dfa << "\nKeywords:\n";
    for (const auto& [k, v] : lexer.keyword_to_token)
        out << k << ": " << v << '\n';
    return out;
}

template <token_type TokenType, int num_keywords>
constexpr auto get_keywords_map(const auto& keywords)
{
    flatmap<std::string_view, TokenType, num_keywords> keyword_to_token{};
    for (auto& k : keywords)
        keyword_to_token.insert(k.keyword, k.token_type);

    return keyword_to_token;
}

export constexpr auto get_lexer(auto transition_callback, auto final_states_callback, auto keywords_callback)
{
    constexpr auto transitions = transition_callback();
    constexpr auto final_states = final_states_callback();
    constexpr auto keywords = keywords_callback();

    constexpr auto num_states = 1 + std::accumulate(transitions.begin(), transitions.end(), 0,
        [](int ans, const auto& t) {
            return std::max({ ans, t.from, t.to, t.default_transition_state });
        }
    );
    constexpr auto num_keywords = keywords.size();
    using TokenType = decltype(final_states.begin()->token_type);

    constexpr auto lexer = Lexer<TokenType, num_states, num_keywords> { 
        .dfa = get_dfa<num_states>(
            []() -> auto&& { return transitions; }, 
            []() -> auto&& { return final_states; }
        ),
        .keyword_to_token = get_keywords_map<TokenType, num_keywords>(keywords)
    };

    static_assert(lexer.keyword_to_token.size() == num_keywords, "Keywords have a duplicate key!");

    return [](std::string_view sv) {
        return lexer;
        };
}