module helpers:constexpr_stream;

consteval static bool test_c_style_array()
{
	constexpr_stream stream{};
	stream << "Hello, world!";
	return stream.sv() == "Hello, world!";
}

consteval static bool test_string()
{
	constexpr_stream stream{};
	std::string str = "Hello";
	stream << str;
	return stream.sv() == "Hello";
}

consteval static bool test_char()
{
	constexpr_stream stream{};
	stream << 'a' << 'b';
	return stream.sv() == "ab";
}

consteval static bool test_signed_int()
{
	constexpr_stream stream{};
	stream 
		<< std::int8_t{ -128 } 
		<< std::int16_t{ -32768 } 
		<< std::int32_t{ -2147483648 } 
		<< std::int64_t{ -9223372036854775807l } 
		<< " " 
		<< 159;
	return stream.sv() == "-128-32768-2147483648-9223372036854775807 159";
}

consteval static bool test_unsigned_int()
{
	constexpr_stream stream{};
	stream 
		<< std::uint8_t{ 255 } 
		<< std::uint16_t{ 65535 } 
		<< std::uint32_t{ 4294967295 } 
		<< std::uint64_t{ 18446744073709551615ul } 
		<< " " 
		<< 159u;
	return stream.sv() == "25565535429496729518446744073709551615 159";
}

consteval static bool test_bool()
{
	constexpr_stream stream{};
	stream << true << false;
	return stream.sv() == "truefalse";
}

consteval static bool test_nullptr()
{
	constexpr_stream stream{};
	stream << nullptr;
	return stream.sv() == "nullptr";
}

consteval static bool test_pointer()
{
	constexpr_stream stream{};
	std::uint64_t address = 16;
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