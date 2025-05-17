
#ifndef UTIL_POLY_H
#define UTIL_POLY_H

#include <stack>
#include <util/circular_buffer.h>

namespace util
{
struct player_intersection
{
	inline static constexpr auto sphere_count = 10;

	struct
	{
		bool valid{};
		sdk::cs_player_t::hitbox index{};
		int32_t group{};
		float radius = -1.f;
		std::array<std::array<sdk::vec3, sphere_count>, 3> spheres{};
	} hitboxes[static_cast<int32_t>(sdk::cs_player_t::hitbox::max)]{};
	uint8_t cnt;

	player_intersection(const sdk::studiohdr *hdr, int32_t set, std::array<sdk::mat3x4 *, 3> &mat)
	{
		cnt = !!mat[0] + !!mat[1] + !!mat[2];

		for (const auto &hb : sdk::cs_player_t::hitboxes)
		{
			auto &box = hitboxes[static_cast<int32_t>(hb)];
			box.valid = false;

			const auto hitbox = hdr->get_hitbox(static_cast<int32_t>(hb), set);
			if (!hitbox || hitbox->radius <= 0.f)
				continue;

			box = {true, hb, hitbox->group, hitbox->radius, {}};

			for (auto j = 0; j < cnt; j++)
			{
				auto &spheres = box.spheres[j];
				auto num_spheres = 0;
				vector_transform(hitbox->bbmax, mat[j][hitbox->bone], spheres[num_spheres++]);
				vector_transform(hitbox->bbmin, mat[j][hitbox->bone], spheres[num_spheres++]);

				const auto delta = (spheres[0] - spheres[1]) / static_cast<float>(sphere_count - 1);
				for (auto p = 0; p < sphere_count - 2; p++, num_spheres++)
					spheres[num_spheres] = spheres[num_spheres - 1] + delta;
			}
		}
	}

	inline int32_t trace_hitgroup(const sdk::ray &r, bool scan_secure) const
	{
		auto res = hitgroup_gear;
		auto hit_head = false;
		auto head_frac = FLT_MAX;
		const auto dir = sdk::vec3(r.delta).normalize();

		for (const auto &hb : sdk::cs_player_t::hitboxes)
		{
			const auto &box = hitboxes[static_cast<int32_t>(hb)];
			if (!box.valid)
				continue;

			for (auto i = 0; i < cnt; i++)
			{
				if (!scan_secure && i > 0)
					break;

				for (auto s = 0; s < sphere_count; s++)
				{
					if (box.group == hitgroup_head || box.group == hitgroup_chest || box.group == hitgroup_stomach)
					{
						const auto frac = intersect_sphere(r, dir, box.spheres[i][s], box.radius);
						if (frac == FLT_MAX)
							continue;

						if (box.group == hitgroup_head)
						{
							hit_head = true;
							if (frac < head_frac)
								head_frac = frac;
						}
						else if (hit_head && frac < head_frac + .1f)
							hit_head = false;

						if (box.group != hitgroup_head)
							res = best_hg(box.group, res);
					}
					else if (intersect_sphere_approx(r, dir, box.spheres[i][s], box.radius))
						res = best_hg(box.group, res);
				}
			}
		}

		if (hit_head)
			res = hitgroup_head;

		return res;
	}

	__forceinline int32_t rank(int32_t i) const
	{
		switch (i)
		{
		case hitgroup_head:
			return 1;
		case hitgroup_stomach:
			return 2;
		case hitgroup_chest:
			return 3;
		case hitgroup_leftarm:
		case hitgroup_rightarm:
			return 4;
		case hitgroup_leftleg:
		case hitgroup_rightleg:
			return 5;
		default:
			return 6;
		}
	}

	inline int32_t best_hg(int32_t n, int32_t o) const { return rank(n) < rank(o) ? n : o; }

	inline bool intersect_sphere_approx(const sdk::ray &r, const sdk::vec3 &dir, const sdk::vec3 &sphere,
										const float radius) const
	{
		const auto q = sphere - r.start;
		const auto v = q.dot(dir);
		return radius - (q.length_sqr() - v * v);
	}

