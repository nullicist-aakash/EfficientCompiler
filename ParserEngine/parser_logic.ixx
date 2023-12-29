export module compiler.parser:parser;

import :structures;
import :parse_table;
import compiler.lexer;

import <string_view>;
import <iostream>;
import <expected>;
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
	using ILexerToken = ParserTypes::ILexerToken;

	using InternalNodeType = ParseTreeNode::InternalNodeType;

	std::stack<EParserSymbol> stack{};
	std::unique_ptr<ParseTreeNode> root;

	ParseTreeNode* parent;
	int child_index;

public:
	constexpr ParserStack()
	{
		stack.push(ENonTerminal::start);
		root = std::make_unique<ParseTreeNode>(ENonTerminal::start);

		parent = nullptr;
		child_index = 0;
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

	constexpr auto& get_root()
	{
		return root;
	}

	constexpr void on_terminal_matched(CLexerToken auto& token)
	{
		parent->descendants[child_index] = std::make_unique<std::decay_t<decltype(token)>>(token);
		pop_with_tree();
	}

	constexpr void on_non_terminal_expand(CLexerToken auto& token, auto& parse_table)
	{
		const auto stack_top = std::get<ENonTerminal>(stack.top());
		const auto input_terminal = std::get<ETerminal>(token.type);

		const auto production_number = parse_table.parse_table[(int)stack_top][(int)input_terminal];
		const auto& production = parse_table.productions[production_number];

		// empty production
		if (production.size == 1 && production.production[0] == ETerminal::eps)
		{
			pop_with_tree();
			return;
		}

		auto& node = parent ? *(std::get<InternalNodeType>(parent->descendants[child_index]).get()) : *root.get();
		node.production_number = production_number;

		for (auto [index, symbol] : production.production | std::views::take(production.size) | std::views::enumerate)
			if (std::holds_alternative<ETerminal>(symbol))
				node.descendants.push_back(std::make_unique<ILexerToken>());
			else
				node.descendants.push_back(std::make_unique<ParseTreeNode>(
					std::get<ENonTerminal>(symbol),
					&node,
					(int)index
				));

		stack.pop();
		for (auto &symbol: production.production | std::views::take(production.size) | std::views::reverse)
			stack.push(symbol);

		parent = &node;
		child_index = 0;
	}
};

template <CParserTypes ParserTypes>
struct ParserOutput
{
	std::unique_ptr<typename ParserTypes::ParseTreeNode> root;
	std::string logs;
	std::string error;
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
		std::stringstream cerr{};

		ParserStack<ParserTypes<LexerTypes, ENonTerminal>> stack{};

		for (auto token : lexer(source_code))
		{
			if (stack.empty())
			{
				cerr << "Parser error (" << token << "): Input source code is syntactically incorrect.\n";
				break;
			}

			if (std::holds_alternative<ELexerError>(token.type))
			{
				cerr << "Lexer error: " << token << "\n";
				continue;
			}

			auto status = parse_table.get_parse_status(stack.top(), token.type);
			while (status == ParserStatus::NON_TERMINAL_EXPAND)
			{
				stack.on_non_terminal_expand(token, parse_table); 
				status = parse_table.get_parse_status(stack.top(), token.type);
			}

			if (status == ParserStatus::TERMINAL_MATCHED)
				stack.on_terminal_matched(token);
			else
			{
				cerr << "Parser error (" << token << ") Invalid token encountered with stack top " << stack.top() << "\n";
				if (status != ParserStatus::NON_TERMINAL_ERROR)
					stack.pop_with_tree();
			}
		}

		if (!stack.empty() && stack.top() != ETerminal::TK_EOF)
			cerr << "Parser error: No more tokens but parsing still left!\n";

		ParserOutput<ParserTypes<LexerTypes, ENonTerminal>> output;
		output.error = std::move(cerr.str());
		if (output.error.empty())
			output.root = std::move(stack.get_root());

		return output;
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