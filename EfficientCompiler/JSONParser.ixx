export module JSONParser;

import std;
import compiler;
import helpers;

using namespace std;

namespace JSONParser
{
    enum class Terminal
    {
        eps,
        TRUE,
        FALSE,
        NULL,
        INTEGER,
        DOUBLE,
        STRING,
        OBJ_START,
        OBJ_END,
        ARR_START,
        ARR_END,
        COLON,
        COMMA,
        WHITESPACE,
        TK_EOF
    };

    enum class NonTerminal
    {
        start,
        _json,
        json_object_start,
        json_object_member_start,
        json_object_member_end,
        json_array_start,
        json_array_member_start,
        json_array_member_end
    };

    struct LexerToken
    {
        std::variant<Terminal, ELexerError> type = ELexerError::UNINITIALISED;
        std::string_view lexeme{};
        int line_num{ 1 };

        constexpr void after_construction(const LexerToken& previous_token)
        {
            line_num = previous_token.line_num +
                (int)std::count(previous_token.lexeme.begin(), previous_token.lexeme.end(), '\n');
        }

        constexpr bool discard() const
        {
            return type == Terminal::WHITESPACE;
        }

        template <typename T>
        friend constexpr T& operator<<(T& out, const LexerToken& tk)
        {
            return out << "{ line_number: " << tk.line_num << ", type: " << tk.type << ", lexeme: '" << tk.lexeme << "' }";
        }
    };

    static constexpr auto get_lexer()
    {
        using enum Terminal;

        constexpr auto transitions = []()
            {
                return array
                {
                    TransitionInfo{0, 1, " \n\r\t"},
                    TransitionInfo{0, 2, "t"},
                    TransitionInfo{2, 3, "r"},
                    TransitionInfo{3, 4, "u"},
                    TransitionInfo{4, 5, "e"},
                    TransitionInfo{0, 6, "f"},
                    TransitionInfo{6, 7, "a"},
                    TransitionInfo{7, 8, "l"},
                    TransitionInfo{8, 9, "s"},
                    TransitionInfo{9, 10, "e"},
                    TransitionInfo{0, 11, "n"},
                    TransitionInfo{11, 12, "u"},
                    TransitionInfo{12, 13, "l"},
                    TransitionInfo{13, 14, "l"},
                    TransitionInfo{0, 15, "{"},
                    TransitionInfo{0, 16, "}"},
                    TransitionInfo{0, 17, "["},
                    TransitionInfo{0, 18, "]"},
                    TransitionInfo{0, 19, ":"},
                    TransitionInfo{0, 20, ","},
                    TransitionInfo{0, 21, "\""},
                    TransitionInfo{21, -1, "\\\"", 21},
                    TransitionInfo{21, 22, "\\"},
                    TransitionInfo{22, 21, "\"\\/bfnrt"},
                    TransitionInfo{21, 23, "\""},
                    TransitionInfo{0, 24, "-"},
                    TransitionInfo{0, 25, "0123456789"},
                    TransitionInfo{24, 25, "0123456789"},
                    TransitionInfo{25, 25, "0123456789"},
                    TransitionInfo{25, 26, "."},
                    TransitionInfo{26, 27, "0123456789"},
                    TransitionInfo{27, 27, "0123456789"},
                    TransitionInfo{25, 28, "eE"},
                    TransitionInfo{27, 28, "eE"},
                    TransitionInfo{28, 29, "0123456789"},
                    TransitionInfo{28, 30, "+-"},
                    TransitionInfo{29, 29, "0123456789"},
                    TransitionInfo{29, 30, "+-"},
                    TransitionInfo{30, 31, "0123456789"},
                    TransitionInfo{31, 31, "0123456789"}
                };
            };

        constexpr auto final_states = []()
            {
                return array
                {
                    FinalStateInfo{1, WHITESPACE},
                    FinalStateInfo{5, TRUE},
                    FinalStateInfo{10, FALSE},
                    FinalStateInfo{14, NULL},
                    FinalStateInfo{15, OBJ_START},
                    FinalStateInfo{16, OBJ_END},
                    FinalStateInfo{17, ARR_START},
                    FinalStateInfo{18, ARR_END},
                    FinalStateInfo{19, COLON},
                    FinalStateInfo{20, COMMA},
                    FinalStateInfo{23, STRING},
                    FinalStateInfo{25, INTEGER},
                    FinalStateInfo{27, DOUBLE},
                    FinalStateInfo{29, DOUBLE},
                    FinalStateInfo{31, DOUBLE},
                };
            };

        return build_lexer<LexerTypes<LexerToken>>(transitions, final_states);
    }
    
