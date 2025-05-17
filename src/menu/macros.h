
#ifndef MENU_MACROS_H
#define MENU_MACROS_H

#define MAKE_TAB(i, n)                                                                                                 \
	std::make_shared<tab>(evo::gui::control_id{GUI_HASH(i), i}, XOR(n), GUI_HASH(i "_container"),                      \
						  draw.textures[GUI_HASH("icon_" i)])
#define MAKE(h, t, ...) std::make_shared<t>(evo::gui::control_id{GUI_HASH(h), h}, __VA_ARGS__)
#define MAKE_RUNTIME(h, t, ...) std::make_shared<t>(evo::gui::control_id{hash((h).c_str()), h}, __VA_ARGS__)

#define ADD(n, h, t, ...) e->add(make_control(XOR(n), MAKE(h, t, __VA_ARGS__)))
#define ADD_NEW(n, h, t, ...) e->add(make_control(XOR(n), MAKE(h, t, __VA_ARGS__), {}, true))
#define ADD_TOOLTIP(n, h, x, t, ...) e->add(make_control(XOR(n), MAKE(h, t, __VA_ARGS__)->make_tooltip(XOR(x))))
#define ADD_TOOLTIP_NEW(n, h, x, t, ...)                                                                               \
	e->add(make_control(XOR(n), MAKE(h, t, __VA_ARGS__)->make_tooltip(XOR(x)), {}, true))
#define ADD_RUNTIME(n, h, t, ...) e->add(make_control(XOR(n), MAKE_RUNTIME((h), t, __VA_ARGS__)))
#define ADD_INLINE(l, ...)                                                                                             \
	e->add(make_inlined(GUI_HASH(l "_content"),                                                                        \
						[&](std::shared_ptr<layout> &e)                                                                \
						{                                                                                              \
							const std::vector<std::shared_ptr<control>> _{__VA_ARGS__};                                \
							e->add(std::make_shared<label>(XOR(l)));                                                   \
							for (const auto &c : _)                                                                    \
								e->add(c);                                                                             \
						}))
#define ADD_INLINE_NEW(l, ...)                                                                                         \
	e->add(make_inlined(GUI_HASH(l "_content"),                                                                        \
						[&](std::shared_ptr<layout> &e)                                                                \
						{                                                                                              \
							const std::vector<std::shared_ptr<control>> _{__VA_ARGS__};                                \
							const auto lab = std::make_shared<label>(XOR(l));                                          \
							lab->is_new = true;                                                                        \
							e->add(lab);                                                                               \
							for (const auto &c : _)                                                                    \
								e->add(c);                                                                             \
						}))
#define ADD_SECTION(l, w)                                                                                              \
	e->add(MAKE(l "_section_label", label, XOR(l), colors.text, true));                                                \
	e->add(MAKE(l "_section_spacer", spacer, vec2(), vec2(w, 10.f), true, true));

#define ADD_ALIASES(n, h, t, a, ...) e->add(make_control(XOR(n), MAKE(h, t, __VA_ARGS__), a))
#define ADD_TOOLTIP_ALIASES(n, h, x, t, a, ...)                                                                        \
	e->add(make_control(XOR(n), MAKE(h, t, __VA_ARGS__)->make_tooltip(XOR(x)), a))
#define ADD_RUNTIME_ALIASES(n, h, t, a, ...) e->add(make_control(XOR(n), MAKE_RUNTIME((h), t, __VA_ARGS__), a))
#define ADD_INLINE_ALIASES(l, a, ...)                                                                                  \
	e->add(make_inlined(GUI_HASH(l "_content"),                                                                        \
						[&](std::shared_ptr<layout> &e)                                                                \
						{                                                                                              \
							const std::vector<std::shared_ptr<control>> _{__VA_ARGS__};                                \
							const auto lab = std::make_shared<label>(XOR(l));                                          \
							lab->aliases = a;                                                                          \
							e->add(lab);                                                                               \
							for (const auto &c : _)                                                                    \
								e->add(c);                                                                             \
						}))

#define ADD_INLINE_RUNTIME(l, ...)                                                                                     \
	e->add(make_inlined(hash((l + std::string(XOR("_content"))).c_str()),                                              \
						[&](std::shared_ptr<layout> &e)                                                                \
						{                                                                                              \
							const std::vector<std::shared_ptr<control>> _{__VA_ARGS__};                                \
							e->add(std::make_shared<label>(XOR(l)));                                                   \
							for (const auto &c : _)                                                                    \
								e->add(c);                                                                             \
						}))

#define ADD_LINE(l, f, ...)                                                                                            \
	e->add(make_inlined(GUI_HASH(l "_content"),                                                                        \
						[&](std::shared_ptr<layout> &e)                                                                \
						{                                                                                              \
							const std::vector<std::shared_ptr<control>> _{__VA_ARGS__};                                \
							e->add(f);                                                                                 \
							for (const auto &c : _)                                                                    \
								e->add(c);                                                                             \
						}))

#define USE_NAMESPACES                                                                                                 \
	using namespace menu;                                                                                              \
	using namespace evo;                                                                                               \
	using namespace evo::gui;                                                                                          \
	using namespace evo::ren;                                                                                          \
	using namespace helpers;

#endif // MENU_MACROS_H
