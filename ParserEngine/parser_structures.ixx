export module compiler.parser:structures;

import compiler.lexer;
import <algorithm>;
import <array>;
import <concepts>;
import <functional>;
import <iterator>;
import <memory>;
import <string_view>;
import <type_traits>;
import <variant>;
import <vector>;

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
export template <CLexerTypes LexerTypes, CENonTerminal ENonTerminal>
struct ParseTreeNode
{
    using LeafType = std::unique_ptr<typename LexerTypes::ILexerToken>;
    using InternalNodeType = std::unique_ptr<ParseTreeNode>;

    const ENonTerminal node_type{};

    ParseTreeNode* const parent{};
    const int parent_child_index{ -1 };

    int production_number { -1 };

    std::vector<std::variant<LeafType, InternalNodeType>> descendants{};

    template<typename ostream>
    constexpr void print(ostream& os, int depth = 0) const
    {
        for (int i = 0; i < depth; ++i) os << "\t";
		os << node_type << '\n';
		for (const auto& descendant : descendants)
            if (std::holds_alternative<LeafType>(descendant))
            {
                for (int i = 0; i <= depth; ++i) os << "\t";
                os << *std::get<LeafType>(descendant) << '\n';
            }
            else
				std::get<InternalNodeType>(descendant)->print(os, depth + 1);
    }
};

export template<typename ostream, CLexerTypes LexerTypes, CENonTerminal ENonTerminal>
constexpr ostream& operator<<(ostream& os, const ParseTreeNode<LexerTypes, ENonTerminal>& node)
{
    node.print(os, 0);
    return os;
}

export template <CLexerTypes _LexerTypes, CENonTerminal _ENonTerminal, int max_prod_len = 30>
struct ProductionInfo
{
    using LexerTypes = _LexerTypes;
    using ETerminal = LexerTypes::ETerminal;
    using ENonTerminal = _ENonTerminal;
    using EParserSymbol = std::variant<ETerminal, ENonTerminal>;

    const ENonTerminal start;
    const std::array<EParserSymbol, max_prod_len> production;
    const std::size_t size;

    template <typename StartType, typename... ProdType>
    constexpr ProductionInfo(StartType start, ProdType... production)
        : start(start), production{ production... }, size(sizeof...(production))
    {
        static_assert(sizeof...(production) > 0, "Can't have a production with no expansion.");
    }
};

// Parser types
export template <typename T>
concept CParserTypes = requires()
{
    requires CLexerTypes<LexerTypes<typename T::ILexerToken>>;
    requires CLexerToken<typename T::ILexerToken>;
    requires CENonTerminal<typename T::ENonTerminal>;
    requires std::same_as<typename T::EParserSymbol, std::variant<typename T::ETerminal, typename T::ENonTerminal>>;
    requires std::same_as<typename T::ProductionInfo, ProductionInfo<LexerTypes<typename T::ILexerToken>, typename T::ENonTerminal>>;
    requires std::same_as<typename T::ParseTreeNode, ParseTreeNode<LexerTypes<typename T::ILexerToken>, typename T::ENonTerminal>>;
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

    using ProductionInfo = ProductionInfo<LexerTypes, ENonTerminal>;
    using ParseTreeNode = ParseTreeNode<LexerTypes, ENonTerminal>;
};