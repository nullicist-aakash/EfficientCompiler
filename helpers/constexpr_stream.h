#pragma once
#include <string>
#include <cstdint>
#include <algorithm>
#include <memory>
#include <vector>
#include <optional>

namespace helpers
{
    class constexpr_ostream
    {
        std::string output{};
        friend class constexpr_iostream;
    public:
        constexpr constexpr_ostream() noexcept = default;

        constexpr auto operator<<(std::string_view str) noexcept -> constexpr_ostream&
        {
            output += str;
            return *this;
        }

        constexpr auto operator<<(const char* str) noexcept -> constexpr_ostream&
        {
            output += str;
            return *this;
        }

        constexpr auto operator<<(const char c) noexcept -> constexpr_ostream&
        {
            output += c;
            return *this;
        }

        constexpr auto operator<<(std::signed_integral auto i) noexcept -> constexpr_ostream&
        {
            if (i == std::numeric_limits<decltype(i)>::min())
            {
                if constexpr (std::is_same_v<decltype(i), std::int8_t>)
                    output += "-128";
                else if constexpr (std::is_same_v<decltype(i), std::int16_t>)
                    output += "-32768";
                else if constexpr (std::is_same_v<decltype(i), std::int32_t>)
                    output += "-2147483648";
                else if constexpr (std::is_same_v<decltype(i), std::int64_t>)
                    output += "-9223372036854775808";
                else
                    std::terminate();
                return *this;
            }

            if (i < 0)
            {
                output += "-";
                i = -i;
            }

            std::string digits{};
            for (; i != 0; i /= 10)
                digits += '0' + (i % 10);

            std::reverse(digits.begin(), digits.end());
            output += digits;
            return *this;
        }

        constexpr auto operator<<(std::unsigned_integral auto i) noexcept -> constexpr_ostream&
        {
            std::string digits {};
            for (; i != 0; i /= 10)
                digits += '0' + (i % 10);

            std::reverse(digits.begin(), digits.end());
            output += digits;
            return *this;
        }

        constexpr auto operator<<(const bool b) noexcept -> constexpr_ostream&
        {
            output += b ? "true" : "false";
            return *this;
        }

        constexpr auto operator<<(std::nullptr_t) noexcept -> constexpr_ostream&
        {
            output += "nullptr";
            return *this;
        }

        constexpr auto operator<<(const auto* ptr) noexcept -> constexpr_ostream&
        {
            if (ptr == nullptr)
            {
                output += "nullptr";
                return *this;
            }

            std::string hex_digits{};
            for (auto address = std::bit_cast<std::uint64_t>(ptr); address; address /= 16)
            {
                const auto digit = address % 16;
                if (digit < 10)
                    hex_digits += static_cast<char>('0' + digit);
                else
                    hex_digits += static_cast<char>('A' + digit - 10);
            }

            while (hex_digits.size() < sizeof(int*) * 2)
                hex_digits += '0';

            std::reverse(hex_digits.begin(), hex_digits.end());
            output += "0x" + hex_digits;
            return *this;
        }

        template <typename T>
        constexpr auto operator<<(const std::unique_ptr<T>& t) noexcept -> constexpr_ostream&
        {
            return *this << t.get();
        }

        template <typename T>
        constexpr auto operator<<(const std::shared_ptr<T>& t) noexcept -> constexpr_ostream&
        {
            return *this << t.get();
        }

        [[nodiscard]] constexpr auto sv() const noexcept -> std::string_view
        {
            return output;
        }
    };

    class constexpr_iostream
    {
        std::vector<std::string> strings{};
        std::size_t start_index = 0;

        [[nodiscard]]
        static constexpr auto str_to_uint64(std::string_view str) noexcept -> std::optional<std::uint64_t>
        {
            std::uint64_t result = 0;

            for (const auto& c : str)
            {
                if (c < '0' || c > '9')
                    return std::nullopt;

                result *= 10;
                result += (std::uint8_t)(c - '0');
            }

            return result;
        }

    public:
        constexpr constexpr_iostream() = default;
        constexpr constexpr_iostream(const constexpr_iostream&) = delete;
        constexpr constexpr_iostream(const constexpr_iostream&& other) noexcept
        {
            strings = std::move(strings);
            start_index = other.start_index;
        }

