import RegexParser;

#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

static auto read_file(string_view filename)
{
	ifstream file{ filename.data() };
	ostringstream ss;
	ss << file.rdbuf(); // reading data
	return ss.str();
}

int main()
{
	constexpr auto parser = RegexParser::get_parser();
	cout << parser << endl;
	auto result = parser(R"([A-Z]\?)");
	cout << result.errors << endl;
	cout << result.logs << endl;
	if (result.root)
	{
		cout << *result.root << endl;
		auto ast = RegexParser::get_ast(std::move(result.root));
	}
}