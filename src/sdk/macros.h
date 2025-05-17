
#ifndef SDK_MACROS_H
#define SDK_MACROS_H

#define FORWARD_ARGS(...)                                                                                              \
	(this, __VA_ARGS__);                                                                                               \
	}
#define FUNC(fn, func, sig)                                                                                            \
	__forceinline auto func                                                                                            \
	{                                                                                                                  \
		return reinterpret_cast<sig>(fn) FORWARD_ARGS
#define MEMBER(offset, name, type)                                                                                     \
	__forceinline type &get_##name()                                                                                   \
	{                                                                                                                  \
		constexpr auto z = ::util::random::_uint<__COUNTER__, 0xFFFFFFFF>::value;                                      \
		constexpr auto p = offset - z;                                                                                 \
		return *(type *)::util::add<uintptr_t>(uintptr_t(this), z, p);                                                 \
	}
#define AMEMBER(offset, name, type)                                                                                    \
	__forceinline type *get_##name()                                                                                   \
	{                                                                                                                  \
		constexpr auto z = ::util::random::_uint<__COUNTER__, 0xFFFFFFFF>::value;                                      \
		constexpr auto p = offset - z;                                                                                 \
		return (type *)::util::add<uintptr_t>(uintptr_t(this), z, p);                                                  \
	}
#define OFFSET( type, name, offset ) \
	__forceinline type& name( ) const { \
		return *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>( this ) + offset); \
	}
#define server_offset( offset ) []() -> uintptr_t { return (uint32_t)(offset); }()
#define ANETVAR(funcname, type, num, offset) __forceinline std::array<type, num>& funcname() \
{ \
	return *reinterpret_cast<std::array<type, num>*>(uintptr_t(this) + offset); \
}
#define NETVARA(funcname, type, offset, add) type& funcname() \
{ \
	return *reinterpret_cast<type*>( uintptr_t( this ) + offset + add ); \
}
#define VAR(cl, name, type) MEMBER(cl::name, name, type)
#define AVAR(cl, name, type) AMEMBER(cl::name, name, type)
#define VIRTUAL(index, func, sig)                                                                                      \
	__forceinline auto func                                                                                            \
	{                                                                                                                  \
		return reinterpret_cast<sig>((*reinterpret_cast<uint32_t **>(this))[XOR_16(index)]) FORWARD_ARGS

#endif // SDK_MACROS_H
