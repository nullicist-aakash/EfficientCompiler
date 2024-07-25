import compiler;
import helpers;
import JSONParser;
import std;

using namespace std;

int main()
{
	constexpr auto parser = JSONParser::get_parser();
	cout << parser << endl;
	return 0;
}