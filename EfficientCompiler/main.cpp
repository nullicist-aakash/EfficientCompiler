import <string_view>;
import <array>;
import <vector>;
import <variant>;
import <ranges>;
import <algorithm>;
import compiler_engine.lexer;
import compiler_engine.parser;
import compiler_engine.structures;
import helpers.reflection;

using std::string_view;
using std::array;

#include <numeric>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

enum class Terminal
{
    eps,
    COMMENT,
    CLASS,
    CONSTRUCTOR,
    FUNCTION,
    METHOD,
    FIELD,
    STATIC,
    VAR,
    INT,
    CHAR,
    BOOLEAN,
    VOID,
    TRUE,
    FALSE,
    TK_NULL,
    THIS,
    LET,
    DO,
    IF,
    ELSE,
    WHILE,
    RETURN,
    CURO,
    CURC,
    PARENO,
    PARENC,
    BRACKO,
    BRACKC,
    DOT,
    COMMA,
    SEMICOLON,
    PLUS,
    MINUS,
    MULT,
    DIV,
    AND,
    OR,
    LE,
    GE,
    EQ,
    NOT,
    NUM,
    STR,
    IDENTIFIER,
    WHITESPACE,
    TK_EOF
};

enum class NonTerminal
{
    start,
    _class,
    class_vars,
    subroutineDecs,
    class_var,
    class_var_prefix,
    type,
    more_identifiers,
    subroutineDec,
    subroutine_prefix,
    subroutine_type,
    parameters,
    more_parameters,
    subroutine_body,
    routine_vars,
    routine_var,
    statements,
    statement,
    let_statement,
    identifier_suffix,
    if_statement,
    else_statement,
    while_statement,
    do_statement,
    return_statement,
    return_suffix,
    expression,
    expression_suffix,
    term,
    term_sub_iden,
    op,
    subroutine_call,
    subroutine_scope,
    expression_list,
    more_expressions
};

struct LexerToken
{
    variant<Terminal, LexerErrorToken> type = LexerErrorToken::UNINITIALISED;
    std::string_view lexeme{};
    int line_num{1};

    constexpr void afterConstruction(const LexerToken& previous_token)
    {
        line_num = previous_token.line_num + 
            (int)std::count(previous_token.lexeme.begin(), previous_token.lexeme.end(), '\n');
    }
};

template <typename T, typename U, typename V>
constexpr T& operator<<(T& out, const std::variant<U, V>& vr)
{
    if (holds_alternative<U>(vr))
        return out << get<U>(vr);
    return out << get<V>(vr);
}

template <typename T>
constexpr T& operator<<(T& out, const LexerToken& tk)
{
    out << tk.line_num << " " << tk.type << " " << tk.lexeme;
    return out;
}

