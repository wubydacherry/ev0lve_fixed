#ifndef VALUES_E2A14AB316F946FC9D04D93BE95D0975_H
#define VALUES_E2A14AB316F946FC9D04D93BE95D0975_H

#include <ren/types/color.h>

#include <cstdint>
#include <istream>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <unordered_map>

namespace evo::gui
{
class bits
{
public:
	bits(uint64_t val = {}) : v(val) {}

	operator uint64_t() const { return v; }

	void reset() { v = 0; }

	std::optional<char> first_set_bit() const
	{
		for (char i = 0; i < sizeof(uint64_t) * 8; i++)
		{
			if (get(i))
				return i;
		}

		return std::nullopt;
	}

	int selected_bits() const
	{
		int s{};
		for (char i = 0; i < sizeof(uint64_t) * 8; i++)
		{
			if (get(i))
				++s;
		}

		return s;
	}

	bool none() const { return !first_set_bit(); }

	void set(char n)
	{
		assert_n(n);
		v |= (uint64_t)1 << (uint64_t)n;
	}

	void unset(char n)
	{
		assert_n(n);
		v &= ~((uint64_t)1 << (uint64_t)n);
	}

	bool get(char n) const
	{
		assert_n(n);
		return v & ((uint64_t)1 << (uint64_t)n);
	}

	// std::bitset compatibility
	bool test(char n) const { return get(n); }

	void toggle(char n) { get(n) ? unset(n) : set(n); }

private:
	void assert_n(char n) const
	{
		if (n > sizeof(v) * 8 - 1 || n < 0)
			throw std::runtime_error("Bit number is out of bounds");
	}

	uint64_t v{};
};

class value_entry
{
public:
	virtual void reset() = 0;
	virtual void serialize(std::ostream &stream) = 0;
	virtual bool deserialize(std::istream &stream) = 0;
};

enum value_type
{
	vt_variable,
	vt_bool,
	vt_int,
	vt_float,
	vt_color,
	vt_bits,
	vt_max
};

template <typename T> class value_param : public value_entry
{
public:
	value_param() { deduct_type(); }

	value_param(const T &v) : value(v), def(v) // NOLINT(google-explicit-constructor)
	{
		deduct_type();
	}

	value_param(const value_param<T> &other)
		: value(other.value), def(other.def), hotkey_mode(other.hotkey_mode), hotkeys(other.hotkeys)
	{
		deduct_type();
	}

	void operator=(const T &v) { value = v; }

	void operator=(const value_param<T> &other)
	{
		value = other.value;
		def = other.def;
		hotkey_mode = other.hotkey_mode;
		hotkeys = other.hotkeys;
	}

	operator T() const { return value; }

	T *operator->() { return &value; }

	T &operator*() { return value; }

	void reset() override
	{
		value = def;
		hotkeys.clear();
		hotkey_mode = 0;
	}

	void serialize(std::ostream &stream) override
	{
		const auto size = sizeof(T);

		// type
		stream.write(reinterpret_cast<const char *>(&type), sizeof(type));

		// size & value
		stream.write(reinterpret_cast<const char *>(&size), sizeof(size));
		stream.write(reinterpret_cast<const char *>(&value), size);

		// hotkeys
		stream.write(&hotkey_mode, 1);

		const auto hk_count = hotkeys.size();
		stream.write(reinterpret_cast<const char *>(&hk_count), sizeof(hk_count));

		for (auto &hk : hotkeys)
		{
			stream.write(reinterpret_cast<const char *>(&hk.first), sizeof(hk.first));
			stream.write(reinterpret_cast<const char *>(&hk.second), sizeof(hk.second));
		}
	}

	bool deserialize(std::istream &stream) override
	{
		// type
		stream.read(reinterpret_cast<char *>(&type), sizeof(type));
		if (type < vt_variable || type >= vt_max)
			return false;

		// size and value
		size_t size{};
		stream.read(reinterpret_cast<char *>(&size), sizeof(size));

		if (size != sizeof(value))
			return false;

		stream.read(reinterpret_cast<char *>(&value), size);

		// hotkeys
		stream.read(&hotkey_mode, 1);

		size_t hk_count{};
		stream.read(reinterpret_cast<char *>(&hk_count), sizeof(hk_count));

		for (auto i = 0; i < hk_count; i++)
		{
			int hk_key{};
			uint32_t hk_value{};

			stream.read(reinterpret_cast<char *>(&hk_key), sizeof(hk_key));
			stream.read(reinterpret_cast<char *>(&hk_value), sizeof(hk_value));

			hotkeys[hk_key] = hk_value;
		}

		return true;
	}

	void update_hotkeys(char mode, const std::unordered_map<int, uint32_t> &hk)
	{
		hotkeys = hk;
		hotkey_mode = mode;
	}

	char get_hotkey_mode() const { return hotkey_mode; }

	const std::unordered_map<int, uint32_t> &get_hotkey_table() { return hotkeys; }

	void set(const T &v) { value = v; }

	T &get() { return value; }

	bool was_changed() { return value != def; }

	value_type type{};

protected:
	void deduct_type()
	{
		if constexpr (std::is_same<T, bool>::value)
		{
			type = vt_bool;
		}
		else if constexpr (std::is_same<T, int>::value)
		{
			type = vt_int;
		}
		else if constexpr (std::is_same<T, float>::value)
		{
			type = vt_float;
		}
		else if constexpr (std::is_same<T, ren::color>::value)
		{
			type = vt_color;
		}
		else if constexpr (std::is_same<T, bits>::value)
		{
			type = vt_bits;
		}
	}

	T value{};
	char hotkey_mode{};
	std::unordered_map<int, uint32_t> hotkeys{};

private:
	T def{};
};
} // namespace evo::gui

#endif // VALUES_E2A14AB316F946FC9D04D93BE95D0975_H
