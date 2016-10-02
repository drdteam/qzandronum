// Emacs style mode select	 -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:  none
//		Implements special effects:
//		Texture animation, height or lighting changes
//		 according to adjacent sectors, respective
//		 utility functions, etc.
//
//-----------------------------------------------------------------------------

#ifndef __P_SPEC__
#define __P_SPEC__

#include "dsectoreffect.h"
#include "doomdata.h"
#include "r_state.h"

class FScanner;
struct level_info_t;

struct FThinkerCollection
{
	int RefNum;
	DThinker *Obj;
};

enum class EScroll : int
{
		sc_side,
		sc_floor,
		sc_ceiling,
		sc_carry,
		sc_carry_ceiling,	// killough 4/11/98: carry objects hanging on ceilings
};

enum EScrollPos : int
{
	scw_top = 1,
	scw_mid = 2,
	scw_bottom = 4,
	scw_all = 7,
};

void P_CreateScroller(EScroll type, double dx, double dy, int control, int affectee, int accel, EScrollPos scrollpos = EScrollPos::scw_all);


//jff 2/23/98 identify the special classes that can share sectors

typedef enum
{
	floor_special,
	ceiling_special,
	lighting_special,
} special_e;

// Factor to scale scrolling effect into mobj-carrying properties = 3/32.
// (This is so scrolling floors and objects on them can move at same speed.)
const double CARRYFACTOR = 3 / 32.;

// Flags for P_SectorDamage
#define DAMAGE_PLAYERS				1
#define DAMAGE_NONPLAYERS			2
#define DAMAGE_IN_AIR				4
#define DAMAGE_SUBCLASSES_PROTECT	8


// [RH] If a deathmatch game, checks to see if noexit is enabled.
//		If so, it kills the player and returns false. Otherwise,
//		it returns true, and the player is allowed to live.
bool	CheckIfExitIsGood (AActor *self, level_info_t *info);

// at map load
void	P_SpawnSpecials (void);

// every tic
void	P_UpdateSpecials (void);

// when needed
bool	P_ActivateLine (line_t *ld, AActor *mo, int side, int activationType, DVector3 *optpos = NULL);
bool	P_TestActivateLine (line_t *ld, AActor *mo, int side, int activationType, DVector3 *optpos = NULL);
bool	P_PredictLine (line_t *ld, AActor *mo, int side, int activationType);

void 	P_PlayerInSpecialSector (player_t *player, sector_t * sector=NULL);
void	P_PlayerOnSpecialFlat (player_t *player, int floorType);
void	P_SectorDamage(int tag, int amount, FName type, PClassActor *protectClass, int flags);
void	P_SetSectorFriction (int tag, int amount, bool alterFlag);
double FrictionToMoveFactor(double friction);
void P_GiveSecret(AActor *actor, bool printmessage, bool playsound, int sectornum);

//
// getNextSector()
// Return sector_t * of sector next to current.
// NULL if not two-sided line
//
inline sector_t *getNextSector (line_t *line, const sector_t *sec)
{
	if (!(line->flags & ML_TWOSIDED))
		return NULL;

	return line->frontsector == sec ?
		   (line->backsector != sec ? line->backsector : NULL) :
	       line->frontsector;
}


#include "p_tags.h"

class DLighting : public DSectorEffect
{
	DECLARE_CLASS(DLighting, DSectorEffect)
public:
	DLighting(sector_t *sector);

	// [BB] Necessary for GAME_ResetMap
	bool bNotMapSpawned;
protected:
	DLighting();
};

void	EV_StartLightFlickering (int tag, int upper, int lower);
void	EV_StartLightStrobing (int tag, int upper, int lower, int utics, int ltics);
void	EV_StartLightStrobing (int tag, int utics, int ltics);
void	EV_TurnTagLightsOff (int tag);
void	EV_LightTurnOn (int tag, int bright);
void	EV_LightTurnOnPartway (int tag, double frac);	// killough 10/98
void	EV_LightChange (int tag, int value);
void	EV_StopLightEffect (int tag);

void	P_SpawnGlowingLight (sector_t *sector);

void	EV_StartLightGlowing (int tag, int upper, int lower, int tics);
void	EV_StartLightFading (int tag, int value, int tics);


