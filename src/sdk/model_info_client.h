
#ifndef SDK_MODEL_INFO_CLIENT_H
#define SDK_MODEL_INFO_CLIENT_H

#include <sdk/client_entity.h>
#include <sdk/cutlvector.h>
#include <sdk/macros.h>
#include <sdk/mat.h>

namespace sdk
{
struct studiohdr2_t;

inline int model_to_clientside(int i) { return (i <= -2 && (i & 1)) ? (-2 - i) >> 1 : -1; }

class quaternion
{
public:
	quaternion() : x(0), y(0), z(0), w(0) {}
	quaternion(float ix, float iy, float iz, float iw) : x(ix), y(iy), z(iz), w(iw) {}

	void init(float ix = 0.0f, float iy = 0.0f, float iz = 0.0f, float iw = 0.0f)
	{
		x = ix;
		y = iy;
		z = iz;
		w = iw;
	}

	float *base() { return reinterpret_cast<float *>(this); }
	const float *base() const { return reinterpret_cast<const float *>(this); }

	float x, y, z, w;
};

class alignas(16) quaternion_aligned
{
public:
	quaternion_aligned() : x(0), y(0), z(0), w(0) {}
	quaternion_aligned(float ix, float iy, float iz, float iw) : x(ix), y(iy), z(iz), w(iw) {}
	quaternion_aligned(const quaternion &o)
	{
		x = o.x;
		y = o.y;
		z = o.z;
		w = o.w;
	}

	void init(float ix = 0.0f, float iy = 0.0f, float iz = 0.0f, float iw = 0.0f)
	{
		x = ix;
		y = iy;
		z = iz;
		w = iw;
	}

	float *base() { return reinterpret_cast<float *>(this); }
	const float *base() const { return reinterpret_cast<const float *>(this); }

	float x, y, z, w;
};

class radian_euler
{
public:
	inline radian_euler(void) {}
	inline radian_euler(float X, float Y, float Z)
	{
		x = X;
		y = Y;
		z = Z;
	}

	inline void Init(float ix = 0.0f, float iy = 0.0f, float iz = 0.0f)
	{
		x = ix;
		y = iy;
		z = iz;
	}

	inline float *Base() { return &x; }
	inline const float *Base() const { return &x; }

	float x, y, z;
};

struct mstudiobone_t
{
	int sznameindex;

	char *get_name() const { return reinterpret_cast<char *>(const_cast<mstudiobone_t *>(this)) + sznameindex; }

	int parent;
	int bonecontroller[6];

	vec3 pos;
	quaternion quat;
	angle rot;
	vec3 posscale;
	vec3 rotscale;

	mat3x4 pose_to_bone;
	quaternion q_alignment;
	int flags;
	int proctype;
	int procindex;
	mutable int physicsbone;

	int surfacepropidx;

	char *get_surface_id() const
	{
		return reinterpret_cast<char *>(const_cast<mstudiobone_t *>(this)) + surfacepropidx;
	}

	int get_surface_prop() const { return surfacepropLookup; }

	int contents;
	int surfacepropLookup;
	int unused[7];
};

struct mstudioiklink_t
{
	int bone;
	vec3 kneeDir;
	vec3 unused0;
};

struct mstudioikchain_t
{
	int sznameindex;
	inline char *const get_name(void) const { return ((char *)this) + sznameindex; }
	int linktype;
	int numlinks;
	int linkindex;
	inline mstudioiklink_t *link(int i) const { return (mstudioiklink_t *)(((uintptr_t)this) + linkindex) + i; };
};

struct mstudiobbox_t
{
	int bone;
	int group;
	vec3 bbmin;
	vec3 bbmax;
	int hitboxnameindex;
	vec3 rotation;
	float radius;
	int pad2[4];

	char *get_hitbox_name()
	{
		if (hitboxnameindex == 0)
			return nullptr;

		return reinterpret_cast<char *>(const_cast<mstudiobbox_t *>(this)) + hitboxnameindex;
	}
};

struct mstudiohitboxset_t
{
	int sznameindex;