        template <typename T>
        constexpr auto operator<<(T&& t) noexcept -> constexpr_iostream&
        {
            constexpr_ostream stream{};
            stream << t;
            strings.emplace_back(std::move(stream.output));
            return *this;
        }

        constexpr auto operator>>(bool& b) noexcept -> constexpr_iostream&
        {
            b = strings.at(start_index++) == "true";
            return *this;
        }

        constexpr auto operator>>(std::string& str) noexcept -> constexpr_iostream&
        {
            str = std::move(strings.at(start_index++));
            return *this;
        }

        constexpr auto operator>>(std::unsigned_integral auto& i) noexcept -> constexpr_iostream&
        {
            const auto result = *str_to_uint64(strings.at(start_index++));

            using itype = std::remove_cvref_t<decltype(i)>;

            if (result < std::numeric_limits<itype>::min() || result > std::numeric_limits<itype>::max())
                std::terminate();

            i = static_cast<itype>(result);
            return *this;
        }

        constexpr auto operator>>(std::signed_integral auto& i) noexcept -> constexpr_iostream&
        {
            const auto str = std::move(strings.at(start_index++));

            const auto result = *str_to_uint64(str[0] == '-' ? str.substr(1) : str);

            i = static_cast<std::remove_cvref_t<decltype(i)>>(result);
            if (str[0] == '-')
                i = -i;

            return *this;
        }
    };
}

// Compile time test
namespace helpers::tests
{
    using namespace helpers;
    consteval bool test_c_style_array()
    {
        constexpr_ostream stream{};
        stream << "Hello, world!";
        return stream.sv() == "Hello, world!";
    }

    consteval bool test_string()
    {
        constexpr_ostream stream{};
        std::string str = "Hello";
        stream << str;

        constexpr_iostream iostream;
        iostream << "Hem kd";
        std::string out;
        iostream >> out;

        return stream.sv() == "Hello" && out == "Hem kd";
    }

    consteval bool test_char()
    {
        constexpr_ostream stream{};
        stream << 'a' << 'b';
        return stream.sv() == "ab";
    }

    consteval bool test_signed_int()
    {
        constexpr_ostream stream{};
        stream
                << std::int8_t{ -128 }
                << std::int16_t{ -32768 }
                << std::int32_t{ -2147483648 }
                << std::int64_t{ -9223372036854775807l }
                << " "
                << 159;

        constexpr_iostream iostream;
        iostream << -9223372036854775807l;
        std::int64_t i;
        iostream >> i;

        return stream.sv() == "-128-32768-2147483648-9223372036854775807 159" && i == -9223372036854775807l;
    }

    consteval bool test_unsigned_int()
    {
        constexpr_ostream stream{};
        stream
                << std::uint8_t{ 255 }
                << std::uint16_t{ 65535 }
                << std::uint32_t{ 4294967295 }
                << std::uint64_t{ 18446744073709551615ul }
                << " "
                << 159u;

        constexpr_iostream iostream;
        iostream << 18446744073709551615ul;
        std::uint64_t i;
        iostream >> i;
        return stream.sv() == "25565535429496729518446744073709551615 159" && i == 18446744073709551615ul;
    }

    consteval bool test_bool()
    {
        constexpr_ostream ostream{};
        ostream << true << false;

        constexpr_iostream iostream;
        iostream << true << false;

        bool b1, b2;
        iostream >> b1 >> b2;

        return ostream.sv() == "truefalse" && b1 && !b2;
    }

    consteval bool test_nullptr()
    {
        constexpr_ostream stream{};
        stream << nullptr;
        return stream.sv() == "nullptr";
    }

    consteval bool test_pointer()
    {
        constexpr_ostream stream{};
        void* ptr = nullptr;
        stream << ptr;
        // idk how to cast an address to void* in constexpr
        // Maybe relevant: https://stackoverflow.com/questions/70543372/how-to-cast-from-const-void-in-a-constexpr-expression
        return stream.sv() == "nullptr";
    }

    static_assert(test_c_style_array(), "C-style array test failed!");
    static_assert(test_string(), "String test failed!");
    static_assert(test_char(), "Char test failed!");
    static_assert(test_signed_int(), "Signed int test failed!");
    static_assert(test_unsigned_int(), "Unsigned int test failed!");
    static_assert(test_bool(), "Bool test failed!");
    static_assert(test_nullptr(), "Nullptr test failed!");
    static_assert(test_pointer(), "Void test failed!");
}
