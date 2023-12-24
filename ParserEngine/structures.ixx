export module compiler_engine.structures;

import helpers.flatmap;
import <string_view>;
import <type_traits>;
import <array>;
import <vector>;
import <concepts>;
import <variant>;
import <cassert>;

export enum class SpecialToken;

export template<typename T>
concept is_terminal = requires(T t)
{
    requires std::is_enum_v<T>;
    { T::IDENTIFIER };
};

export template<typename T>
concept is_non_terminal = requires(T t)
{
	requires std::is_enum_v<T>;
    { T::eps };
    { T::start };
};

export template<typename T, typename TerminalType = std::variant_alternative_t<0, T>>
concept is_token_type = requires(T t)
{
    requires is_terminal<TerminalType>;
    requires !std::is_same_v<TerminalType, SpecialToken>;
    requires std::is_same_v<std::variant_alternative_t<1, T>, SpecialToken>;
};

export template<class T>
concept is_lexer_token = requires(T t, const T& u)
{
    requires is_token_type<decltype(t.type)>;
    { t.lexeme } -> std::convertible_to<std::string_view>;
    { t.afterConstruction(u) } -> std::same_as<void>;
};

enum class SpecialToken
{
    UNINITIALISED,
    ERR_SYMBOL,
    ERR_PATTERN,
    END_INPUT
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

export template <is_non_terminal NonTerminalType, is_terminal TerminalType, int max_prod_len=30>
struct ProductionInfo
{
    NonTerminalType start;
    std::array<std::variant<TerminalType, NonTerminalType>, max_prod_len> production;
    std::size_t size;

    constexpr ProductionInfo(NonTerminalType start, std::vector<std::variant<TerminalType, NonTerminalType>> production)
        : start(start), size(production.size())
    {
        for (std::size_t i = 0; i < production.size(); ++i)
			this->production[i] = production[i];
	}
};