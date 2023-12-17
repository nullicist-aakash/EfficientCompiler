export module parser.dfa;
import helpers.checks;
import helpers.reflection;
import parser.structures;

import <array>;
import <string_view>;
import <algorithm>;
import <vector>;
import <type_traits>;

template <int num_states>
consteval auto validate_transitions(const auto& transitions)
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
}

template <token_type TokenType, int num_states>
consteval auto validate_final_states(const auto& final_states)
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
}

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

export template <int num_states>
consteval auto get_dfa(auto transition_callback, auto final_states_callback)
{
    constexpr auto&& transitions = transition_callback();
    constexpr auto&& final_states = final_states_callback();

    constexpr auto fsc = []() -> auto&& { return std::forward<decltype(final_states)>(final_states); };

    using TokenType = decltype(final_states.begin()->token_type);

    // We can directly use static_assert here once we have C++26.
    ct_assert([]() { return validate_transitions<num_states>(transitions); });
    ct_assert([]() { return validate_final_states<TokenType, num_states>(final_states); });

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