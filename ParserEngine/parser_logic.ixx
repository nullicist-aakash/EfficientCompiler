export module compiler.parser:parser;

import :structures;
import :parse_table;
import compiler.lexer;

template <CEParserSymbol EParserSymbol, CLexerTypes LexerTypes, int num_states, int num_keywords, int max_prod_len, int num_productions>
struct Parser
{
	Lexer<LexerTypes, num_states, num_keywords> lexer{};
	ParseTable<EParserSymbol, max_prod_len, num_productions> parse_table{};
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