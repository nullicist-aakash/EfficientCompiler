export module compiler_engine.structures;

import helpers.flatmap;
import <string_view>;
import <type_traits>;
import <array>;
import <vector>;
import <concepts>;
import <variant>;
import <cassert>;
import <memory>;
import <functional>;

export enum class LexerErrorToken;

export template<typename T>
concept is_terminal = requires(T t)
{
    requires std::is_enum_v<T>;
    { T::IDENTIFIER };
    { T::eps };
    { T::TK_EOF };
};

export template<typename T>
concept is_non_terminal = requires(T t)
{
	requires std::is_enum_v<T>;
    { T::start };
};

export template<typename T, typename TerminalType = std::variant_alternative_t<0, T>>
concept is_token_type = requires(T t)
{
    requires is_terminal<TerminalType>;
};

export template<class T>
concept is_lexer_token = requires(T t, const T& u)
{
    requires is_token_type<decltype(t.type)>;
    { t.lexeme } -> std::convertible_to<std::string_view>;
    { t.afterConstruction(u) } -> std::same_as<void>;
};

enum class LexerErrorToken
{
    UNINITIALISED,
    ERR_SYMBOL,
    ERR_PATTERN
};

export struct TransitionInfo
{
    int from{};
    int to{};
    std::string_view pattern{};
    int default_transition_state{ -1 };
};

export template <is_terminal TerminalType>
struct FinalStateInfo
{
    int state_no;
    TerminalType token_type;
};

export template <is_terminal TerminalType>
struct KeywordInfo
{
    std::string_view keyword;
    TerminalType token_type;
};

export template <is_lexer_token LT, is_non_terminal NonTerminalType>
struct ASTNode
{
    using TerminalType = std::variant_alternative_t<0, decltype(LT().type)>;
    std::variant<TerminalType, NonTerminalType> node_symbol_type;

    std::unique_ptr<LT> lexer_token;

    std::vector<std::unique_ptr<ASTNode>> children{};
    std::unique_ptr<ASTNode> sibling = nullptr;
};

export template <is_lexer_token LT, is_non_terminal NonTerminalType>
struct ParseTreeNode
{
    using LeafType = std::unique_ptr<LT>;
    using InternalNodeType = std::unique_ptr<ParseTreeNode>;

    NonTerminalType node_type;
    int parent_child_index;

    ParseTreeNode const* parent;
    std::variant<std::vector<LeafType>, std::vector<InternalNodeType>> descendants;
};

export template <is_non_terminal NonTerminalType, is_terminal TerminalType, int max_prod_len=30>
struct ProductionInfo
{
    using TType = TerminalType;
    using NTType = NonTerminalType;
    using ProdType = std::vector<std::variant<NonTerminalType, TerminalType>>;

    NonTerminalType start;
    std::array<std::variant<NonTerminalType, TerminalType>, max_prod_len> production;
    std::size_t size;

    constexpr ProductionInfo(NonTerminalType start, const ProdType& production)
        : start(start), size(production.size())
    {
        for (std::size_t i = 0; i < production.size(); ++i)
			this->production[i] = production[i];
	}
};

export template<is_terminal TT, is_non_terminal NTT, is_lexer_token LT>
struct EngineTypes
{
    using TerminalType = TT;
    using NonTerminalType = NTT;
    using SymbolType = std::variant<TerminalType, NonTerminalType>;

    using TransitionInfo = TransitionInfo;
    using FinalStateInfo = FinalStateInfo<TerminalType>;
    using KeywordInfo = KeywordInfo<TerminalType>;

    using LexerErrorToken = LexerErrorToken;
    using LexerTokenType = LT;

    using ProductionInfo = ProductionInfo<NonTerminalType, TerminalType>;
    using ParseTreeNode = ParseTreeNode<LexerTokenType, NonTerminalType>;
    using ASTNode = ASTNode<LexerTokenType, NonTerminalType>;
};

export template <typename T>
concept engine_types_t = requires(T t)
{
    {T::TerminalType};
    {T::NonTerminalType};
    {T::SymbolType};

    {T::TransitionInfo};
    {T::FinalStateInfo};
    {T::KeywordInfo};

    {T::LexerErrorToken};
    {T::LexerTokenType};

    {T::ProductionInfo};
    {T::ParseTreeNode};
    {T::ASTNode};
};