//
// P_SWITCH
//

#define BUTTONTIME TICRATE		// 1 second, in ticks. 

bool	P_ChangeSwitchTexture (side_t *side, int useAgain, BYTE special, bool *quest=NULL);
bool	P_CheckSwitchRange(AActor *user, line_t *line, int sideno, const DVector3 *optpos = NULL);

//
// P_PLATS
//
class DPlat : public DMovingFloor
{
	DECLARE_CLASS (DPlat, DMovingFloor)
public:
	enum EPlatState
	{
		up,
		down,
		waiting,
		in_stasis
	};

	enum EPlatType
	{
		platPerpetualRaise,
		platDownWaitUpStay,
		platDownWaitUpStayStone,
		platUpWaitDownStay,
		platUpNearestWaitDownStay,
		platDownByValue,
		platUpByValue,
		platUpByValueStay,
		platRaiseAndStay,
		platToggle,
		platDownToNearestFloor,
		platDownToLowestCeiling,
		platRaiseAndStayLockout,
	};

	// [BC] Make this constructor public to clients can create it.
	DPlat (sector_t *sector);

	void Serialize(FSerializer &arc);
	void Tick ();

	bool IsLift() const { return m_Type == platDownWaitUpStay || m_Type == platDownWaitUpStayStone; }

	// [BC] Create this object for this new client entering the game.
	void UpdateToClient( ULONG ulClient );

	// [BC] Access function(s).
	LONG	GetID( void );
	void	SetID( LONG lID );

	fixed_t	GetLow( void );
	void	SetLow( fixed_t Low );

	fixed_t	GetHigh( void );
	void	SetHigh( fixed_t High );

	EPlatState	GetStatus( void );
	void		SetStatus( LONG lStatus );

	void	SetType( EPlatType Type );
	void	SetCrush( LONG lCrush );
	void	SetTag( LONG lTag );
	void	SetSpeed( LONG lSpeed );
	void	SetDelay( LONG lDelay );

	// [BC] Make this public so clients can use it.
	void PlayPlatSound (const char *sound);

protected:

	double	 	m_Speed;
	double	 	m_Low;
	double	 	m_High;
	int 		m_Wait;
	int 		m_Count;
	EPlatState	m_Status;
	EPlatState	m_OldStatus;
	int			m_Crush;
	int 		m_Tag;
	EPlatType	m_Type;

	// [BC] This is the platform's unique network ID.
	// [EP] TODO: remove the 'l' prefix from this variable, it isn't LONG anymore
	int			m_lPlatID;

	void Reactivate ();
	void Stop ();

private:
	DPlat ();

	friend bool	EV_DoPlat (int tag, line_t *line, EPlatType type,
						   double height, double speed, int delay, int lip, int change);
	friend void EV_StopPlat (int tag, bool remove);
	friend void P_ActivateInStasis (int tag);
};

bool EV_DoPlat (int tag, line_t *line, DPlat::EPlatType type,
				double height, double speed, int delay, int lip, int change);
void EV_StopPlat (int tag, bool remove);
void P_ActivateInStasis (int tag);

//
// [RH]
// P_PILLAR
//

class DPillar : public DMover
{
	DECLARE_CLASS (DPillar, DMover)
	HAS_OBJECT_POINTERS
public:
	enum EPillar
	{
		pillarBuild,
		pillarOpen

	};

	DPillar (sector_t *sector, EPillar type, double speed, double height,
			 double height2, int crush, bool hexencrush);

	// [BC] New constructor where we just pass in the sector.
	DPillar (sector_t *sector);

	void Serialize(FSerializer &arc);
	void Tick ();
	void Destroy();

	// [BC] Create this object for this new client entering the game.
	void UpdateToClient( ULONG ulClient );

	// [BC] Access function(s).
	LONG	GetID( void );
	void	SetID( LONG lID );