static consteval auto get_lexer()
{
    using enum Terminal;

    constexpr auto transitions = []()
        {
            return array
            {
                TransitionInfo{0, 1, "{"},
                TransitionInfo{0, 2, "}"},
                TransitionInfo{0, 3, "("},
                TransitionInfo{0, 4, ")"},
                TransitionInfo{0, 5, "["},
                TransitionInfo{0, 6, "]"},
                TransitionInfo{0, 7, "."},
                TransitionInfo{0, 8, ","},
                TransitionInfo{0, 9, ";"},
                TransitionInfo{0, 10, "+"},
                TransitionInfo{0, 11, "-"},
                TransitionInfo{0, 12, "*"},
                TransitionInfo{0, 13, "/"},
                TransitionInfo{13, 25, "/"},
                TransitionInfo{.from = 25, .to = -1, .pattern = "\n", .default_transition_state = 25 },
                TransitionInfo{13, 26, "*"},
                TransitionInfo{.from = 26, .to = -1, .pattern = "*", .default_transition_state = 26 },
                TransitionInfo{.from = 27, .to = -1, .pattern = "*/", .default_transition_state = 26 },
                TransitionInfo{26, 27, "*"},
                TransitionInfo{27, 27, "*"},
                TransitionInfo{27, 28, "/" },
                TransitionInfo{0, 14, "&"},
                TransitionInfo{0, 15, "|"},
                TransitionInfo{0, 16, "<"},
                TransitionInfo{0, 17, ">"},
                TransitionInfo{0, 18, "="},
                TransitionInfo{0, 19, "~"},
                TransitionInfo{0, 20, "0123456789"},
                TransitionInfo{20, 20, "0123456789"},
                TransitionInfo{0, 21, "\""},
                TransitionInfo{.from = 21, .to = -1, .pattern = "\r\n\"", .default_transition_state = 21 },
                TransitionInfo{21, 22, "\""},
                TransitionInfo{0, 23, "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPLKJHGFDSAZXCVBNM_"},
                TransitionInfo{23, 23, "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM_1234567890*/"},
                TransitionInfo{0, 24, " \r\t\n"},
                TransitionInfo{24, 24, " \r\t\n"},
            };
        };

    constexpr auto final_states = []()
        {
            return array
            {
                FinalStateInfo{1, CURO},
                FinalStateInfo{2, CURC},
                FinalStateInfo{3, PARENO},
                FinalStateInfo{4, PARENC},
                FinalStateInfo{5, BRACKO},
                FinalStateInfo{6, BRACKC},
                FinalStateInfo{7, DOT},
                FinalStateInfo{8, COMMA},
                FinalStateInfo{9, SEMICOLON},
                FinalStateInfo{10, PLUS},
                FinalStateInfo{11, MINUS},
                FinalStateInfo{12, MULT},
                FinalStateInfo{13, DIV},
                FinalStateInfo{14, AND},
                FinalStateInfo{15, OR},
                FinalStateInfo{16, LE},
                FinalStateInfo{17, GE},
                FinalStateInfo{18, EQ},
                FinalStateInfo{19, NOT},
                FinalStateInfo{20, NUM},
                FinalStateInfo{22, STR},
                FinalStateInfo{23, IDENTIFIER},
                FinalStateInfo{24, WHITESPACE},
                FinalStateInfo{25, COMMENT},
                FinalStateInfo{28, COMMENT},
            };
        };

    constexpr auto keywords = []()
        {
            return array
            {
                KeywordInfo{ "class", CLASS },
                KeywordInfo{ "constructor", CONSTRUCTOR },
                KeywordInfo{ "function", FUNCTION },
                KeywordInfo{ "method", METHOD },
                KeywordInfo{ "field", FIELD },
                KeywordInfo{ "static", STATIC },
                KeywordInfo{ "var", VAR },
                KeywordInfo{ "int", INT },
                KeywordInfo{ "char", CHAR },
                KeywordInfo{ "boolean", BOOLEAN },
                KeywordInfo{ "void", VOID },
                KeywordInfo{ "true", TRUE },
                KeywordInfo{ "false", FALSE },
                KeywordInfo{ "null", TK_NULL },
                KeywordInfo{ "this", THIS },
                KeywordInfo{ "let", LET },
                KeywordInfo{ "do", DO },
                KeywordInfo{ "if", IF },
                KeywordInfo{ "else", ELSE },
                KeywordInfo{ "while", WHILE },
                KeywordInfo{ "return", RETURN }
            };
        };

    return build_lexer<LexerToken>(transitions, final_states, keywords);
}

