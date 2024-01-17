import RegexParser;
import compiler;
import helpers;

import <iostream>;
import <vector>;
import <memory>;
import <limits>;
import <string>;
import <map>;
import <variant>;
import <array>;

using namespace std;

struct Node
{
private:
	friend class NodeManager;
	std::size_t unique_index{ std::numeric_limits<std::size_t>::max() };
public:
	std::vector<std::shared_ptr<Node>> empty_transitions{};
	std::vector<std::pair<std::string, std::shared_ptr<Node>>> tran_nodes{};
};

struct NFA
{
	std::shared_ptr<Node> start_node{};
	std::shared_ptr<Node> final_node{};
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

	auto create_node() -> std::shared_ptr<Node>
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
	struct NeutralVisitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
		{
			std::terminate();
		}
	};

	struct OR_Visitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
		{
			for (auto& x : node->descendants)
				parent->get_nfa(x.get(), err_stream);
			return nullptr;
		}
	};

	struct ConcatVisitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
		{
			for (auto& x : node->descendants)
				parent->get_nfa(x.get(), err_stream);
			return nullptr;
		}
	};

	struct LeafVisitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
		{
			for (auto& x : node->descendants)
				parent->get_nfa(x.get(), err_stream);
			return nullptr;
		}
	};

	struct KleeneStarVisitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
		{
			for (auto& x : node->descendants)
				parent->get_nfa(x.get(), err_stream);
			return nullptr;
		}
	};

	struct PlusVisitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
		{
			for (auto& x : node->descendants)
				parent->get_nfa(x.get(), err_stream);
			return nullptr;
		}
	};

	struct QuestionVisitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
		{
			for (auto& x : node->descendants)
				parent->get_nfa(x.get(), err_stream);
			return nullptr;
		}
	};

	struct ClassVisitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
		{
			for (auto& x : node->descendants)
				parent->get_nfa(x.get(), err_stream);
			return nullptr;
		}
	};

	struct RangeMinusVisitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
		{
			const char left = node->descendants[0]->lexer_token->lexeme[0];
			const char right = node->descendants[1]->lexer_token->lexeme[0];

			if (left > right)
				err_stream << "Invalid range: " << left << "-" << right << '\n';

			return nullptr;
		}
	};

	using ETerminal = RegexParser::Terminal;
	using ENonTerminal = RegexParser::NonTerminal;
	using EParserSymbol = std::variant<ETerminal, ENonTerminal>;
	using terminal_visitors_type = std::variant<
		NeutralVisitor,
		OR_Visitor,
		ConcatVisitor,
		LeafVisitor,
		KleeneStarVisitor,
		PlusVisitor,
		QuestionVisitor,
		RangeMinusVisitor>;

	template <IsASTNode astn, class out_stream>
	constexpr auto get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
	{
		return std::visit([&](auto&& visitor)
			{
				if constexpr (std::is_same_v<std::decay_t<decltype(visitor)>, ETerminal>)
					return std::visit([&](auto&& visitor) { return visitor.get_nfa(node, err_stream); }, terminal_visitors[(int)visitor]);
				else
					return ClassVisitor{this}.get_nfa(node, err_stream);
			}, node->node_symbol_type);
	}

	std::array<terminal_visitors_type, get_enum_count<ETerminal>()> terminal_visitors;
public:
	constexpr RegexVisitor()
	{
		terminal_visitors[(int)ETerminal::OR] = OR_Visitor{this};
		terminal_visitors[(int)ETerminal::CONCAT] = ConcatVisitor{ this };
		terminal_visitors[(int)ETerminal::CHAR] = LeafVisitor{ this };
		terminal_visitors[(int)ETerminal::DOT] = LeafVisitor{ this };
		terminal_visitors[(int)ETerminal::EMPTY] = LeafVisitor{ this };
		terminal_visitors[(int)ETerminal::STAR] = KleeneStarVisitor{ this };
		terminal_visitors[(int)ETerminal::PLUS] = PlusVisitor{ this };
		terminal_visitors[(int)ETerminal::QUESTION_MARK] = QuestionVisitor{ this };
		terminal_visitors[(int)ETerminal::MINUS] = RangeMinusVisitor{ this };
	}

	template <IsASTNode astn>
	constexpr auto get_nfa(const astn* node) const -> std::variant<std::string, std::shared_ptr<NFA>>
	{
		constexpr_ostream errors;

		if (!node)
			return "Empty node passed!";

		auto res = get_nfa(node, errors);

		if (errors.str().empty())
			return std::move(res);
	
		return std::move(errors.str());
	}
};

int main()
{
	auto ast = RegexParser::get_ast(R"(abc|[def-Z]*)");
	cout << ast.errors << endl;
	cout << ast.logs << endl;
	if (ast.root)
	{
		cout << *ast.root << endl;
		cout << RegexVisitor().get_nfa(ast.root.get()) << endl;
	}
}