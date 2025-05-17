
#ifndef UTIL_HOOK_H
#define UTIL_HOOK_H

#include <base/game.h>
#include <type_traits>
#include <util/mem_guard.h>

namespace util
{
class hook_interface
{
public:
	hook_interface() = default;
	hook_interface(hook_interface const &other) = delete;
	hook_interface &operator=(hook_interface const &other) = delete;
	hook_interface(hook_interface &&other) = delete;
	hook_interface &operator=(hook_interface &&other) = delete;

	virtual ~hook_interface() = default;
	virtual void attach() = 0;
	virtual void detach() = 0;
	virtual uintptr_t get_endpoint() const = 0;
	virtual uintptr_t get_target() const = 0;
};

/*
 * Redirects program flow from one location to another.
 * Typically this is used for intercepting method calls.
 */
template <typename T> class hook final : public hook_interface
{
public:
	explicit hook(const encrypted_ptr<T> target, const encrypted_ptr<T> endpoint)
		: hook_interface(), target(target), original_bytes(), is_attached(false)
	{
		const auto length_disasm = [](uint8_t *op) -> uint32_t
		{
			uint32_t size, code;
			if (*op == 0x8A)
				return 2;
			return ((bool(__cdecl *)(uint8_t *, uint32_t *, uint32_t *))game->gameoverlayrenderer.at(
					   sdk::functions::gameoverlayrenderer::length_disasm))(op, &size, &code)
					   ? size
					   : 0xFFFF;
		};

		const auto ptr = reinterpret_cast<uint8_t *>(target());
		auto opcode = ptr;

		auto prolog_length = 0u;
		while (opcode - ptr < 5)
		{
			prolog_length += length_disasm(opcode);
			opcode = ptr + prolog_length;
		}

		std::vector<uint8_t> jmp_instruction = {0xE9 /* JMP rel32 */, 0x00, 0x00, 0x00, 0x00};
		trampoline.reserve(prolog_length + jmp_instruction.size());
		opcode = ptr;

		while (opcode - ptr < 5)
		{
			const auto length = length_disasm(opcode);

			std::vector<uint8_t> instruction(length);
			std::copy(opcode, &opcode[length], instruction.data());

			// Fix up relative instructions.
			if ((opcode[0] == 0xE8	   // CALL rel32
				 || opcode[0] == 0xE9) // JMP rel32
				&& length == 5)
				*reinterpret_cast<uint32_t *>(&instruction[1]) = reinterpret_cast<uint32_t>(&opcode[0]) +
																 *reinterpret_cast<uint32_t *>(&opcode[1]) -
																 reinterpret_cast<uint32_t>(trampoline.data());
			else if (opcode[0] == 0x0F						   // JCC?
					 && opcode[1] >= 0x80 && opcode[1] <= 0x8F // Flag?
					 && length == 6)
				*reinterpret_cast<uint32_t *>(&instruction[2]) = reinterpret_cast<uint32_t>(&opcode[0]) +
																 *reinterpret_cast<uint32_t *>(&opcode[2]) -
																 reinterpret_cast<uint32_t>(trampoline.data());

			trampoline.insert(trampoline.end(), instruction.begin(), instruction.end());
			opcode = opcode + length;
		}

		*reinterpret_cast<uint32_t *>(&jmp_instruction[1]) =
			reinterpret_cast<uint32_t>(ptr) - reinterpret_cast<uint32_t>(trampoline.data()) - jmp_instruction.size();
		trampoline.insert(trampoline.end(), jmp_instruction.begin(), jmp_instruction.end());
		set_endpoint(endpoint);

		mem_guard(PAGE_EXECUTE_READWRITE, trampoline.data(), trampoline.size()).detach();
		const auto fptr = reinterpret_cast<uint8_t *>(target());
		std::copy(&fptr[0], &fptr[5], original_bytes.data());
	}

	~hook()
	{
		if (is_attached)
		{
			try
			{
				hook<T>::detach();
			}
			catch (const std::runtime_error &)
			{
				// Fine then.
			}
		}
	}

	NO_COPY_OR_MOVE(hook);

	void attach() override
	{
		if (is_attached)
			RUNTIME_ERROR("Hooking mechanism already attached.");

		const auto ptr = reinterpret_cast<uint8_t *>(target());
		mem_guard mem(PAGE_EXECUTE_READWRITE, target(), original_bytes.size());
		ptr[0] = 0xE9; // JMP rel32
		*reinterpret_cast<uint32_t *>(&ptr[1]) =
			reinterpret_cast<uint32_t>(endpoint()) - reinterpret_cast<uint32_t>(&ptr[5]);

		is_attached = true;
	}

	void detach() override
	{
		if (!is_attached)
			RUNTIME_ERROR("Hooking mechanism not attached.");

		is_attached = false;

		mem_guard mem(PAGE_EXECUTE_READWRITE, target(), original_bytes.size());
		const auto ptr = reinterpret_cast<uint8_t *>(target());
		std::copy(original_bytes.begin(), original_bytes.end(), ptr);
	}

	uintptr_t get_endpoint() const override { return reinterpret_cast<uintptr_t>(endpoint()); }

	uintptr_t get_target() const override { return reinterpret_cast<uintptr_t>(target()); }

	T *get_original() const { return reinterpret_cast<T *>(const_cast<uint8_t *>(trampoline.data())); }

	template <typename... A> auto call(const A... args) const { return get_original()(args...); }

private:
	void set_endpoint(const encrypted_ptr<T> endpoint)
	{
		const auto was_attached = is_attached;

		if (is_attached)
			detach();

		this->endpoint = endpoint;

		if (this->endpoint && was_attached)
			attach();
	}

	encrypted_ptr<T> target, endpoint;
	std::array<uint8_t, 5> original_bytes;
	std::vector<uint8_t> trampoline;
	bool is_attached;
};
} // namespace util

#define MAKE_HOOK(fn, name, pat)                                                                                       \
	std::make_unique<util::hook<decltype(fn)>>(PATTERN(decltype(fn), name, pat), util::encrypted_ptr(fn))
#define MAKE_HOOK_OFFSET(fn, name, pat, off)                                                                           \
	std::make_unique<util::hook<decltype(fn)>>(PATTERN_OFFSET(decltype(fn), name, pat, off), util::encrypted_ptr(fn))
#define MAKE_HOOK_REL(fn, name, pat)                                                                                   \
	std::make_unique<util::hook<decltype(fn)>>(PATTERN_REL(decltype(fn), name, pat), util::encrypted_ptr(fn))
#define MAKE_HOOK_PRIMITIVE(fn, to)                                                                                    \
	std::make_unique<util::hook<decltype(fn)>>(util::encrypted_ptr<decltype(fn)>((decltype(&fn))to),                   \
											   util::encrypted_ptr(fn))

#endif // UTIL_HOOK_H
