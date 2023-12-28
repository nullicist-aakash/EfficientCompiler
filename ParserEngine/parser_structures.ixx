export module compiler.parser:structures;

import compiler.lexer;
import <type_traits>;
import <concepts>;
import <string_view>;
import <variant>;
import <array>;
import <memory>;
import <vector>;

// Forward declarations


// Concepts
export template<typename T>
concept CENonTerminal = requires(T t)
{
	requires std::is_enum_v<T>;
	{ T::start };
};

export template <typename T>
concept CEParserSymbol = requires(T t)
{
    requires CETerminal<std::variant_alternative_t<0, T>>;
    requires CENonTerminal<std::variant_alternative_t<1, T>>;
};

// Structures
export template <CEParserSymbol EParserSymbol, int max_prod_len = 30>
struct ProductionInfo
{
    using ENonTerminal = std::variant_alternative_t<1, EParserSymbol>;

    ENonTerminal start;
    std::array<EParserSymbol, max_prod_len> production;
    std::size_t size;

    constexpr ProductionInfo(auto start, const auto& production)
        : start(start), size(production.size())
    {
        for (std::size_t i = 0; i < production.size(); ++i)
            this->production[i] = production[i];
    }
};

export template <CLexerTypes LexerTypes, CENonTerminal ENonTerminal>
struct ParseTreeNode
{
    using LeafType = std::unique_ptr<typename LexerTypes::ILexerToken>;
    using InternalNodeType = std::unique_ptr<ParseTreeNode>;

    ENonTerminal node_type;
    int parent_child_index;

    ParseTreeNode const* parent;
    std::variant<std::vector<LeafType>, std::vector<InternalNodeType>> descendants;
};

export template <CLexerTypes LexerTypes, CENonTerminal ENonTerminal>
struct ASTNode
{
    using ETerminal = LexerTypes::ETerminal;
    using ILexerToken = LexerTypes::ILexerToken;
    std::variant<ETerminal, ENonTerminal> node_symbol_type;

    std::unique_ptr<ILexerToken> lexer_token;

    std::vector<std::unique_ptr<ASTNode>> children{};
    std::unique_ptr<ASTNode> sibling = nullptr;
};

// Parser types
export template <typename T>
concept CParserTypes = requires()
{
    requires CLexerToken<typename T::ILexerToken>;
    requires CENonTerminal<typename T::ENonTerminal>;
    requires std::same_as<typename T::EParserSymbol, std::variant<typename T::ETerminal, typename T::ENonTerminal>>;
    requires std::same_as<typename T::ProductionInfo, ProductionInfo<typename T::EParserSymbol>>;
    requires std::same_as<typename T::ParseTreeNode, ParseTreeNode<typename T::ILexerToken, typename T::ENonTerminal>>;
    requires std::same_as<typename T::ASTNode, ASTNode<typename T::ILexerToken, typename T::ENonTerminal>>;
};

export template <CLexerTypes LexerTypes, CENonTerminal ENT>
struct ParserTypes
{
    using ELexerSymbol = LexerTypes::ELexerSymbol;
    using ETerminal = LexerTypes::ETerminal;
    using ELexerError = LexerTypes::ELexerError;
    using ILexerToken = LexerTypes::ILexerToken;

    using ENonTerminal = ENT;
    using EParserSymbol = std::variant<ETerminal, ENonTerminal>;

    using ProductionInfo = ProductionInfo<EParserSymbol>;
    using ParseTreeNode = ParseTreeNode<LexerTypes, ENonTerminal>;
    using ASTNode = ASTNode<LexerTypes, ENonTerminal>;
};