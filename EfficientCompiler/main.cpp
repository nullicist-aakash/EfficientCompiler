import JackParser;
import ArithmeticParser;
import RegexParser;

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
	auto lxr = RegexParser::get_lexer();
}