	char *get_name() const { return reinterpret_cast<char *>(const_cast<mstudiohitboxset_t *>(this)) + sznameindex; }

	int numhitboxes;
	int hitboxindex;

	__forceinline mstudiobbox_t *get_hitbox(int i) const
	{
		const auto set = reinterpret_cast<unsigned char *>(const_cast<mstudiohitboxset_t *>(this));
		return reinterpret_cast<mstudiobbox_t *>(set + hitboxindex) + i;
	}
};

struct mstudioattachment_t
{
	int sznameindex;

	char *get_name() const { return reinterpret_cast<char *>(const_cast<mstudioattachment_t *>(this)) + sznameindex; }

	unsigned int flags;
	int localbone;
	float local[12];
	int unused[8];
};

struct mstudioactivitymodifier_t
{
	int sznameindex;
	inline char *get_name() { return (sznameindex) ? (char *)(((uint8_t *)this) + sznameindex) : NULL; }
};

// sequence descriptions
struct mstudioseqdesc_t
{
	int baseptr;
	int szlabelindex;
	int szactivitynameindex;
	int flags;
	int activity;
	int actweight;
	int numevents;
	int eventindex;
	vec3 bbmin;
	vec3 bbmax;
	int numblends;
	int animindexindex;
	int movementindex; // [blend] float array for blended movement
	int groupsize[2];
	int paramindex[2];	 // X, Y, Z, XR, YR, ZR
	float paramstart[2]; // local (0..1) starting value
	float paramend[2];	 // local (0..1) ending value
	int paramparent;
	float fadeintime;	// ideal cross fate in time (0.2 default)
	float fadeouttime;	// ideal cross fade out time (0.2 default)
	int localentrynode; // transition node at entry
	int localexitnode;	// transition node at exit
	int nodeflags;		// transition rules
	float entryphase;	// used to match entry gait
	float exitphase;	// used to match exit gait
	float lastframe;	// frame that should generation EndOfSequence
	int nextseq;		// auto advancing sequences
	int pose;			// index of delta animation between end and nextseq
	int numikrules;
	int numautolayers; //
	int autolayerindex;
	int weightlistindex;
	inline float *pBoneweight(int i) const { return ((float *)(((uint8_t *)this) + weightlistindex) + i); };
	inline float weight(int i) const { return *(pBoneweight(i)); };
	int posekeyindex;
	float *pPoseKey(int iParam, int iAnim) const
	{
		return (float *)(((uint8_t *)this) + posekeyindex) + iParam * groupsize[0] + iAnim;
	}
	float poseKey(int iParam, int iAnim) const { return *(pPoseKey(iParam, iAnim)); }
	int numiklocks;
	int iklockindex;
	int keyvalueindex;
	int keyvaluesize;
	inline const char *KeyValueText(void) const { return keyvaluesize != 0 ? ((char *)this) + keyvalueindex : NULL; }
	int cycleposeindex; // index of pose parameter to use as cycle index
	int activitymodifierindex;
	int numactivitymodifiers;
	inline mstudioactivitymodifier_t *get_activity_modifier(int i) const
	{
		return activitymodifierindex != 0 ? (mstudioactivitymodifier_t *)(((uint8_t *)this) + activitymodifierindex) + i
										  : NULL;
	};
	int animtagindex;
	int numanimtags;
	int rootDriverIndex;
	int unused[2]; // remove/add as appropriate (grow back to 8 ints on version change!)

	mstudioseqdesc_t() {}

private:
	// No copy constructors allowed
	mstudioseqdesc_t(const mstudioseqdesc_t &vOther);
};

struct studiohdr
{
	int id;
	int version;
	long checksum;
	char name[64];
	int length;

	vec3 eyeposition;
	vec3 illumposition;
	vec3 hull_min;
	vec3 hull_max;
	vec3 view_bbmin;
	vec3 view_bbmax;

	int flags;
	int numbones;
	int boneindex;

