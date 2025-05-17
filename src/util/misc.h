
#ifndef UTIL_MISC_H
#define UTIL_MISC_H

#include <util/value_obfuscation.h>

namespace util
{
inline static std::wstring utf8_decode(const std::string &str)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], int32_t(str.size()), nullptr, 0);
	std::wstring wstr(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], int32_t(str.size()), &wstr[0], size_needed);
	return wstr;
}

inline static std::string utf8_encode(const std::wstring &wstr)
{
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

inline static std::vector<std::string> split(const std::string &s, const std::string &d)
{
	std::vector<std::string> out{};

	size_t off;
	size_t last_off{};
	while ((off = s.find(d, last_off)) != s.npos)
	{
		out.emplace_back(s.substr(last_off, off - last_off));
		last_off = off + 1;
	}

	out.emplace_back(s.substr(s.rfind(d) + 1));
	return out;
}
} // namespace util

#define XOR_STR_UTF8(s) util::utf8_decode(XOR_STR(s)).c_str()
#define XOR_FORMAT_UTF8(s, ...) util::utf8_decode(XOR_FORMAT(s, __VA_ARGS__)).c_str()

#endif // UTIL_MISC_H
