/*
===========================================================================
ARX FATALIS GPL Source Code
Copyright (C) 1999-2010 Arkane Studios SA, a ZeniMax Media company.

This file is part of the Arx Fatalis GPL Source Code ('Arx Fatalis Source Code'). 

Arx Fatalis Source Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public 
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Arx Fatalis Source Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Arx Fatalis Source Code.  If not, see 
<http://www.gnu.org/licenses/>.

In addition, the Arx Fatalis Source Code is also subject to certain additional terms. You should have received a copy of these 
additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Arx 
Fatalis Source Code. If not, please request a copy in writing from Arkane Studios at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing Arkane Studios, c/o 
ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/
//////////////////////////////////////////////////////////////////////////////////////
//   @@        @@@        @@@                @@                           @@@@@     //
//   @@@       @@@@@@     @@@     @@        @@@@                         @@@  @@@   //
//   @@@       @@@@@@@    @@@    @@@@       @@@@      @@                @@@@        //
//   @@@       @@  @@@@   @@@  @@@@@       @@@@@@     @@@               @@@         //
//  @@@@@      @@  @@@@   @@@ @@@@@        @@@@@@@    @@@            @  @@@         //
//  @@@@@      @@  @@@@  @@@@@@@@         @@@@ @@@    @@@@@         @@ @@@@@@@      //
//  @@ @@@     @@  @@@@  @@@@@@@          @@@  @@@    @@@@@@        @@ @@@@         //
// @@@ @@@    @@@ @@@@   @@@@@            @@@@@@@@@   @@@@@@@      @@@ @@@@         //
// @@@ @@@@   @@@@@@@    @@@@@@           @@@  @@@@   @@@ @@@      @@@ @@@@         //
// @@@@@@@@   @@@@@      @@@@@@@@@@      @@@    @@@   @@@  @@@    @@@  @@@@@        //
// @@@  @@@@  @@@@       @@@  @@@@@@@    @@@    @@@   @@@@  @@@  @@@@  @@@@@        //
//@@@   @@@@  @@@@@      @@@      @@@@@@ @@     @@@   @@@@   @@@@@@@    @@@@@ @@@@@ //
//@@@   @@@@@ @@@@@     @@@@        @@@  @@      @@   @@@@   @@@@@@@    @@@@@@@@@   //
//@@@    @@@@ @@@@@@@   @@@@             @@      @@   @@@@    @@@@@      @@@@@      //
//@@@    @@@@ @@@@@@@   @@@@             @@      @@   @@@@    @@@@@       @@        //
//@@@    @@@  @@@ @@@@@                          @@            @@@                  //
//            @@@ @@@                           @@             @@        STUDIOS    //
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
// EERIEObject
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//
// Updates: (date) (person) (update)
//
// Code: Cyril Meynier
//
// Copyright (c) 1999 ARKANE Studios SA. All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

#include "scene/Object.h"

#include <cstdio>
#include <climits>

#include "animation/AnimationRender.h"

#include "core/Application.h"

#include "core/Dialog.h"

#include "graphics/GraphicsTypes.h"
#include "graphics/Math.h"
#include "graphics/data/Progressive.h"
#include "graphics/data/FTL.h"
#include "graphics/data/Texture.h"

#include "io/FilePath.h"
#include "io/PakManager.h"
#include "io/PakEntry.h"
#include "io/Filesystem.h"
#include "io/Logger.h"

#include "physics/Clothes.h"
#include "physics/Box.h"
#include "physics/CollisionShapes.h"

#include "platform/String.h"

#include "scene/LinkedObject.h"
#include "scene/GameSound.h"
#include "scene/ObjectFormat.h"
#include "scene/Interactive.h"
#include "scene/Light.h"

using std::sprintf;
using std::min;
using std::max;
using std::string;

extern char LastLoadedScene[256];
extern PakManager * pPakManager;

void EERIE_RemoveCedricData(EERIE_3DOBJ * eobj);
void EERIEOBJECT_CreatePFaces(EERIE_3DOBJ * eobj);
void EERIEOBJECT_DeletePFaces(EERIE_3DOBJ * eobj);

void Clear3DScene(EERIE_3DSCENE	* eerie);

long GetGroupOriginByName(const EERIE_3DOBJ * eobj, const string & text) {
	
	if(!eobj) {
		return -1;
	}
	
	for(long i = 0; i < eobj->nbgroups; i++) {
		// TODO use string compare
		if(!strcasecmp(text, eobj->grouplist[i].name)) {
			return eobj->grouplist[i].origin;
		}
	}
	
	return -1;
}

long GetActionPointIdx(const EERIE_3DOBJ * eobj, const string & text) {
	
	if(!eobj) {
		return -1;
	}
	
	for(vector<EERIE_ACTIONLIST>::const_iterator i = eobj->actionlist.begin();
	    i != eobj->actionlist.end(); ++i) {
		// TODO use string compare
		if(!strcasecmp(text, i->name)) {
			return i->idx;
		}
	}
	
	return -1;
}

long GetActionPointGroup(const EERIE_3DOBJ * eobj, long idx) {
	
	if(!eobj) {
		return -1;
	}
	
	for(long i = eobj->nbgroups - 1; i >= 0; i--) {
		const vector<long> & indices = eobj->grouplist[i].indexes;
		for(size_t j = 0; j < indices.size(); j++){
			if(indices[j] == idx) {
				return i;
			}
		}
	}
	
	return -1;
}

void EERIE_Object_Precompute_Fast_Access(EERIE_3DOBJ * eerie) {
	
	if(!eerie) return;

	long lVRight		=	GetActionPointIdx(eerie, "V_RIGHT");
	long lURight		=	GetActionPointIdx(eerie, "U_RIGHT");
	long lViewAttach	=	GetActionPointIdx(eerie, "View_attach") ;
	long lPrimAttach	=	GetActionPointIdx(eerie, "PRIMARY_ATTACH");
	long lLeftAttach	=	GetActionPointIdx(eerie, "LEFT_ATTACH");

	ARX_CHECK_SHORT(lVRight);
	ARX_CHECK_SHORT(lURight);
	ARX_CHECK_SHORT(lViewAttach);
	ARX_CHECK_SHORT(lPrimAttach);
	ARX_CHECK_SHORT(lLeftAttach);

	eerie->fastaccess.V_right		=	ARX_CLEAN_WARN_CAST_SHORT(lVRight);
	eerie->fastaccess.U_right		=	ARX_CLEAN_WARN_CAST_SHORT(lURight);
	eerie->fastaccess.view_attach	=	ARX_CLEAN_WARN_CAST_SHORT(lViewAttach);
	eerie->fastaccess.primary_attach =	ARX_CLEAN_WARN_CAST_SHORT(lPrimAttach);
	eerie->fastaccess.left_attach	=	ARX_CLEAN_WARN_CAST_SHORT(lLeftAttach);


	long lWeapAttach				=	GetActionPointIdx(eerie, "WEAPON_ATTACH");
	long lSecAttach					=	GetActionPointIdx(eerie, "SECONDARY_ATTACH");
	long lJaw						=	EERIE_OBJECT_GetGroup(eerie, "jaw");
	long lMouthAll					=	EERIE_OBJECT_GetGroup(eerie, "mouth all");

	ARX_CHECK_SHORT(lWeapAttach);
	ARX_CHECK_SHORT(lSecAttach);
	ARX_CHECK_SHORT(lJaw);
	ARX_CHECK_SHORT(lMouthAll);

	eerie->fastaccess.weapon_attach		=	ARX_CLEAN_WARN_CAST_SHORT(lWeapAttach);
	eerie->fastaccess.secondary_attach	=	ARX_CLEAN_WARN_CAST_SHORT(lSecAttach);
	eerie->fastaccess.jaw_group			=	ARX_CLEAN_WARN_CAST_SHORT(lJaw);
	eerie->fastaccess.mouth_group		=	ARX_CLEAN_WARN_CAST_SHORT(lMouthAll);


	if (eerie->fastaccess.mouth_group == -1)
		eerie->fastaccess.mouth_group_origin = -1;
	else
	{
		long lMouthOrigin = eerie->grouplist[eerie->fastaccess.mouth_group].origin;
		ARX_CHECK_SHORT(lMouthOrigin);
		eerie->fastaccess.mouth_group_origin = ARX_CLEAN_WARN_CAST_SHORT(lMouthOrigin);
	}

	long lHeadGroup					=	EERIE_OBJECT_GetGroup(eerie, "head");
	ARX_CHECK_SHORT(lHeadGroup);
	eerie->fastaccess.head_group	=	ARX_CLEAN_WARN_CAST_SHORT(lHeadGroup);

	if (eerie->fastaccess.head_group == -1)
		eerie->fastaccess.head_group_origin = -1;
	else
	{
		long lHeadOrigin  = eerie->grouplist[eerie->fastaccess.head_group].origin;
		ARX_CHECK_SHORT(lHeadOrigin);
		eerie->fastaccess.head_group_origin = ARX_CLEAN_WARN_CAST_SHORT(lHeadOrigin);
	}


	long lFire = GetActionPointIdx(eerie, "FIRE");
	long lCarryAttach = GetActionPointIdx(eerie, "CARRY_ATTACH");
	long lHead = EERIE_OBJECT_GetSelection(eerie, "head");
	long lChest = EERIE_OBJECT_GetSelection(eerie, "chest");
	long lLeggings = EERIE_OBJECT_GetSelection(eerie, "leggings") ;

	ARX_CHECK_SHORT(lFire);
	ARX_CHECK_SHORT(lCarryAttach);
	ARX_CHECK_SHORT(lHead);
	ARX_CHECK_SHORT(lChest);
	ARX_CHECK_SHORT(lLeggings);

	eerie->fastaccess.fire = ARX_CLEAN_WARN_CAST_SHORT(lFire);
	eerie->fastaccess.carry_attach = ARX_CLEAN_WARN_CAST_SHORT(lCarryAttach);
	eerie->fastaccess.sel_head = ARX_CLEAN_WARN_CAST_SHORT(lHead);
	eerie->fastaccess.sel_chest = ARX_CLEAN_WARN_CAST_SHORT(lChest);
	eerie->fastaccess.sel_leggings = ARX_CLEAN_WARN_CAST_SHORT(lLeggings);
}

//-----------------------------------------------------------------------------------------------------
void ReleaseAnim(EERIE_ANIM * ea)
{
	if (!ea) return;

	if (ea->frames)
	{
		for (long i = 0; i < ea->nb_key_frames; i++)
		{
			ARX_SOUND_Free(ea->frames[i].sample);
		}

		free(ea->frames);
	}

	if (ea->groups)
		free(ea->groups);

	if (ea->voidgroups)
		free(ea->voidgroups);

	free(ea);
}
//-----------------------------------------------------------------------------------------------------
float GetTimeBetweenKeyFrames(EERIE_ANIM * ea, long f1, long f2)
{
	if (!ea) return 0;

	if (f1 < 0) return 0;

	if (f1 > ea->nb_key_frames - 1) return 0;

	if (f2 < 0) return 0;

	if (f2 > ea->nb_key_frames - 1) return 0;

	float time = 0;

	for (long kk = f1 + 1; kk <= f2; kk++)
	{
		time += ea->frames[kk].time;
	}

	return time;
}

template <class T>
static T * allocStructZero(size_t n = 1) {
	T * result = (T*)malloc(n * sizeof(T));
	memset(result, 0, n * sizeof(T));
	return result;
}

template <class T>
static T * copyStruct(const T * src, size_t n = 1) {
	T * result = (T*)malloc(n * sizeof(T));
	memcpy(result, src, sizeof(T) * n);
	return result;
}

EERIE_ANIM * TheaToEerie(unsigned char * adr, size_t size, const string & file) {
	
	(void)size; // TODO use size
	
	LogDebug << "Loading animation file " << file;
	
	size_t pos = 0;
	
	EERIE_ANIM * eerie = allocStructZero<EERIE_ANIM>();
	
	THEA_HEADER th;
	memcpy(&th, adr + pos, sizeof(THEA_HEADER));
	if(th.version < 2014) {
		LogError << "Invalid TEA Version " << th.version << " in " << file;
		free(eerie);
		return NULL;
	}
	pos += sizeof(THEA_HEADER);
	
	LogDebug << "TEA header size: " << sizeof(THEA_HEADER);
	LogDebug << "Identity " << th.identity;
	LogDebug << "Version - " << th.version << "  Frames " << th.nb_frames
	         << "  Groups " << th.nb_groups << "  KeyFrames " << th.nb_key_frames;
	
	eerie->nb_groups = th.nb_groups;
	eerie->nb_key_frames = th.nb_key_frames;
	
	eerie->frames = allocStructZero<EERIE_FRAME>(th.nb_key_frames);
	eerie->groups = allocStructZero<EERIE_GROUP>(th.nb_key_frames * th.nb_groups);
	eerie->voidgroups = allocStructZero<unsigned char>(th.nb_groups);
	
	eerie->anim_time = 0.f;
	
	// Go For Keyframes read
	for(long i = 0; i < th.nb_key_frames; i++) {
		LogDebug << "Loading keyframe " << i;
		
		THEA_KEYFRAME_2015 tkf2015;
		if(th.version >= 2015) {
			LogDebug << " New keyframe version THEA_KEYFRAME_2015:" << sizeof(THEA_KEYFRAME_2015);
			memcpy(&tkf2015, adr + pos, sizeof(THEA_KEYFRAME_2015));
			pos += sizeof(THEA_KEYFRAME_2015);
		} else {
			LogDebug << " Old keyframe version THEA_KEYFRAME:" << sizeof(THEA_KEYFRAME);
			THEA_KEYFRAME tkf;
			memcpy(&tkf, adr + pos, sizeof(THEA_KEYFRAME));
			pos += sizeof(THEA_KEYFRAME);
			memset(&tkf2015, 0, sizeof(THEA_KEYFRAME_2015));
			tkf2015.num_frame = tkf.num_frame;
			tkf2015.flag_frame = tkf.flag_frame;
			tkf2015.master_key_frame = tkf.master_key_frame;
			tkf2015.key_frame = tkf.key_frame;
			tkf2015.key_move = tkf.key_move;
			tkf2015.key_orient = tkf.key_orient;
			tkf2015.key_morph = tkf.key_morph;
			tkf2015.time_frame = tkf.time_frame;
		}
		
		eerie->frames[i].master_key_frame = tkf2015.master_key_frame;
		eerie->frames[i].num_frame = tkf2015.num_frame;
		
		long lKeyOrient = tkf2015.key_orient ;
		long lKeyMove = tkf2015.key_move ;
		ARX_CHECK_SHORT(tkf2015.key_orient);
		ARX_CHECK_SHORT(tkf2015.key_move);
		eerie->frames[i].f_rotate = ARX_CLEAN_WARN_CAST_SHORT(lKeyOrient);
		eerie->frames[i].f_translate = ARX_CLEAN_WARN_CAST_SHORT(lKeyMove);
		
		tkf2015.time_frame = tkf2015.num_frame * 1000;
		eerie->frames[i].time = tkf2015.time_frame * ( 1.0f / 24 );
		eerie->anim_time += tkf2015.time_frame;
		eerie->frames[i].flag = tkf2015.flag_frame;
		
		LogDebug << " pos " << pos << " - NumFr " << eerie->frames[i].num_frame
		         << " MKF " << tkf2015.master_key_frame << " THEA_KEYFRAME " << sizeof(THEA_KEYFRAME)
		         << " TIME " << (float)(eerie->frames[i].time / 1000.f) << "s -Move " << tkf2015.key_move
		         << " Orient " << tkf2015.key_orient << " Morph " << tkf2015.key_morph;
		
		// Is There a Global translation ?
		if(tkf2015.key_move != 0) {
			
			THEA_KEYMOVE tkm;
			memcpy(&tkm, adr + pos, sizeof(THEA_KEYMOVE));
			pos += sizeof(THEA_KEYMOVE);
			
			LogDebug << " -> move x " << tkm.x << " y " << tkm.y << " z " << tkm.z
			         << " THEA_KEYMOVE:" << sizeof(THEA_KEYMOVE);
			
			eerie->frames[i].translate = tkm;
		}
		
		// Is There a Global Rotation ?
		if(tkf2015.key_orient != 0) {
			pos += 8; // THEO_ANGLE
			
			ArxQuat quat;
			memcpy(&quat, adr + pos, sizeof(ArxQuat));
			pos += sizeof(ArxQuat);
			
			LogDebug << " -> rotate x " << quat.x << " y " << quat.y << " z " << quat.z
			         << " w " << quat.w << " ArxQuat:" << sizeof(ArxQuat);
			
			eerie->frames[i].quat = quat;
		}
		
		// Is There a Global Morph ? (IGNORED!)
		if(tkf2015.key_morph != 0) {
			pos += 16; // THEA_MORPH
		}
		
		// Now go for Group Rotations/Translations/scaling for each GROUP
		for(long j = 0; j < th.nb_groups; j++) {
			
			THEO_GROUPANIM tga;
			memcpy(&tga, adr + pos, sizeof(THEO_GROUPANIM));
			pos += sizeof(THEO_GROUPANIM);
			
			EERIE_GROUP * eg = &eerie->groups[j + i * th.nb_groups];
			eg->key = tga.key_group;
			eg->quat = tga.Quaternion;
			eg->translate = tga.translate;
			eg->zoom = tga.zoom;
		}
		
		// Now Read Sound Data included in this frame
		s32 num_sample;
		memcpy(&num_sample, adr + pos, sizeof(s32));
		pos += sizeof(s32);
		LogDebug << " -> num_sample " << num_sample << " s32:" << sizeof(s32);
		
		eerie->frames[i].sample = -1;
		if(num_sample != -1) {
			
			THEA_SAMPLE ts;
			memcpy(&ts, adr + pos, sizeof(THEA_SAMPLE));
			pos += sizeof(THEA_SAMPLE);
			pos += ts.sample_size;
			
			LogDebug << " -> sample " << ts.sample_name << " size " << ts.sample_size
			         << " THEA_SAMPLE:" << sizeof(THEA_SAMPLE);
			
			eerie->frames[i].sample = ARX_SOUND_Load(ts.sample_name);
		}
		
		pos += 4; // num_sfx
	}
	
	for(long i = 0; i < th.nb_key_frames; i++) {
		
		if(!eerie->frames[i].f_translate) {
			
			long k = i;
			while((k >= 0) && (!eerie->frames[k].f_translate)) {
				k--;
			}
			
			long j = i;
			while((j < th.nb_key_frames) && (!eerie->frames[j].f_translate)) {
				j++;
			}
			
			if((j < th.nb_key_frames) && (k >= 0)) {
				float r1 = GetTimeBetweenKeyFrames(eerie, k, i);
				float r2 = GetTimeBetweenKeyFrames(eerie, i, j);
				float tot = 1.f / (r1 + r2);
				r1 *= tot;
				r2 *= tot;
				// TODO use overloaded operators
				eerie->frames[i].translate.x = eerie->frames[j].translate.x * r1 + eerie->frames[k].translate.x * r2;
				eerie->frames[i].translate.y = eerie->frames[j].translate.y * r1 + eerie->frames[k].translate.y * r2;
				eerie->frames[i].translate.z = eerie->frames[j].translate.z * r1 + eerie->frames[k].translate.z * r2;
			}
		}
		
		if(!eerie->frames[i].f_rotate) {
			
			long k = i;
			while((k >= 0) && (!eerie->frames[k].f_rotate)) {
				k--;
			}
			
			long j = i;
			while ((j < th.nb_key_frames) && (!eerie->frames[j].f_rotate)) {
				j++;
			}
			
			if ((j < th.nb_key_frames) && (k >= 0)) {
				float r1 = GetTimeBetweenKeyFrames(eerie, k, i);
				float r2 = GetTimeBetweenKeyFrames(eerie, i, j);
				float tot = 1.f / (r1 + r2);
				r1 *= tot;
				r2 *= tot;
				// TODO use overloaded operators
				eerie->frames[i].quat.w = eerie->frames[j].quat.w * r1 + eerie->frames[k].quat.w * r2;
				eerie->frames[i].quat.x = eerie->frames[j].quat.x * r1 + eerie->frames[k].quat.x * r2;
				eerie->frames[i].quat.y = eerie->frames[j].quat.y * r1 + eerie->frames[k].quat.y * r2;
				eerie->frames[i].quat.z = eerie->frames[j].quat.z * r1 + eerie->frames[k].quat.z * r2;
			}
		}
	}
	
	for(long i = 0; i < th.nb_key_frames; i++) {
		eerie->frames[i].f_translate = true;
		eerie->frames[i].f_rotate = true;
	}
	
	// Sets Flag for voidgroups (unmodified groups for whole animation)
	for(long i = 0; i < eerie->nb_groups; i++) {
		
		bool voidd = true;
		for(long j = 0; j < eerie->nb_key_frames; j++) {
			long pos = i + (j * eerie->nb_groups);
			
			if((eerie->groups[pos].quat.x != 0.f)
			   || (eerie->groups[pos].quat.y != 0.f)
			   || (eerie->groups[pos].quat.z != 0.f)
			   || (eerie->groups[pos].quat.w != 1.f)
			   || (eerie->groups[pos].translate.x != 0.f)
			   || (eerie->groups[pos].translate.y != 0.f)
			   || (eerie->groups[pos].translate.z != 0.f)
			   || (eerie->groups[pos].zoom.x != 0.f)
			   || (eerie->groups[pos].zoom.y != 0.f)
			   || (eerie->groups[pos].zoom.z != 0.f)) {
				voidd = false;
				break;
			}
		}
		
		if(voidd) {
			eerie->voidgroups[i] = 1;
		}
	}
	
	eerie->anim_time = (float)th.nb_frames * 1000.f * ( 1.0f / 24 );
	if(eerie->anim_time < 1) {
		eerie->anim_time = 1;
	}
	
	LogDebug << "Finished Conversion TEA -> EERIE - " << (eerie->anim_time / 1000) << " seconds";
	
	return eerie;
}

void MakeUserFlag(TextureContainer * tc) {
	
	if(tc == NULL) {
		return;
	}
	
	if(NC_IsIn(tc->m_texName, "NPC_")) {
		tc->userflags |= POLY_LATE_MIP;
	}
	
	if(NC_IsIn(tc->m_texName, "nocol")) {
		tc->userflags |= POLY_NOCOL;
	}
	
	if(NC_IsIn(tc->m_texName, "climb")) {
		tc->userflags |= POLY_CLIMB;
	}
	
	if(NC_IsIn(tc->m_texName, "fall")) {
		tc->userflags |= POLY_FALL;
	}
	
	if(NC_IsIn(tc->m_texName, "lava")) {
		tc->userflags |= POLY_LAVA;
	}
	
	if (NC_IsIn(tc->m_texName, "water") || NC_IsIn(tc->m_texName, "spider_web")) {
		tc->userflags |= POLY_WATER;
		tc->userflags |= POLY_TRANS;
	} else if(NC_IsIn(tc->m_texName, "[metal]")) {
		tc->userflags |= POLY_METAL;
	}
	
}

#ifdef BUILD_EDIT_LOADSAVE

static void ReCreateUVs(EERIE_3DOBJ * eerie) {
	
	if(eerie->texturecontainer.empty()) return;

	float sxx, syy;

	for (size_t i = 0; i < eerie->facelist.size(); i++)
	{
		if (eerie->facelist[i].texid == -1) continue;

		if (eerie->texturecontainer[eerie->facelist[i].texid])
		{
			sxx = eerie->texturecontainer[eerie->facelist[i].texid]->m_odx;
			syy = eerie->texturecontainer[eerie->facelist[i].texid]->m_ody;
		}
		else
		{
			sxx = ( 1.0f / 256 );
			syy = ( 1.0f / 256 );
		}

		eerie->facelist[i].u[0] = (float)eerie->facelist[i].ou[0] * sxx; 
		eerie->facelist[i].u[1] = (float)eerie->facelist[i].ou[1] * sxx; 
		eerie->facelist[i].u[2] = (float)eerie->facelist[i].ou[2] * sxx; 
		eerie->facelist[i].v[0] = (float)eerie->facelist[i].ov[0] * syy; 
		eerie->facelist[i].v[1] = (float)eerie->facelist[i].ov[1] * syy; 
		eerie->facelist[i].v[2] = (float)eerie->facelist[i].ov[2] * syy; 
	}
}

static void _THEObjLoad(EERIE_3DOBJ * eerie, const unsigned char * adr, size_t * poss, long version) {
	
	LogWarning << "_THEObjLoad";
	
	size_t pos = *poss;
	
	THEO_OFFSETS to;
	memcpy(&to, adr + pos, sizeof(THEO_OFFSETS));
	pos += sizeof(THEO_OFFSETS);
	
	THEO_NB tn;
	memcpy(&tn, adr + pos, sizeof(THEO_NB));
	pos += sizeof(THEO_NB);
	
	LogDebug << "Nb Vertex " << tn.nb_vertex << " Nb Action Points " << tn.nb_action_point
	         << " Nb Lines " << tn.nb_lines;
	LogDebug << "Nb Faces " << tn.nb_faces << " Nb Groups " << tn.nb_groups;
	
	eerie->vertexlist.resize(tn.nb_vertex);
	eerie->facelist.resize(tn.nb_faces);
	eerie->nbgroups = tn.nb_groups;
	eerie->actionlist.resize(tn.nb_action_point);
	
	eerie->ndata = NULL;
	eerie->pdata = NULL;
	eerie->cdata = NULL;
	eerie->sdata = NULL;
	
	eerie->vertexlist3.resize(tn.nb_vertex);
	
	if(tn.nb_groups == 0) {
		eerie->grouplist = NULL;
	} else {
		eerie->grouplist = new EERIE_GROUPLIST[tn.nb_groups]; 
	}
	
	// read vertices
	
	pos = to.vertex_seek;
	
	if(tn.nb_vertex > 65535) {
		LogError << ("Warning Vertex Number Too High...");
	}
	
	for(long i = 0; i < tn.nb_vertex; i++) {
		THEO_VERTEX ptv;
		memcpy(&ptv, adr + pos, sizeof(THEO_VERTEX));
		pos += sizeof(THEO_VERTEX);
		eerie->vertexlist[i].v = ptv.pos;
		eerie->cub.xmin = min(eerie->cub.xmin, ptv.pos.x);
		eerie->cub.xmax = max(eerie->cub.xmax, ptv.pos.x);
		eerie->cub.ymin = min(eerie->cub.ymin, ptv.pos.y);
		eerie->cub.ymax = max(eerie->cub.ymax, ptv.pos.y);
		eerie->cub.zmin = min(eerie->cub.zmin, ptv.pos.z);
		eerie->cub.zmax = max(eerie->cub.zmax, ptv.pos.z);
	}

	// Lecture des FACES THEO
	pos = to.faces_seek;

	for(long i = 0; i < tn.nb_faces; i++) {
		
		THEO_FACES_3006 ptf3006;
		if(version >= 3006) {
			memcpy(&ptf3006, adr + pos, sizeof(THEO_FACES_3006));
			pos += sizeof(THEO_FACES_3006);
		} else {
			memset(&ptf3006, 0, sizeof(THEO_FACES_3006));
			THEO_FACES ptf;
			memcpy(&ptf, adr + pos, sizeof(THEO_FACES));
			pos += sizeof(THEO_FACES);
			ptf3006.color = ptf.color;
			ptf3006.index1 = ptf.index1;
			ptf3006.index2 = ptf.index2;
			ptf3006.index3 = ptf.index3;
			ptf3006.ismap = ptf.ismap;
			ptf3006.liste_uv = ptf.liste_uv;
			ptf3006.element_uv = ptf.element_uv;
			ptf3006.num_map = ptf.num_map;
			ptf3006.tile_x = ptf.tile_x;
			ptf3006.tile_y = ptf.tile_y;
			ptf3006.user_tile_x = ptf.user_tile_x;
			ptf3006.user_tile_y = ptf.user_tile_y;
			ptf3006.flag = ptf.flag;
			ptf3006.collision_type = ptf.collision_type;
			ptf3006.rgb = ptf.rgb;
			ptf3006.rgb1 = ptf.rgb1;
			ptf3006.rgb2 = ptf.rgb2;
			ptf3006.rgb3 = ptf.rgb3;
			ptf3006.double_side = ptf.double_side;
			ptf3006.transparency = ptf.transparency;
			ptf3006.trans = ptf.trans;
		}
		
		eerie->facelist[i].vid[0] = (unsigned short)ptf3006.index1;
		eerie->facelist[i].vid[1] = (unsigned short)ptf3006.index2;
		eerie->facelist[i].vid[2] = (unsigned short)ptf3006.index3;
		
		if((size_t)ptf3006.num_map >= eerie->texturecontainer.size()) {
			ptf3006.num_map = -1;
		}
		
		if(ptf3006.ismap) {
			eerie->facelist[i].texid = (short)ptf3006.num_map;
			eerie->facelist[i].facetype = 1;
			
			if(ptf3006.num_map >= 0 && eerie->texturecontainer[ptf3006.num_map] && (eerie->texturecontainer[ptf3006.num_map]->userflags & POLY_NOCOL)) {
				eerie->facelist[i].facetype |= POLY_NOCOL;
			}
		} else if(ptf3006.rgb) {
			eerie->facelist[i].texid = -1;
		} else {
			eerie->facelist[i].texid = -1;
		}
		
		switch(ptf3006.flag) {
			case 0:
				eerie->facelist[i].facetype |= POLY_GLOW;
				break;
			case 1:
				eerie->facelist[i].facetype |= POLY_NO_SHADOW;
				break;
			case 4:
				eerie->facelist[i].facetype |= POLY_METAL;
				break;
			case 10:
				eerie->facelist[i].facetype |= POLY_NOPATH;
				break;
			case 11:
				eerie->facelist[i].facetype |= POLY_CLIMB;
				break;
			case 12:
				eerie->facelist[i].facetype |= POLY_NOCOL;
				break;
			case 13:
				eerie->facelist[i].facetype |= POLY_NODRAW;
				break;
			case 14:
				eerie->facelist[i].facetype |= POLY_PRECISE_PATH;
				break;
			case 16:
				eerie->facelist[i].facetype |= POLY_NO_CLIMB;
				break;
		}
		
		eerie->facelist[i].ou[0] = (short)ptf3006.liste_uv.u1;
		eerie->facelist[i].ov[0] = (short)ptf3006.liste_uv.v1;
		eerie->facelist[i].ou[1] = (short)ptf3006.liste_uv.u2;
		eerie->facelist[i].ov[1] = (short)ptf3006.liste_uv.v2;
		eerie->facelist[i].ou[2] = (short)ptf3006.liste_uv.u3;
		eerie->facelist[i].ov[2] = (short)ptf3006.liste_uv.v3;
		
		if(ptf3006.double_side) {
			eerie->facelist[i].facetype |= POLY_DOUBLESIDED;
		}
		
		if(ptf3006.transparency > 0) {
			if(ptf3006.transparency == 2) {
				// NORMAL TRANS 0.00001 to 0.999999
				if(ptf3006.trans < 1.f) {
					eerie->facelist[i].facetype |= POLY_TRANS;
					eerie->facelist[i].transval = ptf3006.trans;
				}
			}
			else if (ptf3006.transparency == 1) {
				if(ptf3006.trans < 0.f) {
					// SUBTRACTIVE -0.000001 to -0.999999
					eerie->facelist[i].facetype |= POLY_TRANS;
					eerie->facelist[i].transval = ptf3006.trans;
				} else {
					// ADDITIVE 1.000001 to 1.9999999
					eerie->facelist[i].facetype |= POLY_TRANS;
					eerie->facelist[i].transval = ptf3006.trans + 1.f;
				}
			} else {
				// MULTIPLICATIVE 2.000001 to 2.9999999
				eerie->facelist[i].facetype |= POLY_TRANS;
				eerie->facelist[i].transval = ptf3006.trans + 2.f;
			}
		}
		
		if(eerie->facelist[i].texid != -1 && !eerie->texturecontainer.empty() && eerie->texturecontainer[eerie->facelist[i].texid] != NULL) {
			
			if(eerie->texturecontainer[eerie->facelist[i].texid]->userflags & POLY_TRANS) {
				if(!(eerie->facelist[i].facetype & POLY_TRANS)) {
					eerie->facelist[i].facetype |= POLY_TRANS;
					eerie->facelist[i].transval = ptf3006.trans;
				}
			}
			
			if(eerie->texturecontainer[eerie->facelist[i].texid]->userflags & POLY_WATER) {
				eerie->facelist[i].facetype |= POLY_WATER;
			}
			
			if(eerie->texturecontainer[eerie->facelist[i].texid]->userflags & POLY_LAVA) {
				eerie->facelist[i].facetype |= POLY_LAVA;
			}
			
			if(eerie->texturecontainer[eerie->facelist[i].texid]->userflags & POLY_FALL) {
				eerie->facelist[i].facetype |= POLY_FALL;
			}

			if(eerie->texturecontainer[eerie->facelist[i].texid]->userflags & POLY_CLIMB) {
				eerie->facelist[i].facetype |= POLY_CLIMB;
			}
		}
		
	}
	
	// Groups Data
	pos = to.groups_seek;
	
	for(long i = 0; i < tn.nb_groups; i++) {
		
		THEO_GROUPS_3011 ptg3011;
		if(version >= 3011) {
			memcpy(&ptg3011, adr + pos, sizeof(THEO_GROUPS_3011));
			pos += sizeof(THEO_GROUPS_3011);
		} else {
			THEO_GROUPS ltg;
			memcpy(&ltg, adr + pos, sizeof(THEO_GROUPS));
			pos += sizeof(THEO_GROUPS);
			memset(&ptg3011, 0, sizeof(THEO_GROUPS_3011));
			ptg3011.origin = ltg.origin;
			ptg3011.nb_index = ltg.nb_index;
		}
		
		eerie->grouplist[i].origin = ptg3011.origin;
		eerie->grouplist[i].indexes.resize(ptg3011.nb_index);
		
		std::copy((long*)(adr + pos), (long*)(adr + pos) + ptg3011.nb_index, eerie->grouplist[i].indexes.begin());
		pos += ptg3011.nb_index * sizeof(long);
		
		char groupname[256];
		memcpy(groupname, adr + pos, 256);
		pos += 256;
		
		eerie->grouplist[i].name = groupname;
		eerie->grouplist[i].siz = 0.f;
		
		for(long o = 0; o < ptg3011.nb_index; o++) {
			eerie->grouplist[i].siz = max(eerie->grouplist[i].siz,
			                              fdist(eerie->vertexlist[eerie->grouplist[i].origin].v,
			                                           eerie->vertexlist[eerie->grouplist[i].indexes[o]].v));
		}
		
		eerie->grouplist[i].siz = EEsqrt(eerie->grouplist[i].siz) * ( 1.0f / 16 );
		
	}

	// SELECTIONS
	s32	THEO_nb_selected;
	memcpy(&THEO_nb_selected, adr + pos, sizeof(s32));
	pos += sizeof(s32);
	
	eerie->selections.resize(THEO_nb_selected);
	for(long i = 0; i < THEO_nb_selected; i++) {
		
		THEO_SELECTED pts;
		memcpy(&pts, adr + pos, sizeof(THEO_SELECTED));
		pos += sizeof(THEO_SELECTED);
		
		if(strlen(pts.name) > 63) {
			pts.name[63] = 0;
		}
		
		eerie->selections[i].name = pts.name;
		eerie->selections[i].selected.resize(pts.nb_index);
		
		if(pts.nb_index > 0) {
			std::copy((long*)(adr + pos), (long*)(adr + pos) + pts.nb_index, eerie->selections[i].selected.begin());
			pos += sizeof(long) * pts.nb_index;
		}
	}
	
	// Theo Action Points Read
	pos = to.action_point_seek;

	for(long i = 0; i < tn.nb_action_point; i++) {
		
		THEO_ACTION_POINT ptap;
		memcpy(&ptap, adr + pos, sizeof(THEO_ACTION_POINT));
		pos += sizeof(THEO_ACTION_POINT);
		
		eerie->actionlist[i].act = ptap.action;
		eerie->actionlist[i].sfx = ptap.num_sfx;
		eerie->actionlist[i].idx = ptap.vert_index;
		eerie->actionlist[i].name = ptap.name;
		MakeUpcase(eerie->actionlist[i].name);
	}
	
	eerie->angle.g = eerie->angle.b = eerie->angle.a = 0.f;
	eerie->pos.z = eerie->pos.y = eerie->pos.x = 0.f;
	
	// Now Interpret Extra Data chunk
	pos = to.extras_seek + 4;
	
	if(version >= 3005) {
		
		THEO_EXTRA_DATA_3005 pted3005;
		memcpy(&pted3005, adr + pos, sizeof(THEO_EXTRA_DATA_3005));
		pos += sizeof(THEO_EXTRA_DATA_3005);
		
		eerie->pos = pted3005.pos;
		
		eerie->angle.a = (float)(pted3005.angle.alpha & 0xfff) * THEO_ROTCONVERT;
		eerie->angle.b = (float)(pted3005.angle.beta & 0xfff) * THEO_ROTCONVERT;
		eerie->angle.g = (float)(pted3005.angle.gamma & 0xfff) * THEO_ROTCONVERT;
		
		eerie->point0 = eerie->vertexlist[pted3005.origin_index].v;
		eerie->origin = pted3005.origin_index;
		
		eerie->quat = pted3005.quat;
	
	} else {
		
		THEO_EXTRA_DATA pted;
		memcpy(&pted, adr + pos, sizeof(THEO_EXTRA_DATA));
		pos += sizeof(THEO_EXTRA_DATA);
		
		eerie->pos = pted.pos;
		
		eerie->angle.a = (float)(pted.angle.alpha & 0xfff) * THEO_ROTCONVERT;
		eerie->angle.b = (float)(pted.angle.beta & 0xfff) * THEO_ROTCONVERT;
		eerie->angle.g = (float)(pted.angle.gamma & 0xfff) * THEO_ROTCONVERT;
		
		eerie->point0 = eerie->vertexlist[pted.origin_index].v;
		eerie->origin = pted.origin_index;
		
	}
	
	*poss = pos;
	
	eerie->vertexlist3 = eerie->vertexlist;
	ReCreateUVs(eerie);
	EERIE_Object_Precompute_Fast_Access(eerie);
}

static EERIE_3DSCENE * ScnToEerie(const unsigned char * adr, size_t size, const string & fic) {
	
	(void)size; // TODO use size
	
	LogDebug << "Loading Scene " << fic;
	
	if(!adr) {
		return NULL;
	}
	
	size_t pos = 0;
	
	EERIE_3DSCENE * seerie = allocStructZero<EERIE_3DSCENE>();
	Clear3DScene(seerie);
	
	TSCN_HEADER psth;
	memcpy(&psth, adr + pos, sizeof(TSCN_HEADER));
	pos += sizeof(TSCN_HEADER);
	
	LogDebug << "SCNtoEERIE " << fic << " Version " << psth.version << " Nb Textures " << psth.nb_maps;
	
	if(psth.version < 3008 || psth.version > 3024) {
		LogError << "ScnToEerie: invalid version in " << fic << ": found " << psth.version
		         << " expected 3008 to 3024";
		free(seerie);
		return NULL;
	}
	
	seerie->nbtex = psth.nb_maps;
	
	const string temp = "Graph\\Obj3D\\Textures\\";
	
	if(psth.type_write == 0) {
		
		seerie->texturecontainer = allocStructZero<TextureContainer *>(psth.nb_maps); 
		
		for(long i = 0; i < psth.nb_maps; i++) {
			
			THEO_TEXTURE tt;
			memcpy(&tt, adr + pos, sizeof(THEO_TEXTURE));
			pos += sizeof(THEO_TEXTURE);
			
			string mapsname = temp + tt.texture_name + ".bmp";
			seerie->texturecontainer[i] = TextureContainer::Load(mapsname, TextureContainer::Level);
		}
		
	} else {
		
		if((psth.type_write & SAVE_MAP_BMP) || (psth.type_write & SAVE_MAP_TGA)) {
			
			seerie->texturecontainer = allocStructZero<TextureContainer *>(psth.nb_maps);
			
			for(long i = 0; i < psth.nb_maps; i++) {
				
				string name;
				if(psth.version >= 3019) {
					THEO_SAVE_MAPS_IN_3019 tsmi3019;
					memcpy(&tsmi3019, adr + pos, sizeof(THEO_SAVE_MAPS_IN_3019));
					pos += sizeof(THEO_SAVE_MAPS_IN_3019);
					name = tsmi3019.texture_name;
				} else {
					THEO_SAVE_MAPS_IN tsmi;
					memcpy(&tsmi, adr + pos, sizeof(THEO_SAVE_MAPS_IN));
					pos += sizeof(THEO_SAVE_MAPS_IN);
					name = tsmi.texture_name;
				}
				
				if(!name.empty()) {
					
					string mapsname = temp + name;
					if(psth.type_write & SAVE_MAP_BMP) {
						mapsname += ".bmp";
					} else {
						mapsname += ".tga";
					}
					
					seerie->texturecontainer[i] = TextureContainer::Load(mapsname, TextureContainer::Level);
				}
			}
		}
	}
	
	// read objects
	pos = psth.object_seek;
	
	s32 nbo;
	memcpy(&nbo, adr + pos, sizeof(s32));
	pos += sizeof(s32);
	
	seerie->nbobj = nbo;
	seerie->objs = allocStructZero<EERIE_3DOBJ *>(nbo); 
	
	seerie->point0.x = -999999999999.f;
	seerie->point0.y = -999999999999.f;
	seerie->point0.z = -999999999999.f;
	
	long id = 0;
	
	for(long i = 0; i < nbo; i++) {
		// TODO is this even used?
		
		TSCN_OBJHEADER ptoh;
		memcpy(&ptoh, adr + pos, sizeof(TSCN_OBJHEADER));
		pos += sizeof(TSCN_OBJHEADER);
		
		seerie->objs[id] = new EERIE_3DOBJ(); 
		// TODO most is done in the constructor already
		seerie->objs[id]->clear();
		
		seerie->objs[id]->texturecontainer.resize(seerie->nbtex);
		std::copy(seerie->texturecontainer, seerie->texturecontainer + seerie->nbtex, seerie->objs[id]->texturecontainer.begin());
		
		long objVersion;
		if(psth.version < 3013) {
			objVersion = 3004;
		} else if(psth.version < 3015) {
			objVersion = 3005;
		} else if(psth.version < 3019) {
			objVersion = 3006;
		} else if(psth.version < 3023) {
			objVersion = 3008;
		} else {
			objVersion = 3011;
		}
		_THEObjLoad(seerie->objs[id], adr, &pos, objVersion);
		
		seerie->cub.xmin = min(seerie->cub.xmin, seerie->objs[id]->cub.xmin + seerie->objs[id]->pos.x);
		seerie->cub.xmax = max(seerie->cub.xmax, seerie->objs[id]->cub.xmax + seerie->objs[id]->pos.x);
		seerie->cub.ymin = min(seerie->cub.ymin, seerie->objs[id]->cub.ymin + seerie->objs[id]->pos.y);
		seerie->cub.ymax = max(seerie->cub.ymax, seerie->objs[id]->cub.ymax + seerie->objs[id]->pos.y);
		seerie->cub.zmin = min(seerie->cub.zmin, seerie->objs[id]->cub.zmin + seerie->objs[id]->pos.z);
		seerie->cub.zmax = max(seerie->cub.zmax, seerie->objs[id]->cub.zmax + seerie->objs[id]->pos.z);
		
		if(!strcmp(ptoh.object_name, "map_origin")) {
			seerie->point0.x = seerie->objs[id]->point0.x + seerie->objs[id]->pos.x;
			seerie->point0.y = seerie->objs[id]->point0.y + seerie->objs[id]->pos.y;
			seerie->point0.z = seerie->objs[id]->point0.z + seerie->objs[id]->pos.z;
			delete seerie->objs[id];
			seerie->nbobj--;
			id--;
		} else {
			seerie->objs[id]->name = ptoh.object_name;
		}
		
		id++;
		
		pos = ptoh.next_obj;
	}
	
	pos = psth.light_seek; // ambient
	memcpy(&seerie->ambient_r, adr + pos, sizeof(f32));
	pos += sizeof(f32);
	memcpy(&seerie->ambient_g, adr + pos, sizeof(f32));
	pos += sizeof(f32);
	memcpy(&seerie->ambient_b, adr + pos, sizeof(f32));
	pos += sizeof(f32);
	
	s32 nbl;
	memcpy(&nbl, adr + pos, sizeof(s32));
	pos += sizeof(s32);
	
	seerie->light = NULL; 
	seerie->nblight = nbl;
	
	for(long i = 0; i < nbl; i++) {
		
		TSCN_LIGHT_3024 tsl3024;
		
		if(psth.version >= 3024) {
			memcpy(&tsl3024, adr + pos, sizeof(TSCN_LIGHT_3024));
			pos += sizeof(TSCN_LIGHT_3024);
		} else if(psth.version >= 3019) {
			TSCN_LIGHT_3019 tsl3019;
			memcpy(&tsl3019, adr + pos, sizeof(TSCN_LIGHT_3019));
			pos += sizeof(TSCN_LIGHT_3019);
			memset(&tsl3024, 0, sizeof(TSCN_LIGHT_3024));
			tsl3024.red = tsl3019.red;
			tsl3024.green = tsl3019.green;
			tsl3024.blue = tsl3019.blue;
			tsl3024.pos = tsl3019.pos;
			tsl3024.hotspot = tsl3019.hotspot;
			tsl3024.falloff = tsl3019.falloff;
			tsl3024.intensity = tsl3019.intensity;
		} else {
			TSCN_LIGHT tsl;
			memcpy(&tsl, adr + pos, sizeof(TSCN_LIGHT));
			pos += sizeof(TSCN_LIGHT);
			memset(&tsl3024, 0, sizeof(TSCN_LIGHT_3024));
			tsl3024.red = tsl.red;
			tsl3024.green = tsl.green;
			tsl3024.blue = tsl.blue;
			tsl3024.pos = tsl.pos;
			tsl3024.hotspot = tsl.hotspot;
			tsl3024.falloff = tsl.falloff;
			tsl3024.intensity = tsl.intensity;
		}
		
		EERIE_LIGHT light;
		
		light.rgb.r = (float)tsl3024.red * ( 1.0f / 255 );
		light.rgb.g = (float)tsl3024.green * ( 1.0f / 255 );
		light.rgb.b = (float)tsl3024.blue * ( 1.0f / 255 );
		light.pos = tsl3024.pos;
		light.fallstart = (float)tsl3024.hotspot;
		light.fallend = (float)tsl3024.falloff;
		
		float t = light.fallend - light.fallstart;
		if(t < 150.f) {
			light.fallend += 150.f - t;
		}
		
		light.intensity = (float)tsl3024.intensity;
		light.exist = 1;
		light.treat = 1;
		light.selected = 0;
		light.type = 0;
		EERIE_LIGHT_GlobalAdd(&light);
	}
	
	return seerie;
}

static void ReleaseScene(EERIE_3DSCENE * scene) {
	
	if(scene->texturecontainer != NULL) {
		free(scene->texturecontainer);
		scene->texturecontainer = NULL;
	}
	
	for(long i = 0; i < scene->nbobj; i++) {
		delete scene->objs[i];
	}
	
	if(scene->objs != NULL) {
		free(scene->objs);
		scene->objs = NULL;
	}
	
	if(scene->texturecontainer != NULL) {
		free(scene->texturecontainer);
		scene->texturecontainer = NULL;
	}
	
	if(scene->light) {
		for(long i = 0; i < scene->nblight; i++) {
			if(scene->light[i] != NULL) {
				free(scene->light[i]);
				scene->light[i] = NULL;
			}
		}
		
		free(scene->light);
		scene->light = NULL;
	}
	
	free(scene);
}

void ReleaseMultiScene(EERIE_MULTI3DSCENE * ms) {
	
	if(ms) {
		for(long i = 0; i < ms->nb_scenes; i++) {
			ReleaseScene(ms->scenes[i]);
			ms->scenes[i] = NULL;
		}
	}
	
	free(ms);
}

static EERIE_MULTI3DSCENE * MultiSceneToEerie(const string & dirr) {
	
	EERIE_MULTI3DSCENE * es;
	char pathh[512];

	es = allocStructZero<EERIE_MULTI3DSCENE>();

	strcpy(LastLoadedScene, dirr.c_str());
	sprintf(pathh, "%s*.scn", dirr.c_str());

	LogWarning << "partially unimplemented MultiSceneToEerie";
//	TODO: finddata
//	long idx;
//	struct _finddata_t fd;
//	if ((idx = _findfirst(pathh, &fd)) != -1)
//	{
//		do
//		{
//			if (!(fd.attrib & _A_SUBDIR))
//			{
//				char * tex = GetExt(fd.name);
//
//				if (!strcasecmp(tex, ".SCN"))
//				{
//					char path[512];
//					sprintf(path, "%s%s", dirr, fd.name);
//					size_t SizeAlloc = 0;
//
//					unsigned char * adr;
//					if (adr = (unsigned char *)PAK_FileLoadMalloc(path, &SizeAlloc))
//					{
//						es->scenes[es->nb_scenes] = (EERIE_3DSCENE *)ScnToEerie(adr, SizeAlloc, path);
//						es->nb_scenes++;
//						free(adr);
//					}
//				}
//			}
//		}
//		while (!(_findnext(idx, &fd)));
//
//		_findclose(idx);
//	}

	es->cub.xmax = -9999999999.f;
	es->cub.xmin = 9999999999.f;
	es->cub.ymax = -9999999999.f;
	es->cub.ymin = 9999999999.f;
	es->cub.zmax = -9999999999.f;
	es->cub.zmin = 9999999999.f;

	for (long i = 0; i < es->nb_scenes; i++)
	{
		es->cub.xmax = max(es->cub.xmax, es->scenes[i]->cub.xmax);
		es->cub.xmin = min(es->cub.xmin, es->scenes[i]->cub.xmin);
		es->cub.ymax = max(es->cub.ymax, es->scenes[i]->cub.ymax);
		es->cub.ymin = min(es->cub.ymin, es->scenes[i]->cub.ymin);
		es->cub.zmax = max(es->cub.zmax, es->scenes[i]->cub.zmax);
		es->cub.zmin = min(es->cub.zmin, es->scenes[i]->cub.zmin);
		es->pos.x = es->scenes[i]->pos.x;
		es->pos.y = es->scenes[i]->pos.y;
		es->pos.z = es->scenes[i]->pos.z;

		if ((es->scenes[i]->point0.x != -999999999999.f) &&
		        (es->scenes[i]->point0.y != -999999999999.f) &&
		        (es->scenes[i]->point0.z != -999999999999.f))
		{
			es->point0.x = es->scenes[i]->point0.x;
			es->point0.y = es->scenes[i]->point0.y;
			es->point0.z = es->scenes[i]->point0.z;
		}
	}

	if (es->nb_scenes == 0)
	{
		free(es);
		return NULL;
	}

	return es;
}

static EERIE_MULTI3DSCENE * _PAK_MultiSceneToEerie(const string & dirr) {
	
	EERIE_MULTI3DSCENE * es;
	
	es = allocStructZero<EERIE_MULTI3DSCENE>();

	strcpy(LastLoadedScene, dirr.c_str());

	string path = dirr;
	RemoveName(path);

	vector<PakDirectory *> directories;
	pPakManager->GetDirectories(path, directories);

	vector<PakDirectory *>::iterator it;
	for(it = directories.begin(); it < directories.end(); ++it) {
		int nb = (*it)->nbfiles;
		PakFile * et;
		et = (*it)->files;
		
		while(nb--) {
			if(!strcasecmp(GetExt( et->name), ".scn")) {
				
				size_t SizeAlloc;
				unsigned char * adr = (unsigned char*)PAK_FileLoadMalloc(dirr + et->name, SizeAlloc);
				if(adr) {
					es->scenes[es->nb_scenes] = ScnToEerie(adr, SizeAlloc, path);
					es->nb_scenes++;
					free(adr);
				}
			}
			
			et = et->next;
		}
	}
	
	es->cub.xmax = -9999999999.f;
	es->cub.xmin = 9999999999.f;
	es->cub.ymax = -9999999999.f;
	es->cub.ymin = 9999999999.f;
	es->cub.zmax = -9999999999.f;
	es->cub.zmin = 9999999999.f;
	
	for(long i = 0; i < es->nb_scenes; i++) {
		es->cub.xmax = max(es->cub.xmax, es->scenes[i]->cub.xmax);
		es->cub.xmin = min(es->cub.xmin, es->scenes[i]->cub.xmin);
		es->cub.ymax = max(es->cub.ymax, es->scenes[i]->cub.ymax);
		es->cub.ymin = min(es->cub.ymin, es->scenes[i]->cub.ymin);
		es->cub.zmax = max(es->cub.zmax, es->scenes[i]->cub.zmax);
		es->cub.zmin = min(es->cub.zmin, es->scenes[i]->cub.zmin);
		es->pos.x = es->scenes[i]->pos.x;
		es->pos.y = es->scenes[i]->pos.y;
		es->pos.z = es->scenes[i]->pos.z;
		
		if((es->scenes[i]->point0.x != -999999999999.f) &&
		   (es->scenes[i]->point0.y != -999999999999.f) &&
		   (es->scenes[i]->point0.z != -999999999999.f)) {
			es->point0.x = es->scenes[i]->point0.x;
			es->point0.y = es->scenes[i]->point0.y;
			es->point0.z = es->scenes[i]->point0.z;
		}
	}
	
	if(es->nb_scenes == 0) {
		free(es);
		return NULL;
	}
	
	return es;
}

EERIE_MULTI3DSCENE * PAK_MultiSceneToEerie(const string & dirr) {
	
	LogDebug << "Loading Multiscene " << dirr;
	
	EERIE_MULTI3DSCENE * em = NULL;

// TODO create unified implementation for both pak and non-pak
// TODO is this even used?
	
	em = _PAK_MultiSceneToEerie(dirr);

	if(!em)
		em = MultiSceneToEerie(dirr);


	EERIEPOLY_Compute_PolyIn();
	return em;
}

#endif // BUILD_EDIT_LOADSAVE

//-----------------------------------------------------------------------------------------------------
// Warning Clear3DObj/Clear3DScene don't release Any pointer Just Clears Structures
void EERIE_3DOBJ::clear() {
	
		point0 = pos = Vec3f::ZERO;
		angle = Anglef::ZERO;

		origin = 0;
		ident = 0;
		nbpfaces = 0;
		nbgroups = 0;
		drawflags = 0;

		vertexlocal = NULL;
		vertexlist.clear();
		vertexlist3.clear();

		facelist.clear();
		pfacelist = NULL;
		grouplist = NULL;
		texturecontainer.clear();

		originaltextures = NULL;
		linked = NULL;

		// TODO Default constructor
		quat.x = quat.y = quat.z = quat.w = 0;
		nblinked = 0;

		pbox = 0;
		pdata = 0;
		ndata = 0;
		cdata = 0;
		sdata = 0;

		fastaccess.view_attach = 0;
		fastaccess.primary_attach = 0;
		fastaccess.left_attach = 0;
		fastaccess.weapon_attach = 0;
		fastaccess.secondary_attach = 0;
		fastaccess.mouth_group = 0;
		fastaccess.jaw_group = 0;
		fastaccess.head_group_origin = 0;
		fastaccess.head_group = 0;
		fastaccess.mouth_group_origin = 0;
		fastaccess.V_right = 0;
		fastaccess.U_right = 0;
		fastaccess.fire = 0;
		fastaccess.sel_head = 0;
		fastaccess.sel_chest = 0;
		fastaccess.sel_leggings = 0;
		fastaccess.carry_attach = 0;
		fastaccess.__padd = 0;

		c_data = 0;
		
	cub.xmin = cub.ymin = cub.zmin = EEdef_MAXfloat;
	cub.xmax = cub.ymax = cub.zmax = EEdef_MINfloat;
}

void Clear3DScene(EERIE_3DSCENE * eerie) {
	
	if(eerie == NULL) {
		return;
	}
	
	memset(eerie, 0, sizeof(EERIE_3DSCENE));
	eerie->cub.xmin = eerie->cub.ymin = eerie->cub.zmin = EEdef_MAXfloat;
	eerie->cub.xmax = eerie->cub.ymax = eerie->cub.zmax = EEdef_MINfloat;
}

// TODO move to destructor?
EERIE_3DOBJ::~EERIE_3DOBJ() {
	
	if(originaltextures) {
		free(originaltextures);
		originaltextures = NULL;
	}
	
	if(ndata) {
		KillNeighbours(this);
	}
	
	if(pdata) {
		KillProgressiveData(this);
	}
	
	if(cdata) {
		KillClothesData(this);
	}
	
	EERIEOBJECT_DeletePFaces(this);
	EERIE_RemoveCedricData(this);
	EERIE_PHYSICS_BOX_Release(this);
	EERIE_COLLISION_SPHERES_Release(this);
	
	if(grouplist) {
		delete[] grouplist;
		grouplist = NULL;
	}
	
	// TODO why check for nblinked?
	if(nblinked && linked) {
		free(linked);
		linked = NULL;
	}
}

EERIE_3DOBJ * Eerie_Copy(const EERIE_3DOBJ * obj) {
	
	EERIE_3DOBJ * nouvo = new EERIE_3DOBJ(); 
	
	nouvo->vertexlist = obj->vertexlist;
	
	nouvo->vertexlist3 = obj->vertexlist3;
	
	nouvo->linked = NULL;
	nouvo->ndata = NULL;
	nouvo->pbox = NULL;
	nouvo->pdata = NULL;
	nouvo->cdata = NULL;
	nouvo->sdata = NULL;
	nouvo->c_data = NULL;
	nouvo->vertexlocal = NULL;
	
	nouvo->angle = obj->angle;
	nouvo->pos = obj->pos;
	nouvo->cub.xmax = obj->cub.xmax;
	nouvo->cub.xmin = obj->cub.xmin;
	nouvo->cub.ymax = obj->cub.ymax;
	nouvo->cub.ymin = obj->cub.ymin;
	nouvo->cub.zmax = obj->cub.zmax;
	nouvo->cub.zmin = obj->cub.zmin;
	nouvo->drawflags = obj->drawflags;
	
	if ( !obj->file.empty() )
		nouvo->file = obj->file;
	
	nouvo->ident = obj->ident;

	if ( !obj->name.empty() )
		nouvo->name = obj->name;

	nouvo->origin = obj->origin;
	nouvo->point0 = obj->point0;
	Quat_Copy(&nouvo->quat, &obj->quat);


	if (obj->ndata)
	{
		nouvo->ndata = copyStruct(obj->ndata, obj->vertexlist.size());
	}
	else nouvo->ndata = NULL;

	nouvo->facelist = obj->facelist;

	if (obj->nbgroups) {
		nouvo->nbgroups = obj->nbgroups;
		nouvo->grouplist = new EERIE_GROUPLIST[obj->nbgroups];
		std::copy(obj->grouplist, obj->grouplist + obj->nbgroups, nouvo->grouplist);
	}

	nouvo->actionlist = obj->actionlist;

	nouvo->selections = obj->selections;

	nouvo->texturecontainer = obj->texturecontainer;
	
	memcpy(&nouvo->fastaccess, &obj->fastaccess, sizeof(EERIE_FASTACCESS));
	EERIE_CreateCedricData(nouvo);
	EERIEOBJECT_CreatePFaces(nouvo);

	if (obj->pbox)
	{
		nouvo->pbox = allocStructZero<PHYSICS_BOX_DATA>();
		nouvo->pbox->nb_physvert = obj->pbox->nb_physvert;
		nouvo->pbox->stopcount = 0;
		nouvo->pbox->radius = obj->pbox->radius;
		
		nouvo->pbox->vert = copyStruct(obj->pbox->vert, obj->pbox->nb_physvert);
	}

	nouvo->linked = NULL;
	nouvo->nblinked = 0;
	nouvo->originaltextures = NULL;
	return nouvo;
}

long EERIE_OBJECT_GetSelection(const EERIE_3DOBJ * obj, const string & selname) {
	
	if(!obj) {
		return -1;
	}
	
	for(size_t i = 0; i < obj->selections.size(); i++) {
		if(!strcasecmp(obj->selections[i].name, selname)) {
			return i;
		}
	}
	
	return -1;
}

long EERIE_OBJECT_GetGroup(const EERIE_3DOBJ * obj, const string & groupname) {
	
	if(!obj) {
		return -1;
	}
	
	for(long i = 0; i < obj->nbgroups; i++) {
		if(!strcasecmp(obj->grouplist[i].name, groupname)) {
			return i;
		}
	}
	
	return -1;
}

void AddIdxToBone(EERIE_BONE * bone, long idx)
{
	bone->idxvertices = (long *)realloc(bone->idxvertices, sizeof(long) * (bone->nb_idxvertices + 1));

	if (bone->idxvertices)
	{
		bone->idxvertices[bone->nb_idxvertices] = idx;
		bone->nb_idxvertices++;
	}
}
//-----------------------------------------------------------------------------------------------------
long GetFather(EERIE_3DOBJ * eobj, long origin, long startgroup)
{
	for (long i = startgroup; i >= 0; i--)
	{
		for (size_t j = 0; j < eobj->grouplist[i].indexes.size(); j++)
		{
			if (eobj->grouplist[i].indexes[j] == origin)
			{
				return i;
			}
		}
	}

	return -1;
}
//-----------------------------------------------------------------------------------------------------
void EERIE_RemoveCedricData(EERIE_3DOBJ * eobj)
{
	if (!eobj) return;

	if (!eobj->c_data) return;

	for (long i = 0; i < eobj->c_data->nb_bones; i++)
	{
		if (eobj->c_data->bones[i].idxvertices)
			free(eobj->c_data->bones[i].idxvertices);

		eobj->c_data->bones[i].idxvertices = NULL;
	}

	if (eobj->c_data->bones) delete[] eobj->c_data->bones;

	eobj->c_data->bones = NULL;
	delete eobj->c_data;
	eobj->c_data = NULL;

	if (eobj->vertexlocal) delete[] eobj->vertexlocal;

	eobj->vertexlocal = NULL;
}
//-----------------------------------------------------------------------------------------------------
void EERIE_CreateCedricData(EERIE_3DOBJ * eobj)
{
	eobj->c_data = new EERIE_C_DATA();
	memset(eobj->c_data, 0, sizeof(EERIE_C_DATA));

	if (eobj->nbgroups <= 0) // If no groups were specified
	{
		// Make one bone
		eobj->c_data->nb_bones = 1;
		eobj->c_data->bones = new EERIE_BONE[eobj->c_data->nb_bones];
		memset(eobj->c_data->bones, 0, sizeof(EERIE_BONE)*eobj->c_data->nb_bones);

		// Add all vertices to the bone
		for (size_t i = 0; i < eobj->vertexlist.size(); i++)
			AddIdxToBone(&eobj->c_data->bones[0], i);

		// Initialize the bone
		Quat_Init(&eobj->c_data->bones[0].quatinit);
		Quat_Init(&eobj->c_data->bones[0].quatanim);
		eobj->c_data->bones[0].scaleinit = Vec3f::ZERO;
		eobj->c_data->bones[0].scaleanim = Vec3f::ZERO;
		eobj->c_data->bones[0].transinit = Vec3f::ZERO;
		eobj->c_data->bones[0].transinit_global = eobj->c_data->bones[0].transinit;
		eobj->c_data->bones[0].original_group = NULL;
		eobj->c_data->bones[0].father = -1;
	}
	else // Groups were specified
	{
		// Alloc the bones
		eobj->c_data->nb_bones = eobj->nbgroups;
		eobj->c_data->bones = new EERIE_BONE[eobj->c_data->nb_bones];
		// TODO memset -> use constructor instead
		memset(eobj->c_data->bones, 0, sizeof(EERIE_BONE)*eobj->c_data->nb_bones);

		bool * temp = new bool[eobj->vertexlist.size()];
		memset(temp, 0, eobj->vertexlist.size());

		for (long i = eobj->nbgroups - 1; i >= 0; i--)
		{
			EERIE_VERTEX * v_origin = &eobj->vertexlist[eobj->grouplist[i].origin];

			for (size_t j = 0; j < eobj->grouplist[i].indexes.size(); j++)
			{
				if (!temp[eobj->grouplist[i].indexes[j]])
				{
					temp[eobj->grouplist[i].indexes[j]] = true;
					AddIdxToBone(&eobj->c_data->bones[i], eobj->grouplist[i].indexes[j]);
				}
			}

			Quat_Init(&eobj->c_data->bones[i].quatinit);
			Quat_Init(&eobj->c_data->bones[i].quatanim);
			eobj->c_data->bones[i].scaleinit = Vec3f::ZERO;
			eobj->c_data->bones[i].scaleanim = Vec3f::ZERO;
			eobj->c_data->bones[i].transinit = Vec3f(v_origin->v.x, v_origin->v.y, v_origin->v.z);
			eobj->c_data->bones[i].transinit_global = eobj->c_data->bones[i].transinit;
			eobj->c_data->bones[i].original_group = &eobj->grouplist[i];
			eobj->c_data->bones[i].father = GetFather(eobj, eobj->grouplist[i].origin, i - 1);
		}

		delete[] temp;

		// Try to correct lonely vertex
		for (size_t i = 0; i < eobj->vertexlist.size(); i++)
		{
			long ok = 0;

			for (long j = 0; j < eobj->nbgroups; j++)
			{
				for (size_t k = 0; k < eobj->grouplist[j].indexes.size(); k++)
				{
					if ((size_t)eobj->grouplist[j].indexes[k] == i)
					{
						ok = 1;
						break;
					}
				}

				if (ok)
					break;
			}

			if (!ok)
			{
				AddIdxToBone(&eobj->c_data->bones[0], i);
			}
		}

		for (long i = eobj->nbgroups - 1; i >= 0; i--)
		{
			if (eobj->c_data->bones[i].father >= 0)
			{
				eobj->c_data->bones[i].transinit.x -= eobj->c_data->bones[eobj->c_data->bones[i].father].transinit.x;
				eobj->c_data->bones[i].transinit.y -= eobj->c_data->bones[eobj->c_data->bones[i].father].transinit.y;
				eobj->c_data->bones[i].transinit.z -= eobj->c_data->bones[eobj->c_data->bones[i].father].transinit.z;
			}

			eobj->c_data->bones[i].transinit_global = eobj->c_data->bones[i].transinit;
		}

	}

	/* Build proper mesh */
	{
		EERIE_C_DATA* obj = eobj->c_data;


		for (long i = 0; i != obj->nb_bones; i++)
		{
			EERIE_QUAT	qt1;

			if (obj->bones[i].father >= 0)
			{
				/* Rotation*/
				Quat_Copy(&qt1, &obj->bones[i].quatinit);
				Quat_Multiply(&obj->bones[i].quatanim, &obj->bones[obj->bones[i].father].quatanim, &qt1);
				/* Translation */
				TransformVertexQuat(&obj->bones[obj->bones[i].father].quatanim, &obj->bones[i].transinit, &obj->bones[i].transanim);
				obj->bones[i].transanim = obj->bones[obj->bones[i].father].transanim + obj->bones[i].transanim;
			}
			else
			{
				/* Rotation*/
				Quat_Copy(&obj->bones[i].quatanim, &obj->bones[i].quatinit);
				/* Translation */
				obj->bones[i].transanim = obj->bones[i].transinit;
			}
			obj->bones[i].scaleanim = Vec3f(1.0f, 1.0f, 1.0f);
		}

		eobj->vertexlocal = new EERIE_3DPAD[eobj->vertexlist.size()];
		// TODO constructor is better than memset
		memset(eobj->vertexlocal, 0, sizeof(EERIE_3DPAD)*eobj->vertexlist.size());

		for (long i = 0; i != obj->nb_bones; i++) {
			Vec3f vector = obj->bones[i].transanim;
			
			for (int v = 0; v != obj->bones[i].nb_idxvertices; v++) {
				
				long idx = obj->bones[i].idxvertices[v];
				const EERIE_VERTEX & inVert = eobj->vertexlist[idx];
				EERIE_3DPAD & outVert = eobj->vertexlocal[idx];
				
				Vec3f temp = inVert.v - vector;
				TransformInverseVertexQuat(&obj->bones[i].quatanim, &temp, &temp);
				outVert.x = temp.x, outVert.y = temp.y, outVert.z = temp.z;
			}
		}
	}
}