	mstudiobone_t *get_bone(int i) const
	{
		if (!(i >= 0 && i < numbones))
			return nullptr;

		const auto hdr = reinterpret_cast<unsigned char *>(const_cast<studiohdr *>(this));
		return reinterpret_cast<mstudiobone_t *>(hdr + boneindex) + i;
	}

	mstudioattachment_t *get_local_attachment(int i) const
	{
		const auto hdr = reinterpret_cast<unsigned char *>(const_cast<studiohdr *>(this));
		return reinterpret_cast<mstudioattachment_t *>(hdr + localattachmentindex) + i;
	}

	int numbonecontrollers;
	int bonecontrollerindex;
	int numhitboxsets;
	int hitboxsetindex;

	__forceinline mstudiohitboxset_t *get_hitbox_set(int i) const
	{
		const auto hdr = reinterpret_cast<unsigned char *>(const_cast<studiohdr *>(this));
		return reinterpret_cast<mstudiohitboxset_t *>(hdr + hitboxsetindex) + i;
	}

	__forceinline mstudiobbox_t *get_hitbox(int i, int set) const
	{
		mstudiohitboxset_t const *s = get_hitbox_set(set);
		if (!s)
			return nullptr;

		return s->get_hitbox(i);
	}

	int get_hitbox_count(int set) const
	{
		mstudiohitboxset_t const *s = get_hitbox_set(set);
		if (!s)
			return 0;

		return s->numhitboxes;
	}

	int numlocalanim;
	int localanimindex;
	int numlocalseq;
	int localseqindex;

	mutable int activitylistversion;
	mutable int eventsindexed;

	int numtextures;
	int textureindex;
	int numcdtextures;
	int cdtextureindex;

	int numskinref;
	int numskinfamilies;
	int skinindex;

	short *get_skin_ref(int i) const
	{
		const auto hdr = reinterpret_cast<unsigned char *>(const_cast<studiohdr *>(this));
		return reinterpret_cast<short *>(hdr + skinindex) + i;
	}

	int numbodyparts;
	int bodypartindex;
	int numlocalattachments;
	int localattachmentindex;

	int numlocalnodes;
	int localnodeindex;
	int localnodenameindex;

	char *get_local_node_name(int iNode) const
	{
		const auto hdr = reinterpret_cast<unsigned char *>(const_cast<studiohdr *>(this));
		return reinterpret_cast<char *>(hdr) + *(reinterpret_cast<int *>(hdr + localnodenameindex) + iNode);
	}

	unsigned char *get_local_transition(int i) const
	{
		const auto hdr = reinterpret_cast<unsigned char *>(const_cast<studiohdr *>(this));
		return static_cast<unsigned char *>(hdr + localnodeindex) + i;
	}

	int numflexdesc;
	int flexdescindex;
	int numflexcontrollers;
	int flexcontrollerindex;
	int numflexrules;
	int flexruleindex;
	int numikchains;
	int ikchainindex;
	inline mstudioikchain_t *ik_chain(int i) const
	{
		return (mstudioikchain_t *)(((uintptr_t)this) + ikchainindex) + i;
	};
	int nummouths;
	int mouthindex;
	int numlocalposeparameters;
	int localposeparamindex;
	int surfacepropindex;

	char *get_surface_prop() const
	{
		return reinterpret_cast<char *>(const_cast<studiohdr *>(this)) + surfacepropindex;
	}

	int keyvalueindex;
	int keyvaluesize;

	const char *get_key_value_text() const
	{
		return keyvaluesize != 0 ? reinterpret_cast<char *>(const_cast<studiohdr *>(this)) + keyvalueindex : nullptr;
	}

	int numlocalikautoplaylocks;
	int localikautoplaylockindex;

	float mass;
	int contents;
	int numincludemodels;
	int includemodelindex;

	mutable void *virtual_model;
	int szanimblocknameindex;

	char *get_anim_block_name() const
	{
		return reinterpret_cast<char *>(const_cast<studiohdr *>(this)) + szanimblocknameindex;
	}

	int numanimblocks;
	int animblockindex;
	mutable void *animblock_model;
	int bonetablebynameindex;

