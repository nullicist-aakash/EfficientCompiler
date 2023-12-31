export module helpers:constexpr_stream;
import <string_view>;
import <concepts>;
import <numeric>;
import <cstdint>;
import <algorithm>;
import <format>;
import <memory>;
import <bit>;

export class constexpr_stream
{
	std::string output;
public:
	constexpr constexpr_stream() = default;

	constexpr constexpr_stream& operator<<(std::string_view str)
	{
		output += str;
		return *this;
	}

	constexpr constexpr_stream& operator<<(const char* str)
	{
		output += str;
		return *this;
	}

	constexpr constexpr_stream& operator<<(const char c)
	{
		output += c;
		return *this;
	}

	constexpr constexpr_stream& operator<<(std::signed_integral auto i)
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
			return *this;
		}

		if (i < 0)
		{
			output += "-";
			i = -i;
		}

		std::string digits{};
		while (i)
		{
			const auto digit = i % 10;
			digits += static_cast<char>('0' + digit);
			i /= 10;
		}

		std::reverse(digits.begin(), digits.end());
		output += digits;
		return *this;
	}

	constexpr constexpr_stream& operator<<(std::unsigned_integral auto i)
	{
		std::string digits{};
		while (i)
		{
			const auto digit = i % 10;
			digits += static_cast<char>('0' + digit);
			i /= 10;
		}

		std::reverse(digits.begin(), digits.end());
		output += digits;
		return *this;
	}

	constexpr constexpr_stream& operator<<(bool b)
	{
		output += b ? "true" : "false";
		return *this;
	}

	constexpr constexpr_stream& operator<<(std::nullptr_t)
	{
		output += "nullptr";
		return *this;
	}

	template <typename T>
	constexpr constexpr_stream& operator<<(T* ptr)
	{
		if (ptr == nullptr)
		{
			output += "nullptr";
			return *this;
		}

		std::uint64_t address = std::bit_cast<std::uint64_t>(ptr);
		std::string hex_digits;
		while (address)
		{
			const auto digit = address % 16;
			if (digit < 10)
				hex_digits += static_cast<char>('0' + digit);
			else
				hex_digits += static_cast<char>('A' + digit - 10);
			address /= 16;
		}

		while (hex_digits.size() < 16)
			hex_digits += '0';
		std::reverse(hex_digits.begin(), hex_digits.end());
		output += "0x" + hex_digits;
		return *this;
	}

	template <typename T>
	constexpr constexpr_stream& operator<<(const std::unique_ptr<T>& t)
	{
		return *this << t.get();
	}

	constexpr auto str() const -> const std::string&
	{
		return output;
	}

	constexpr auto sv() const -> std::string_view
	{
		return output;
	}
};