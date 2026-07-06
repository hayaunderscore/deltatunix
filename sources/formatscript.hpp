#pragma once

#include <functional>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace fsc
{

enum class NodeType
{
	Literal,
	Variable,
	Function
};

struct Node;

using NodePointer = std::unique_ptr<Node>;
using Template = std::vector<NodePointer>;

struct Node
{
	NodeType type;
	std::string text;
	std::vector<Template> arguments;
};

struct Context
{
	std::unordered_map<std::string, std::string> vars;

	const std::string &get(const std::string &name) const;
};

class Parser
{
  public:
	explicit Parser(std::string_view code) : s(code) {}
	inline Template parse() { return parseTemplate(""); }

  private:
	std::string_view s;
	size_t pos = 0;

	inline bool end() const { return pos >= s.size(); };
	inline char8_t next() const { return s[pos]; };

	Template parseTemplate(const char *stop);

	NodePointer variable();
	NodePointer function();
	std::vector<Template> arguments();
};

std::string render(const Template &temp, const Context &ctx);

using FSCFunction = std::function<std::string(const Node &, const Context &)>;
const std::unordered_map<std::string, FSCFunction> &functionTable();

} // namespace fsc
