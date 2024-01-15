import RegexParser;

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>
#include <limits>

using namespace std;

struct Node
{
private:
	friend class NodeManager;
	std::size_t unique_index{ std::numeric_limits<std::size_t>::max() };
public:
	std::vector<std::shared_ptr<Node>> empty_transitions;
	std::vector<std::pair<std::string, std::shared_ptr<Node>>> tran_nodes;

	~Node()
	{
		cout << "~Node" << endl;
	}
};

class NodeManager
{
private:
	NodeManager() = default;
	NodeManager(const NodeManager&) = delete;
	NodeManager(NodeManager&&) = delete;
	NodeManager& operator=(const NodeManager&) = delete;
	NodeManager& operator=(NodeManager&&) = delete;

public:
	std::vector<std::shared_ptr<Node>> all_nodes;

	static NodeManager& get_instance()
	{
		static NodeManager instance;
		return instance;
	}

	auto create_node()
	{
		all_nodes.emplace_back(std::make_shared<Node>());
		all_nodes.back()->unique_index = all_nodes.size() - 1;
		return all_nodes.back();
	}

	void del_node(std::shared_ptr<Node> node)
	{
		auto index = std::exchange(node->unique_index, std::numeric_limits<std::size_t>::max());
		all_nodes[index] = nullptr;

		if (index != all_nodes.size() - 1)
		{
			std::swap(all_nodes[index], all_nodes.back());
			all_nodes[index]->unique_index = index;
		}

		all_nodes.pop_back();
	}

	void clear_unused()
	{
		for (int i = 0; i < all_nodes.size(); ++i)
		{
			if (all_nodes[i].use_count() > 1)
				continue;

			del_node(all_nodes[i]);
		}
	}

	void clear_all()
	{
		all_nodes.clear();
	}
};


static auto read_file(string_view filename)
{
	ifstream file{ filename.data() };
	ostringstream ss;
	ss << file.rdbuf(); // reading data
	return ss.str();
}

int main()
{/*
	auto ast = RegexParser::get_ast(R"(abc|[def-z]*)");
	cout << ast.errors << endl;
	cout << ast.logs << endl;
	if (ast.root)
		cout << *ast.root << endl;*/
	auto& nm = NodeManager::get_instance();
	auto node1 = nm.create_node();
	auto node2 = nm.create_node();

}