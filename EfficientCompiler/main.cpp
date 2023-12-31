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
#include <stdexcept>

using namespace std;


template<CENonTerminal ENonTerminal, typename T>
using P = std::pair<ENonTerminal, T>;

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


template<int array_size, typename... Types>
class ASTVisitor
{
	using vtype = std::variant<Types...>;

	template <IsParseTreeNode ptn, IsASTNode astn>
	constexpr auto visitor(unique_ptr<ptn> node, unique_ptr<astn> inherited) const -> unique_ptr<astn>
	{
		if (!node)
			return nullptr;

		std::function<unique_ptr<astn>(unique_ptr<ptn>, unique_ptr<astn>)> converter =
			[this](unique_ptr<ptn> node, unique_ptr<astn> inherited) -> unique_ptr<astn>
			{
				return this->visitor(std::move(node), std::move(inherited));
			};

		return std::visit(
			[&](auto& _class_to_call) -> unique_ptr<astn>
			{
				using visitor_type = std::remove_cvref_t<decltype(_class_to_call)>;
				return visitor_type::to_ast(converter, std::move(node), std::move(inherited));
			},
			m_visitors[static_cast<int>(node->node_type)]
		);
	}

public:
	const std::array<vtype, array_size> m_visitors;

	template <IsParseTreeNode ptn, IsASTNode astn>
	constexpr auto visit(unique_ptr<ptn> node) const -> unique_ptr<astn>
	{
		if (!node)
			return nullptr;

		return visitor<ptn, astn>(std::move(node), nullptr);
	}
};

template<CENonTerminal ENonTerminal, typename... Types>
constexpr auto build_visitor(std::pair<ENonTerminal, Types> ...args)
{
	using vtype = std::variant<Types...>;

	constexpr auto array_size = 2; // get_enum_size<ENonTerminal>();
	std::vector<std::pair<ENonTerminal, vtype>> arr{ args... };

	std::array<vtype, array_size> m_visitors;
	std::array<int, array_size> counts{};

	for (auto& [key, value] : arr)
		m_visitors[static_cast<int>(key)] = value, counts[static_cast<int>(key)]++;

	for (auto& x : counts)
		if (x == 0)
			throw std::runtime_error("Mapping of visitor not found for atleast one Non Terminal.");
		else if (x > 1)
			throw std::runtime_error("There exist atleas two mappings for single Non Terminal.");

	return ASTVisitor<array_size, Types...> {
		.m_visitors = m_visitors
	};
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

	build_visitor(
		P{ANonTerminal::start, start_parser{}},
		P{ANonTerminal::expression, expression_parser{}}
	).visit<std::remove_cvref_t<decltype(*output2.root)>, ASTNode<LexerTypes, ENonTerminal>>(std::move(output2.root));
}