	void	SetType( EPillar Type );
	EPillar	GetType( );
	void	SetFloorSpeed( LONG lSpeed );
	LONG	GetFloorSpeed( );
	void	SetCeilingSpeed( LONG lSpeed );
	LONG	GetCeilingSpeed( );
	void	SetFloorTarget( LONG lTarget );
	LONG	GetFloorTarget( );
	void	SetCeilingTarget( LONG lTarget );
	LONG	GetCeilingTarget( );
	LONG	GetCrush( void );
	void	SetCrush( LONG Crush );
	bool	GetHexencrush( void );
	void	SetHexencrush( bool hexencrush );

protected:
	EPillar		m_Type;
	double		m_FloorSpeed;
	double		m_CeilingSpeed;
	double		m_FloorTarget;
	double		m_CeilingTarget;
	int			m_Crush;
	bool		m_Hexencrush;
	TObjPtr<DInterpolation> m_Interp_Ceiling;
	TObjPtr<DInterpolation> m_Interp_Floor;

	// [BC] This is the pillar's unique network ID.
	// [EP] TODO: remove the 'l' prefix from this variable, it isn't LONG anymore
	int			m_lPillarID;

private:
	DPillar ();

	// [BC] Make this a friend.
	friend bool	EV_DoPillar (DPillar::EPillar type, int tag, fixed_t speed, fixed_t height,
							 fixed_t height2, int crush);
};

bool EV_DoPillar (DPillar::EPillar type, line_t *line, int tag,
				  double speed, double height, double height2, int crush, bool hexencrush);

//
// P_DOORS
//
class DDoor : public DMovingCeiling
{
	DECLARE_CLASS (DDoor, DMovingCeiling)
public:
	enum EVlDoor
	{
		doorClose,
		doorOpen,
		doorRaise,
		doorWaitRaise,
		doorCloseWaitOpen,
		doorWaitClose,
	};

	DDoor (sector_t *sector);
	// [BC] Added option to create doors soundlessly.
	DDoor (sector_t *sec, EVlDoor type, double speed, int delay, int lightTag, int topcountdown, bool bNoSound = false);

	void Serialize(FSerializer &arc);
	void Tick ();

	// [BC] Create this object for this new client entering the game.
	void UpdateToClient( ULONG ulClient );

	// [BC] Access function(s).
	int		GetDirection( void );
	void	SetDirection( LONG lDirection );

	LONG	GetID( void );
	void	SetID( LONG lID );

	EVlDoor	GetType( void );
	void	SetType( EVlDoor Type );

	LONG	GetSpeed( void );
	void	SetSpeed( LONG lSpeed );

	LONG	GetLightTag( void );
	void	SetLightTag( LONG lTag );

protected:
	EVlDoor		m_Type;
	double	 	m_TopDist;
	double		m_BotDist, m_OldFloorDist;
	vertex_t	*m_BotSpot;
	double	 	m_Speed;

	// 1 = up, 0 = waiting at top, -1 = down
	int 		m_Direction;
	
	// tics to wait at the top
	int 		m_TopWait;
	// (keep in case a door going down is reset)
	// when it reaches 0, start going down
	int 		m_TopCountdown;

	int			m_LightTag;

	// [BC] Make this public so clients can call it.
public:
	void DoorSound (bool raise, class DSeqNode *curseq=NULL) const;
protected:

	// [BC] This is the door's unique network ID.
	// [EP] TODO: remove the 'l' prefix from this variable, it isn't LONG anymore
	int			m_lDoorID;

	friend bool	EV_DoDoor (DDoor::EVlDoor type, line_t *line, AActor *thing,
						   int tag, double speed, int delay, int lock,
						   int lightTag, bool boomgen, int topcountdown);
private:
	DDoor ();

};

bool EV_DoDoor (DDoor::EVlDoor type, line_t *line, AActor *thing,
				int tag, double speed, int delay, int lock,
				int lightTag, bool boomgen = false, int topcountdown = 0);


class DAnimatedDoor : public DMovingCeiling
{
	DECLARE_CLASS (DAnimatedDoor, DMovingCeiling)
public:
	DAnimatedDoor (sector_t *sector);
	DAnimatedDoor (sector_t *sec, line_t *line, int speed, int delay, FDoorAnimation *anim);

	void Serialize(FSerializer &arc);
	void Tick ();

