
#ifndef MACROS_H
#define MACROS_H

#define STRINGIFY(x) #x
#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

template <typename t, t v> struct constant_holder
{
	enum class val_holder : t
	{
		val = v
	};
};

#define CONSTANT(value) ((decltype(value))constant_holder<decltype(value), value>::val_holder::val)

#define NO_COPY_OR_MOVE(t)                                                                                             \
	t(t const &other) = delete;                                                                                        \
	t &operator=(t const &other) = delete;                                                                             \
	t(t &&other) = delete;                                                                                             \
	t &operator=(t &&other) = delete;

#endif // MACROS_H
