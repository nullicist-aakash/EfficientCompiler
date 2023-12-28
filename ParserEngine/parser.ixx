/*export module compiler_engine.parser;

import compiler_engine.lexer;
import compiler_engine.structures;
import helpers.flatmap;
import helpers.reflection;

import <array>;
import <bitset>;
import <map>;
import <string_view>;
import <type_traits>;
import <variant>;
import <ranges>;
import <algorithm>;
import <bitset>;
import <functional>;
import <utility>;
import <exception>;
import <format>;

template <is_non_terminal NonTerminal, is_terminal Terminal, int max_prod_len, int num_productions>
struct Parser
{
private:
    using ProductionNumber = int;
	static constexpr auto num_terminals = get_size<Terminal>();
	static constexpr auto num_non_terminals = get_size<NonTerminal>();
	static constexpr auto num_symbols = num_terminals + num_non_terminals;
public:
    std::array<ProductionInfo<NonTerminal, Terminal, max_prod_len>, num_productions> productions{};
    std::array<std::array<ProductionNumber, num_terminals>, num_non_terminals> parse_table{};
};

template <typename T, typename U>
consteval bool operator==(const std::variant<T, U>& lhs, const T& rhs)
{
    return std::holds_alternative<T>(lhs) && std::get<T>(lhs) == rhs;
}

template <typename T, typename U>
consteval bool operator==(const std::variant<T, U>& lhs, const U& rhs)
{
    return std::holds_alternative<U>(lhs) && std::get<U>(lhs) == rhs;
}

template <typename T, typename U>
consteval bool operator!=(const std::variant<T, U>& lhs, const T& rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename U>
consteval bool operator!=(const std::variant<T, U>& lhs, const U& rhs)
{
    return !(lhs == rhs);
}

template <is_non_terminal NonTerminal, is_terminal Terminal>
consteval auto get_nullable_set(auto production_callback)
{
    constexpr auto& productions = production_callback();

	// Returns an array of size num_non_terminals, where ith index=true means (Nonterminal)i is nullable
	std::bitset<get_size<NonTerminal>()> nullable_set;
	nullable_set.reset();

    // T -> eps means T is nullable
    for (auto& x : productions)
        if (x.size == 1 && x.production[0] == Terminal::eps)
			nullable_set[(int)x.start] = true;

    // Go till convergence
	for (bool changed = true; std::exchange(changed, false); )
		for (auto &x: productions
			| std::views::filter([&](auto& x) { return !nullable_set[(int)x.start]; })
			| std::views::filter([&](auto& x) { return std::ranges::none_of(
				x.production
				| std::views::take(x.size)
				| std::views::transform([&](auto& y)
					{ return std::holds_alternative<Terminal>(y) || !nullable_set[(int)std::get<NonTerminal>(y)]; }),
				std::identity()); })
			)
			changed = nullable_set[(int)x.start] = true;

	return nullable_set;
}

template <is_non_terminal NonTerminal, is_terminal Terminal>
consteval auto get_first_sets(auto production_callback, const auto& nullable_set)
{
    constexpr auto& productions = production_callback();
    constexpr auto num_terminals = get_size<Terminal>();
    constexpr auto num_non_terminals = get_size<NonTerminal>();
    constexpr auto num_symbols = num_terminals + num_non_terminals;

    std::array<std::bitset<num_terminals>, num_non_terminals> first_set{};

    // Add eps to first set of nullables
    for (int i = 0; i < num_non_terminals; ++i)
        if (nullable_set[i])
            first_set[i][(int)Terminal::eps] = true;

    // Iterate untill no update
    for (bool updated = true; std::exchange(updated, false); )
        for (const auto& x : productions)
        {
            auto& bits = first_set[(int)x.start];
            auto copy = bits;

            for (auto y : x.production | std::views::take(x.size))
            {
                if (std::holds_alternative<Terminal>(y) && y != Terminal::eps)
                {
                    bits[(int)std::get<Terminal>(y)] = true;
                    break;
                }
                
                if (y == Terminal::eps)
                {
                    bits[(int)std::get<Terminal>(y)] = true;
                    continue;
                }

                int index = (int)std::get<NonTerminal>(y);
                bits |= first_set[index];

                if (!nullable_set[index])
					break;
            }

            updated = updated || (copy != bits);
        }

    return first_set;
}

template <is_non_terminal NonTerminal, is_terminal Terminal>
consteval auto get_follow_sets(auto production_callback, const auto& nullable_set, const auto& first_set)
{
	constexpr auto& productions = production_callback();
	constexpr auto num_terminals = get_size<Terminal>();
	constexpr auto num_non_terminals = get_size<NonTerminal>();
	constexpr auto num_symbols = num_terminals + num_non_terminals;

	std::array<std::bitset<num_terminals>, num_non_terminals> follow_set{};

    for (bool updated = true; std::exchange(updated, false); )
        for (auto& x : productions)
        {
            for (int j = 0; j < x.size; ++j)
            {
                if (std::holds_alternative<Terminal>(x.production[j]))
                    continue;

                auto& symbol = x.production[j];
                auto& bits = follow_set[(int)std::get<NonTerminal>(symbol)];
                auto copy = bits;

                for (int k = j + 1; k < x.size; ++k)
                {
                    auto& follow_symbol = x.production[k];
                    if (std::holds_alternative<Terminal>(follow_symbol))
                    {
                        bits[(int)std::get<Terminal>(follow_symbol)] = true;

                        if (follow_symbol != Terminal::eps)
							break;
                    }
                    else
                    {
                        bits |= first_set[(int)std::get<NonTerminal>(follow_symbol)];

                        if (!nullable_set.test((int)std::get<NonTerminal>(follow_symbol)))
                            break;
                    }

                    if (k == x.size - 1)
                        bits |= follow_set[(int)x.start];
                }

                if (j == x.size - 1)
                    bits |= follow_set[(int)x.start];

                updated = updated || (copy != bits);
            }
        }

    // remove eps from follow set
    for (auto& follow : follow_set)
        follow[(int)Terminal::eps] = false;

	return follow_set;
}

export consteval auto build_parser(auto production_callback, const auto& keyword_to_token_map)
{
	constexpr auto productions = production_callback();
    
    using ProdType = std::remove_cvref_t<decltype(productions[0])>;
	using Terminal = ProdType::TType;
	using NonTerminal = ProdType::NTType;
    
    constexpr auto num_terminals = get_size<Terminal>();
    constexpr auto num_non_terminals = get_size<NonTerminal>();
    constexpr auto num_symbols = num_terminals + num_non_terminals;

	constexpr auto nullable_set = get_nullable_set<NonTerminal, Terminal>([]() -> auto& { return productions; });
    constexpr auto first_set = get_first_sets<NonTerminal, Terminal>([]() -> auto& { return productions; }, nullable_set);
    constexpr auto follow_set =  get_follow_sets<NonTerminal, Terminal>([]() -> auto& { return productions; }, nullable_set, first_set);

    constexpr auto test_nullable = [](const auto& x) 
        {
            return (std::holds_alternative<Terminal>(x) && std::get<Terminal>(x) == Terminal::eps) 
                || (std::holds_alternative<NonTerminal>(x) && nullable_set[(int)std::get<NonTerminal>(x)]);
        };
    constexpr auto get_first = [](const auto& x) 
		{
            std::bitset<num_terminals> bits{};
			if (std::holds_alternative<Terminal>(x))
                bits[(int)std::get<Terminal>(x)] = true;
			else
				bits = first_set[(int)std::get<NonTerminal>(x)];
            return bits;
		};

    std::array<std::bitset<num_terminals>, productions.size()> selection_sets{};
    for (int i = 0; i < productions.size(); ++i)
    {
        auto& x = productions[i];
        auto& target = selection_sets[i];
        bool all_nullable = true;

        for (auto& y : x.production | std::views::take(x.size))
        {
            target |= get_first(y);

            if (!test_nullable(y))
            {
                all_nullable = false;
                break;
            }
        }

        if (all_nullable)
			target |= follow_set[(int)x.start];

        target[(int)Terminal::eps] = false;
    }

    Parser<NonTerminal, Terminal, productions[0].production.size(), productions.size()> parser
    {
        .productions = productions,
		.parse_table = {}
    };

    for (auto& row : parser.parse_table)
		row.fill(-1);

    for (int i = 0; i < productions.size(); ++i)
    {
        auto lhs = productions[i].start;
        for (int token = 0; token < selection_sets[i].size(); ++token)
        {
            if (!selection_sets[i][token])
                continue;

            if (parser.parse_table[(int)lhs][token] != -1)
                throw std::format("There is a problem for '{}' with the terminal '{}'.", get_token_string(lhs), get_token_string((Terminal)token));

            parser.parse_table[(int)lhs][token] = i;
        }
    }

    // Fill for sync set: If j is in FOLLOW(i) and (i,j) is -1, then (i,j) = -2
    for (int i = 0; i < num_non_terminals; ++i)
	{
        for (int j = 0; j < num_terminals; ++j)
        {
            if (parser.parse_table[i][j] == -1 && follow_set[i][j])
				parser.parse_table[i][j] = -2;
        }
	}

    for (auto& [_, keyword_token] : keyword_to_token_map)
        for (int i = 0; i < parser.parse_table.size(); ++i)
            if (auto& ref = parser.parse_table[i][(int)keyword_token]; ref == -1)
                ref = -2;

    return parser;
}*/