	bool StartClosing ();
protected:
	line_t *m_Line1, *m_Line2;
	int m_Frame;
	FDoorAnimation *m_DoorAnim;
	int m_Timer;
	double m_BotDist;
	int m_Status;
	enum
	{
		Opening,
		Waiting,
		Closing,
		Dead
	};
	int m_Speed;
	int m_Delay;
	bool m_SetBlocking1, m_SetBlocking2;

	friend bool EV_SlidingDoor (line_t *line, AActor *thing, int tag, int speed, int delay);
private:
	DAnimatedDoor ();
};

bool EV_SlidingDoor (line_t *line, AActor *thing, int tag, int speed, int delay);

//
// P_CEILNG
//

// [RH] Changed these
class DCeiling : public DMovingCeiling
{
	DECLARE_CLASS (DCeiling, DMovingCeiling)
public:
	enum ECeiling
	{
		ceilLowerByValue,
		ceilRaiseByValue,
		ceilMoveToValue,
		ceilLowerToHighestFloor,
		ceilLowerInstant,
		ceilRaiseInstant,
		ceilCrushAndRaise,
		ceilLowerAndCrush,
		ceil_placeholder,
		ceilCrushRaiseAndStay,
		ceilRaiseToNearest,
		ceilLowerToLowest,
		ceilLowerToFloor,

		// The following are only used by Generic_Ceiling
		ceilRaiseToHighest,
		ceilLowerToHighest,
		ceilRaiseToLowest,
		ceilLowerToNearest,
		ceilRaiseToHighestFloor,
		ceilRaiseToFloor,
		ceilRaiseByTexture,
		ceilLowerByTexture,

		genCeilingChg0,
		genCeilingChgT,
		genCeilingChg
	};

	enum class ECrushMode
	{
		crushDoom = 0,
		crushHexen = 1,
		crushSlowdown = 2
	};


	DCeiling (sector_t *sec);
	DCeiling (sector_t *sec, double speed1, double speed2, int silent);

	void Serialize(FSerializer &arc);
	void Tick ();

	// [BC] Create this object for this new client entering the game.
	void UpdateToClient( ULONG ulClient );

	// [BC] Make this public so clients can use it.
	void PlayCeilingSound ();

	// [BC] Access function(s).
	LONG	GetID( void );
	void	SetID( LONG lID );

	fixed_t	GetTopHeight( void );
	void	SetTopHeight( fixed_t TopHeight );

	fixed_t	GetBottomHeight( void );
	void	SetBottomHeight( fixed_t BottomHeight );

	fixed_t	GetSpeed( void );
	void	SetSpeed( fixed_t Speed );

	LONG	GetDirection( void );
	void	SetDirection( LONG lDirection );

	ECeiling	GetType( void );
	void		SetType( ECeiling Type );

	LONG	GetCrush( void );
	void	SetCrush( LONG lCrush );

	ECrushMode	GetCrushMode ( void );
	void	SetCrushMode ( ECrushMode Hexencrush );

protected:
	ECeiling	m_Type;
	double	 	m_BottomHeight;
	double	 	m_TopHeight;
	double	 	m_Speed;
	double		m_Speed1;		// [RH] dnspeed of crushers
	double		m_Speed2;		// [RH] upspeed of crushers
	int 		m_Crush;
	ECrushMode	m_CrushMode;
	int			m_Silent;
	int 		m_Direction;	// 1 = up, 0 = waiting, -1 = down

	// [RH] Need these for BOOM-ish transferring ceilings
	FTextureID	m_Texture;
	secspecial_t m_NewSpecial;

	// ID
	int 		m_Tag;
	int 		m_OldDirection;

	// [BC] This is the ceiling's unique network ID.
	// [EP] TODO: remove the 'l' prefix from this variable, it isn't LONG anymore
	int			m_lCeilingID;

private:
	DCeiling ();

	friend bool P_CreateCeiling(sector_t *sec, DCeiling::ECeiling type, line_t *line, int tag, double speed, double speed2, double height, int crush, int silent, int change, DCeiling::ECrushMode hexencrush);
	friend bool EV_CeilingCrushStop (int tag, bool remove);
	friend void P_ActivateInStasisCeiling (int tag);
};

