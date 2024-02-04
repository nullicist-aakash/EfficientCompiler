export module RegexParser:structures;

import std;

import compiler;
import helpers;

namespace RegexParser
{
    export enum class Terminal
    {
        eps,
        TK_EOF,

        CHAR,
        DOT,
        EMPTY,

        OR,
        CONCAT, // only for AST purposes

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

    export enum class NonTerminal
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
        class_mid,
        class_end
    };

    export struct LexerToken
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

    export struct ast_output
    {
        std::string errors;
        std::string logs;
        std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>> root;
    };

    export struct Node
    {
    private:
        friend class NodeManager;
        std::size_t unique_index{ std::numeric_limits<std::size_t>::max() };
    public:
        std::vector<std::shared_ptr<Node>> empty_transitions{};
        std::vector<std::pair<std::string, std::shared_ptr<Node>>> tran_nodes{};
    };

    export struct NFA
    {
        std::shared_ptr<Node> start_node{};
        std::shared_ptr<Node> final_node{};
    };

    export class NodeManager
    {
    private:
        NodeManager() = default;
        NodeManager(const NodeManager&) = delete;
        NodeManager(NodeManager&&) = delete;
        NodeManager& operator=(const NodeManager&) = delete;
        NodeManager& operator=(NodeManager&&) = delete;

    public:
        std::vector<std::shared_ptr<Node>> all_nodes;
        static auto get_instance() -> NodeManager&
        {
            static NodeManager instance;
            return instance;
        }

        auto create_node() -> std::shared_ptr<Node>
        {
            all_nodes.emplace_back(std::make_shared<Node>());
            all_nodes.back()->unique_index = all_nodes.size() - 1;
            return all_nodes.back();
        }

        void del_node(std::shared_ptr<Node> node)
        {
            auto index = std::exchange(node->unique_index, std::numeric_limits<std::size_t>::max());
            all_nodes[index] = nullptr;

            if (index != all_nodes.size() - 1)
            {
                std::swap(all_nodes[index], all_nodes.back());
                all_nodes[index]->unique_index = index;
            }

            all_nodes.pop_back();
        }

        void garbage_collect()
        {
            for (int i = 0; i < all_nodes.size(); ++i)
            {
                if (all_nodes[i].use_count() > 1)
                    continue;

                del_node(all_nodes[i]);
            }
        }

        void delete_all_nodes()
        {
            all_nodes.clear();
        }
    };
}