
#ifndef SDK_RECV_H
#define SDK_RECV_H

#include <cstdint>
#include <sdk/vec3.h>

namespace sdk
{
class recv_prop;
class recv_table;

enum send_prop_type : int32_t
{
	dpt_int = 0,
	dpt_float,
	dpt_vector,
	dpt_vectorxy,
	dpt_string,
	dpt_array,
	dpt_datatable,
	dpt_int64,
	dpt_numsendproptypes
};

struct variant
{
	~variant() = delete;

	union
	{
		float float_;
		long int_;
		char *string_;
		void *data_;
		vec3 vector_;
		int64_t int64_;
	};

	int32_t type;
};

struct recv_proxy_data
{
	~recv_proxy_data() = delete;

	const recv_prop *prop;
	variant value;
	int element;
	int id;
};

using fnv_t = unsigned;

class recv_prop
{
public:
	char *name;
	send_prop_type type;
	int flags;
	int size;
	bool inside_array;
	const void *extra_data;
	recv_prop *prop;
	void *length_proxy;
	uintptr_t proxy;
	void *data_table_proxy;
	recv_table *data_table;
	int offset;
	int element_stride;
	int count;
	const char *prop_name;
};

class recv_table
{
public:
	recv_prop *props;
	int count;
	void *decoder;
	char *table_name;
	bool initialized;
	bool in_main_list;
};
} // namespace sdk

#endif // SDK_RECV_H
