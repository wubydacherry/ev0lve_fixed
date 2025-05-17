#include <gui/gui.h>
#include <menu/effects/snow_effect.h>
#include <ren/adapter.h>
#include <ren/renderer.h>

namespace menu::effects
{
snow_effect_t snow;

using namespace evo::ren;
using namespace evo::gui;

void snow_effect_t::render()
{
	spawn();

	auto &d = draw.layers[31];
	for (auto &p : particles)
	{
		simulate(p);
		if (p.life <= 0.f)
			continue;

		d->add_circle_filled(p.pos, p.size * 0.5f, color::white().mod_a(p.life));
	}

	erase_dead_particles();
}

void snow_effect_t::spawn()
{
	if (particles.size() > 250)
		return;

	particle p;
	std::random_device rng;
	p.pos.x = std::uniform_real_distribution<float>(0.f, draw.display.x)(rng);
	p.vel.x = std::uniform_real_distribution<float>(-0.25f, 0.25f)(rng);
	p.vel.y = std::uniform_real_distribution<float>(0.5f, 1.f)(rng);
	p.size = std::uniform_real_distribution<float>(1.f, 5.f)(rng);
	p.life = std::uniform_real_distribution<float>(0.1f, 1.f)(rng) * 20.f;
	particles.push_back(p);
}

void snow_effect_t::simulate(particle &p)
{
	// advance pos unless hit
	if (!p.did_hit)
		p.pos += p.vel;

	// hit test root controls
	p.did_hit = false;
	ctx->for_each_control(
		[&](const std::shared_ptr<control> &c)
		{
			const auto pos = c->pos_abs();
			if (pos.y <= p.pos.y && p.pos.x >= pos.x && p.pos.x <= pos.x + c->size.x)
			{
				p.did_hit = true;
				p.pos.y = pos.y;
			}
		});

	// decrease life
	p.life -= draw.frame_time;
	if (p.pos.y > draw.display.y || p.pos.x < 0.f || p.pos.x > draw.display.x)
		p.life = 0.f;
}

void snow_effect_t::erase_dead_particles()
{
	particles.erase(std::remove_if(particles.begin(), particles.end(), [](const particle &p) { return p.life <= 0.f; }),
					particles.end());
}
} // namespace menu::effects
