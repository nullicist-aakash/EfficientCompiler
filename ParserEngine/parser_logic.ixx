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
		std::vector<std::string> errors{};
		std::stack<EParserSymbol> stack{};
		stack.push(ENonTerminal::start);

		for (auto token : lexer(source_code))
		{
			const auto status = parse_table.get_parse_status(stack.top(), token.type);

		}
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