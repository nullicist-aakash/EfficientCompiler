export module RegexParser;
/*
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

enum class RegexTerminal
{
    eps,
    IDENTIFIER,
    TK_EOF,

    CHAR,
    OR,
    BRACKET_OPEN,
    BRACKET_CLOSE,
    CLASS_OPEN,
    CLASS_CLOSE,
    STAR,
    PLUS,
    QUESTION_MARK,
    DOT,
    CARET,
    MINUS,
    BACKSLASH,

    NEWLINE,
    TAB,
    SPACE
};

enum class RegexNonTerminal
{
    start
};

struct ALexerToken
{
    std::variant<RegexTerminal, ELexerError> type = ELexerError::UNINITIALISED;
    std::string_view lexeme{};

    constexpr void after_construction(const ALexerToken& previous_token)
    {

    }

    constexpr bool discard() const
    {
        return false;
    }

    template <typename T>
    friend constexpr T& operator<<(T& out, const ALexerToken& tk)
    {
        return out << "{ type: " << tk.type << ", lexeme: '" << tk.lexeme << "' }";
    }
};*/