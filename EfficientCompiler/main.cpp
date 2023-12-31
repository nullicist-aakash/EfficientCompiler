import JackParser;
import ArithmeticParser;
import compiler;

#include <fstream>
#include <iostream>
#include <numeric>
#include <set>
#include <sstream>
#include <utility>
#include <memory>
#include <cassert>
#include <functional>
#include <stdexcept>

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
	constexpr auto aparser = get_arithmetic_parser();
	auto output2 = aparser("1 + 2 + 4");
	cout << aparser << endl;
	cout << output2.logs << endl;
	cout << output2.errors << endl;
	cout << *output2.root << endl;

	const auto ast = get_arithmetic_ast(std::move(output2.root));
}