void EERIEOBJECT_DeletePFaces(EERIE_3DOBJ * eobj)
{
	// todo Why does this return? Nonfunctional?
	return;

	if (eobj->pfacelist) free(eobj->pfacelist);

	eobj->pfacelist = NULL;
	eobj->nbpfaces = 0;
}

bool Is_Svert(EERIE_PFACE * epf, long epi, EERIE_FACE * ef, long ei)
{
	if ((epf->vid[epi] == ef->vid[ei])
	        &&	(epf->u[epi] == ef->u[ei])
	        &&	(epf->v[epi] == ef->v[ei])) 
			return true;

	return false;
}

long Strippable(EERIE_PFACE * epf, EERIE_FACE * ef)
{
	if ((Is_Svert(epf, epf->nbvert - 1, ef, 0))
	        &&	(Is_Svert(epf, epf->nbvert - 2, ef, 1))) return 2;

	if ((Is_Svert(epf, epf->nbvert - 1, ef, 1))
	        &&	(Is_Svert(epf, epf->nbvert - 2, ef, 2))) return 0;

	if ((Is_Svert(epf, epf->nbvert - 1, ef, 2))
	        &&	(Is_Svert(epf, epf->nbvert - 2, ef, 0))) return 1;

	return -1;
}

static bool EERIEOBJECT_AddFaceToPFace(EERIE_3DOBJ * eobj, EERIE_FACE * face) {
	
	for (long i = 0; i < eobj->nbpfaces; i++)
	{
		// TODO pfacelist is never really used
		EERIE_PFACE * epf = &eobj->pfacelist[i];

		if (epf->nbvert >= MAX_PFACE) continue;

		if (epf->facetype != face->facetype) continue;

		if (face->facetype & POLY_TRANS) continue;

		long r;

		if ((r = Strippable(epf, face)) >= 0)
		{
			epf->color[epf->nbvert] = face->color[r];
			epf->u[epf->nbvert] = face->u[r];
			epf->v[epf->nbvert] = face->v[r];
			epf->vid[epf->nbvert] = face->vid[r];
			epf->nbvert++;
			return true;
		}
	}

	return false;
}

