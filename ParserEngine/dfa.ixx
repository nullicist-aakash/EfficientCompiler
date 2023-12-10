module;
#include <iostream>

export module parser.dfa;
import helpers.flatmap;
import helpers.checks;
import helpers.reflection;

import <array>;
import <string_view>;
import <algorithm>;
import <vector>;
import <type_traits>;

export template<typename T>
concept is_token_type = requires(T t)
{
    std::is_enum_v<T>;
    { T::UNINITIALISED } -> std::convertible_to<T>;
    { T::TK_WHITESPACE } -> std::convertible_to<T>;
    { T::TK_NEWLINE } -> std::convertible_to<T>;
    { T::TK_SYMBOL } -> std::convertible_to<T>;
    { T::TK_EOF } -> std::convertible_to<T>;
    { T::TK_ERROR_SYMBOL } -> std::convertible_to<T>;
    { T::TK_ERROR_PATTERN } -> std::convertible_to<T>;
    { T::TK_ERROR_LENGTH } -> std::convertible_to<T>;
};

template <typename T>
concept is_transition_info = requires(T t)
{
	{ t.from } -> std::convertible_to<int>;
	{ t.to } -> std::convertible_to<int>;
	{ t.pattern } -> std::convertible_to<std::string_view>;
	{ t.default_transition_state } -> std::convertible_to<int>;
};

template <typename T, typename Enum>
concept is_final_state_info = requires(T t)
{
	{ t.state_no } -> std::convertible_to<int>;
	{ t.token_type } -> std::convertible_to<Enum>;
    std::is_enum_v<Enum>;
};

template <typename T, typename Enum>
concept is_keyword_info = requires(T t)
{
	{ t.keyword } -> std::convertible_to<std::string_view>;
	{ t.token_type } -> std::convertible_to<Enum>;
	std::is_enum_v<Enum>;
};

template <typename TokenType, typename TransitionInfo, typename FinalStateInfo, typename KeywordInfo>
requires is_token_type<TokenType> && is_transition_info<TransitionInfo> && is_final_state_info<FinalStateInfo, TokenType> && is_keyword_info<KeywordInfo, TokenType>
struct TypeChecker
{
    using passed = std::true_type;
};

export template <
    typename TokenType,
    int num_states,
    int num_keywords
>
struct DFA
{
    std::array<std::array<int, 128>, num_states> productions{};
    std::array<TokenType, num_states> final_states{};
    flatmap<std::string_view, TokenType, num_keywords> keyword_to_token{};

    static const int state_count = num_states;
    static const int keyword_count = num_keywords;
};

export template <
    typename TokenType,
    int num_states,
    int num_keywords
>
std::ostream& operator<<(std::ostream& out, const  DFA<TokenType, num_states, num_keywords>& dfa)
{
    std::cout << "Productions: " << std::endl;
    for (int i = 0; i < num_states; ++i)
    {
        out << i << ": ";
        for (int j = 0; j < 128; ++j)
            out << dfa.productions[i][j] << " ";
        out << std::endl;
    }

    out << std::endl << "Final States: " << std::endl;
    for (int i = 0; i < num_states; ++i)
        if (dfa.final_states[i] != TokenType::UNINITIALISED)
            out << i << ": " << dfa.final_states[i] << std::endl;

    out << std::endl << "Keywords: " << std::endl;
    for (const auto& [k, v] : dfa.keyword_to_token)
        out << k << ": " << v << std::endl;

    return out;
}