    static constexpr auto get_parser()
    {
        using enum NonTerminal;
        using enum Terminal;
        using PI = ProductionInfo<LexerTypes<LexerToken>, NonTerminal, 30>;
        return build_parser([]()
            {
                return std::array
                {
                    PI(start, _json, TK_EOF),
                    PI(_json, TRUE),
                    PI(_json, FALSE),
                    PI(_json, NULL),
                    PI(_json, STRING),
                    PI(_json, INTEGER),
                    PI(_json, DOUBLE),
                    PI(_json, json_object_start),
                    PI(_json, json_array_start),
                    PI(json_object_start, OBJ_START, json_object_member_start),
                    PI(json_object_member_start, STRING, COLON, _json, json_object_member_end),
                    PI(json_object_member_end, COMMA, json_object_member_start),
                    PI(json_object_member_end, OBJ_END),
                    PI(json_array_start, ARR_START, json_array_member_start),
                    PI(json_array_member_start, _json, json_array_member_end),
                    PI(json_array_member_end, COMMA, json_array_member_start),
                    PI(json_array_member_end, ARR_END)
                };
            }, []() { return JSONParser::get_lexer(); });
    }

    static auto compile_ast(auto parse_ptr, std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>> inherited) -> decltype(inherited)
    {
        using ParseNodeType = decltype(parse_ptr)::element_type;
        using ASTType = decltype(inherited)::element_type;

        const auto& descendant_token = [&](std::integral auto index) { return parse_ptr->extract_child_leaf(index); };
        const auto& descendant_nt = [&](std::integral auto index) { return parse_ptr->extract_child_node(index); };
        const auto& descendant_token_to_ast = [&](std::integral auto index) { return std::make_unique<ASTType>(descendant_token(index)); };
        const auto& is_descendant_token = [&](std::integral auto index) { return std::holds_alternative<ParseNodeType::LeafType>(parse_ptr->descendants[index]); };
        const auto& descendant_count = [&]() { return parse_ptr->descendants.size(); };

        if (parse_ptr == nullptr) std::terminate();

        const NonTerminal& node_type = parse_ptr->node_type;

        if (node_type == NonTerminal::start)
        {
            // start => _json TK_EOF
            return compile_ast(descendant_nt(0), {});
        }
        else if (node_type == NonTerminal::_json)
        {
            // _json => TRUE
            // _json => FALSE
            // _json => NULL
            // _json => STRING
            // _json => INTEGER
            // _json => DOUBLE
            // _json => json_object_start
            // _json => json_array_start
            if (is_descendant_token(0))
                return descendant_token_to_ast(0);
            return compile_ast(descendant_nt(0), {});
        }
        else if (node_type == NonTerminal::json_object_start)
        {
            // json_object_start => OBJ_START json_object_member_start
            if (inherited == nullptr)
                inherited = std::make_unique<ASTType>(NonTerminal::json_object_start);

            return compile_ast(descendant_nt(1), std::move(inherited));
        }
        else if (node_type == NonTerminal::json_object_member_start)
        {
            // json_object_member_start => STRING COLON _json json_object_member_end
            auto key = descendant_token_to_ast(0);
            auto value = compile_ast(descendant_nt(2), {});
            auto node = std::make_unique<ASTType>(NonTerminal::json_object_member_start);
            node->descendants.push_back(std::move(key));
            node->descendants.push_back(std::move(value));
            inherited->descendants.push_back(std::move(node));
            return compile_ast(descendant_nt(3), std::move(inherited));
        }
        else if (node_type == NonTerminal::json_object_member_end)
        {
            // json_object_member_end => COMMA json_object_member_start
            // json_object_member_end => OBJ_END
            if (descendant_count() == 2)
                return compile_ast(descendant_nt(1), std::move(inherited));
            return inherited;
        }
        else if (node_type == NonTerminal::json_array_start)
        {
            // json_array_start => ARR_START json_array_member_start
            inherited = std::make_unique<ASTType>(NonTerminal::json_array_start);
            return compile_ast(descendant_nt(1), std::move(inherited));
        }
        else if (node_type == NonTerminal::json_array_member_start)
        {
            // json_array_member_start => _json json_array_member_end
            auto value = compile_ast(descendant_nt(0), {});
            inherited->descendants.push_back(std::move(value));
            return compile_ast(descendant_nt(1), std::move(inherited));
        }
        else if (node_type == NonTerminal::json_array_member_end)
        {
            // json_array_member_end => COMMA json_array_member_start
            // json_array_member_end => ARR_END
            if (descendant_count() == 2)
                return compile_ast(descendant_nt(1), std::move(inherited));
            return inherited;
        }

        std::unreachable();
    }

