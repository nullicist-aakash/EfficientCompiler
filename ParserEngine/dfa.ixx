export module parser.dfa;
import helpers.checks;
import helpers.reflection;

import <array>;
import <string_view>;
import <algorithm>;
import <vector>;
import <type_traits>;

export template<typename T>
concept token_type = requires(T t)
{
    std::is_enum_v<T>;
    { T::UNINITIALISED } -> std::convertible_to<T>;
    { T::TK_NEWLINE } -> std::convertible_to<T>;
    { T::TK_SYMBOL } -> std::convertible_to<T>;
    { T::TK_EOF } -> std::convertible_to<T>;
    { T::TK_ERROR_SYMBOL } -> std::convertible_to<T>;
    { T::TK_ERROR_PATTERN } -> std::convertible_to<T>;
    { T::TK_ERROR_LENGTH } -> std::convertible_to<T>;
};

template <typename T>
concept transition_info = requires(T t)
{
	{ t.from } -> std::convertible_to<int>;
	{ t.to } -> std::convertible_to<int>;
	{ t.pattern } -> std::convertible_to<std::string_view>;
	{ t.default_transition_state } -> std::convertible_to<int>;
};

template <typename TokenType, typename T>
concept final_state_info = requires(T t)
{
	{ t.state_no } -> std::convertible_to<int>;
    { t.token_type } -> std::convertible_to<TokenType>;
    token_type<TokenType>;
};

template <token_type TokenType, transition_info TransitionInfo, typename FinalStateInfo>
requires final_state_info<TokenType, FinalStateInfo>
struct TypeChecker
{
    using passed = std::true_type;
};

export template <
    token_type TokenType,
    int num_states
>
struct DFA
{
    static const int state_count = num_states;
    std::array<std::array<int, 128>, num_states> productions{};
    std::array<TokenType, num_states> final_states{};
};

export template <
    typename T,
    token_type TokenType,
    int num_states
>
constexpr T& operator<<(T& out, const  DFA<TokenType, num_states>& dfa)
{
    out << "Productions: \n";
    for (int i = 0; i < num_states; ++i)
    {
        out << i << ": ";
        for (int j = 0; j < 128; ++j)
            out << dfa.productions[i][j] << " ";
        out << '\n';
    }

    out << "\nFinal States :\n";
    for (int i = 0; i < num_states; ++i)
        if (dfa.final_states[i] != TokenType::UNINITIALISED)
            out << i << ": " << dfa.final_states[i] << '\n';

    return out;
}

template <int num_states>
consteval auto validate_transitions(auto transition_callback)
{
    constexpr auto&& transitions = transition_callback();
    constexpr auto validate_transitions = []() constexpr
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

    ct_assert([]() { return validate_transitions(); });
}

template <token_type TokenType, int num_states>
consteval auto validate_final_states(auto final_states_callback)
{
    constexpr auto&& final_states = final_states_callback();
    constexpr auto validate_final_states = []() constexpr
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
}

export template <int num_states>
consteval auto get_dfa(auto transition_callback, auto final_states_callback)
{
    constexpr auto&& transitions = transition_callback();
    constexpr auto&& final_states = final_states_callback();

    constexpr auto tc = []() -> auto&& { return std::forward<decltype(transitions)>(transitions); };
    constexpr auto fsc = []() -> auto&& { return std::forward<decltype(final_states)>(final_states); };

    using TokenType = decltype(final_states.begin()->token_type);
    using TransitionInfo = std::remove_reference_t<decltype(transitions)>::value_type;
    using FinalStateInfo = std::remove_reference_t<decltype(final_states)>::value_type;

    static_assert(TypeChecker<TokenType, TransitionInfo, FinalStateInfo>::passed::value);

    validate_transitions<num_states>(tc);
    validate_final_states<TokenType, num_states>(fsc);

    DFA<TokenType, num_states> dfa;
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

    return dfa;
}