export consteval auto get_dfa(auto transition_callback, auto final_states_callback, auto keywords_callback)
{
    constexpr auto transitions = transition_callback();
    constexpr auto final_states = final_states_callback();
    constexpr auto keywords = keywords_callback();

    using TokenType = decltype(final_states[0].token_type);
    using TransitionInfo = decltype(transitions)::value_type;
    using FinalStateInfo = decltype(final_states)::value_type;
    using KeywordInfo = decltype(keywords)::value_type;

    static_assert(TypeChecker<TokenType, TransitionInfo, FinalStateInfo, KeywordInfo>::passed::value);

    constexpr auto num_keywords = keywords.size();
    constexpr auto num_states_callback = [transitions, final_states]() constexpr
        {
            int ans = 0;
            for (const auto& t : transitions)
                ans = std::max({ ans, t.from, t.to, t.default_transition_state });
            return ans;
        };
    constexpr auto num_states = 1 + num_states_callback();
    
    // IDK why this works, but I can't reduce it further without getting capture error in g++
    constexpr auto validate_transitions = [transitions]() constexpr
        {
            std::vector<int> def_val(num_states + 1, -1);
            for (int i = 0; i < transitions.size(); ++i)
            {
                auto& a = transitions[i];

                if (a.from < 0)
                    return "Start state can't be less than 0";

                if (a.to < -1)
                    return "End state can't be less than -1";

                if (a.default_transition_state != -1)
                {
                    if (def_val[a.from] == -1)
                        def_val[a.from] = a.default_transition_state;
                    else if (def_val[a.from] != a.default_transition_state)
                        return "Multiple default values found that emerge from a state";
                }

                for (int j = i + 1; j < transitions.size(); ++j)
                {
                    auto& b = transitions[j];

                    if (a.from == b.from && a.to == b.to)
                        return "Multiple transitions specified with same 'from' and 'to'";
                }
            }

            // Reachability
            std::vector<std::vector<int>> adj(num_states);
            for (auto& t : transitions)
                adj[t.from].push_back(t.to);

            std::vector<bool> visited(num_states, false);
            int count_visited = 0;
            std::vector<int> nodes(1, 0);
            while (!nodes.empty())
            {
                int tp = nodes.back();
                nodes.pop_back();
                if (tp == -1)
                    continue;

                if (visited[tp])
                    continue;

                visited[tp] = true;
                ++count_visited;
                for (auto x : adj[tp])
                    nodes.push_back(x);
            }

            if (count_visited != num_states)
                return "Not all states are reachable";

            return "";
        };
    constexpr auto res_tran = validate_transitions();
    ct_assert([]() { return res_tran; });

    constexpr auto validate_final_states = [final_states]() constexpr
        {
            for (auto& f : final_states)
                if (f.state_no < 0 || f.state_no > num_states)
                    return "An entry for state in final states is out of range";

            std::vector<bool> visited(num_states, false);
            for (auto& f : final_states)
            {
                if (visited[f.state_no])
                    return "Multiple entries for same state in final states";

                if (f.token_type == TokenType::TK_ERROR_SYMBOL)
                    return "Final state can't be TK_ERROR_SYMBOL";

                if (f.token_type == TokenType::TK_ERROR_PATTERN)
                    return "Final state can't be TK_ERROR_PATTERN";

                if (f.token_type == TokenType::TK_ERROR_LENGTH)
                    return "Final state can't be TK_ERROR_LENGTH";

                if (f.token_type == TokenType::TK_EOF)
                    return "Final state can't be TK_EOF";

                visited[f.state_no] = true;
            }

            return "";
        };
    constexpr auto res_fin = validate_final_states();
    ct_assert([]() { return res_fin; });

    DFA<TokenType, num_states, num_keywords> dfa;

    for (auto& x : dfa.productions)
        x.fill(-1);

    for (auto& t : transitions)
    {
        int fr = t.from;
        int def = t.default_transition_state;

        if (def == -1)
            continue;

        for (auto& x : dfa.productions[fr])
            x = def;
    }

    for (auto& t : transitions)
    {
        int fr = t.from;
        int to = t.to;
        int def = t.default_transition_state;

        for (auto c : t.pattern)
            dfa.productions[fr][c] = to;
    }

    for (auto& f : final_states)
        dfa.final_states[f.state_no] = f.token_type;

    for (auto& k : keywords)
        dfa.keyword_to_token.insert(k.keyword, k.token_type);

    return dfa;
}