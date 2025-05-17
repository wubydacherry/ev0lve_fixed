
#ifndef UTIL_CIRCULAR_BUFFER_H
#define UTIL_CIRCULAR_BUFFER_H

#include <algorithm>
#include <cstdint>
#include <vector>

namespace util
{
template <class T, size_t N = 0> struct circular_buffer
{
	struct circular_iterator
	{
		T *start, *brk, *end, *ptr;

		inline circular_iterator &operator++()
		{
			if (ptr == end)
			{
				ptr = nullptr;
				return *this;
			}

			if (brk == ptr)
			{
				ptr = start + 1;
				brk = nullptr;
			}

			ptr--;
			return *this;
		}

		inline T *operator->() const { return ptr; }

		inline T &operator*() const { return *ptr; }

		inline bool operator==(circular_iterator const &other) const { return ptr == other.ptr; }

		inline bool operator!=(circular_iterator const &other) const { return ptr != other.ptr; }
	};

	struct circular_reverse_iterator
	{
		T *start, *brk, *end, *ptr;

		inline circular_reverse_iterator &operator++()
		{
			if (ptr == end)
			{
				ptr = nullptr;
				return *this;
			}

			if (brk == ptr)
			{
				ptr = start - 1;
				brk = nullptr;
			}

			ptr++;
			return *this;
		}

		inline T *operator->() const { return ptr; }

		inline T &operator*() const { return *ptr; }

		inline bool operator==(circular_reverse_iterator const &other) const { return ptr == other.ptr; }

		inline bool operator!=(circular_reverse_iterator const &other) const { return ptr != other.ptr; }
	};

	circular_buffer()
	{
		if ((max_size = N) > 0)
			elem.resize(max_size);
	}

	circular_buffer(const size_t sz) : max_size(sz) { elem.resize(max_size); }

	circular_buffer(const circular_buffer &other)
	{
		max_size = other.max_size;
		current = other.current;
		count = other.count;
		elem = other.elem;
	}

	circular_buffer &operator=(const circular_buffer &other)
	{
		max_size = other.max_size;
		current = other.current;
		count = other.count;
		elem = other.elem;

		return *this;
	}

	circular_buffer &operator=(circular_buffer &&other) noexcept
	{
		max_size = other.max_size;
		current = other.current;
		count = other.count;
		elem = std::move(other.elem);

		other.count = other.max_size = 0u;
		return *this;
	}

	circular_buffer(circular_buffer &&other) noexcept
	{
		max_size = other.max_size;
		current = other.current;
		count = other.count;
		elem = std::move(other.elem);

		other.count = other.max_size = 0u;
	}

	~circular_buffer() = default;

	[[nodiscard]] inline T &operator[](size_t idx) { return elem[(current + (count - idx - 1)) % max_size]; }
	[[nodiscard]] inline const T &operator[](size_t idx) const
	{
		return elem[(current + (count - idx - 1)) % max_size];
	}
	[[nodiscard]] inline int size() const { return count; }
	[[nodiscard]] inline bool empty() const { return count == 0; }
	[[nodiscard]] inline bool exhausted() const { return count >= max_size; }

	inline void clear()
	{
		count = 0;
		current = 0;
	}
	inline void clear_all_but_first()
	{
		current += count - 1;
		count = 1;
	}
	inline void pop_front() { --count; }
	inline void pop_back()
	{
		++current;
		--count;
	}

	inline void reserve(size_t new_size)
	{
		if (new_size == max_size)
			return;

		elem.resize(max_size = new_size);
		count = 0;
		current = 0;
	}

	inline void resize(size_t new_size)
	{
		elem.resize(max_size = new_size);
		count = max_size;
	}

	[[nodiscard]] inline T *push_front() { return count >= max_size ? nullptr : &elem[(current + count++) % max_size]; }
	[[nodiscard]] inline T &back() { return elem[current % max_size]; }
	[[nodiscard]] inline T &front() { return elem[(current + count - 1) % max_size]; }

	inline void sort(std::function<bool(const T &, const T &)> pred)
	{
		current = current % max_size;
		std::sort(elem.begin() + current % max_size, elem.begin() + current % max_size + count,
				  [&pred](const T &a, const T &b) { return !pred(a, b); });
	}

	[[nodiscard]] inline circular_iterator begin()
	{
		if (empty())
			return {};
		return {&elem[max_size - 1], &elem[0], &back(), &front()};
	}

	[[nodiscard]] inline circular_iterator end() { return {}; }

	[[nodiscard]] inline circular_reverse_iterator rbegin()
	{
		if (empty())
			return {};
		return {&elem[0], &elem[max_size - 1], &front(), &back()};
	}

	[[nodiscard]] inline circular_reverse_iterator rend() { return {}; }

	std::vector<T> elem{};
	size_t current = 0;
	size_t count = 0;
	size_t max_size = 0;
};
} // namespace util

#endif /* UTIL_CIRCULAR_BUFFER_H */