	const unsigned char *get_bone_table_sorted_by_name() const
	{
		return reinterpret_cast<unsigned char *>(const_cast<studiohdr *>(this)) + bonetablebynameindex;
	}

	void *vertex_base;
	void *index_base;
	unsigned char constdirectionallightdot;
	unsigned char root_lod;
	unsigned char num_allowed_root_lods;
	unsigned char unused[1];
	int unused4;
	int numflexcontrollerui;
	int flexcontrolleruiindex;
	int unused3[2];
	int studiohdr2index;
	int unused2[1];

	studiohdr2_t *studiohdr2() const { return (studiohdr2_t *)(((uint8_t *)this) + studiohdr2index); }

	mstudioseqdesc_t *get_local_seqdesc(int i) const
	{
		const auto hdr = reinterpret_cast<unsigned char *>(const_cast<studiohdr *>(this));
		if (i < 0 || i >= numlocalseq)
			i = 0;
		return (mstudioseqdesc_t *)(static_cast<unsigned char *>(hdr + localseqindex) + i);
	}

	studiohdr() {}

private:
	studiohdr(const studiohdr &other);
};

class virtualgroup_t
{
public:
	void *cache;
	const studiohdr *get_studio_hdr(void) const { return (studiohdr *)cache; }

	cutlvector<int> boneMap;		  // maps global bone to local bone
	cutlvector<int> masterBone;		  // maps local bone to global bone
	cutlvector<int> masterSeq;		  // maps local sequence to master sequence
	cutlvector<int> masterAnim;		  // maps local animation to master animation
	cutlvector<int> masterAttachment; // maps local attachment to global
	cutlvector<int> masterPose;		  // maps local pose parameter to global
	cutlvector<int> masterNode;		  // maps local transition nodes to global
};

struct virtualsequence_t
{
	int flags;
	int activity;
	int group;
	int index;
};

struct virtualgeneric_t
{
	int group;
	int index;
};

struct virtualmodel_t
{
	char pad_mutex[0x8];
	cutlvector<virtualsequence_t> m_seq;
	cutlvector<virtualgeneric_t> m_anim;
	cutlvector<virtualgeneric_t> m_attachment;
	cutlvector<virtualgeneric_t> m_pose;
	cutlvector<virtualgroup_t> m_group;
	cutlvector<virtualgeneric_t> m_node;
	cutlvector<virtualgeneric_t> m_iklock;
	cutlvector<unsigned short> m_autoplaySequences;
};

struct mstudiolinearbone_t
{
	int numbones;

	int flagsindex;
	inline int flags(int i) const { return *((int *)(((uint8_t *)this) + flagsindex) + i); };
	inline int *pflags(int i) { return ((int *)(((uint8_t *)this) + flagsindex) + i); };

	int parentindex;
	inline int parent(int i) const { return *((int *)(((uint8_t *)this) + parentindex) + i); };

	int posindex;
	inline const vec3 &pos(int i) const { return *((vec3 *)(((uint8_t *)this) + posindex) + i); };

	int quatindex;
	inline const quaternion &quat(int i) const { return *((quaternion *)(((uint8_t *)this) + quatindex) + i); };

	int rotindex;
	inline const radian_euler &rot(int i) const { return *((radian_euler *)(((uint8_t *)this) + rotindex) + i); };

	int posetoboneindex;
	inline const mat3x4 &poseToBone(int i) const { return *((mat3x4 *)(((uint8_t *)this) + posetoboneindex) + i); };

	int posscaleindex;
	inline const vec3 &posscale(int i) const { return *((vec3 *)(((uint8_t *)this) + posscaleindex) + i); };

	int rotscaleindex;
	inline const vec3 &rotscale(int i) const { return *((vec3 *)(((uint8_t *)this) + rotscaleindex) + i); };

	int qalignmentindex;
	inline const quaternion &qalignment(int i) const
	{
		return *((quaternion *)(((uint8_t *)this) + qalignmentindex) + i);
	};

	int unused[6];