void EERIEOBJECT_AddFace(EERIE_3DOBJ * eobj, EERIE_FACE * face, long faceidx)
{
	if (EERIEOBJECT_AddFaceToPFace(eobj, face)) return;

	eobj->pfacelist = (EERIE_PFACE *)realloc(eobj->pfacelist, sizeof(EERIE_PFACE) * (eobj->nbpfaces + 1));
	EERIE_PFACE * epf = &eobj->pfacelist[eobj->nbpfaces];
	epf->facetype = face->facetype;
	epf->nbvert = 3;
	epf->texid = face->texid;
	epf->transval = face->transval;

	ARX_CHECK_SHORT(faceidx);
	short sfaceIdx = static_cast<short>(faceidx);

	for (long i = 0; i < 3; i++)
	{
		epf->faceidx[i] = sfaceIdx;
		epf->color[i] = face->color[i];
		epf->u[i] = face->u[i];
		epf->v[i] = face->v[i];
		epf->vid[i] = face->vid[i];
	}

	epf->faceidx[0] = 0;
	eobj->nbpfaces++;
}

void EERIEOBJECT_CreatePFaces(EERIE_3DOBJ * eobj)
{
	// todo Find out why this function doesn't work
	return;
	EERIEOBJECT_DeletePFaces(eobj);

	for (size_t i = 0; i < eobj->facelist.size(); i++)
		EERIEOBJECT_AddFace(eobj, &eobj->facelist[i], i);
}

