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

struct sentinel {};

template <token_type TokenType, lexer_token LT, int num_states, int num_keywords>
class Iterator;

template <token_type TokenType, lexer_token LT, int num_states, int num_keywords>
struct LexerStringWrapper
{
    const DFA<TokenType, num_states>& dfa{};
    const flatmap<std::string_view, TokenType, num_keywords>& keyword_to_token{};
    const std::string_view source_code{};

    LexerStringWrapper(const auto& lexer, std::string_view source_code) :
        dfa{ lexer.dfa },
        keyword_to_token{ lexer.keyword_to_token },
        source_code{ source_code }
    {}

    constexpr auto begin() const
    {
        return Iterator<TokenType, LT, num_states, num_keywords>(*this);
    }

    constexpr auto end() const
    {
        return sentinel{};
    }
};

template <token_type TokenType, lexer_token LT, int num_states, int num_keywords>
class Iterator
{
    const LexerStringWrapper<TokenType, LT, num_states, num_keywords>& lexer_string;
    LT token;
    std::size_t cur_position{ 0 };

    constexpr auto get_token_from_dfa()
    {
        auto tk = lexer_string.dfa.get_next_token<LT>(lexer_string.source_code, cur_position);

        if (tk.type == TokenType::TK_ERROR_SYMBOL || tk.type == TokenType::TK_ERROR_PATTERN)
            cur_position += 1;
        else
            cur_position += tk.lexeme.size();

        // Keyword Pass
        if (tk.type == TokenType::TK_SYMBOL && lexer_string.keyword_to_token.exists(tk.lexeme))
            tk.type = lexer_string.keyword_to_token.at(tk.lexeme);

        tk.afterConstruction(token);

		return tk;
    }

public:
    constexpr Iterator(const LexerStringWrapper<TokenType, LT, num_states, num_keywords>& lsw) : lexer_string{lsw}, cur_position{0}
    {
        token = get_token_from_dfa();
    }
    constexpr bool operator!=(sentinel) const { return token.type != TokenType::TK_EOF; }
    constexpr Iterator& operator++() { token = get_token_from_dfa(); return *this; }
    constexpr const auto& operator*()
    {
        return token;
    }
};

template <token_type TokenType, lexer_token LT, int num_states, int num_keywords>
struct Lexer
{
    DFA<TokenType, num_states> dfa{};
    flatmap<std::string_view, TokenType, num_keywords> keyword_to_token{};

    auto operator()(std::string_view source_code) const
    {
		return LexerStringWrapper<TokenType, LT, num_states, num_keywords>(*this, source_code);
	}
};

export template <typename T, token_type TokenType, lexer_token LT, int num_states, int num_keywords>
constexpr T& operator<<(T& out, const Lexer<TokenType, LT, num_states, num_keywords>& lexer)
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

export template <lexer_token LT>
constexpr auto build_lexer(auto transition_callback, auto final_states_callback, auto keywords_callback)
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

    constexpr auto lexer = Lexer<TokenType, LT, num_states, num_keywords>{
        .dfa = build_dfa<num_states>(
            []() -> auto&& { return transitions; },
            []() -> auto&& { return final_states; }
        ),
        .keyword_to_token = get_keywords_map<TokenType, num_keywords>(keywords)
    };

    static_assert(lexer.keyword_to_token.size() == num_keywords, "Keywords have a duplicate key!");

    return lexer;
}