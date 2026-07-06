#include "formatscript.hpp"
#include <cstring>
#include <memory>
#include <raylib.h>
#include <stdexcept>
#include <unordered_map>

namespace fsc
{

const std::string &Context::get(const std::string &name) const
{
	static const std::string empty;
	auto it = vars.find(name);
	return it != vars.end() ? it->second : empty;
}

Template Parser::parseTemplate(const char *stop)
{
	Template temp;
	size_t start = pos;

	auto flush = [&](size_t end)
	{
		if (end > start)
		{
			temp.push_back(std::make_unique<Node>(
				NodeType::Literal,
				std::string(s.substr(start, end - start))));
		}
	};

	while (!end())
	{
		char8_t c = next();

		if (*stop && std::strchr(stop, c))
			break;

		if (c == '{')
		{
			flush(pos);
			temp.push_back(variable());
			start = pos;
		}
		else if (c == '<')
		{
			flush(pos);
			temp.push_back(function());
			start = pos;
		}
		else
		{
			++pos;
		}
	}

	flush(pos);
	return temp;
}

NodePointer Parser::variable()
{
	++pos; // {
	size_t start = pos;
	while (!end() && next() != '}')
		++pos;
	if (end())
		throw std::runtime_error("Variable not terminated");
	std::string name(s.substr(start, pos - start));
	++pos; // }
	return std::make_unique<Node>(NodeType::Variable, std::move(name));
}

NodePointer Parser::function()
{
	++pos; // <
	if (pos + 3 > s.size())
		throw std::runtime_error("Function code is 3 letters");
	std::string code(s.substr(pos, 3));
	pos += 3; // skip function code
	if (end() || next() != '(')
		throw std::runtime_error("Expected '(' after function code");
	++pos; // (
	auto node = std::make_unique<Node>(NodeType::Function, std::move(code));
	node->arguments = arguments();
	return node;
}

std::vector<Template> Parser::arguments()
{
	std::vector<Template> args;
	while (true)
	{
		args.push_back(parseTemplate(",)"));
		if (end())
			throw std::runtime_error("Unterminated function: Expected ')'");
		if (next() == ',')
		{
			++pos;
			continue;
		}
		++pos; // )
		break;
	}
	return args;
}

inline static bool to_bool(std::string s)
{
	return s == "true" || s == "1";
}

const std::unordered_map<std::string, FSCFunction> &functionTable()
{
	static const std::unordered_map<std::string, FSCFunction> table = {
		{"EXS", [](const Node &node, const Context &ctx) -> std::string
		 {
			 if (node.arguments.size() != 3)
				 throw std::runtime_error("EXS expected three args, got " + std::to_string(node.arguments.size()));
			 std::string name = render(node.arguments[0], ctx);
			 const std::string &val = ctx.get(name);
			 TraceLog(LOG_INFO, TextFormat("FSC: EXS (%s, %s, %s)", name.c_str(), render(node.arguments[1], ctx).c_str(), render(node.arguments[2], ctx).c_str()));
			 return render(val.empty() ? node.arguments[2] : node.arguments[1], ctx);
		 }},
		{"BOL", [](const Node &node, const Context &ctx) -> std::string
		 {
			 if (node.arguments.size() != 3)
				 throw std::runtime_error("BOL expected three args, got " + std::to_string(node.arguments.size()));
			 std::string name = render(node.arguments[0], ctx);
			 const std::string &boolean = ctx.get(name);
			 TraceLog(LOG_INFO, TextFormat("FSC: BOL (%s, %s, %s)", boolean.c_str(), render(node.arguments[1], ctx).c_str(), render(node.arguments[2], ctx).c_str()));
			 return render(to_bool(boolean) ? node.arguments[1] : node.arguments[2], ctx);
		 }},
	};
	return table;
}

std::string render(const Template &temp, const Context &ctx)
{
	std::string out;
	for (const auto &n : temp)
	{
		switch (n->type)
		{
		case NodeType::Literal:
			out += n->text;
			break;
		case NodeType::Variable:
			out += ctx.get(n->text);
			break;
		case NodeType::Function:
			auto &table = functionTable();
			auto it = table.find(n->text);
			if (it == table.end())
			{
				TraceLog(LOG_WARNING, TextFormat("FSC: Unknown function: ", (const char *)n->text.c_str()));
				break;
			}
			out += it->second(*n, ctx);
			break;
		}
	}
	return out;
}

} // namespace fsc
