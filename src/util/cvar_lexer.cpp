
#include <string>
#include <util/cvar_lexer.h>
#include <vector>

namespace util
{
std::vector<std::string> parse_cvars(std::string cmd)
{
	std::vector<std::string> cvars;

	bool is_quoted = false;
	std::string current_cmd = "";

	for (size_t i = 0; i < cmd.length(); i++)
	{
		if (cmd[i] == '\"')
			is_quoted = !is_quoted;

		if (cmd[i] == ';' && !is_quoted)
		{
			cvars.emplace_back(current_cmd);
			current_cmd = "";
			continue;
		}

		if (!current_cmd.empty() || cmd[i] != ' ')
			current_cmd.push_back(cmd[i]);
	}

	if (!current_cmd.empty())
		cvars.emplace_back(current_cmd);

	return cvars;
}
} // namespace util
