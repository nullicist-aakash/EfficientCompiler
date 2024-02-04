export module RegexParser:nfa;
import :structures;

import <vector>;
import <memory>;
import <string>;
import <variant>;
import compiler;
import helpers;

namespace
{
	using RegexParser::NFA;
	using RegexParser::NodeManager;
	using RegexParser::Terminal;
	using RegexParser::NonTerminal;

	struct RegexVisitor;

	struct OR_Visitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const->std::shared_ptr<NFA>;
	};

	struct ConcatVisitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const->std::shared_ptr<NFA>;
	};

	struct LeafVisitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const->std::shared_ptr<NFA>;
	};

	struct KleeneStarVisitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const->std::shared_ptr<NFA>;
	};

	struct PlusVisitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const->std::shared_ptr<NFA>;
	};

	struct QuestionVisitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const->std::shared_ptr<NFA>;
	};

	struct RangeMinusVisitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const->std::shared_ptr<NFA>;
	};

	struct ClassVisitor
	{
		const RegexVisitor* parent{ nullptr };

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const->std::shared_ptr<NFA>;
	};

	struct RegexVisitor
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

		using terminal_visitors_type = std::variant<
			NeutralVisitor,
			OR_Visitor,
			ConcatVisitor,
			LeafVisitor,
			KleeneStarVisitor,
			PlusVisitor,
			QuestionVisitor,
			RangeMinusVisitor>;

		std::array<terminal_visitors_type, get_enum_count<Terminal>()> terminal_visitors;

		constexpr RegexVisitor()
		{
			terminal_visitors[(int)Terminal::OR] = OR_Visitor{ this };
			terminal_visitors[(int)Terminal::CONCAT] = ConcatVisitor{ this };
			terminal_visitors[(int)Terminal::CHAR] = LeafVisitor{ this };
			terminal_visitors[(int)Terminal::DOT] = LeafVisitor{ this };
			terminal_visitors[(int)Terminal::EMPTY] = LeafVisitor{ this };
			terminal_visitors[(int)Terminal::STAR] = KleeneStarVisitor{ this };
			terminal_visitors[(int)Terminal::PLUS] = PlusVisitor{ this };
			terminal_visitors[(int)Terminal::QUESTION_MARK] = QuestionVisitor{ this };
			terminal_visitors[(int)Terminal::MINUS] = RangeMinusVisitor{ this };
		}

	public:
		static auto get_instance() -> RegexVisitor&
		{
			static RegexVisitor instance;
			return instance;
		}

		template <IsASTNode astn, class out_stream>
		constexpr auto get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
		{
			return std::visit([&](auto&& visitor)
				{
					if constexpr (std::is_same_v<std::decay_t<decltype(visitor)>, Terminal>)
						return std::visit([&](auto&& visitor) { return visitor.get_nfa(node, err_stream); }, terminal_visitors[(int)visitor]);
					else
						return ClassVisitor{ this }.get_nfa(node, err_stream);
				}, node->node_symbol_type);
		}
	};

	template <IsASTNode astn, class out_stream>
	constexpr auto OR_Visitor::get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
	{
		auto& nm = NodeManager::get_instance();

		if (node->descendants.size() < 2)
			err_stream << "Bug: OR node must have at least 2 children!\n";

		auto nfa = std::make_shared<NFA>(nm.create_node(), nm.create_node());

		for (auto& x : node->descendants)
		{
			auto child_nfa = this->parent->get_nfa(x.get(), err_stream);

			if (!child_nfa)
			{
				err_stream << "Bug: OR children must be non null!\n";
				continue;
			}

			nfa->start_node->empty_transitions.emplace_back(child_nfa->start_node);
			child_nfa->final_node->empty_transitions.emplace_back(nfa->final_node);
		}

		return nfa;
	}

	template <IsASTNode astn, class out_stream>
	constexpr auto ConcatVisitor::get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
	{
		auto& nm = NodeManager::get_instance();

		if (node->descendants.size() < 2)
			err_stream << "Bug: Concat node must have at least 2 children!\n";

		std::shared_ptr<NFA> nfa = nullptr;

		for (auto& x : node->descendants)
		{
			auto child_nfa = this->parent->get_nfa(x.get(), err_stream);

			if (!child_nfa)
			{
				err_stream << "Bug: CONCAT children must be non null!\n";
				continue;
			}

			if (nfa == nullptr)
			{
				nfa = child_nfa;
				continue;
			}

			nfa->final_node->empty_transitions.emplace_back(child_nfa->start_node);
			nfa->final_node = child_nfa->final_node;
		}

		return nfa;
	}

	template <IsASTNode astn, class out_stream>
	constexpr auto LeafVisitor::get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
	{
		auto& nm = NodeManager::get_instance();

		const auto type = node->lexer_token->type;
		const auto character = node->lexer_token->lexeme[0];

		auto nfa = std::make_shared<NFA>(nm.create_node(), nm.create_node());

		if (type == Terminal::EMPTY)
		{
			nfa->start_node->empty_transitions.emplace_back(nfa->final_node);
			return nfa;
		}

		if (type == Terminal::CHAR)
		{
			nfa->start_node->tran_nodes.emplace_back(std::string{ character }, nfa->final_node);
			return nfa;
		}

		std::string chars = "\r\n\t";
		for (int i = 32; i < 127; ++i)
			chars += (char)i;

		nfa->start_node->tran_nodes.emplace_back(chars, nfa->final_node);
		return nfa;
	}

	template <IsASTNode astn, class out_stream>
	constexpr auto KleeneStarVisitor::get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
	{
		auto& nm = NodeManager::get_instance();

		if (node->descendants.size() != 1)
			err_stream << "Bug: Kleene Star node must have exactly one child!\n";

		auto child_nfa = this->parent->get_nfa(node->descendants[0].get(), err_stream);

		if (!child_nfa)
		{
			err_stream << "Bug: Kleene Star node child must be non null!\n";
			return nullptr;
		}

		child_nfa->final_node->empty_transitions.emplace_back(child_nfa->start_node);
		child_nfa->start_node->empty_transitions.emplace_back(child_nfa->final_node);

		return child_nfa;
	}

	template <IsASTNode astn, class out_stream>
	constexpr auto PlusVisitor::get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
	{
		auto& nm = NodeManager::get_instance();

		if (node->descendants.size() != 1)
			err_stream << "Bug: Plus node must have exactly one child!\n";

		auto child_nfa = this->parent->get_nfa(node->descendants[0].get(), err_stream);

		if (!child_nfa)
		{
			err_stream << "Bug: Plus node child must be non null!\n";
			return nullptr;
		}

		child_nfa->final_node->empty_transitions.emplace_back(child_nfa->start_node);

		return child_nfa;
	}

	template <IsASTNode astn, class out_stream>
	constexpr auto QuestionVisitor::get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
	{
		auto& nm = NodeManager::get_instance();

		if (node->descendants.size() != 1)
			err_stream << "Bug: Question node must have exactly one child!\n";

		auto child_nfa = this->parent->get_nfa(node->descendants[0].get(), err_stream);

		if (!child_nfa)
		{
			err_stream << "Bug: Question node child must be non null!\n";
			return nullptr;
		}

		child_nfa->start_node->empty_transitions.emplace_back(child_nfa->final_node);

		return child_nfa;
	}

	template <IsASTNode astn, class out_stream>
	constexpr auto ClassVisitor::get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
	{
		auto& nm = NodeManager::get_instance();

		auto nfa = std::make_shared<NFA>(nm.create_node(), nm.create_node());

		bool is_negated = false;
		std::array<bool, 128> chars{};

		for (auto& x : node->descendants)
		{
			if (x->node_symbol_type == Terminal::CARET)
			{
				is_negated = true;
				continue;
			}
			auto child_nfa = this->parent->get_nfa(x.get(), err_stream);

			if (!child_nfa)
			{
				err_stream << "Bug: class children must be non null!\n";
				continue;
			}

			for (auto& c : child_nfa->start_node->tran_nodes[0].first)
				chars[c] = true;
		}

		if (is_negated)
			for (auto& x : chars)
				x = !x;

		std::string s;
		if (chars['\r'])
			s += '\r';
		if (chars['\n'])
			s += '\n';
		if (chars['\t'])
			s += '\t';

		for (int i = 32; i < 127; ++i)
			if (chars[i])
				s.push_back(i);

		nfa->start_node->tran_nodes.emplace_back(s, nfa->final_node);
		return nfa;
	}

	template <IsASTNode astn, class out_stream>
	constexpr auto RangeMinusVisitor::get_nfa(const astn* node, out_stream& err_stream) const -> std::shared_ptr<NFA>
	{
		const char left = node->descendants[0]->lexer_token->lexeme[0];
		const char right = node->descendants[1]->lexer_token->lexeme[0];

		if (left > right)
		{
			err_stream << "Invalid range: " << left << "-" << right << '\n';
			return nullptr;
		}

		auto& nm = NodeManager::get_instance();

		std::string chars = "";
		for (int i = left; i <= right; ++i)
			chars += (char)i;

		auto nfa = std::make_shared<NFA>(nm.create_node(), nm.create_node());
		nfa->start_node->tran_nodes.emplace_back(chars, nfa->final_node);
		return nfa;
	}
}

namespace RegexParser
{
	export template <IsASTNode astn>
		constexpr auto get_nfa(const astn* node) -> std::variant<std::string, std::shared_ptr<NFA>>
	{
		constexpr_ostream errors;

		if (!node)
			return "Empty node passed!";

		auto res = RegexVisitor::get_instance().get_nfa(node, errors);

		if (errors.str().empty())
			return std::move(res);

		return std::move(errors.str());
	}
};