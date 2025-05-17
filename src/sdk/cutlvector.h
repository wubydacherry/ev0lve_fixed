
#ifndef SDK_CUTLVECTOR_H
#define SDK_CUTLVECTOR_H

#include <base/game.h>
#include <sdk/mem_alloc.h>

template <class T, class I = int32_t> struct cutlmemory
{
	static I invalid_index() { return (I)-1; }

	struct iterator_t
	{
		iterator_t(I i) : index(i) {}
		I index;

		bool operator==(const iterator_t it) const { return index == it.index; }
		bool operator!=(const iterator_t it) const { return index != it.index; }
	};

	inline bool is_idx_valid(I i) const
	{
		long x = i;
		return x >= 0 && x < count;
	}

	inline bool is_idx_after(I i, const iterator_t &it) const { return i > it.index; }

	inline T &operator[](I i) { return mem[i]; }
	inline const T &operator[](I i) const { return mem[i]; }
	T *base() { return mem; }
	inline int size() const { return count; }

	void grow(size_t num)
	{
		const auto requested = count + num;
		auto next_count = count;

		if (grow_size)
			next_count = ((1 + ((requested - 1) / grow_size)) * grow_size);
		else
		{
			if (!next_count)
				next_count = (31 + sizeof(T)) / sizeof(T);

			while (next_count < requested)
				next_count *= 2;
		}

		if ((int32_t)(I)next_count < requested)
		{
			if ((int32_t)(I)next_count == 0 && (int32_t)(I)(next_count - 1) >= requested)
			{
				--next_count;
			}
			else
				while ((int32_t)(I)next_count < requested)
				{
					next_count = (next_count + requested) / 2;
				}
		}

		count = next_count;

		if (mem)
			mem = (T *)game->mem_alloc->realloc(mem, count * sizeof(T));
		else
			mem = (T *)game->mem_alloc->alloc(count * sizeof(T));
	}

	T *mem;
	int32_t count;
	int32_t grow_size;
};

template <class T, class A = cutlmemory<T>> struct cutlvector
{
	typedef T *iterator;
	typedef const T *const_iterator;

	inline T &operator[](size_t i) { return mem[i]; }
	inline const T &operator[](size_t i) const { return mem[i]; }

	int count() const { return size; }

	T *base() { return mem.base(); }

	iterator begin() { return base(); }
	const_iterator begin() const { return base(); }
	iterator end() { return base() + count(); }
	const_iterator end() const { return base() + count(); }

	void remove_all()
	{
		for (int i = size; --i >= 0;)
			mem[i].~T();

		size = 0;
	}

	void add_to_tail(const T &src)
	{
		if (size + 1 > mem.size())
			mem.grow(size + 1 - mem.size());

		mem[size] = T(src);
		++size;
	}

	void fast_remove(int elem)
	{
		mem[elem].~T();

		if (size > 0)
		{
			if (elem != size - 1)
				memcpy(&mem[elem], &mem[size - 1], sizeof(T));
			--size;
		}
	}

	A mem;
	int32_t size;
	T *elems;
};

#endif // SDK_CUTLVECTOR_H
