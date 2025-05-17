
#ifndef HACKS_ADVERTISEMENT_H
#define HACKS_ADVERTISEMENT_H

namespace hacks
{
class advertising
{
	bool enabled = false;
	std::pair<std::string, char> clantag = XOR_STR_STORE("ev0lve.xyz");
	int32_t last_update_time = 0;
	size_t iter_build = 0, iter_scroll = 0, iter_pause = 0;

public:
	void clantag_changer();
};

extern advertising advert;
} // namespace hacks

#endif // HACKS_ADVERTISEMENT_H
