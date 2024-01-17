import RegexParser;
import compiler;
import helpers;

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>
#include <limits>
#include <string>
#include <map>
#include <variant>

using namespace std;

struct Node
{
private:
	friend class NodeManager;
	std::size_t unique_index{ std::numeric_limits<std::size_t>::max() };
public:
	std::vector<std::shared_ptr<Node>> empty_transitions{};
	std::vector<std::pair<std::string, std::shared_ptr<Node>>> tran_nodes{};
	bool is_final_state{ false };
};

class NodeManager
{
private:
	NodeManager() = default;
	NodeManager(const NodeManager&) = delete;
	NodeManager(NodeManager&&) = delete;
	NodeManager& operator=(const NodeManager&) = delete;
	NodeManager& operator=(NodeManager&&) = delete;

public:
	std::vector<std::shared_ptr<Node>> all_nodes;

	static NodeManager& get_instance()
	{
		static NodeManager instance;
		return instance;
	}

	auto create_node()
	{
		all_nodes.emplace_back(std::make_shared<Node>());
		all_nodes.back()->unique_index = all_nodes.size() - 1;
		return all_nodes.back();
	}

	void del_node(std::shared_ptr<Node> node)
	{
		auto index = std::exchange(node->unique_index, std::numeric_limits<std::size_t>::max());
		all_nodes[index] = nullptr;

		if (index != all_nodes.size() - 1)
		{
			std::swap(all_nodes[index], all_nodes.back());
			all_nodes[index]->unique_index = index;
		}

		all_nodes.pop_back();
	}

	void garbage_collect()
	{
		for (int i = 0; i < all_nodes.size(); ++i)
		{
			if (all_nodes[i].use_count() > 1)
				continue;

			del_node(all_nodes[i]);
		}
	}

	void delete_all_nodes()
	{
		all_nodes.clear();
	}
};

class RegexVisitor
{
	class OR_Visitor
	{
		template <IsASTNode astn, class out_stream>
		constexpr bool verify_ast(const astn* node, out_stream& err_stream)
		{
			return true;
		}
	};

	class ConcatVisitor
	{
		template <IsASTNode astn, class out_stream>
		constexpr bool verify_ast(const astn* node, out_stream& err_stream)
		{
			return true;
		}
	};

	class LeafParser
	{
		template <IsASTNode astn, class out_stream>
		constexpr bool verify_ast(const astn* node, out_stream& err_stream)
		{
			return true;
		}
	};

	class KleeneStarParser
	{
		template <IsASTNode astn, class out_stream>
		constexpr bool verify_ast(const astn* node, out_stream& err_stream)
		{
			return true;
		}
	};

	class PlusParser
	{
		template <IsASTNode astn, class out_stream>
		constexpr bool verify_ast(const astn* node, out_stream& err_stream)
		{
			return true;
		}
	};

	class QuestionParser
	{
		template <IsASTNode astn, class out_stream>
		constexpr bool verify_ast(const astn* node, out_stream& err_stream)
		{
			return true;
		}
	};

	class ClassParser
	{
		template <IsASTNode astn, class out_stream>
		constexpr bool verify_ast(const astn* node, out_stream& err_stream)
		{
			return true;
		}
	};

	class RangeMinusParser
	{
		template <IsASTNode astn, class out_stream>
		constexpr bool verify_ast(const astn* node, out_stream& err_stream)
		{

		}
	};

	using ETerminal = RegexParser::Terminal;
	using ENonTerminal = RegexParser::NonTerminal;
	using EParserSymbol = std::variant<ETerminal, ENonTerminal>;

	using visitors_type = std::variant<
		OR_Visitor,
		ConcatVisitor,
		LeafParser,
		KleeneStarParser,
		PlusParser,
		QuestionParser,
		ClassParser,
		RangeMinusParser>;

	// std::map<EParserSymbol, visitors_type> visitors;

public:
	constexpr RegexVisitor()
	{
/*		visitors[ETerminal::OR] = OR_Visitor{};
		visitors[ETerminal::CONCAT] = ConcatVisitor{};
		visitors[ETerminal::CHAR] = LeafParser{};
		visitors[ETerminal::DOT] = LeafParser{};
		visitors[ETerminal::EMPTY] = LeafParser{};
		visitors[ETerminal::STAR] = KleeneStarParser{};
		visitors[ETerminal::PLUS] = PlusParser{};
		visitors[ETerminal::QUESTION_MARK] = QuestionParser{};
		visitors[ETerminal::MINUS] = RangeMinusParser{};
		visitors[ENonTerminal::_class] = ClassParser{};*/
	}

	template <IsASTNode astn>
	constexpr bool verify_regex_ast(const astn* node)
	{
		constexpr_ostream errors;

		if (!node)
			return false;

		return true;
	}
};

static auto read_file(string_view filename)
{
	ifstream file{ filename.data() };
	ostringstream ss;
	ss << file.rdbuf(); // reading data
	return ss.str();
}

int main()
{
	auto ast = RegexParser::get_ast(R"(abc|[def-z]*)");
	cout << ast.errors << endl;
	cout << ast.logs << endl;
	if (ast.root)
	{
		cout << *ast.root << endl;
		RegexVisitor().verify_regex_ast(std::move(ast.root));
	}
}