
#ifndef SDK_WEAPON_SYSTEM_H
#define SDK_WEAPON_SYSTEM_H

#include <sdk/macros.h>

namespace sdk
{
class weapon_t;

class cs_weapon_data
{
public:
	virtual ~cs_weapon_data() = default;

	char *consolename;		 // 0x0004
	char pad_0008[8];		 // 0x0008
	void *m_pweapondef;		 // 0x0010
	int maxclip1;			 // 0x0014
	int imaxclip2;			 // 0x0018
	int idefaultclip1;		 // 0x001c
	int idefaultclip2;		 // 0x0020
	char pad_0024[8];		 // 0x0024
	char *szworldmodel;		 // 0x002c
	char *szviewmodel;		 // 0x0030
	char *szdroppedmodel;	 // 0x0034
	char pad_0038[4];		 // 0x0038
	char *n0000023e;		 // 0x003c
	char pad_0040[56];		 // 0x0040
	char *szemptysound;		 // 0x0078
	char pad_007c[4];		 // 0x007c
	char *szbullettype;		 // 0x0080
	char pad_0084[4];		 // 0x0084
	char *szhudname;		 // 0x0088
	char *szweaponname;		 // 0x008c
	char pad_0090[60];		 // 0x0090
	int weapontype;			 // 0x00c8
	int iweaponprice;		 // 0x00cc
	int ikillaward;			 // 0x00d0
	char *szanimationprefix; // 0x00d4
	float cycle_time;		 // 0x00d8
	float cycle_time_alt;	 // 0x00dc
	float fltimetoidle;		 // 0x00e0
	float flidleinterval;	 // 0x00e4
	bool bfullauto;			 // 0x00e8
	char pad_0x00e5[3];		 // 0x00e9
	int idamage;			 // 0x00ec
	float padding;
	float flarmorratio;					 // 0x00f0
	int ibullets;						 // 0x00f4
	float flpenetration;				 // 0x00f8
	float flflinchvelocitymodifierlarge; // 0x00fc
	float flflinchvelocitymodifiersmall; // 0x0100
	float range;						 // 0x0104
	float flrangemodifier;				 // 0x0108
	float flthrowvelocity;				 // 0x010c
	char pad_0x010c[16];				 // 0x0110
	bool bhassilencer;					 // 0x011c
	char pad_0x0119[3];					 // 0x011d
	char *psilencermodel;				 // 0x0120
	int icrosshairmindistance;			 // 0x0124
	float flmaxplayerspeed;				 // 0x0128
	float flmaxplayerspeedalt;			 // 0x012c
	char pad_0x0130[4];					 // 0x0130
	float flspread;						 // 0x0134
	float flspreadalt;					 // 0x0138
	float flinaccuracycrouch;			 // 0x013c
	float flinaccuracycrouchalt;		 // 0x0140
	float flinaccuracystand;			 // 0x0144
	float flinaccuracystandalt;			 // 0x0148
	float flinaccuracyjumpinitial;		 // 0x014c
	float flinaccuracyjump;				 // 0x0150
	float flinaccuracyjumpalt;			 // 0x0154
	float flinaccuracyland;				 // 0x0158
	float flinaccuracylandalt;			 // 0x015c
	float flinaccuracyladder;			 // 0x0160
	float flinaccuracyladderalt;		 // 0x0164
	float flinaccuracyfire;				 // 0x0168
	float flinaccuracyfirealt;			 // 0x016c
	float flinaccuracymove;				 // 0x0170
	float flinaccuracymovealt;			 // 0x0174
	uint8_t pad_0x0180[0x4C];
	int32_t zoom_levels;
	int32_t zoom_fov1;
	int32_t zoom_fov2;
	uint8_t pad_0x01D4[0xC];
};

class weapon_system_t
{
public:
	VIRTUAL(2, get_weapon_data(item_definition_index item),
			cs_weapon_data *(__thiscall *)(void *, item_definition_index))
	(item);
};
} // namespace sdk

#endif // SDK_WEAPON_SYSTEM_H