    export auto get_ast(std::string_view json_content) -> std::expected<std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>>, std::string>
    {
        constexpr auto parser = get_parser();
        auto result = parser(json_content);

        if (!result.errors.empty())
            return std::unexpected(result.errors);

        return compile_ast(std::move(result.root), {});
    }

    struct json
    {
        using json_null = std::nullptr_t;
        using json_true = std::true_type;
        using json_false = std::false_type;
        using json_string = std::string_view;
        using json_integer = std::int64_t;
        using json_double = long double;
        using json_array = std::vector<std::unique_ptr<json>>;
        using json_object = std::map<json_string, std::unique_ptr<json>>;
        using underlying_type = std::variant<json_null, json_true, json_false, json_string, json_integer, json_double, json_array, json_object>;
        underlying_type value;

        constexpr bool is_null() const noexcept
        {
            return std::holds_alternative<json_null>(value);
        }

        constexpr bool is_true() const noexcept
        {
            return std::holds_alternative<json_true>(value);
        }

        constexpr bool is_false() const noexcept
        {
            return std::holds_alternative<json_false>(value);
        }

        constexpr bool is_string() const noexcept
        {
            return std::holds_alternative<json_string>(value);
        }

        constexpr bool is_integer() const noexcept
        {
            return std::holds_alternative<json_integer>(value);
        } 

        constexpr bool is_double() const noexcept
        {
            return std::holds_alternative<json_double>(value);
        }

        constexpr bool is_object() const
        {
            return std::holds_alternative<json_object>(value);
        }

        constexpr bool is_array() const
        {
            return std::holds_alternative<json_array>(value);
        }

        constexpr std::string_view get_string() const noexcept
		{
			return std::get<json_string>(value);
		}

        constexpr const json_integer& get_integer() const noexcept
        {
            return std::get<json_integer>(value);
        }

        constexpr const json_double& get_double() const noexcept
		{
			return std::get<json_double>(value);
		}

        constexpr const json_array& get_array() const noexcept
        {
            return std::get<json_array>(value);
        }

        constexpr const json_object& get_object() const noexcept
		{
			return std::get<json_object>(value);
		}

        const json& operator[](std::string_view key) const
        {
            auto&& map = std::get<json_object>(value);
            if (auto it = map.find(key); it != map.end())
				return *it->second;

            std::unreachable();
        }

        const json& operator[](std::integral auto index) const
		{
			return *std::get<json_array>(value).at(index);
		}

        friend std::ostream& operator<<(std::ostream& os, const json& j)
        {
            j.print(os, 0);
            return os;
        }

        friend bool operator==(const json& lhs, const json& rhs)
		{
			return lhs.value == rhs.value;
		}

        friend bool operator==(const json& lhs, bool rhs)
        {
            return (lhs.is_true() && rhs) || (lhs.is_false() && !rhs);
        }

        friend bool operator==(const json& lhs, std::string_view rhs)
        {
            return lhs.is_string() && lhs.get_string() == rhs;
        }

