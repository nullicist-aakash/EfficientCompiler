export module compiler_engine.parser;

import compiler_engine.lexer;
import compiler_engine.structures;
import helpers.flatmap;
import helpers.reflection;

import <array>;
import <bitset>;
import <map>;
import <string_view>;

template <is_terminal Terminal, is_non_terminal NonTerminal>
struct Parser
{
	static constexpr auto num_terminals = get_size<Terminal>();
	static constexpr auto num_non_terminals = get_size<NonTerminal>();
	static constexpr auto num_symbols = num_terminals + num_non_terminals;
public:
	std::array<std::array<int, num_terminals>, num_symbols> parseTable;
};

template <is_terminal Terminal, is_non_terminal NonTerminal>
consteval auto get_parser(auto production_callback)
{
	return parser;
}