export module compiler.ast:structures;

import compiler.lexer;
import compiler.parser;
import <memory>;
import <variant>;
import <vector>;

export template <CLexerTypes LexerTypes, CENonTerminal ENonTerminal>
struct ASTNode
{
    using ETerminal = LexerTypes::ETerminal;
    using ILexerToken = LexerTypes::ILexerToken;
    std::variant<ETerminal, ENonTerminal> node_symbol_type{};

    std::unique_ptr<ILexerToken> lexer_token{};

    std::vector<std::unique_ptr<ASTNode>> descendants{};
    std::unique_ptr<ASTNode> sibling{};
};

struct ASTVisitorBase
{
    
};