static consteval auto get_parser()
{
    using enum NonTerminal;
    using enum Terminal;
    using PI = ProductionInfo<NonTerminal, Terminal, 30>;
    return build_parser([]() { return array {
        PI(start, { _class, TK_EOF }),
        PI(_class, { CLASS, IDENTIFIER, CURO, class_vars, subroutineDecs, CURC }),
        PI(class_vars, { class_var, class_vars }),
        PI(class_vars, { eps }),
        PI(subroutineDecs, { subroutineDec, subroutineDecs }),
        PI(subroutineDecs, { eps }),
        PI(class_var, { class_var_prefix, type, IDENTIFIER, more_identifiers, SEMICOLON }),
        PI(class_var_prefix, { STATIC }),
        PI(class_var_prefix, { FIELD }),
        PI(type, { INT }),
        PI(type, { CHAR }),
        PI(type, { BOOLEAN }),
        PI(type, { IDENTIFIER }),
        PI(more_identifiers, { COMMA, IDENTIFIER, more_identifiers }),
        PI(more_identifiers, { eps }),
        PI(subroutineDec, { subroutine_prefix, subroutine_type, IDENTIFIER, PARENO, parameters, PARENC, subroutine_body }),
        PI(subroutine_prefix, { CONSTRUCTOR }),
        PI(subroutine_prefix, { FUNCTION }),
        PI(subroutine_prefix, { METHOD }),
        PI(subroutine_type, { type }),
        PI(subroutine_type, { VOID }),
        PI(parameters, { type, IDENTIFIER, more_parameters }),
        PI(parameters, { eps }),
        PI(more_parameters, { COMMA, type, IDENTIFIER, more_parameters }),
        PI(more_parameters, { eps }),
        PI(subroutine_body, { CURO, routine_vars, statements, CURC }),
        PI(routine_vars, { routine_var, routine_vars }),
        PI(routine_vars, { eps }),
        PI(routine_var, { VAR, type, IDENTIFIER, more_identifiers, SEMICOLON }),
        PI(statements, { statement, statements }),
        PI(statements, { eps }),
        PI(statement, { let_statement }),
        PI(statement, { if_statement }),
        PI(statement, { while_statement }),
        PI(statement, { do_statement }),
        PI(statement, { return_statement }),
        PI(let_statement, { LET, IDENTIFIER, identifier_suffix, EQ, expression, SEMICOLON }),
        PI(identifier_suffix, { eps }),
        PI(identifier_suffix, { BRACKO, expression, BRACKC }),
        PI(if_statement, { IF, PARENO, expression, PARENC, CURO, statements, CURC, else_statement }),
        PI(else_statement, { eps }),
        PI(else_statement, { ELSE, CURO, statements, CURC }),
        PI(while_statement, { WHILE, PARENO, expression, PARENC, CURO, statements, CURC }),
        PI(do_statement, { DO, subroutine_call, SEMICOLON }),
        PI(return_statement, { RETURN, return_suffix, SEMICOLON }),
        PI(return_suffix, { eps }),
        PI(return_suffix, { expression }),
        PI(expression, { term, expression_suffix }),
        PI(expression_suffix, { op, term, expression_suffix }),
        PI(expression_suffix, { eps }),
        PI(term, { NUM }),
        PI(term, { STR }),
        PI(term, { TRUE }),
        PI(term, { FALSE }),
        PI(term, { TK_NULL }),
        PI(term, { THIS }),
        PI(term, { IDENTIFIER, term_sub_iden }),
        PI(term_sub_iden, { eps }),
        PI(term_sub_iden, { PARENO, expression_list, PARENC }),
        PI(term_sub_iden, { DOT, IDENTIFIER, PARENO, expression_list, PARENC }),
        PI(term_sub_iden, { BRACKO, expression, BRACKC }),
        PI(term, { PARENO, expression, PARENC }),
        PI(term, { MINUS, term }),
        PI(term, { NOT, term }),
        PI(op, { PLUS }),
        PI(op, { MINUS }),
        PI(op, { MULT }),
        PI(op, { DIV }),
        PI(op, { AND }),
        PI(op, { OR }),
        PI(op, { LE }),
        PI(op, { GE }),
        PI(op, { EQ }),
        PI(subroutine_call, { IDENTIFIER, subroutine_scope, PARENO, expression_list, PARENC }),
        PI(subroutine_scope, { DOT, IDENTIFIER }),
        PI(subroutine_scope, { eps }),
        PI(expression_list, { expression, more_expressions }),
        PI(expression_list, { eps }),
        PI(more_expressions, { COMMA, expression, more_expressions }),
        PI(more_expressions, { eps }),
    }; });
}

static auto read_file(string_view filename)
{
	ifstream file{ filename.data() };
	ostringstream ss;
	ss << file.rdbuf(); // reading data
	return ss.str();
}

int main()
{
    constexpr auto lexer = get_lexer();
    auto contents = read_file("source.jack");

    auto par = get_parser();
    for (int i = 0; i < par.size(); ++i)
    {
        cout << (NonTerminal)i << " -> ";
        for (int j = 0; j < par[i].size(); ++j)
            if (par[i][j])
                cout << (Terminal)j << " ";
        cout << endl;
    }
}