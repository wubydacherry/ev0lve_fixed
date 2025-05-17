
#ifndef SDK_DATAMAP_H
#define SDK_DATAMAP_H

#include <sdk/cutlvector.h>
#include <util/fnv1a.h>

namespace sdk
{
struct inputdata_t;
typedef enum fieldtypes
{
	field_void = 0,
	field_float,
	field_string,
	field_vector,
	field_quaternion,
	field_integer,
	field_boolean,
	field_short,
	field_character,
	field_color32,
	field_embedded,
	field_custom,
	field_classptr,
	field_ehandle,
	field_edict,
	field_position_vector,
	field_time,
	field_tick,
	field_modelname,
	field_soundname,
	field_input,
	field_function,
	field_vmatrix,
	field_vmatrix_worldspace,
	field_matrix3_x4_worldspace,
	field_interval,
	field_modelindex,
	field_materialindex,
	field_vector2d,
	field_typecount
} fieldtype_t;

struct datamap_t;
class typedescription_t;

enum
{
	td_offset_normal = 0,
	td_offset_packed = 1,
	td_offset_count
};

enum
{
	pc_non_networked_only = 0,
	pc_networked_only,
	pc_copytype_count,
	pc_everything = pc_copytype_count
};

struct typedescription_t
{
	typedescription_t(int32_t type, char *name, int32_t offset, int32_t size, float tolerance = 0.f,
					  bool no_error_check = false)
		: field_type(type), field_name(name), offset(offset), size(1), flags(0x100 | (no_error_check ? 0x400 : 0)),
		  external_name(name), save_ops(0), input_fn(0), td(nullptr), field_size(size), override_field(nullptr),
		  override_count(0), tolerance(tolerance), flat_group(pc_networked_only)
	{
		flat_offsets[pc_non_networked_only] = flat_offsets[pc_networked_only] = 0;
	}

	int32_t field_type;
	char *field_name;
	int32_t offset;
	uint16_t size;
	int16_t flags;
	char *external_name;
	uintptr_t save_ops, input_fn;
	datamap_t *td;
	int32_t field_size;
	typedescription_t *override_field;
	int32_t override_count;
	float tolerance;
	int32_t flat_offsets[td_offset_count];
	uint32_t flat_group;
};

struct datarun_t
{
	datarun_t() : start_flat_field(0), end_flat_field(0), length(0)
	{
		for (int32_t i = 0; i < td_offset_count; ++i)
			start_offset[i] = 0;
	}

	int start_flat_field;
	int end_flat_field;
	int start_offset[td_offset_count];
	int length;
};

struct optimized_datamap_t;

struct datamap_t
{
	typedescription_t *data_desc;
	int data_num_fields;
	char const *data_class_name;
	datamap_t *base_map;

	int32_t packed_size;
	optimized_datamap_t *optimized_map;
};

__declspec(noinline) static typedescription_t* find_in_data_map_hash(datamap_t* map, const uint32_t hash)
{
	while (map)
	{
		for (auto i = 0; i < map->data_num_fields; i++)
		{
			if (map->data_desc[i].field_name == nullptr)
				continue;

			//if ( hash == fnv1a_rt( map->dataDesc[ i ].fieldName ) )
			if (util::fnv1a(map->data_desc[i].field_name) == hash)
				return &map->data_desc[i];

			if (map->data_desc[i].field_type == field_embedded)
			{
				if (map->data_desc[i].td)
				{
					typedescription_t* ptr;

					if ((ptr = find_in_data_map_hash(map->data_desc[i].td, hash)))
						return ptr;
				}
			}
		}
		map = map->base_map;
	}

	return nullptr;
}
//===============================================
#define DATAMAPVAR(funcname, type, _offset) type& funcname() \
{ \
    return *reinterpret_cast< type* >( uintptr_t( this ) + _offset ); \
}
#if 0
inline util::encrypted_ptr<typedescription_t> find_in_data_map_ref(datamap_t *map, const uint32_t hash)
{
	while (map)
	{
		for (auto i = 0; i < map->data_num_fields; i++)
		{
			if (map->data_desc[i].field_name == nullptr)
				continue;

			if (util::fnv1a(map->data_desc[i].field_name) == hash)
				return util::encrypted_ptr(&map->data_desc[i]);

			if (map->data_desc[i].field_type == field_embedded)
			{
				const auto td = map->data_desc[i].td;

				if (td)
				{
					util::encrypted_ptr<typedescription_t> ptr;

					if ((ptr = find_in_data_map_ref(td, hash)))
						return ptr;
				}
			}
		}

		map = map->base_map;
	}

	return util::encrypted_ptr<typedescription_t>(nullptr);
}

inline util::encrypted_ptr<uint32_t> find_in_data_map(datamap_t *map, const uint32_t hash)
{
	const auto entry = find_in_data_map_ref(map, hash);

	if (entry())
		return util::encrypted_ptr<uint32_t>((uint32_t *)entry->offset);

	RUNTIME_ERROR("Could not find datamap variable.");
}
#endif
} // namespace sdk

#endif // SDK_DATAMAP_H
