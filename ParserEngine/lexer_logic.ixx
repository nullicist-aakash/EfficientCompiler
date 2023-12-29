export module compiler.lexer:lexer;

import :dfa;
import :structures;
import helpers.flatmap;

import <algorithm>;
import <numeric>;
import <string_view>;
import <variant>;

template <CLexerTypes LexerTypes, int num_states, int num_keywords>
struct Lexer;

template <typename T>
concept IsLexer = requires(T t)
{
    [] <CLexerTypes LexerTypes, int num_states, int num_keywords>(Lexer<LexerTypes, num_states, num_keywords>&) {}(t);
};

template <CLexerTypes LexerTypes, int num_states, int num_keywords>
class LexerStringWrapper
{
    using ETerminal = LexerTypes::ETerminal;
    using ELexerSymbol = LexerTypes::ELexerSymbol;
    using ELexerError = LexerTypes::ELexerError;
    using ILexerToken = LexerTypes::ILexerToken;
    
    const DFA<ELexerSymbol, num_states>& dfa{};
    const flatmap<std::string_view, ETerminal, num_keywords>& keyword_to_token{};
    const std::string_view source_code{};

    struct sentinel {};
    class Iterator
    {
        const LexerStringWrapper& lexer_string;
        ILexerToken token;
        std::size_t cur_position{ 0 };

        constexpr auto get_token_from_dfa()
        {
            auto tk = lexer_string.dfa.get_next_token<ILexerToken>(lexer_string.source_code, cur_position);

            if (!std::get_if<ELexerError>(&tk.type))
                cur_position += tk.lexeme.size();
            else if (tk.type == ELexerError::ERR_SYMBOL || tk.type == ELexerError::ERR_PATTERN)
                cur_position += 1;
            else
                throw "Guddi pakdo developer ki";

            if (tk.type == ETerminal::IDENTIFIER && lexer_string.keyword_to_token.exists(tk.lexeme))
                tk.type = lexer_string.keyword_to_token.at(tk.lexeme);

            tk.after_construction(token);

            return tk;
        }

    public:
        constexpr Iterator(const LexerStringWrapper& lsw) : lexer_string{ lsw }, cur_position{ 0 }
        {
            do
            {
                token = get_token_from_dfa();
            } while (token.discard() && token.type != ETerminal::TK_EOF);
        }
        constexpr bool operator!=(sentinel) const
        {
            return token.type != ETerminal::TK_EOF;
        }
        constexpr const auto& operator++() 
        {
            do
            {
                token = get_token_from_dfa();
            } while (token.discard() && token.type != ETerminal::TK_EOF);

            return *this;
        }
        constexpr const auto& operator*() const
        {
            return token;
        }
    };

public:
    constexpr LexerStringWrapper(const IsLexer auto& lexer, std::string_view source_code) :
        dfa{ lexer.dfa },
        keyword_to_token{ lexer.keyword_to_token },
        source_code{ source_code }
    {}

    constexpr auto begin() const
    {
        return Iterator(*this);
    }

    constexpr auto end() const
    {
        return sentinel{};
    }
};

export template <CLexerTypes LexerTypes, int num_states, int num_keywords>
struct Lexer
{
    using ETerminal = LexerTypes::ETerminal;
    using ELexerSymbol = LexerTypes::ELexerSymbol;
     
    const DFA<ELexerSymbol, num_states> dfa;
    const flatmap<std::string_view, ETerminal, num_keywords> keyword_to_token;

    constexpr auto operator()(std::string_view source_code) const
    {
        return LexerStringWrapper<LexerTypes, num_states, num_keywords>(*this, source_code);
    }
};

export template <typename ostream>
constexpr ostream& operator<<(ostream& out, const IsLexer auto& lexer)
{
    out << lexer.dfa << "\n";
    out << "Number of Keywords: " << lexer.keyword_to_token.size() << "\n";
    for (const auto& [k, v] : lexer.keyword_to_token)
        out << k << " -> " << v << '\n';

    return out;
}

template <CETerminal ETerminal, int num_keywords>
consteval auto get_keywords_map(const auto& keywords)
{
    flatmap<std::string_view, ETerminal, num_keywords> keyword_to_token{};
    for (auto& k : keywords)
        keyword_to_token.insert(k.keyword, k.token_type);

    return keyword_to_token;
}

export template <CLexerTypes LexerTypes>
consteval auto build_lexer(auto transition_callback, auto final_states_callback, auto keywords_callback)
{
    using ETerminal = LexerTypes::ETerminal;
    using ELexerSymbol = LexerTypes::ELexerSymbol;
    using ELexerError = LexerTypes::ELexerError;
    using ILexerToken = LexerTypes::ILexerToken;

    constexpr auto transitions = transition_callback();
    constexpr auto final_states = final_states_callback();
    constexpr auto keywords = keywords_callback();

    static_assert(std::is_same_v<TransitionInfo, std::remove_cvref_t<decltype(transitions[0])>>, "Transitions array doesn't contain type: TransitionInfo");
    static_assert(std::is_same_v<FinalStateInfo<ETerminal>, std::remove_cvref_t<decltype(final_states[0])>>, "Final states array doesn't contain type: FinalStateInfo<ETerminal>");
    static_assert(std::is_same_v<KeywordInfo<ETerminal>, std::remove_cvref_t<decltype(keywords[0])>>, "Keywords array doesn't contain type: KeywordInfo<ETerminal>");

    constexpr auto num_states = 1 + std::accumulate(transitions.begin(), transitions.end(), 0,
        [](int ans, const auto& t) {
            return std::max({ ans, t.from, t.to, t.default_transition_state });
        }
    );
    constexpr auto num_keywords = keywords.size();

    constexpr auto lexer = Lexer<LexerTypes, num_states, num_keywords>{
        .dfa = build_dfa<ELexerSymbol, num_states>(
            []() -> auto&& { return transitions; },
            []() -> auto&& { return final_states; }
            ),
        .keyword_to_token = get_keywords_map<ETerminal, num_keywords>(keywords)
    };

    static_assert(lexer.keyword_to_token.size() == num_keywords, "Keywords have a duplicate entry!");

    return lexer;
}