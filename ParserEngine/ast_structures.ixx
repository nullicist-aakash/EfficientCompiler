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
    using LeafType = std::unique_ptr<typename LexerTypes::ILexerToken>;

    const std::variant<ETerminal, ENonTerminal> node_symbol_type{};
    const std::unique_ptr<ILexerToken> lexer_token{};

    std::vector<std::unique_ptr<ASTNode>> descendants{};

    ASTNode() = default;

    ASTNode(ParseTreeNode<LexerTypes, ENonTerminal>* ptnode) :
        node_symbol_type{ ptnode->node_type },
        lexer_token { nullptr }
    {

    }

    ASTNode(LeafType leaf) :
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
