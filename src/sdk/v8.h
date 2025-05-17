#ifndef SDK_V8_H
#define SDK_V8_H

namespace sdk::v8
{
class isolate_t
{
public:
	FUNC(game->v8.at(functions::v8::isolate::enter), enter(), void(__thiscall *)(void *))();
	FUNC(game->v8.at(functions::v8::isolate::exit), exit(), void(__thiscall *)(void *))();
};

class context_t
{
public:
	FUNC(game->v8.at(functions::v8::context::enter), enter(), void(__thiscall *)(void *))();
	FUNC(game->v8.at(functions::v8::context::exit), exit(), void(__thiscall *)(void *))();
};

class try_catch_t
{
public:
	FUNC(game->v8.at(functions::v8::try_catch::ctor), enter(isolate_t *isolate), void(__thiscall *)(void *, void *))
	(isolate);
	FUNC(game->v8.at(functions::v8::try_catch::dtor), exit(), void(__thiscall *)(void *))();

private:
	char pad[0x19]{};
};

class handle_scope_t
{
public:
	FUNC(game->v8.at(functions::v8::handle_scope::ctor), enter(isolate_t *isolate), void(__thiscall *)(void *, void *))
	(isolate);
	FUNC(game->v8.at(functions::v8::handle_scope::dtor), exit(), void(__thiscall *)(void *))();

	static context_t *create_handle(isolate_t *isolate, void *object)
	{
		return ((context_t * (__cdecl *)(isolate_t *, void *))
					game->v8.at(functions::v8::handle_scope::create_handle))(isolate, object);
	}

private:
	char pad[0xC]{};
};

class local_t;
class value_t
{
public:
	FUNC(game->v8.at(functions::v8::value::is_string), is_string(), bool(__thiscall *)(void *))();
	FUNC(game->v8.at(functions::v8::value::is_string_object), is_string_object(), bool(__thiscall *)(void *))();
	FUNC(game->v8.at(functions::v8::value::is_array), is_array(), bool(__thiscall *)(void *))();
	FUNC(game->v8.at(functions::v8::value::is_object), is_object(), bool(__thiscall *)(void *))();
	FUNC(game->v8.at(functions::v8::value::is_boolean), is_boolean(), bool(__thiscall *)(void *))();
	FUNC(game->v8.at(functions::v8::value::is_boolean_object), is_boolean_object(), bool(__thiscall *)(void *))();
	FUNC(game->v8.at(functions::v8::value::is_number), is_number(), bool(__thiscall *)(void *))();
	FUNC(game->v8.at(functions::v8::value::is_number_object), is_number_object(), bool(__thiscall *)(void *))();
	FUNC(game->v8.at(functions::v8::value::is_function), is_function(), bool(__thiscall *)(void *))();
	FUNC(game->v8.at(functions::v8::value::boolean_value), boolean_value(), bool(__thiscall *)(void *))();
	FUNC(game->v8.at(functions::v8::value::number_value), number_value(), double(__thiscall *)(void *))();
	FUNC(game->v8.at(functions::v8::value::to_object), to_object(void *r), local_t *(__thiscall *)(void *, void *))(r);
};

class object_t : public value_t
{
public:
	FUNC(game->v8.at(functions::v8::object::get), get(void *r, value_t *k),
		 local_t *(__thiscall *)(void *, void *, value_t *))
	(r, k);
	FUNC(game->v8.at(functions::v8::object::get_property_names), get_property_names(void *r),
		 local_t *(__thiscall *)(void *, void *))
	(r);
};

class array_t : public value_t
{
public:
	FUNC(game->v8.at(functions::v8::array::length), length(), int(__thiscall *)(void *))();
	FUNC(game->v8.at(functions::v8::array::get), get(void *r, int index), local_t *(__thiscall *)(void *, void *, int))
	(r, index);
};

class utf8_string_t
{
public:
	FUNC(game->v8.at(functions::v8::string::utf8_ctor), ctor(value_t *val), void(__thiscall *)(void *, void *))(val);
	FUNC(game->v8.at(functions::v8::string::utf8_dtor), dtor(), void(__thiscall *)(void *))();

	char *value{};
	int length{};
};

class local_t
{
public:
	value_t *value;
};
} // namespace sdk::v8

#endif // SDK_V8_H
