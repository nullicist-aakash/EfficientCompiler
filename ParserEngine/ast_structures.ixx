export module compiler.ast:structures;

import compiler.lexer;
import compiler.parser;
import <memory>;
import <variant>;
import <vector>;

export template <CLexerTypes LexerTypes, CENonTerminal ENonTerminal>
struct ASTNode
{
    using ETerminal = LexerTypes::ETerminal;
    using ILexerToken = LexerTypes::ILexerToken;
    using InternalNodeType = std::unique_ptr<ParseTreeNode<LexerTypes, ENonTerminal>>;
    using LeafType = std::unique_ptr<ILexerToken>;

    const std::variant<ETerminal, ENonTerminal> node_symbol_type{};
    const LeafType lexer_token{};

    std::vector<std::unique_ptr<ASTNode>> descendants{};

    constexpr ASTNode(std::variant<ETerminal, ENonTerminal> node_symbol_type)
        : node_symbol_type{ node_symbol_type }, lexer_token{ nullptr }
    {

    }

    constexpr ASTNode(LeafType leaf) :
        node_symbol_type{ std::get<ETerminal>(leaf->type) },
        lexer_token{ std::move(leaf) }
    {

    }
};

export template <typename T>
concept IsASTNode = requires(T t)
{
    [] <CLexerTypes LexerTypes, CENonTerminal ENonTerminal>
        (ASTNode<LexerTypes, ENonTerminal>&) {}(t);
};