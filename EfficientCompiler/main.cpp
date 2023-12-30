import JackParser;
import ArithmeticParser;
import compiler;

#include <fstream>
#include <iostream>
#include <numeric>
#include <set>
#include <sstream>
#include <utility>
#include <memory>
#include <cassert>
#include <functional>

using namespace std;


template<typename T>
using P = std::pair<int, T>;

struct start_parser
{
	template <IsParseTreeNode ptn, IsASTNode astn>
	static constexpr auto to_ast(auto converter, unique_ptr<ptn> node, unique_ptr<astn> inherited) -> unique_ptr<astn>
	{
		assert(node->descendants.size() == 3);

		auto e = node->extract_child_node(1);
		auto ast = make_unique<astn>(node->extract_child_leaf(0));

		return converter(std::move(e), std::move(ast));
	}
};

struct expression_parser
{
	template <IsParseTreeNode ptn, IsASTNode astn>
	static constexpr auto to_ast(auto converter, unique_ptr<ptn> node, unique_ptr<astn> inherited) -> unique_ptr<astn>
	{
		assert(node->descendants.size() == 0 || node->descendants.size() == 3);
		
		if (node->descendants.size() == 0)
			return std::move(inherited);

		// E -> + Num E
		auto plus = node->extract_child_leaf(0);
		auto num = node->extract_child_leaf(1);
		auto e = node->extract_child_node(2);
		
		auto ast = make_unique<astn>(std::move(plus));

		if (inherited->descendants.size() > 0)
			ast->descendants = std::move(inherited->descendants);
		else
			ast->descendants.push_back(std::move(inherited));
		ast->descendants.push_back(make_unique<astn>(std::move(num)));

		return converter(std::move(e), std::move(ast));
	}
};

template <IsParseTreeNode ptn, IsASTNode astn>
constexpr unique_ptr<astn> ast_converter(unique_ptr<ptn> node, unique_ptr<astn> inherited)
{
	std::function<unique_ptr<astn>(unique_ptr<ptn>, unique_ptr<astn>)> converter = ast_converter<ptn, astn>;

	if (node->node_type == ANonTerminal::start)
		return start_parser::to_ast(converter, std::move(node), std::move(inherited));
	
	if (node->node_type == ANonTerminal::expression)
		return expression_parser::to_ast(converter, std::move(node), std::move(inherited));
	
	assert(false);
}

static auto read_file(string_view filename)
{
	ifstream file{ filename.data() };
	ostringstream ss;
	ss << file.rdbuf(); // reading data
	return ss.str();
}

int main()
{
	constexpr auto aparser = get_arithmetic_parser();
	auto output2 = aparser("1 + 2 + 4");
	cout << aparser << endl;
	cout << output2.logs << endl;
	cout << output2.errors << endl;
	cout << *output2.root << endl;

	using LexerTypes = typename std::remove_cvref_t<decltype(*output2.root)>::LexerTypes;
	using ENonTerminal = typename std::remove_cvref_t<decltype(*output2.root)>::ENonTerminal;

	const auto res = ast_converter(move(output2.root), std::make_unique<ASTNode<LexerTypes, ENonTerminal>>());
	cout << output2.root << endl;
}