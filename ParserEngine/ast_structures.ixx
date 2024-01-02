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
    using LeafType = std::unique_ptr<typename LexerTypes::ILexerToken>;

    const std::variant<ETerminal, ENonTerminal> node_symbol_type{};
    const std::unique_ptr<ILexerToken> lexer_token{};

    std::vector<std::unique_ptr<ASTNode>> descendants{};

    constexpr ASTNode() = default;

    constexpr ASTNode(InternalNodeType ptnode) :
        node_symbol_type{ ptnode->node_type },
        lexer_token { nullptr }
    {

    }

    constexpr ASTNode(LeafType leaf) :
        node_symbol_type{ std::get<ETerminal>(leaf->type) },
        lexer_token{ std::move(leaf) }
    {

    }

    template <typename T, typename U>
    constexpr ASTNode(std::variant<T, U>& var) : ASTNode{ build_ast(var) }
    {

    }

private:
    constexpr ASTNode(ASTNode&& other) noexcept :
        node_symbol_type{ std::move(other.node_symbol_type) },
        lexer_token{ std::move(other.lexer_token) },
        descendants{ std::move(other.descendants) }
    {

    }

    template <typename T, typename U>
    static constexpr auto build_ast(std::variant<T, U>& var)
    {
		if (std::holds_alternative<T>(var))
            return ASTNode(std::move(std::get<0>(var)));
        return ASTNode(std::move(std::get<1>(var)));
    }
};

export template <typename T>
concept IsASTNode = requires(T t)
{
    [] <CLexerTypes LexerTypes, CENonTerminal ENonTerminal>
        (ASTNode<LexerTypes, ENonTerminal>&) {}(t);
};