#ifdef BUILD_EDIT_LOADSAVE

// Converts a Theo Object to an EERIE object
static EERIE_3DOBJ * TheoToEerie(unsigned char * adr, long size, const string & texpath, const string & fic) {
	
	LogWarning << "TheoToEerie " << fic;
	
	if(!adr) {
		return NULL;
	}
	
	string txpath;
	if(texpath.empty()) {
		txpath = "Graph\\Obj3D\\Textures\\";
	} else {
		txpath = texpath;
	}
	
	if(size < 10) {
		return NULL;
	}
	
	size_t pos = 0;
	
	THEO_HEADER pth;
	memcpy(&pth, adr + pos, sizeof(THEO_HEADER));
	pos += sizeof(THEO_HEADER);
	
	if (pth.version < 3003 || pth.version > 3011) {
		LogError << "TheoToEerie: invalid version in " << fic << ": found " << pth.version
		         << " expected 3004 to 3011";
		return NULL;
	}
	
	EERIE_3DOBJ * eerie = new EERIE_3DOBJ;
	eerie->clear();
	
	eerie->file = fic;
	
	if(pth.type_write == 0) {
		// read the texture
		
		LogError <<  "WARNING object " << fic << " SAVE MAP IN OBJECT = INVALID... Using Dummy Textures...";
		
		eerie->texturecontainer.resize(pth.nb_maps);
		for(long i = 0; i < pth.nb_maps; i++) {
			THEO_TEXTURE tt;
			memcpy(&tt, adr + pos, sizeof(THEO_TEXTURE));
			pos += sizeof(THEO_TEXTURE);
			eerie->texturecontainer[i] = GetAnyTexture();
		}
		
	} else {
		
		if((pth.type_write & SAVE_MAP_BMP) || (pth.type_write & SAVE_MAP_TGA)) {
			
			eerie->texturecontainer.resize(pth.nb_maps);
			for(long i = 0; i < pth.nb_maps; i++) {
				
				string name;
				if(pth.version >= 3008) {
					THEO_SAVE_MAPS_IN_3019 tsmi3019;
					memcpy(&tsmi3019, adr + pos, sizeof(THEO_SAVE_MAPS_IN_3019));
					pos += sizeof(THEO_SAVE_MAPS_IN_3019);
					name = tsmi3019.texture_name;
				} else {
					THEO_SAVE_MAPS_IN tsmi;
					memcpy(&tsmi, adr + pos, sizeof(THEO_SAVE_MAPS_IN));
					pos += sizeof(THEO_SAVE_MAPS_IN);
					name = tsmi.texture_name;
				}
				
				if(!name.empty()) {
					
					string mapsname = txpath + name;
					if(pth.type_write & SAVE_MAP_BMP) {
						mapsname += ".bmp";
					} else {
						mapsname += ".tga";
					}
					
					eerie->texturecontainer[i] = TextureContainer::Load(mapsname, TextureContainer::Level);
				}
			}
		}
	}
	
	pos = pth.object_seek;
	_THEObjLoad(eerie, adr, &pos, pth.version);
	eerie->angle.a = eerie->angle.b = eerie->angle.g = 0.f;
	eerie->pos.x = eerie->pos.y = eerie->pos.z = 0.f;

	// NORMALS CALCULATIONS
	Vec3f nrml;
	Vec3f nrrr;
	float count;
	long j, j2;

	//Compute Faces Areas
	for (size_t i = 0; i < eerie->facelist.size(); i++)
	{
		D3DTLVERTEX * ev[3];
		ev[0] = (D3DTLVERTEX *)&eerie->vertexlist[eerie->facelist[i].vid[0]].v;
		ev[1] = (D3DTLVERTEX *)&eerie->vertexlist[eerie->facelist[i].vid[1]].v;
		ev[2] = (D3DTLVERTEX *)&eerie->vertexlist[eerie->facelist[i].vid[2]].v;
		eerie->facelist[i].temp = TRUEDistance3D((ev[0]->sx + ev[1]->sx) * ( 1.0f / 2 ),
		                          (ev[0]->sy + ev[1]->sy) * ( 1.0f / 2 ),
		                          (ev[0]->sz + ev[1]->sz) * ( 1.0f / 2 ),
		                          ev[2]->sx, ev[2]->sy, ev[2]->sz)
		                          * TRUEDistance3D(ev[0]->sx, ev[0]->sy, ev[0]->sz,
		                                  ev[1]->sx, ev[1]->sy, ev[1]->sz) * ( 1.0f / 2 );
	}

	for (size_t i = 0; i < eerie->facelist.size(); i++)
	{
		CalcObjFaceNormal(
		    &eerie->vertexlist[eerie->facelist[i].vid[0]].v,
		    &eerie->vertexlist[eerie->facelist[i].vid[1]].v,
		    &eerie->vertexlist[eerie->facelist[i].vid[2]].v,
		    &eerie->facelist[i]
		);
		float area = eerie->facelist[i].temp;

		for (j = 0; j < 3; j++)
		{
			float mod = area * area;
			nrrr.x = nrml.x = eerie->facelist[i].norm.x * mod;
			nrrr.y = nrml.y = eerie->facelist[i].norm.y * mod;
			nrrr.z = nrml.z = eerie->facelist[i].norm.z * mod;
			count = mod;

			for (size_t i2 = 0; i2 < eerie->facelist.size(); i2++)
			{
				if (i != i2)
				{
					float area2 = eerie->facelist[i].temp;

					for (j2 = 0; j2 < 3; j2++)
					{
						float seuil2 = 0.1f; 
						
						float dist = TRUEDistance3D(eerie->vertexlist[eerie->facelist[i2].vid[j2]].v.x, eerie->vertexlist[eerie->facelist[i2].vid[j2]].v.y, eerie->vertexlist[eerie->facelist[i2].vid[j2]].v.z,
						                            eerie->vertexlist[eerie->facelist[i].vid[j]].v.x, eerie->vertexlist[eerie->facelist[i].vid[j]].v.y, eerie->vertexlist[eerie->facelist[i].vid[j]].v.z); 
						if (dist < seuil2)
						{
							mod = (area2 * area2);
							nrml.x += eerie->facelist[i2].norm.x * mod; 
							nrml.y += eerie->facelist[i2].norm.y * mod; 
							nrml.z += eerie->facelist[i2].norm.z * mod; 
							count += mod; 
						}
					}
				}
			}

			count = 1.f / count;
			eerie->vertexlist[eerie->facelist[i].vid[j]].vert.sx = nrml.x * count;
			eerie->vertexlist[eerie->facelist[i].vid[j]].vert.sy = nrml.y * count;
			eerie->vertexlist[eerie->facelist[i].vid[j]].vert.sz = nrml.z * count;
		}
	}

	for (size_t i = 0; i < eerie->facelist.size(); i++)
	{
		for (j = 0; j < 3; j++)
		{
			eerie->vertexlist[eerie->facelist[i].vid[j]].norm.x = eerie->vertexlist[eerie->facelist[i].vid[j]].vert.sx;
			eerie->vertexlist[eerie->facelist[i].vid[j]].norm.y = eerie->vertexlist[eerie->facelist[i].vid[j]].vert.sy;
			eerie->vertexlist[eerie->facelist[i].vid[j]].norm.z = eerie->vertexlist[eerie->facelist[i].vid[j]].vert.sz;
		}
	}

	// Apply Normals Spherical correction for NPC head
	long neck_orgn = GetGroupOriginByName(eerie, "NECK");
	long head_idx = EERIE_OBJECT_GetGroup(eerie, "head");

	if ((head_idx >= 0) && (neck_orgn >= 0))
	{
		Vec3f center(0, 0, 0);
		Vec3f origin = eerie->vertexlist[neck_orgn].v;
		float count = (float)eerie->grouplist[head_idx].indexes.size();

		if (count > 0.f)
		{
			for (size_t idx = 0 ; idx < eerie->grouplist[head_idx].indexes.size() ; idx++)
			{
				center.x += eerie->vertexlist[ eerie->grouplist[head_idx].indexes[idx] ].v.x;
				center.y += eerie->vertexlist[ eerie->grouplist[head_idx].indexes[idx] ].v.y;
				center.z += eerie->vertexlist[ eerie->grouplist[head_idx].indexes[idx] ].v.z;
			}

			float divc = 1.f / count;
			center.x *= divc;
			center.y *= divc;
			center.z *= divc;
			center.x = (center.x + origin.x + origin.x) * ( 1.0f / 3 );
			center.y = (center.y + origin.y + origin.y) * ( 1.0f / 3 );
			center.z = (center.z + origin.z + origin.z) * ( 1.0f / 3 );
			float max_threshold = dist(origin, center);

			for (size_t i = 0; i < eerie->grouplist[head_idx].indexes.size(); i++)
			{
				EERIE_VERTEX * ev = &eerie->vertexlist[eerie->grouplist[head_idx].indexes[i]];
				float d = dist(ev->v, origin);
				float factor = 1.f;

				if (d < max_threshold)
				{
					factor = d / max_threshold;
				}

				float ifactor = 1.f - factor;
				Vec3f fakenorm;
				fakenorm = ev->v - center;
				fakenorm.normalize();
				ev->norm = ev->norm * ifactor + fakenorm * factor;
				ev->norm.normalize();
			}
		}
	}

	// NORMALS CALCULATIONS END
	//***********************************************************

	EERIE_LINKEDOBJ_InitData(eerie);
	eerie->c_data = NULL;
	EERIE_CreateCedricData(eerie);
	EERIEOBJECT_CreatePFaces(eerie);
	return eerie;
}

