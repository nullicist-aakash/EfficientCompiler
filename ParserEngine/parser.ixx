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

template <is_non_terminal NonTerminal, is_terminal Terminal>
consteval auto get_nullable_set(auto production_callback)
{
	// Returns an array of size num_non_terminals, where ith index=true means (Nonterminal)i is nullable
	using SET_TYPE = std::bitset<get_size<NonTerminal>()>;
	SET_TYPE nullable_set;
	nullable_set.reset();
	nullable_set.set((int)NonTerminal::eps);

	constexpr auto& productions = production_callback();
	while (true)
	{
		auto new_nullable = SET_TYPE{};
		for (auto &x: productions
			| std::views::filter([&](auto& x) { return !nullable_set[(int)x.start]; })
			| std::views::filter([&](auto& x) { return std::ranges::none_of(
				x.production
				| std::views::take(x.size)
				| std::views::transform([&](auto& y)
					{ return std::holds_alternative<Terminal>(y) || !nullable_set[(int)std::get<NonTerminal>(y)]; }),
				std::identity()); })
			)
			new_nullable.set((int)x.start);

		if (new_nullable == SET_TYPE{})
			break;

		nullable_set |= new_nullable;
	}

	return nullable_set;
}

export consteval auto build_parser(auto production_callback)
{
	constexpr auto productions = production_callback();
	using Terminal = std::remove_cvref_t<decltype(productions[0])>::TType;
	using NonTerminal = std::remove_cvref_t<decltype(productions[0])>::NTType;

	return get_nullable_set<NonTerminal, Terminal>([]() -> auto& {return productions; });
}