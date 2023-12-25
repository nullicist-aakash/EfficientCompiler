export module compiler_engine.parser;

import compiler_engine.lexer;
import compiler_engine.structures;
import helpers.flatmap;
import helpers.reflection;

import <array>;
import <bitset>;
import <map>;
import <string_view>;
import <type_traits>;
import <variant>;
import <ranges>;
import <algorithm>;
import <bitset>;
import <functional>;
import <utility>;

template <is_non_terminal NonTerminal, is_terminal Terminal>
struct Parser
{
	static constexpr auto num_terminals = get_size<Terminal>();
	static constexpr auto num_non_terminals = get_size<NonTerminal>();
	static constexpr auto num_symbols = num_terminals + num_non_terminals;
public:
	std::array<std::array<int, num_terminals>, num_symbols> parseTable;
};

template <typename T, typename U>
constexpr bool operator==(const std::variant<T, U>& lhs, const T& rhs)
{
    return std::holds_alternative<T>(lhs) && std::get<T>(lhs) == rhs;
}

template <typename T, typename U>
constexpr bool operator==(const std::variant<T, U>& lhs, const U& rhs)
{
    return std::holds_alternative<U>(lhs) && std::get<U>(lhs) == rhs;
}

template <typename T, typename U>
constexpr bool operator!=(const std::variant<T, U>& lhs, const T& rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename U>
constexpr bool operator!=(const std::variant<T, U>& lhs, const U& rhs)
{
    return !(lhs == rhs);
}

template <is_non_terminal NonTerminal, is_terminal Terminal>
consteval auto get_nullable_set(auto production_callback)
{
    constexpr auto& productions = production_callback();

	// Returns an array of size num_non_terminals, where ith index=true means (Nonterminal)i is nullable
	std::bitset<get_size<NonTerminal>()> nullable_set;
	nullable_set.reset();

    // T -> eps means T is nullable
    for (auto& x : productions)
        if (x.size == 1 && x.production[0] == Terminal::eps)
			nullable_set[(int)x.start] = true;

    // Go till convergence
	for (bool changed = true; std::exchange(changed, false); )
		for (auto &x: productions
			| std::views::filter([&](auto& x) { return !nullable_set[(int)x.start]; })
			| std::views::filter([&](auto& x) { return std::ranges::none_of(
				x.production
				| std::views::take(x.size)
				| std::views::transform([&](auto& y)
					{ return std::holds_alternative<Terminal>(y) || !nullable_set[(int)std::get<NonTerminal>(y)]; }),
				std::identity()); })
			)
			changed = nullable_set[(int)x.start] = true;

	return nullable_set;
}

template <is_non_terminal NonTerminal, is_terminal Terminal>
consteval auto get_first_sets(auto production_callback, const auto& nullable_set)
{
    constexpr auto& productions = production_callback();
    constexpr auto num_terminals = get_size<Terminal>();
    constexpr auto num_non_terminals = get_size<NonTerminal>();
    constexpr auto num_symbols = num_terminals + num_non_terminals;

    std::array<std::bitset<num_terminals>, num_non_terminals> first_set{};

    // Add eps to first set of nullables
    for (int i = 0; i < num_non_terminals; ++i)
        if (nullable_set[i])
            first_set[i][(int)Terminal::eps] = true;

    // Iterate untill no update
    for (bool updated = true; std::exchange(updated, false); )
        for (const auto& x : productions)
        {
            auto& bits = first_set[(int)x.start];
            auto copy = bits;

            for (auto y : x.production | std::views::take(x.size))
            {
                if (std::holds_alternative<Terminal>(y) && y != Terminal::eps)
                {
                    bits[(int)std::get<Terminal>(y)] = true;
                    break;
                }
                
                if (y == Terminal::eps)
                {
                    bits[(int)std::get<Terminal>(y)] = true;
                    continue;
                }

                int index = (int)std::get<NonTerminal>(y);
                bits |= first_set[index];

                if (!nullable_set[index])
					break;
            }

            updated = updated || (copy != bits);
        }

    return first_set;
}

template <is_non_terminal NonTerminal, is_terminal Terminal>
consteval auto get_follow_sets(auto production_callback, const auto& nullable_set, const auto& first_set)
{
	constexpr auto& productions = production_callback();
	constexpr auto num_terminals = get_size<Terminal>();
	constexpr auto num_non_terminals = get_size<NonTerminal>();
	constexpr auto num_symbols = num_terminals + num_non_terminals;

	std::array<std::bitset<num_terminals>, num_non_terminals> follow_set{};

    for (bool updated = true; std::exchange(updated, false); )
        for (auto& x : productions)
        {
            for (int j = 0; j < x.size; ++j)
            {
                if (std::holds_alternative<Terminal>(x.production[j]))
                    continue;

                auto& symbol = x.production[j];
                auto& bits = follow_set[(int)std::get<NonTerminal>(symbol)];
                auto copy = bits;

                for (int k = j + 1; k < x.size; ++k)
                {
                    auto& follow_symbol = x.production[k];
                    if (std::holds_alternative<Terminal>(follow_symbol))
                    {
                        bits[(int)std::get<Terminal>(follow_symbol)] = true;

                        if (follow_symbol != Terminal::eps)
							break;
                    }
                    else
                    {
                        bits |= first_set[(int)std::get<NonTerminal>(follow_symbol)];

                        if (!nullable_set.test((int)std::get<NonTerminal>(follow_symbol)))
                            break;
                    }

                    if (k == x.size - 1)
                        bits |= follow_set[(int)x.start];
                }

                if (j == x.size - 1)
                    bits |= follow_set[(int)x.start];

                updated = updated || (copy != bits);
            }
        }

    // remove eps from follow set
    for (auto& follow : follow_set)
        follow[(int)Terminal::eps] = false;

	return follow_set;
}

export consteval auto build_parser(auto production_callback)
{
	constexpr auto productions = production_callback();
	using Terminal = std::remove_cvref_t<decltype(productions[0])>::TType;
	using NonTerminal = std::remove_cvref_t<decltype(productions[0])>::NTType;

	constexpr auto nullable_set = get_nullable_set<NonTerminal, Terminal>([]() -> auto& { return productions; });
    constexpr auto first_set = get_first_sets<NonTerminal, Terminal>([]() -> auto& { return productions; }, nullable_set);
    return get_follow_sets<NonTerminal, Terminal>([]() -> auto& { return productions; }, nullable_set, first_set);
}