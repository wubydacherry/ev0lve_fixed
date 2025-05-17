#ifndef LOAD_INDICATOR_647504C40CCC42DCA3FD01484FDFADE6_H
#define LOAD_INDICATOR_647504C40CCC42DCA3FD01484FDFADE6_H

#include <ren/types/animator.h>
#include <ren/types/texture.h>

namespace network
{
class load_indicator
{
public:
	enum section
	{
		sec_init,
		sec_resource_load,
		sec_script_load,
		sec_config_load,
		sec_max
	};

	void begin();
	void end();

	void render();
	void perform_task();

	void begin_next_section();

	float section_progress{0.f};
	section current_section{sec_resource_load};
	bool did_begin{};
	bool is_ending{};

	std::shared_ptr<evo::ren::texture> first_frame;
	std::vector<std::shared_ptr<evo::ren::texture>> logo_frames;

private:
	static constexpr float logo_fps = 1.f / 30.f;

	std::shared_ptr<evo::ren::anim_float> alpha;
	std::shared_ptr<evo::ren::anim_float> spinner;

	int logo_i{0};
	float logo_time{0.f};

	bool did_finish_section(section s) const;
	std::vector<section> finished_sections{};
};

extern load_indicator load_ind;
} // namespace network

#endif // LOAD_INDICATOR_647504C40CCC42DCA3FD01484FDFADE6_H