bool P_CreateCeiling(sector_t *sec, DCeiling::ECeiling type, line_t *line, int tag, double speed, double speed2, double height, int crush, int silent, int change, DCeiling::ECrushMode hexencrush);
bool EV_DoCeiling (DCeiling::ECeiling type, line_t *line, int tag, double speed, double speed2, double height, int crush, int silent, int change, DCeiling::ECrushMode hexencrush = DCeiling::ECrushMode::crushDoom);

bool EV_CeilingCrushStop (int tag, bool remove);
void P_ActivateInStasisCeiling (int tag);



//
// P_FLOOR
//

class DFloor : public DMovingFloor
{
	DECLARE_CLASS (DFloor, DMovingFloor)
public:
	enum EFloor
	{
		floorLowerToLowest,
		floorLowerToNearest,
		floorLowerToHighest,
		floorLowerByValue,
		floorRaiseByValue,
		floorRaiseToHighest,
		floorRaiseToNearest,
		floorRaiseAndCrush,
		floorRaiseAndCrushDoom,
		floorCrushStop,
		floorLowerInstant,
		floorRaiseInstant,
		floorMoveToValue,
		floorRaiseToLowestCeiling,
		floorRaiseByTexture,

		floorLowerAndChange,
		floorRaiseAndChange,

		floorRaiseToLowest,
		floorRaiseToCeiling,
		floorLowerToLowestCeiling,
		floorLowerByTexture,
		floorLowerToCeiling,

		donutRaise,

		buildStair,
		waitStair,
		resetStair,

		// Not to be used as parameters to EV_DoFloor()
		genFloorChg0,
		genFloorChgT,
		genFloorChg
	};

	// [RH] Changed to use Hexen-ish specials
	enum EStair
	{
		buildUp,
		buildDown
	};

	enum EStairType
	{
		stairUseSpecials = 1,
		stairSync = 2
	};

	DFloor (sector_t *sec);

	void Serialize(FSerializer &arc);
	void Tick ();

	// [BC] Create this object for this new client entering the game.
	void UpdateToClient( ULONG ulClient );

	// [BC] Make these public so clients can use them.
	void StartFloorSound ();
	void SetFloorChangeType (sector_t *sec, int change);

	// [BC] Access function(s).
	LONG	GetID( void );
	void	SetID( LONG lID );

	EFloor	GetType( void );
	void	SetType( EFloor Type );

	LONG	GetCrush( void );
	void	SetCrush( LONG lCrush );

	bool	GetHexencrush( void );
	void	SetHexencrush( bool hexencrush );

	fixed_t	GetSpeed( void );
	void	SetSpeed( fixed_t Speed );

	LONG	GetResetCount( void );
	void	SetResetCount( LONG lResetCount );

	fixed_t	GetOrgDist( void );
	void	SetOrgDist( fixed_t OrgDist );

	LONG	GetDirection( void );
	void	SetDirection( LONG lDirection );

	fixed_t	GetFloorDestDist( void );
	void	SetFloorDestDist( fixed_t FloorDestDist );

	int		GetDelay( void );
	void	SetDelay( int Delay );

	int		GetPauseTime( void );
	void	SetPauseTime( int PauseTime );

	int		GetStepTime( void );
	void	SetStepTime( int StepTime );

	int		GetPerStepTime( void );
	void	SetPerStepTime( int PerStepTime );

//protected:
	EFloor	 	m_Type;
	int 		m_Crush;
	bool		m_Hexencrush;
	int 		m_Direction;
	secspecial_t m_NewSpecial;
	FTextureID	m_Texture;
	double	 	m_FloorDestDist;
	double	 	m_Speed;

	// [RH] New parameters used to reset and delay stairs
	double		m_OrgDist;
	int			m_ResetCount;
	int			m_Delay;
	int			m_PauseTime;
	int			m_StepTime;
	int			m_PerStepTime;

	// [BC] This is the floor's unique network ID.
	// [EP] TODO: remove the 'l' prefix from this variable, it isn't LONG anymore
	int			m_lFloorID;

