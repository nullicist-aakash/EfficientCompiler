export module RegexParser:parser;
import :structures;
import :lexer;

import compiler;

namespace RegexParser
{
    export constexpr auto get_parser()
    {
        using enum NonTerminal;
        using enum Terminal;
        using PI = ProductionInfo<LexerTypes<LexerToken>, NonTerminal, 30>;

        return build_parser([]()
            {
                return std::array
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
            }, []() { return RegexParser::get_lexer(); });
    }
}