import compiler;
import helpers;
import JSONParser;
import std;

using namespace std::filesystem;
using namespace std;

int main()
{
    path csv_path = "file.json";
    std::ifstream t(csv_path);
    t.seekg(0, std::ios::end);
    size_t size = t.tellg();
    std::string buffer(size, ' ');
    t.seekg(0);
    t.read(&buffer[0], size);
    const auto ast = JSONParser::get_json(buffer);
    const auto& json = **ast;
    cout << json << endl;
	return 0;
}