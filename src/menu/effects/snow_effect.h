#ifndef SNOW_EFFECT_A689F6CC851444FA8E4037084D3C9F84_H
#define SNOW_EFFECT_A689F6CC851444FA8E4037084D3C9F84_H

#include <ren/types/color.h>
#include <ren/types/pos.h>

namespace menu::effects
{
class snow_effect_t
{
	struct particle
	{
		evo::ren::vec2 pos, vel;
		float size{}, life{};
		bool did_hit{};
	};

public:
	void render();

private:
	void spawn();
	void simulate(particle &p);
	void erase_dead_particles();

	std::vector<particle> particles;
};

extern snow_effect_t snow;
} // namespace menu::effects

#endif // SNOW_EFFECT_A689F6CC851444FA8E4037084D3C9F84_H