	friend bool EV_BuildStairs (int tag, DFloor::EStair type, line_t *line,
		double stairsize, double speed, int delay, int reset, int igntxt,
		int usespecials);
	friend bool EV_DoFloor (DFloor::EFloor floortype, line_t *line, int tag,
		double speed, double height, int crush, int change, bool hexencrush, bool hereticlower);
	friend bool EV_FloorCrushStop (int tag);
	friend bool EV_DoDonut (int tag, line_t *line, double pillarspeed, double slimespeed);
private:
	DFloor ();
};

bool P_CreateFloor(sector_t *sec, DFloor::EFloor floortype, line_t *line,
	double speed, double height, int crush, int change, bool hexencrush, bool hereticlower);

bool EV_BuildStairs (int tag, DFloor::EStair type, line_t *line,
	double stairsize, double speed, int delay, int reset, int igntxt,
	int usespecials);
bool EV_DoFloor(DFloor::EFloor floortype, line_t *line, int tag,
	double speed, double height, int crush, int change, bool hexencrush, bool hereticlower = false);

bool EV_FloorCrushStop (int tag);
bool EV_DoDonut (int tag, line_t *line, double pillarspeed, double slimespeed);

class DElevator : public DMover
{
	DECLARE_CLASS (DElevator, DMover)
	HAS_OBJECT_POINTERS
public:
	enum EElevator
	{
		elevateUp,
		elevateDown,
		elevateCurrent,
		// [RH] For FloorAndCeiling_Raise/Lower
		elevateRaise,
		elevateLower
	};

	DElevator (sector_t *sec);

	void Destroy();
	void Serialize(FSerializer &arc);
	void Tick ();

	// [BC] Create this object for this new client entering the game.
	void	UpdateToClient( ULONG ulClient );

	// [BC] No longer protected so clients can call it.
	void StartFloorSound ();

	// [BC] Access function(s).
	LONG	GetID( void );
	void	SetID( LONG lID );

	void	SetType( EElevator Type );
	void	SetSpeed( LONG lSpeed );
	void	SetDirection( LONG lDirection );
	void	SetFloorDestDist( LONG lDestDist );
	void	SetCeilingDestDist( LONG lDestDist );

protected:
	EElevator	m_Type;
	int			m_Direction;
	double		m_FloorDestDist;
	double		m_CeilingDestDist;
	double		m_Speed;
	TObjPtr<DInterpolation> m_Interp_Ceiling;
	TObjPtr<DInterpolation> m_Interp_Floor;

	// [BC] This is the elevator's unique network ID.
	// [EP] TODO: remove the 'l' prefix from this variable, it isn't LONG anymore
	int			m_lElevatorID;

	friend bool EV_DoElevator (line_t *line, DElevator::EElevator type, double speed, double height, int tag);
private:
	DElevator ();
};

bool EV_DoElevator (line_t *line, DElevator::EElevator type, double speed, double height, int tag);

class DWaggleBase : public DMover
{
	DECLARE_CLASS (DWaggleBase, DMover)
	HAS_OBJECT_POINTERS
public:
	DWaggleBase (sector_t *sec);

	void Serialize(FSerializer &arc);

	// [BC] Create this object for this new client entering the game.
	void	UpdateToClient( ULONG ulClient );

	// [BC] Access function(s).
	LONG	GetID( void );
	void	SetID( LONG lID );

	void	SetOriginalDistance( LONG lOriginalDistance );
	void	SetAccumulator( LONG lAccumulator );
	void	SetAccelerationDelta( LONG lAccelerationDelta );
	void	SetTargetScale( LONG lTargetScale );
	void	SetScale( LONG lScale );
	void	SetScaleDelta( LONG lScaleDelta );
	void	SetTicker( LONG lTicker );
	void	SetState( LONG lState );

protected:
	double m_OriginalDist;
	double m_Accumulator;
	double m_AccDelta;
	double m_TargetScale;
	double m_Scale;
	double m_ScaleDelta;
	int m_Ticker;
	int m_State;

	// [BC] This is the waggle's unique network ID.
	// [EP] TODO: remove the 'l' prefix from this variable, it isn't LONG anymore
	int			m_lWaggleID;

	friend bool EV_StartWaggle (int tag, line_t *line, int height, int speed,
		int offset, int timer, bool ceiling);

