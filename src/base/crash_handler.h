
#ifndef CSGO_INTERNAL_CRASH_HANDLER_H
#define CSGO_INTERNAL_CRASH_HANDLER_H

class crash_handler_t
{
	struct trace_func_t
	{
		std::string name;
		uintptr_t address;
		std::string mod;
	};

public:
	explicit crash_handler_t();

	void trace();

	EXCEPTION_POINTERS *ex{};
	std::exception std_ex{};

private:
	bool for_each_module(const std::function<void(const std::string &name, uintptr_t base, uintptr_t size)> &func);
	bool stackwalk(const std::function<void(const trace_func_t &func)> &func);

	std::unordered_map<uint32_t, std::string> exception_codes;
};

#endif // CSGO_INTERNAL_CRASH_HANDLER_H
