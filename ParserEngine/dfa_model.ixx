export module compiler_engine.models:dfa;

import compiler_engine.structures;
import <array>;
import <string_view>;
import <limits>;
import <variant>;
import <vector>;

export template <CELexerSymbol ELexerSymbol, int num_states>
struct DFA
{
    using ETerminal = std::variant_alternative_t<0, ELexerSymbol>;
    using ELexerError = std::variant_alternative_t<1, ELexerSymbol>;

    static const int state_count = num_states;

    std::array<std::array<int, 128>, num_states> productions;
    std::array<ETerminal, num_states> final_states;

    constexpr DFA()
    {
        for (auto& x : dfa.productions)
            x.fill(-1);

        for (auto& x : final_states)
            x = ELexerError::UNINITIALISED;
    }

    constexpr auto pass_string(std::string_view input, std::size_t cur_position) const
    {
        using State = int;
        struct Status
        {
            State final_dfa_state;
            std::size_t final_state_code_pos;

            State cur_dfa_state;
            std::size_t cur_code_position;
        };

        Status status =
        {
            .final_dfa_state = -1,
            .final_state_code_pos = std::numeric_limits<std::size_t>::max(),
            .cur_dfa_state = 0,
            .cur_code_position = cur_position
        };

        while (status.cur_code_position < input.size())
        {
            auto cur_symbol = input[status.cur_code_position];
            auto next_dfa_state = productions[status.cur_dfa_state][cur_symbol];

            if (next_dfa_state == -1)
                break;

            if (std::get_if<ETerminal>(&final_states[next_dfa_state]))
            {
                status.final_dfa_state = next_dfa_state;
                status.final_state_code_pos = status.cur_code_position;
            }

            status.cur_dfa_state = next_dfa_state;
            status.cur_code_position++;
        }

        return status;
    }

    template <CLexerToken LexerToken>
    constexpr LexerToken get_next_token(std::string_view input, std::size_t cur_position) const
    {
        using Types = LexerTypes<LexerToken>;
        static_assert(std::is_same_v<typename Types::ELexerSymbol, ELexerSymbol>);

        const auto& status = pass_string(input, cur_position);
        const auto& start = cur_position;

        if (start >= input.size())
            return { ETerminal::TK_EOF, "" };

        // We didn't move at all
        if (status.cur_code_position == start)
            return { ELexerError::ERR_SYMBOL, input.substr(start, 1) };

        // We moved somewhere but didn't reach any final state
        if (status.final_dfa_state == -1)
        {
            auto len = status.cur_code_position - start + 1;
            return { ELexerError::ERR_PATTERN, input.substr(start, len) };
        }

        // We return for the last seen final state
        std::size_t len = status.final_state_code_pos - start + 1;
        return {
            final_states[status.final_dfa_state],
            input.substr(start, len)
        };
    }
};

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

template <int num_states>
consteval auto validate_final_states(const auto& final_states)
{
    for (auto& f : final_states)
        if (f.state_no < 0 || f.state_no > num_states)
            return "An entry for state number in final states is out of range.";

    std::vector<bool> visited(num_states, false);
    for (auto& f : final_states)
    {
        if (visited[f.state_no])
            return "Multiple entries for same state in final states.";

        visited[f.state_no] = true;
    }

    return "";
}

export template <
    typename T,
    CELexerSymbol ELexerSymbol,
    int num_states
>
constexpr T& operator<<(T& out, const DFA<ELexerSymbol, num_states>& dfa)
{
    using ETerminal = std::variant_alternative_t<0, ELexerSymbol>;
    using ELexerError = std::variant_alternative_t<1, ELexerSymbol>;

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
        if (auto usrType = std::get_if<ETerminal>(&dfa.final_states[i]); usrType)
            out << i << ": " << *usrType << '\n';

    return out;
}

export template <CELexerSymbol ELexerSymbol, int num_states>
consteval auto build_dfa(auto transition_callback, auto final_states_callback)
{
    using ETerminal = std::variant_alternative_t<0, ELexerSymbol>;
    using ELexerError = std::variant_alternative_t<1, ELexerSymbol>;

    constexpr auto& transitions = transition_callback();
    constexpr auto& final_states = final_states_callback();

    static_assert(std::is_same_v<TransitionInfo, decltype(transitions[0])>, "Transitions array doesn't contain type: TransitionInfo");
    static_assert(std::is_same_v<FinalStateInfo<ETerminal>, decltype(final_states[0])>, "Final states array doesn't contain type: FinalStateInfo<ETerminal>");

    // We can directly use static_assert here once we have C++26.
    ct_assert([]() { return validate_transitions<num_states>(transitions); });
    ct_assert([]() { return validate_final_states<num_states>(final_states); });

    DFA<ELexerSymbol, num_states> dfa;

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