static EERIE_3DOBJ * GetExistingEerie(const string & file) {
	
	for(long i = 1; i < inter.nbmax; i++) {
		if(inter.iobj[i] != NULL && !inter.iobj[i]->tweaky && inter.iobj[i]->obj) {
			EERIE_3DOBJ * obj = inter.iobj[i]->obj;
			if(!obj->originaltextures && inter.iobj[i]->obj->file == file) {
				return inter.iobj[i]->obj;
			}
		}
	}
	
	return NULL;
}

#endif

static EERIE_3DOBJ * TheoToEerie_Fast(const string & texpath, const string & file, bool pbox) {
	
	EERIE_3DOBJ * ret = ARX_FTL_Load(file);
	if(ret) {
		if(pbox) {
			EERIE_PHYSICS_BOX_Create(ret);
		}
		return ret;
	}
	
#ifndef BUILD_EDIT_LOADSAVE
	ARX_UNUSED(texpath);
#else
	
	ret = GetExistingEerie(file);
	if(ret) {
		ret = Eerie_Copy(ret);
	}
	
	if(!ret) {
		
		size_t size = 0;
		unsigned char * adr = (unsigned char *)PAK_FileLoadMalloc(file, size);
		
		if(!adr) {
			return NULL;
		}
		
		ret = TheoToEerie(adr, size, texpath, file);
		if(!ret) {
			free(adr);
			return NULL;
		}
		
		EERIE_OBJECT_CenterObjectCoordinates(ret);
		free(adr);
	}
	
#ifdef BUILD_EDITOR
	if(FASTLOADS) {
		if(ret->pdata) {
			free(ret->pdata);
			ret->pdata = NULL;
		}
		return ret;
	}
#endif
	
	CreateNeighbours(ret);
	EERIEOBJECT_AddClothesData(ret);
	KillNeighbours(ret);
	
	if(ret->cdata) {
		EERIE_COLLISION_SPHERES_Create(ret); // Must be out of the Neighbours zone
	}
	
	if(pbox) {
		EERIE_PHYSICS_BOX_Create(ret);
	}
	
	ARX_FTL_Save(file, ret);
	
#endif // BUILD_EDIT_LOADSAVE
	
	return ret;
}

