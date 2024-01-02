export module RegexParser;

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

namespace RegexParser
{
    enum class Terminal
    {
        eps,
        TK_EOF,

        CHAR,
        DOT,
        EMPTY,

        OR,

        BRACKET_OPEN,
        BRACKET_CLOSE,
        CLASS_OPEN,
        CLASS_CLOSE,

        STAR,
        PLUS,
        QUESTION_MARK,

        CARET,
        MINUS,

        ESCAPED_CHAR,
    };

    enum class NonTerminal
    {
        start,
        regex,
        terms_continue,
        term,
        factors_continue,
        factor,
        factor_core,
        factor_suffix,
        _class,
        // These following 2 names have no meaning, the names were chosen based on dfa on paper
        class_mid,
        class_end
    };

    struct LexerToken
    {
        std::variant<Terminal, ELexerError> type = ELexerError::UNINITIALISED;
        std::string_view lexeme{};

        constexpr void after_construction(const LexerToken& previous_token)
        {
            if (type != Terminal::ESCAPED_CHAR)
                return;
            
            lexeme = lexeme.substr(1);
            type = Terminal::CHAR;

            if (lexeme[0] == 'e')
            {
                type = Terminal::EMPTY;
                lexeme = "";
            }
            else if (lexeme[0] == 'n')
                lexeme = "\n";
            else if (lexeme[0] == 'r')
                lexeme = "\r";
            else if (lexeme[0] == 't')
                lexeme = "\t";
        }

        constexpr bool discard() const
        {
            return false;
        }

        template <typename T>
        friend constexpr T& operator<<(T& out, const LexerToken& tk)
        {
            return out << "{ type: " << tk.type << ", lexeme: '" << tk.lexeme << "' }";
        }
    };

    constexpr auto get_lexer()
    {
        using enum Terminal;

        constexpr auto transitions = []()
            {
                return array
                {
                    TransitionInfo{.from = 0, .to = -1, .pattern = R"(|()*+[]-^?.\)", .default_transition_state = 1 },
                    TransitionInfo{0, 2, "|"},
                    TransitionInfo{0, 3, "("},
                    TransitionInfo{0, 4, ")"},
                    TransitionInfo{0, 5, "*"},
                    TransitionInfo{0, 6, "+"},
                    TransitionInfo{0, 7, "["},
                    TransitionInfo{0, 8, "]"},
                    TransitionInfo{0, 9, "-"},
                    TransitionInfo{0, 10, "^"},
                    TransitionInfo{0, 11, "?"},
                    TransitionInfo{0, 12, "."},
                    TransitionInfo{0, 13, R"(\)"},
                    TransitionInfo{13, 14, R"(|()*+[]-^?.\nrt)"},
                    TransitionInfo{13, 15, "e"},
                };
            };

        constexpr auto final_states = []()
            {
                return array
                {
                    FinalStateInfo{1, CHAR},
                    FinalStateInfo{2, OR},
                    FinalStateInfo{3, BRACKET_OPEN},
                    FinalStateInfo{4, BRACKET_CLOSE},
                    FinalStateInfo{5, STAR},
                    FinalStateInfo{6, PLUS},
                    FinalStateInfo{7, CLASS_OPEN},
                    FinalStateInfo{8, CLASS_CLOSE},
                    FinalStateInfo{9, MINUS},
                    FinalStateInfo{10, CARET},
                    FinalStateInfo{11, QUESTION_MARK},
                    FinalStateInfo{12, DOT},
                    FinalStateInfo{13, CHAR},
                    FinalStateInfo{14, ESCAPED_CHAR},
                    FinalStateInfo{15, EMPTY},
                };
            };

        return build_lexer<LexerTypes<LexerToken>>(transitions, final_states);
    }

    export constexpr auto get_parser()
    {
        using enum NonTerminal;
        using enum Terminal;
        using PI = ProductionInfo<LexerTypes<LexerToken>, NonTerminal, 30>;

        return build_parser([]() 
            {
                return array
                {
                    PI(start, regex, TK_EOF),
                    PI(regex, term, terms_continue),
                    
                    PI(terms_continue, eps),
                    PI(terms_continue, OR, term, terms_continue),
                    
                    PI(term, factor, factors_continue),
                    
                    PI(factors_continue, eps),
                    PI(factors_continue, factor, factors_continue),
                    
                    PI(factor, factor_core, factor_suffix),
                    
                    PI(factor_core, CHAR),
                    PI(factor_core, DOT),
                    PI(factor_core, EMPTY),
                    PI(factor_core, BRACKET_OPEN, regex, BRACKET_CLOSE),
                    PI(factor_core, CLASS_OPEN, _class, CLASS_CLOSE),
                    
                    PI(factor_suffix, eps),
                    PI(factor_suffix, STAR),
                    PI(factor_suffix, PLUS),
                    PI(factor_suffix, QUESTION_MARK),
                    
                    PI(_class, CHAR, class_mid),
                    PI(_class, CARET, class_end),
                    
                    PI(class_mid, CHAR, class_mid),
                    PI(class_mid, MINUS, CHAR, class_end),
                    PI(class_mid, eps),

                    PI(class_end, CHAR, class_mid),
                    PI(class_end, eps)
                };
            }, []() { return get_lexer(); });
    }
}

namespace RegexParser
{
    struct start_parser
    {
        template <IsParseTreeNode ptn, IsASTNode astn>
        static constexpr auto to_ast(auto converter, unique_ptr<ptn> node, unique_ptr<astn> inherited) -> unique_ptr<astn>
        {
            return std::make_unique<astn>(node->descendants[0]);
        }
    };

    template <typename T>
    using P = std::pair<NonTerminal, T>;

    export template <IsParseTreeNode ParseNodeType>
        constexpr auto get_ast(std::unique_ptr<ParseNodeType> parse_tree)
    {
        using LexerTypes = typename ParseNodeType::LexerTypes;
        using ENonTerminal = typename ParseNodeType::ENonTerminal;
        using ASTNodeType = ASTNode<LexerTypes, ENonTerminal>;
        
        return build_visitor(
            P{ NonTerminal::start, start_parser{} },
            P{ NonTerminal::regex, start_parser{} },
            P{ NonTerminal::terms_continue, start_parser{} },
            P{ NonTerminal::term, start_parser{} },
            P{ NonTerminal::factors_continue, start_parser{} },
            P{ NonTerminal::factor, start_parser{} },
            P{ NonTerminal::factor_core, start_parser{} },
            P{ NonTerminal::factor_suffix, start_parser{} },
            P{ NonTerminal::_class, start_parser{} },
            P{ NonTerminal::class_mid, start_parser{} },
            P{ NonTerminal::class_end, start_parser{} }
        ).visit<ParseNodeType, ASTNodeType>(std::move(parse_tree));
    }
}