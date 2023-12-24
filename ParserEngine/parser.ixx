export module compiler_engine.parser;

import compiler_engine.lexer;
import compiler_engine.structures;
import helpers.flatmap;

import <vector>;
import <bitset>;
import <map>;
import <string_view>;

template <is_terminal Terminal, is_terminal NonTerminal>
class Parser
{
public:
	int num_non_terminals;
	int num_terminals;
	int start_index;

	std::vector<std::vector<int>> productions;
	std::vector<std::string> symbolType2symbolStr;
	std::map<std::string, int> symbolStr2symbolType;
	std::bitset<128> nullable;
	std::vector<std::bitset<128>> firstSet;
	std::vector<std::bitset<128>> followSet;
	std::vector<std::vector<int>> parseTable;

	Parser() : num_non_terminals{ 0 }, num_terminals{ 0 }, start_index{ 0 }
	{

	}
};