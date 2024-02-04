export module RegexParser:ast;

import std;

import :structures;
import compiler;

namespace RegexParser
{
    export template <IsParseTreeNode ParseNodeType>
        constexpr auto get_ast(std::unique_ptr<ParseNodeType> parse_tree) -> std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>;

    export constexpr auto get_ast(std::string_view sv) -> ast_output;
}