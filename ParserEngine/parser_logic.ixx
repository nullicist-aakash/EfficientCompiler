export module compiler.parser:parser;

import :structures;
import :parse_table;
import compiler.lexer;

import <string_view>;
import <iostream>;
import <format>;
import <vector>;
import <stack>;
import <string>;
import <variant>;
import <sstream>;
import <memory>;

template <CParserTypes ParserTypes>
class ParserStack
{
	using ETerminal = ParserTypes::ETerminal;
	using ENonTerminal = ParserTypes::ENonTerminal;
	using ELexerSymbol = ParserTypes::ELexerSymbol;
	using EParserSymbol = ParserTypes::EParserSymbol;
	using ParseTreeNode = ParserTypes::ParseTreeNode;
	using LeafType = ParseTreeNode::LeafType;
	using InternalNodeType = ParseTreeNode::InternalNodeType;

	std::stack<EParserSymbol> stack{};
	std::unique_ptr<ParseTreeNode> root;

	ParseTreeNode* parent;
	int child_index;

	constexpr LeafType* terminal_leaf()
	{
		return std::get_if<LeafType>(&parent->descendants[child_index]);
	}

public:
	constexpr ParserStack()
	{
		stack.push(ENonTerminal::start);
		root = std::make_unique<ParseTreeNode>(ENonTerminal::start);

		parent = root.get();
		child_index = -1;
	}

	constexpr auto pop_with_tree()
	{
		stack.pop();
		while (child_index == parent->descendants.size() - 1)
		{
			child_index = parent->parent_child_index;
			parent = parent->parent;

			if (parent == nullptr)
				return;
		}
		++child_index;

		/*
    while (node->parent_child_index == node->parent->children.size() - 1)
    {
        node = node->parent;
        if (node->parent == nullptr)
            return;
    }

    node = node->parent->children[node->parent_child_index + 1];
*/
	}

	constexpr auto pop_only_stack()
	{
		stack.pop();
	}

	constexpr auto top() const
	{
		return stack.top();
	}

	constexpr auto empty() const
	{
		return stack.empty();
	}

	constexpr void on_terminal_matched(CLexerToken auto& token)
	{
		*terminal_leaf() = std::make_unique<std::decay_t<decltype(token)>>(token);
		pop_with_tree();
	}

	constexpr void on_non_terminal_expand(CLexerToken auto& token, auto& parse_table)
	{
		const auto stack_top = std::get<ENonTerminal>(stack.top());
		const auto input_terminal = std::get<ETerminal>(token.type);

		const auto production_number = parse_table.parse_table[(int)stack_top][(int)input_terminal];
		const auto& production = parse_table.productions[production_number];

		auto& node = *(std::get<InternalNodeType>(parent->descendants[child_index]).get());
		node.production_number = production_number;

		// empty production
		if (production.size == 1 && production.production[0] == ETerminal::eps)
		{
			pop_with_tree();
			return;
		}

		for (auto [index, symbol] : production.production | std::views::take(production.size) | std::views::enumerate)
		{
			if (std::holds_alternative<ETerminal>(symbol))
				continue;

			node.descendants.push_back(std::make_unique<ParseTreeNode>(
				std::get<ENonTerminal>(symbol),
				&node,
				(int)index
			));
		}

		stack.pop();
		for (auto &symbol: production.production | std::views::take(production.size) | std::views::reverse)
			stack.push(symbol);

		parent = &node;

		/*
            for (size_t i = production_size - 1; i > 0; --i)
            {
                st.push(production[i]);

                // -1 here because production[0] is the start symbol
                node->children[i - 1] = new ParseTreeNode;

                node->children[i - 1]->parent = node;
                node->children[i - 1]->symbol_index = production[i];
                node->children[i - 1]->parent_child_index = i - 1;
            }

            node = node->children[0];*/
	}
};

template <CEParserSymbol EParserSymbol, CLexerTypes LexerTypes, int num_states, int num_keywords, int max_prod_len, int num_productions>
class Parser
{
	using ETerminal = std::variant_alternative_t<0, EParserSymbol>;
	using ENonTerminal = std::variant_alternative_t<1, EParserSymbol>;
	using ELexerSymbol = std::variant<ETerminal, ELexerError>;

public:
	Lexer<LexerTypes, num_states, num_keywords> lexer{};
	ParseTable<EParserSymbol, max_prod_len, num_productions> parse_table{};

	auto operator()(std::string_view source_code) const
	{
		std::stringstream out{};

		ParserStack<ParserTypes<LexerTypes, ENonTerminal>> stack{};

		for (auto token : lexer(source_code))
		{
			if (stack.empty())
			{
				out << "Parser error (" << token << "): Input source code is syntactically incorrect.\n";
				break;
			}

			const auto status = parse_table.get_parse_status(stack.top(), token.type);

			if (status == ParserStatus::LEXER_ERROR)
			{
				out << "Lexer error: " << token << "\n";
				continue;
			}

			if (status == ParserStatus::TERMINAL_MATCHED)
				stack.on_terminal_matched(token);
			else if (status == ParserStatus::NON_TERMINAL_EXPAND)
				stack.on_non_terminal_expand(token, parse_table);
			else
			{
				out << "Parser error (" << token << ") Invalid token encountered with stack top " << stack.top() << "\n";
				if (status != ParserStatus::NON_TERMINAL_ERROR)
					stack.pop_with_tree();
			}
		}

		return out.str();
	}
};

template <typename T>
concept IsParser = requires(T t)
{
	[] <CEParserSymbol EParserSymbol, CLexerTypes LexerTypes, int num_states, int num_keywords, int max_prod_len, int num_productions>
		(Parser<EParserSymbol, LexerTypes, num_states, num_keywords, max_prod_len, num_productions>&) {}(t);
};

export template <typename ostream>
constexpr ostream& operator<<(ostream& out, const IsParser auto& parser)
{
    return out << parser.lexer << "\n" << parser.parse_table;
}

export consteval auto build_parser(auto production_callback, const auto& lexer)
{
	return Parser
	{
		.lexer = lexer,
		.parse_table = build_parse_table(production_callback, lexer.keyword_to_token)
	};
}