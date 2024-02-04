module RegexParser:ast;

import :structures;
import :parser;
import compiler;

import <memory>;
import <variant>;
import <string_view>;

using std::unique_ptr;
using RegexParser::ast_output;
using RegexParser::NonTerminal;
using RegexParser::Terminal;

namespace 
{
    struct start_parser
    {
        template <IsParseTreeNode ptn, IsASTNode astn>
        static constexpr auto to_ast(auto converter, unique_ptr<ptn> node, unique_ptr<astn> inherited) -> unique_ptr<astn>
        {
            return converter(node->extract_child_node(0), nullptr);
        }
    };

    struct regex_parser
    {
        template <IsParseTreeNode ptn, IsASTNode astn>
        static constexpr auto to_ast(auto converter, unique_ptr<ptn> node, unique_ptr<astn> inherited) -> unique_ptr<astn>
        {
            // regex -> term terms_continue
            return converter(node->extract_child_node(1), converter(node->extract_child_node(0), nullptr));
        }
    };

    struct terms_continue_parser
    {
        template <IsParseTreeNode ptn, IsASTNode astn>
        static constexpr auto to_ast(auto converter, unique_ptr<ptn> node, unique_ptr<astn> inherited) -> unique_ptr<astn>
        {
            if (node->descendants.size() == 0)
                return std::move(inherited);

            // tc -> OR term tc
            auto root = std::make_unique<astn>(node->extract_child_leaf(0));

            if (inherited->node_symbol_type == Terminal::OR)
                root = std::move(inherited);
            else
                root->descendants.push_back(std::move(inherited));

            root->descendants.push_back(converter(node->extract_child_node(1), nullptr));

            return converter(node->extract_child_node(2), std::move(root));
        }
    };

    struct term_parser : regex_parser {};

    struct factors_continue_parser
    {
        template <IsParseTreeNode ptn, IsASTNode astn>
        static constexpr auto to_ast(auto converter, unique_ptr<ptn> node, unique_ptr<astn> inherited) -> unique_ptr<astn>
        {
            if (node->descendants.size() == 0)
                return std::move(inherited);

            // factors_continue => factor factors_continue
            auto root = std::make_unique<astn>(Terminal::CONCAT);

            if (inherited->node_symbol_type == Terminal::CONCAT)
                root = std::move(inherited);
            else
                root->descendants.push_back(std::move(inherited));

            root->descendants.push_back(converter(node->extract_child_node(0), nullptr));

            return converter(node->extract_child_node(1), std::move(root));
        }
    };

    struct factor_parser : regex_parser {};

    struct factor_core_parser
    {
        template <IsParseTreeNode ptn, IsASTNode astn>
        static constexpr auto to_ast(auto converter, unique_ptr<ptn> node, unique_ptr<astn> inherited) -> unique_ptr<astn>
        {
            // factor_core -> CHAR
            // factor_core -> DOT
            // factor_core -> EMPTY
            if (node->descendants.size() == 1)
                return std::make_unique<astn>(node->extract_child_leaf(0));

            // factor_core -> ( regex )
            // factor_core -> [ class ]
            return converter(node->extract_child_node(1), nullptr);
        }
    };

    struct factor_suffix_parser
    {
        template <IsParseTreeNode ptn, IsASTNode astn>
        static constexpr auto to_ast(auto converter, unique_ptr<ptn> node, unique_ptr<astn> inherited) -> unique_ptr<astn>
        {
            // factor_suffix -> eps
            if (node->descendants.size() == 0)
                return std::move(inherited);

            // factor_suffix -> STAR
            // factor_suffix -> PLUS
            // factor_suffix -> QUESTION_MARK
            auto root = std::make_unique<astn>(node->extract_child_leaf(0));
            root->descendants.push_back(std::move(inherited));
            return root;
        }
    };

    struct class_parser
    {
        template <IsParseTreeNode ptn, IsASTNode astn>
        static constexpr auto to_ast(auto converter, unique_ptr<ptn> node, unique_ptr<astn> inherited) -> unique_ptr<astn>
        {
            // _class => CHAR class_mid
            // _class => CARET class_end
            auto root = std::make_unique<astn>(NonTerminal::_class);
            root->descendants.emplace_back(std::make_unique<astn>(node->extract_child_leaf(0)));
            return converter(node->extract_child_node(1), std::move(root));
        }
    };

    struct class_mid_parser
    {
        template <IsParseTreeNode ptn, IsASTNode astn>
        static constexpr auto to_ast(auto converter, unique_ptr<ptn> node, unique_ptr<astn> inherited) -> unique_ptr<astn>
        {
            // class_mid => CHAR class_mid
            // class_mid => MINUS CHAR class_end
            // class_mid => eps

            if (node->descendants.size() == 0)
                return std::move(inherited);

            if (node->descendants.size() == 2)
            {
                inherited->descendants.emplace_back(std::make_unique<astn>(node->extract_child_leaf(0)));
                return converter(node->extract_child_node(1), std::move(inherited));
            }

            auto child_root = std::make_unique<astn>(Terminal::MINUS);
            child_root->descendants.push_back(std::move(inherited->descendants.back()));
            inherited->descendants.pop_back();

            child_root->descendants.emplace_back(std::make_unique<astn>(node->extract_child_leaf(1)));
            inherited->descendants.push_back(std::move(child_root));
            return converter(node->extract_child_node(2), std::move(inherited));
        }
    };

    struct class_end_parser
    {
        template <IsParseTreeNode ptn, IsASTNode astn>
        static constexpr auto to_ast(auto converter, unique_ptr<ptn> node, unique_ptr<astn> inherited) -> unique_ptr<astn>
        {
            // class_end => CHAR class_mid
            // class_end => eps
            if (node->descendants.size() == 0)
                return std::move(inherited);

            inherited->descendants.emplace_back(std::make_unique<astn>(node->extract_child_leaf(0)));
            return converter(node->extract_child_node(1), std::move(inherited));
        }
    };
}

template <typename T>
using P = std::pair<NonTerminal, T>;

template <IsParseTreeNode ParseNodeType>
constexpr auto RegexParser::get_ast(std::unique_ptr<ParseNodeType> parse_tree) -> std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>
{
    using LexerTypes = typename ParseNodeType::LexerTypes;
    using ENonTerminal = typename ParseNodeType::ENonTerminal;
    using ASTNodeType = ASTNode<LexerTypes, ENonTerminal>;

    return build_visitor(
        P{ NonTerminal::start, start_parser{} },
        P{ NonTerminal::regex, regex_parser{} },
        P{ NonTerminal::terms_continue, terms_continue_parser{} },
        P{ NonTerminal::term, term_parser{} },
        P{ NonTerminal::factors_continue, factors_continue_parser{} },
        P{ NonTerminal::factor, factor_parser{} },
        P{ NonTerminal::factor_core, factor_core_parser{} },
        P{ NonTerminal::factor_suffix, factor_suffix_parser{} },
        P{ NonTerminal::_class, class_parser{} },
        P{ NonTerminal::class_mid, class_mid_parser{} },
        P{ NonTerminal::class_end, class_end_parser{} }
    ).visit<ParseNodeType, ASTNodeType>(std::move(parse_tree));
}

constexpr auto RegexParser::get_ast(std::string_view sv) -> ast_output
{
    auto parser = RegexParser::get_parser();
    auto result = parser(sv);
    ast_output out;
    out.errors = std::move(result.errors);
    out.logs = std::move(result.logs);

    if (out.errors == "")
        out.root = RegexParser::get_ast(std::move(result.root));

    return out;
}