export module compiler_engine.lexer;
import helpers.flatmap;
import compiler_engine.structures;

import helpers.checks;
import compiler_engine.dfa;
import helpers.reflection;
import <limits>;
import <string_view>;
import <numeric>;
import <map>;
import <variant>;

struct sentinel {};

template <is_token_type TokenType, is_lexer_token LT, int num_states, int num_keywords>
class Iterator;

template <is_token_type TokenType, is_lexer_token LT, int num_states, int num_keywords>
struct LexerStringWrapper
{
    using TerminalType = std::variant_alternative_t<0, TokenType>;
    const DFA<TokenType, num_states>& dfa{};
    const flatmap<std::string_view, TerminalType, num_keywords>& keyword_to_token{};
    const std::string_view source_code{};

    constexpr LexerStringWrapper(const auto& lexer, std::string_view source_code) :
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

template <is_token_type TokenType, is_lexer_token LT, int num_states, int num_keywords>
class Iterator
{
    using TerminalType = std::variant_alternative_t<0, TokenType>;

    const LexerStringWrapper<TokenType, LT, num_states, num_keywords>& lexer_string;
    LT token;
    std::size_t cur_position{ 0 };

    constexpr auto get_token_from_dfa()
    {
        auto tk = lexer_string.dfa.get_next_token<LT>(lexer_string.source_code, cur_position);

        auto errType = std::get_if<ErrorTokenType>(&tk.type);
        auto usrType = std::get_if<TerminalType>(&tk.type);

        if (!errType)
            cur_position += tk.lexeme.size();
        else if (*errType == ErrorTokenType::TK_ERROR_SYMBOL || *errType == ErrorTokenType::TK_ERROR_PATTERN)
            cur_position += 1;

        if (usrType && *usrType == TerminalType::TK_SYMBOL && lexer_string.keyword_to_token.exists(tk.lexeme))
            tk.type = lexer_string.keyword_to_token.at(tk.lexeme);

        tk.afterConstruction(token);

		return tk;
    }

public:
    constexpr Iterator(const LexerStringWrapper<TokenType, LT, num_states, num_keywords>& lsw) : lexer_string{lsw}, cur_position{0}
    {
        token = get_token_from_dfa();
    }
    constexpr bool operator!=(sentinel) const 
    {
        auto errType = std::get_if<ErrorTokenType>(&token.type);

        return !errType || *errType != ErrorTokenType::TK_EOF;
    }
    constexpr Iterator& operator++() { token = get_token_from_dfa(); return *this; }
    constexpr const auto& operator*() const
    {
        return token;
    }
};

template <is_token_type TokenType, is_lexer_token LT, int num_states, int num_keywords>
struct Lexer
{
    using TerminalType = std::variant_alternative_t<0, TokenType>;
    
    DFA<TokenType, num_states> dfa;
    flatmap<std::string_view, TerminalType, num_keywords> keyword_to_token;

    constexpr auto operator()(std::string_view source_code) const
    {
		return LexerStringWrapper<TokenType, LT, num_states, num_keywords>(*this, source_code);
	}
};

export template <typename T, is_token_type TokenType, is_lexer_token LT, int num_states, int num_keywords>
constexpr T& operator<<(T& out, const Lexer<TokenType, LT, num_states, num_keywords>& lexer)
{
    out << lexer.dfa << "\nKeywords:\n";
    for (const auto& [k, v] : lexer.keyword_to_token)
        out << k << ": " << v << '\n';

    return out;
}

template <is_terminal TerminalType, int num_keywords>
consteval auto get_keywords_map(const auto& keywords)
{
    flatmap<std::string_view, TerminalType, num_keywords> keyword_to_token{};
    for (auto& k : keywords)
        keyword_to_token.insert(k.keyword, k.token_type);

    return keyword_to_token;
}

export template <is_lexer_token LT>
consteval auto build_lexer(auto transition_callback, auto final_states_callback, auto keywords_callback)
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
    using TokenType = decltype(LT::type);
    using TerminalType = std::variant_alternative_t<0, TokenType>;

    constexpr auto lexer = Lexer<TokenType, LT, num_states, num_keywords>{
        .dfa = build_dfa<TokenType, num_states>(
            []() -> auto&& { return transitions; },
            []() -> auto&& { return final_states; }
            ),
        .keyword_to_token = get_keywords_map<TerminalType, num_keywords>(keywords)
    };

    static_assert(lexer.keyword_to_token.size() == num_keywords, "Keywords have a duplicate key!");

    return lexer;
}