	mstudiolinearbone_t() {}

private:
	mstudiolinearbone_t(const mstudiolinearbone_t &vOther);
};

struct studiohdr2_t
{
	int numsrcbonetransform;
	int srcbonetransformindex;

	int illumpositionattachmentindex;
	inline int IllumPositionAttachmentIndex() const { return illumpositionattachmentindex; }

	float flMaxEyeDeflection;
	inline float MaxEyeDeflection() const
	{
		return flMaxEyeDeflection != 0.0f ? flMaxEyeDeflection : 0.866f;
	} // default to cos(30) if not set

	int linearboneindex;
	inline mstudiolinearbone_t *pLinearBones() const
	{
		return (linearboneindex) ? (mstudiolinearbone_t *)(((uint8_t *)this) + linearboneindex) : NULL;
	}

	int sznameindex;
	inline char *pszName() { return (sznameindex) ? (char *)(((uint8_t *)this) + sznameindex) : NULL; }

	int m_nBoneFlexDriverCount;
	int m_nBoneFlexDriverIndex;

	int reserved[56];
};

struct hash_value_type
{
	int activityIdx;
	int starting_index;
	int count;
	int totalWeight;
};

struct sequence_tuple
{
	short seqnum;
	short weight;
	uint16_t *activity_modifiers;
	int32_t count_activity_modifiers;
	int32_t *calculation_modes;
};

class cstudiohdr;

struct sequence_hash
{
	FUNC(game->client.at(functions::hdr::find_hash), find_hash(hash_value_type *value),
		 uint32_t(__thiscall *)(void *, hash_value_type *))
	(value);

	cutlvector<cutlvector<hash_value_type>> buckets;
	uintptr_t cmp_fn, key_fn;
};

struct activity_mapping
{
	FUNC(game->client.at(functions::hdr::reinitialize), reinitialize(cstudiohdr *hdr),
		 void(__thiscall *)(void *, cstudiohdr *))
	(hdr);

	sequence_tuple *sequence_tuples;
	uint32_t sequence_tuples_count;
	sequence_hash act_to_sequence_hash;
	const void *studio_hdr;
	const void *v_model;
};

struct softbody_t
{
	uint8_t pad[0x28];
	uintptr_t fe;

	//FUNC(game->client.at(functions::soft_body::shutdown), shutdown(), void(__thiscall *)(void *))(); // thankfuly its unused otherwise i wouldnt able to find it lol
};

class cstudiohdr
{
public:
	mutable studiohdr *hdr;
	mutable virtualmodel_t *v_model;
	mutable softbody_t *softbody;
	mutable cutlvector<const studiohdr *> m_pStudioHdrCache;
	mutable int m_nFrameUnlockCounter;
	int *m_pFrameUnlockCounter;
	char pad_mutex[0x8];
	cutlvector<int> bone_flags;
	cutlvector<int> bone_parent;

	inline int numbones(void) const { return hdr->numbones; };
	inline const uint8_t *GetBoneTableSortedByName() const { return (uint8_t *)hdr + hdr->bonetablebynameindex; }
	inline mstudiobone_t *bone(int i) const { return hdr->get_bone(i); };
	inline mstudiolinearbone_t *pLinearBones() const
	{
		return hdr->studiohdr2index ? hdr->studiohdr2()->pLinearBones() : NULL;
	}
	inline virtualmodel_t *get_virtual_model(void) const { return v_model; };
	int GetNumAttachments(void) const
	{
		if (hdr->numincludemodels == 0)
			return hdr->numlocalattachments;

		virtualmodel_t *pVModel = get_virtual_model();

		return pVModel->m_attachment.count();
	}

	studiohdr *group_studio_hdr(int i)
	{
		const studiohdr *pStudioHdr = m_pStudioHdrCache[i];

		if (!pStudioHdr)
		{
			virtualgroup_t *pGroup = &v_model->m_group[i];
			pStudioHdr = pGroup->get_studio_hdr();
		}

		return const_cast<studiohdr *>(pStudioHdr);
	}

