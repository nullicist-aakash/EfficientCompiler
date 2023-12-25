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
import <bitset>;

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
bool operator==(const std::variant<T, U>& lhs, const T& rhs)
{
	return std::holds_alternative<T>(lhs) && std::get<T>(lhs) == rhs;
}

template <typename T, typename U>
bool operator==(const std::variant<T, U>& lhs, const U& rhs)
{
	return std::holds_alternative<U>(lhs) && std::get<U>(lhs) == rhs;
}

template <is_non_terminal NonTerminal, is_terminal Terminal, auto num_non_terminals>
constexpr auto get_nullable_set(auto production_callback)
{
	constexpr auto& productions = production_callback();

	// Returns an array of size num_non_terminals, where ith index=true means (Nonterminal)i is nullable
	std::array<bool, num_non_terminals> nullable_set;
	for (auto& x : nullable_set)
		x = false;

	// Cover eps case
	nullable_set[(int)NonTerminal::eps] = true;

	// Cover case of NT -> eps
	for (auto& x : productions)
		if (x.production.size() == 1 && x.production[0] == NonTerminal::eps)
			nullable_set[(int)x.start] = true;

	// Iterate until no more changes
	while (true)
	{
		bool changed = false;
		for (auto& x : productions)
		{
			// If NT -> A1 A2 ... An, and all Ai are nullable, then NT is nullable
			bool all_nullable = true;
			for (auto& y : x.production)
			{
				if (std::holds_alternative<Terminal>(y))
					all_nullable = false;
				else if (!nullable_set[(int)std::get<NonTerminal>(y)])
					all_nullable = false;
			}

			if (all_nullable && !nullable_set[(int)x.start])
			{
				nullable_set[(int)x.start] = true;
				changed = true;
			}
		}

		if (changed)
			break;
	}

	return nullable_set;
}

export constexpr auto build_parser(auto production_callback)
{
	constexpr auto productions = production_callback();
	using Terminal = std::remove_cvref_t<decltype(productions[0])>::TType;
	using NonTerminal = std::remove_cvref_t<decltype(productions[0])>::NTType;

	constexpr auto num_non_terminals = get_size<NonTerminal>();

	return get_nullable_set<NonTerminal, Terminal, num_non_terminals>([]() -> auto& {return productions; });
}