EERIE_3DOBJ * loadObject(const string & file, bool pbox) {
	return TheoToEerie_Fast("graph\\obj3d\\textures\\", file, pbox);
}

EERIE_3DOBJ * _LoadTheObj(const string & file, const string & texpath) {
	
	std::string path = file;
	RemoveName(path);
	path += texpath;
	
	return TheoToEerie_Fast(path, file, true);
}

// TODO why is this in EERIEobject
ACTIONSTRUCT actions[MAX_ACTIONS];
void RemoveAllBackgroundActions()
{
	memset(actions, 0, sizeof(ACTIONSTRUCT)*MAX_ACTIONS);

	for(size_t i = 0; i < MAX_ACTIONS; i++) actions[i].dl = -1;
}

void EERIE_OBJECT_CenterObjectCoordinates(EERIE_3DOBJ * ret)
{
	if (!ret) return;

	Vec3f offset = ret->vertexlist[ret->origin].v;

	if ((offset.x == 0) && (offset.y == 0) && (offset.z == 0))
		return;

	LogWarning << "NOT CENTERED " << ret->file;


	for (size_t i = 0; i < ret->vertexlist.size(); i++)
	{
		ret->vertexlist[i].v.x -= offset.x;
		ret->vertexlist[i].v.y -= offset.y;
		ret->vertexlist[i].v.z -= offset.z;
		ret->vertexlist[i].vert.sx -= offset.x;
		ret->vertexlist[i].vert.sy -= offset.y;
		ret->vertexlist[i].vert.sz -= offset.z;

		ret->vertexlist3[i].v.x -= offset.x;
		ret->vertexlist3[i].v.y -= offset.y;
		ret->vertexlist3[i].v.z -= offset.z;
		ret->vertexlist3[i].vert.sx -= offset.x;
		ret->vertexlist3[i].vert.sy -= offset.y;
		ret->vertexlist3[i].vert.sz -= offset.z;

		ret->vertexlist3[i].v.x -= offset.x;
		ret->vertexlist3[i].v.y -= offset.y;
		ret->vertexlist3[i].v.z -= offset.z;
		ret->vertexlist3[i].vert.sx -= offset.x;
		ret->vertexlist3[i].vert.sy -= offset.y;
		ret->vertexlist3[i].vert.sz -= offset.z;
	}

	ret->point0.x -= offset.x;
	ret->point0.y -= offset.y;
	ret->point0.z -= offset.z;
}