	const mstudioattachment_t &pAttachment(int i)
	{
		if (v_model == NULL)
			return *hdr->get_local_attachment(i);

		const studiohdr *pStudioHdr = group_studio_hdr(v_model->m_attachment[i].group);

		return *pStudioHdr->get_local_attachment(v_model->m_attachment[i].index);
	}

	int GetAttachmentBone(int i)
	{
		const mstudioattachment_t &attachment = pAttachment(i);

		virtualmodel_t *pVModel = get_virtual_model();
		if (pVModel)
		{
			virtualgroup_t *pGroup = &pVModel->m_group[pVModel->m_attachment[i].group];
			int iBone = pGroup->masterBone[attachment.localbone];
			if (iBone == -1)
				return 0;
			return iBone;
		}
		return attachment.localbone;
	}

	int32_t get_numseq()
	{
		auto numseq = 0u;

		if (!v_model)
			numseq = hdr->numlocalseq;
		else
			numseq = v_model->m_seq.count();

		return numseq;
	}

	bool holds_animation() { return get_numseq() >= 3; }

	mstudioseqdesc_t *get_seq_desc(const int index)
	{
		if (hdr->numincludemodels == 0 || !v_model)
			return hdr->get_local_seqdesc(index);

		return ((mstudioseqdesc_t * (__thiscall *)(void *, int32_t))(game->server.at(functions::hdr::lookup)))(this,
																											   index);
	}

	bool sequences_available() const
	{
		if (hdr->numincludemodels == 0)
			return true;

		return v_model != nullptr;
	}

	int get_activity_list_version()
	{
		if (v_model == nullptr)
			return hdr->activitylistversion;

		auto version = hdr->activitylistversion;

		for (auto i = 1; i < v_model->m_group.count(); i++)
		{
			const auto g_hdr = group_studio_hdr(i);
			version = min(version, g_hdr->activitylistversion);
		}

		return version;
	}

	activity_mapping *&get_activity_mapping()
	{
		return *reinterpret_cast<activity_mapping **const>(uintptr_t(this) + 0x58);
	}

	FUNC(game->client.at(functions::hdr::init), init(studiohdr *studio, void *mdlcache),
		 void(__thiscall *)(void *, studiohdr *, void *))
	(studio, mdlcache);
	FUNC(game->client.at(functions::hdr::init_softbody), init_softbody(uintptr_t softbody_env),
		 void(__thiscall *)(void *, uintptr_t))
	(softbody_env);
	FUNC(game->client.at(functions::hdr::find_mapping), find_mapping(), activity_mapping *(__thiscall *)(void *))();
	FUNC(game->client.at(functions::hdr::index_model_sequences), index_model_sequences(), void(__thiscall *)(void *))();
	//FUNC(game->client.at(functions::hdr::term), term(), void(__thiscall *)(void *))();
};

struct draw_model_state
{
	studiohdr *hdr;
	void *hwdata;
	void *renderable;
	const mat3x4 *model_to_world;
	int32_t decals;
	int32_t draw_flags;
	int32_t lod;
};

struct draw_model_info
{
	studiohdr *hdr;
	void *hwdata;
	void *decals;
	int skin;
	int body;
	int hitbox_set;
	void *entity;
	int lod;
	void *meshes;
	bool static_lighting;
};

class model_info_client_t
{
public:
	VIRTUAL(1, get_model(int32_t index), model *(__thiscall *)(void *, int32_t))(index);
	VIRTUAL(2, get_model_index(const char *name), int(__thiscall *)(void *, const char *))(name);
	VIRTUAL(3, get_model_name(const model *mod), const char *(__thiscall *)(void *, const model *))(mod);
	VIRTUAL(43, find_or_load_model(const char *name), model *(__thiscall *)(void *, const char *))(name);
	VIRTUAL(44, get_cache_handle(const model *mod), uint16_t(__thiscall *)(void *, const model *))(mod);
	VIRTUAL(50, register_dynamic_model(const char *name), int(__thiscall *)(void *, const char *, bool))(name, true);
};
} // namespace sdk

#endif // SDK_MODEL_INFO_CLIENT_H