        friend bool operator==(const json& lhs, std::integral auto rhs)
		{
			return lhs.is_integer() && lhs.get_integer() == rhs;
		}

        friend bool operator==(const json& lhs, long double rhs)
        {
            return lhs.is_double() && lhs.get_double() == rhs;
        }

        friend bool operator==(const json& lhs, std::nullptr_t n)
		{
			return lhs.is_null();
		}

    private:
        void print(std::ostream& os, int tabs) const
        {
            if (is_null())
            {
                os << "null";
                return;
            }

            if (is_true())
            {
                os << "true";
                return;
            }

            if (is_false())
            {
                os << "false";
                return;
            }

            if (is_integer())
            {
                os << get_integer();
                return;
            }

            if (is_double())
            {
                os << get_double();
                return;
            }

            if (is_string())
            {
                os << "\"" << get_string() << "\"";
                return;
            }

            std::string tab_str(tabs, '\t');

            if (is_object())
            {
                const auto& obj = get_object();
                os << "{\n" << tab_str << "\t";
                for (auto it = obj.begin(); it != obj.end(); ++it)
				{
					os << "\"" << it->first << "\": ";
					it->second->print(os, tabs + 1);
					if (std::next(it) != obj.end())
						os << ",\n" << tab_str << "\t";
				}
                os << "\n" << tab_str << "}";
				return;
            }

            if (is_array())
            {
                const auto& arr = get_array();
                os << "[\n" << tab_str << "\t";
                for (auto it = arr.begin(); it != arr.end(); ++it)
                {
                    (*it)->print(os, tabs + 1);
                    if (std::next(it) != arr.end())
                        os << ",\n" << tab_str << "\t";
                }
                os << "\n" << tab_str << "]";
                return;
            }

            std::unreachable();
        }
    };

    static auto json_builder(std::unique_ptr<ASTNode<LexerTypes<LexerToken>, NonTerminal>> ast) -> std::unique_ptr<json>
    {
        if (ast->node_symbol_type == Terminal::TRUE)
            return std::make_unique<json>(json::json_true{});
        if (ast->node_symbol_type == Terminal::FALSE)
            return std::make_unique<json>(json::json_false{});
        if (ast->node_symbol_type == Terminal::NULL)
			return std::make_unique<json>(json::json_null{});
        if (ast->node_symbol_type == Terminal::STRING)
			return std::make_unique<json>(json::json_string{ ast->lexer_token->lexeme.substr(1, ast->lexer_token->lexeme.size() - 2)});
        if (ast->node_symbol_type == Terminal::INTEGER)
            return std::make_unique<json>(json::json_integer{ std::stoi(std::string(ast->lexer_token->lexeme)) });
        if (ast->node_symbol_type == Terminal::DOUBLE)
            return std::make_unique<json>(json::json_double{ std::stod(std::string(ast->lexer_token->lexeme)) });
        if (ast->node_symbol_type == NonTerminal::json_object_start)
        {
            auto mapping = json::json_object{};
            for (const auto& entries : ast->descendants)
            {
                if (entries->descendants.size() != 2)
                    std::terminate();
                const auto key = entries->descendants.at(0)->lexer_token->lexeme.substr(1, entries->descendants.at(0)->lexer_token->lexeme.size() - 2);
                mapping[key] = json_builder(std::move(entries->descendants.at(1)));
            }

            return std::make_unique<json>(std::move(mapping));
        }

        if (ast->node_symbol_type == NonTerminal::json_array_start)
        {
            auto vect = json::json_array{};
            for (auto& entry : ast->descendants)
                vect.emplace_back(json_builder(std::move(entry)));
            return std::make_unique<json>(std::move(vect));
        }

        std::unreachable();
    }

    export auto get_json(std::string_view json_content) -> std::expected<std::unique_ptr<json>, std::string>
    {
        using enum NonTerminal;
        using enum Terminal;

        auto ast = get_ast(json_content);
        if (!ast.has_value())
            return std::unexpected(ast.error());

        return json_builder(std::move(*ast));
    }
}