export module ArithmeticParser;

import <algorithm>;
import <array>;
import <string_view>;
import <vector>;
import <variant>;
import <ranges>;
import <memory>;
import <cassert>;

import compiler;
import helpers;

using namespace std;

enum class ATerminal
{
    eps,
    TK_EOF,

    NUMBER,
    PLUS,
    WHITESPACE,
};

enum class ANonTerminal
{
    start,
    expression
};

struct ALexerToken
{
    std::variant<ATerminal, ELexerError> type = ELexerError::UNINITIALISED;
    std::string_view lexeme{};

    constexpr void after_construction(const ALexerToken& previous_token)
    {

    }

    constexpr bool discard() const
    {
        return type == ATerminal::WHITESPACE;
    }

    template <typename T>
    friend constexpr T& operator<<(T& out, const ALexerToken& tk)
    {
        return out << "{ type: " << tk.type << ", lexeme: '" << tk.lexeme << "' }";
    }
};

static consteval auto get_arithmetic_lexer()
{
    using enum ATerminal;

    constexpr auto transitions = []()
        {
            return array
            {
                TransitionInfo{0, 1, "0123456789"},
                TransitionInfo{1, 1, "0123456789"},
                TransitionInfo{0, 2, "+"},
                TransitionInfo{0, 3, " \t\r\n"},
                TransitionInfo{3, 3, " \t\r\n"},
            };
        };

    constexpr auto final_states = []()
        {
            return array
            {
                FinalStateInfo{1, NUMBER},
                FinalStateInfo{2, PLUS},
                FinalStateInfo{3, WHITESPACE},
            };
        };

    return build_lexer<LexerTypes<ALexerToken>>(transitions, final_states);
}

export consteval auto get_arithmetic_parser()
{
    using enum ANonTerminal;
    using enum ATerminal;
    using PI = ProductionInfo<LexerTypes<ALexerToken>, ANonTerminal, 5>;
    return build_parser([]() { return array{
        PI(start, NUMBER, expression, TK_EOF),
        PI(expression, eps),
        PI(expression, PLUS, NUMBER, expression),
        }; }, get_arithmetic_lexer());
}

struct start_parser
{
    template <IsParseTreeNode ptn, IsASTNode astn>
    static constexpr auto to_ast(auto converter, unique_ptr<ptn> node, unique_ptr<astn> inherited) -> unique_ptr<astn>
    {
        assert(node->descendants.size() == 3);

        auto e = node->extract_child_node(1);
        auto ast = make_unique<astn>(node->extract_child_leaf(0));

        return converter(std::move(e), std::move(ast));
    }
};

struct expression_parser
{
    template <IsParseTreeNode ptn, IsASTNode astn>
    static constexpr auto to_ast(auto converter, unique_ptr<ptn> node, unique_ptr<astn> inherited) -> unique_ptr<astn>
    {
        assert(node->descendants.size() == 0 || node->descendants.size() == 3);

        if (node->descendants.size() == 0)
            return std::move(inherited);

        // E -> + Num E
        auto plus = node->extract_child_leaf(0);
        auto num = node->extract_child_leaf(1);
        auto e = node->extract_child_node(2);

        auto ast = make_unique<astn>(std::move(plus));

        if (inherited->descendants.size() > 0)
            ast->descendants = std::move(inherited->descendants);
        else
            ast->descendants.push_back(std::move(inherited));
        ast->descendants.push_back(make_unique<astn>(std::move(num)));

        return converter(std::move(e), std::move(ast));
    }
};

template<CENonTerminal ENonTerminal, typename T>
using P = std::pair<ENonTerminal, T>;

export template <IsParseTreeNode ParseNodeType>
constexpr auto get_arithmetic_ast(std::unique_ptr<ParseNodeType> parse_tree)
{
    using LexerTypes = typename ParseNodeType::LexerTypes;
    using ENonTerminal = typename ParseNodeType::ENonTerminal;
    using ASTNodeType = ASTNode<LexerTypes, ENonTerminal>;

    return build_visitor(
        P{ ANonTerminal::start, start_parser{} },
        P{ ANonTerminal::expression, expression_parser{} }
    ).visit<ParseNodeType, ASTNodeType>(std::move(parse_tree));
}