	inline float intersect_sphere(const sdk::ray &r, const sdk::vec3 &dir, const sdk::vec3 &sphere,
								  const float radius) const
	{
		const auto q = r.start - sphere;
		const auto v = q.dot(dir);
		const auto w = q.dot(q) - radius * radius;

		if (w > 0.f && v > 0.f)
			return FLT_MAX;

		const auto discr = v * v - w;
		if (discr < 0.f)
			return FLT_MAX;

		const auto t = -v - sqrtf(discr);
		return t < 0.f ? 0.f : t;
	}
};

typedef circular_buffer<sdk::vec3> convex_polygon;

inline bool is_equal(const float d1, const float d2)
{
	constexpr auto equity_tolerance = .001f;
	return fabsf(d1 - d2) <= equity_tolerance;
}

inline bool inside_poly(const sdk::vec3 &test, const convex_polygon &poly)
{
	auto res = false;
	for (auto i = 0, j = poly.size() - 1; i < poly.size(); j = i++)
	{
		if ((poly[i].y > test.y) != (poly[j].y > test.y) &&
			(test.x < (poly[j].x - poly[i].x) * (test.y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x))
		{
			res = !res;
		}
	}
	return res;
}

inline std::optional<sdk::vec3> get_intersection(const sdk::vec3 &l1p1, const sdk::vec3 &l1p2, const sdk::vec3 &l2p1,
												 const sdk::vec3 &l2p2)
{
	const auto a1 = l1p2.y - l1p1.y;
	const auto b1 = l1p1.x - l1p2.x;
	const auto c1 = a1 * l1p1.x + b1 * l1p1.y;
	const auto a2 = l2p2.y - l2p1.y;
	const auto b2 = l2p1.x - l2p2.x;
	const auto c2 = a2 * l2p1.x + b2 * l2p1.y;

	const auto det = a1 * b2 - a2 * b1;
	if (is_equal(det, 0.f))
		return {};

	else
	{
		const auto x = (b2 * c1 - b1 * c2) / det;
		const auto y = (a1 * c2 - a2 * c1) / det;
		const auto online1 = ((min(l1p1.x, l1p2.x) < x || is_equal(min(l1p1.x, l1p2.x), x)) &&
							  (max(l1p1.x, l1p2.x) > x || is_equal(max(l1p1.x, l1p2.x), x)) &&
							  (min(l1p1.y, l1p2.y) < y || is_equal(min(l1p1.y, l1p2.y), y)) &&
							  (max(l1p1.y, l1p2.y) > y || is_equal(max(l1p1.y, l1p2.y), y)));
		const auto online2 = ((min(l2p1.x, l2p2.x) < x || is_equal(min(l2p1.x, l2p2.x), x)) &&
							  (max(l2p1.x, l2p2.x) > x || is_equal(max(l2p1.x, l2p2.x), x)) &&
							  (min(l2p1.y, l2p2.y) < y || is_equal(min(l2p1.y, l2p2.y), y)) &&
							  (max(l2p1.y, l2p2.y) > y || is_equal(max(l2p1.y, l2p2.y), y)));

		if (online1 && online2)
			return sdk::vec3(x, y, 0.f);
	}
	return {};
}

inline void add_points(convex_polygon &points, const sdk::vec3 &np)
{
	auto found = false;
	for (auto j = 0u; j < points.size(); j++)
	{
		auto &p = points[j];
		if (is_equal(p.x, np.x) && is_equal(p.y, np.y))
		{
			found = true;
			break;
		}
	}

	if (!found && !points.exhausted())
		*points.push_front() = np;
}

inline convex_polygon get_intersections(const sdk::vec3 &l1p1, const sdk::vec3 &l1p2, const convex_polygon &poly,
										convex_polygon &out)
{
	convex_polygon intersection_vectors(poly.size());
	for (auto i = 0u; i < poly.size(); i++)
	{
		const int next = i + 1 == poly.size() ? 0 : i + 1;
		if (auto intersection = get_intersection(l1p1, l1p2, poly[i], poly[next]); intersection.has_value())
			add_points(out, *intersection);
	}
	return intersection_vectors;
}

inline void order_clockwise(convex_polygon &points)
{
	auto mx = 0.f;
	auto my = 0.f;

	for (auto i = 0u; i < points.size(); i++)
	{
		const auto &p = points[i];
		mx += p.x;
		my += p.y;
	}

	mx /= points.size();
	my /= points.size();

	for (auto i = 0u; i < points.size(); i++)
		points[i].z = atan2f(points[i].y - my, points[i].x - mx);

	points.sort([](const sdk::vec3 &p1, const sdk::vec3 &p2) { return p1.z > p2.z; });
}

inline float area(const convex_polygon &poly)
{
	auto area = 0.f;

	auto j = poly.size() - 1;
	for (int i = 0; i < poly.size(); i++)
	{
		area += (poly[j].x + poly[i].x) * (poly[j].y - poly[i].y);
		j = i;
	}

	return fabsf(area * .5f);
}

inline convex_polygon get_intersection_poly(const convex_polygon &poly1, const convex_polygon &poly2)
{
	if (poly1.size() < 3)
		return {};

	convex_polygon clipped_corners(poly1.size() + poly2.size());

	// p1 points inside p2
	for (auto i = 0u; i < poly1.size(); i++)
	{
		auto &p = poly1[i];
		if (inside_poly(p, poly2))
			add_points(clipped_corners, p);
	}

	// p2 points inside p1
	for (auto i = 0u; i < poly2.size(); i++)
	{
		auto &p = poly2[i];
		if (inside_poly(p, poly1))
			add_points(clipped_corners, p);
	}

	// intersections
	for (auto i = 0u, next = 1u; i < poly1.size(); i++, next = (i + 1 == poly1.size()) ? 0 : i + 1)
		get_intersections(poly1[i], poly1[next], poly2, clipped_corners);

	if (clipped_corners.size() < 3)
		return {};

	order_clockwise(clipped_corners);
	return clipped_corners;
}

inline float cross(const sdk::vec3 &O, const sdk::vec3 &A, const sdk::vec3 &B)
{
	return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

inline void monotone_chain(convex_polygon &points)
{
	size_t n = points.size(), k = 0;
	if (n <= 3)
		return;
	convex_polygon H(2 * n);

	// sort points lexicographically
	points.sort([](const sdk::vec3 &a, const sdk::vec3 &b) -> bool { return a.x < b.x || (a.x == b.x && a.y < b.y); });

	// build lower hull
	for (size_t i = 0; i < n; ++i)
	{
		while (k >= 2 && cross(H[k - 2], H[k - 1], points[i]) <= 0)
			k--;
		H[k++] = points[i];
	}

	// build upper hull
	for (size_t i = n - 1, t = k + 1; i > 0; --i)
	{
		while (k >= t && cross(H[k - 2], H[k - 1], points[i - 1]) <= 0)
			k--;
		H[k++] = points[i - 1];
	}

	H.current = H.count - k + 1;
	H.count = k - 1;
	points = std::move(H);
}
} // namespace util

#endif // UTIL_POLY_H