	void DoWaggle (bool ceiling);
	// [BB] Changed Destroy to public, so that it can be called in cl_main.cpp.
public:
	void Destroy();
protected:
	DWaggleBase ();
};

bool EV_StartWaggle (int tag, line_t *line, int height, int speed,
	int offset, int timer, bool ceiling);

class DFloorWaggle : public DWaggleBase
{
	DECLARE_CLASS (DFloorWaggle, DWaggleBase)
public:
	DFloorWaggle (sector_t *sec);
	void Tick ();
private:
	DFloorWaggle ();
};

class DCeilingWaggle : public DWaggleBase
{
	DECLARE_CLASS (DCeilingWaggle, DWaggleBase)
public:
	DCeilingWaggle (sector_t *sec);
	void Tick ();
private:
	DCeilingWaggle ();
};

//jff 3/15/98 pure texture/type change for better generalized support
enum EChange
{
	trigChangeOnly,
	numChangeOnly,
};

bool EV_DoChange (line_t *line, EChange changetype, int tag);



//
// P_TELEPT
//
enum
{
	TELF_DESTFOG			= 1,
	TELF_SOURCEFOG			= 2,
	TELF_KEEPORIENTATION	= 4,
	TELF_KEEPVELOCITY		= 8,
	TELF_KEEPHEIGHT			= 16,
	TELF_ROTATEBOOM			= 32,
	TELF_ROTATEBOOMINVERSE	= 64,
};

//Spawns teleport fog. Pass the actor to pluck TeleFogFromType and TeleFogToType. 'from' determines if this is the fog to spawn at the old position (true) or new (false).
void P_SpawnTeleportFog(AActor *mobj, const DVector3 &pos, bool beforeTele = true, bool setTarget = false, bool spawnOnClient = false); // [BB] Added spawnOnClient

bool P_Teleport(AActor *thing, DVector3 pos, DAngle angle, int flags);
bool EV_Teleport (int tid, int tag, line_t *line, int side, AActor *thing, int flags);
bool EV_SilentLineTeleport (line_t *line, int side, AActor *thing, int id, INTBOOL reverse);
bool EV_TeleportOther (int other_tid, int dest_tid, bool fog);
bool EV_TeleportGroup (int group_tid, AActor *victim, int source_tid, int dest_tid, bool moveSource, bool fog);
bool EV_TeleportSector (int tag, int source_tid, int dest_tid, bool fog, int group_tid);


//
// [RH] ACS (see also p_acs.h)
//

#define ACS_BACKSIDE		1
#define ACS_ALWAYS			2
#define ACS_WANTRESULT		4
#define ACS_NET				8

int  P_StartScript (AActor *who, line_t *where, int script, const char *map, const int *args, int argcount, int flags);
void P_SuspendScript (int script, const char *map);
void P_TerminateScript (int script, const char *map);
void P_DoDeferedScripts (void);

//
// [RH] p_quake.c
//
bool P_StartQuakeXYZ(AActor *activator, int tid, int intensityX, int intensityY, int intensityZ, int duration, int damrad, int tremrad, FSoundID quakesfx, int flags, double waveSpeedX, double waveSpeedY, double waveSpeedZ, int falloff, int highpoint, double rollIntensity, double rollWave);
bool P_StartQuake(AActor *activator, int tid, int intensity, int duration, int damrad, int tremrad, FSoundID quakesfx);

// [BC] Prototypes dealing with network IDs for movers.
DDoor		*P_GetDoorByID( LONG lID );
DPlat		*P_GetPlatByID( LONG lID );
DFloor		*P_GetFloorByID( LONG lID );
DElevator	*P_GetElevatorByID( LONG lID );
DWaggleBase	*P_GetWaggleByID( LONG lID );
DPillar		*P_GetPillarByID( LONG lID );
DCeiling	*P_GetCeilingByID( LONG lID );

LONG		P_GetFirstFreeDoorID( void );
LONG		P_GetFirstFreePlatID( void );
LONG		P_GetFirstFreeFloorID( void );
LONG		P_GetFirstFreeElevatorID( void );
LONG		P_GetFirstFreeWaggleID( void );
LONG		P_GetFirstFreePillarID( void );
LONG		P_GetFirstFreeCeilingID( void );

#endif
