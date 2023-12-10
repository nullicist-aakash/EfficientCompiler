module;
#include <iostream>

export module parser.structures.dfa;
import helpers.flatmap;
import <array>;
import <string_view>;

template <
    typename TokenType,
    typename TransitionInfo,
    typename FinalStateInfo,
    typename KeywordInfo,
    int num_states,
    int num_keywords
>
struct DFA
{
    std::array<std::array<int, 128>, num_states> productions{};
    std::array<TokenType, num_states> final_states{};
    flatmap<std::string_view, TokenType, num_keywords> keyword_to_token{};

    friend std::ostream& operator<<(std::ostream& out, const DFA& dfa)
    {
        out << "Productions: " << std::endl;
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
};