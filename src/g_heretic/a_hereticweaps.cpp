/*
#include "templates.h"
#include "actor.h"
#include "info.h"
#include "s_sound.h"
#include "m_random.h"
#include "a_pickups.h"
#include "d_player.h"
#include "p_pspr.h"
#include "p_local.h"
#include "gstrings.h"
#include "gi.h"
#include "r_data/r_translate.h"
#include "thingdef/thingdef.h"
#include "doomstat.h"
*/

static FRandom pr_sap ("StaffAtkPL1");
static FRandom pr_sap2 ("StaffAtkPL2");
static FRandom pr_fgw ("FireWandPL1");
static FRandom pr_fgw2 ("FireWandPL2");
static FRandom pr_boltspark ("BoltSpark");
static FRandom pr_macerespawn ("MaceRespawn");
static FRandom pr_maceatk ("FireMacePL1");
static FRandom pr_gatk ("GauntletAttack");
static FRandom pr_bfx1 ("BlasterFX1");
static FRandom pr_ripd ("RipperD");
static FRandom pr_fb1 ("FireBlasterPL1");
static FRandom pr_bfx1t ("BlasterFX1Tick");
static FRandom pr_hrfx2 ("HornRodFX2");
static FRandom pr_rp ("RainPillar");
static FRandom pr_fsr1 ("FireSkullRodPL1");
static FRandom pr_storm ("SkullRodStorm");
static FRandom pr_impact ("RainImpact");
static FRandom pr_pfx1 ("PhoenixFX1");
static FRandom pr_pfx2 ("PhoenixFX2");
static FRandom pr_fp2 ("FirePhoenixPL2");

#define FLAME_THROWER_TICS (10*TICRATE)

void P_DSparilTeleport (AActor *actor);

#define USE_BLSR_AMMO_1 1
#define USE_BLSR_AMMO_2 5
#define USE_SKRD_AMMO_1 1
#define USE_SKRD_AMMO_2 5
#define USE_PHRD_AMMO_1 1
#define USE_PHRD_AMMO_2 1
#define USE_MACE_AMMO_1 1
#define USE_MACE_AMMO_2 5

extern bool P_AutoUseChaosDevice (player_t *player);

// --- Staff ----------------------------------------------------------------

//----------------------------------------------------------------------------
//
// PROC A_StaffAttackPL1
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION_PARAMS(AActor, A_StaffAttack)
{
	PARAM_ACTION_PROLOGUE;

	DAngle angle;
	DAngle slope;
	player_t *player;
	FTranslatedLineTarget t;

	if (NULL == (player = self->player))
	{
		return 0;
	}

	PARAM_INT	(damage);
	PARAM_CLASS	(puff, AActor);

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return 0;
	}
	if (puff == NULL)
	{
		puff = PClass::FindActor(NAME_BulletPuff);	// just to be sure
	}

	// [BC] Weapons are handled by the server.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	angle = self->Angles.Yaw + pr_sap.Random2() * (5.625 / 256);
	slope = P_AimLineAttack (self, angle, MELEERANGE);
	P_LineAttack (self, angle, MELEERANGE, slope, damage, NAME_Melee, puff, true, &t);

	// [BC] Apply spread.
	if ( player->cheats2 & CF2_SPREAD )
	{
		P_LineAttack(self, angle + 15., MELEERANGE, slope, damage, NAME_Melee, puff, true);
		P_LineAttack(self, angle - 15., MELEERANGE, slope, damage, NAME_Melee, puff, true);
	}

	if (t.linetarget)
	{
		//S_StartSound(player->mo, sfx_stfhit);
		// turn to face target
		self->Angles.Yaw = t.angleFromSource;

		// [BC] If we're the server, tell clients to adjust the player's angle.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_SetThingAngleExact( player->mo );
	}
	return 0;
}


//----------------------------------------------------------------------------
//
// PROC A_FireGoldWandPL1
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireGoldWandPL1)
{
	PARAM_ACTION_PROLOGUE;

	DAngle angle;
	int damage;
	player_t *player;

	if (NULL == (player = self->player))
	{
		return 0;
	}

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo(weapon->bAltFire))
			return 0;
	}

	// [BC] If we're the client, just play the sound and get out.
	if ( NETWORK_InClientMode() )
	{
		S_Sound( player->mo, CHAN_WEAPON, "weapons/wandhit", 1, ATTN_NORM );
		return 0;
	}

	DAngle pitch = P_BulletSlope(self);
	damage = 7 + (pr_fgw() & 7);
	angle = self->Angles.Yaw;
	if (player->refire)
	{
		angle += pr_fgw.Random2() * (5.625 / 256);
	}
	P_LineAttack(self, angle, PLAYERMISSILERANGE, pitch, damage, NAME_Hitscan, "GoldWandPuff1");

	// [BC] Apply spread.
	if ( player->cheats2 & CF2_SPREAD )
	{
		DAngle angle = self->Angles.Yaw;
		if (player->refire)
		{
			angle += pr_fgw.Random2() * (5.625 / 256);;
		}
		P_LineAttack(self, angle + 15, PLAYERMISSILERANGE, pitch, damage, NAME_Hitscan, "GoldWandPuff1");

		angle = self->Angles.Yaw;
		if (player->refire)
		{
			angle += pr_fgw.Random2() * (5.625 / 256);;
		}
		P_LineAttack(self, angle - 15, PLAYERMISSILERANGE, pitch, damage, NAME_Hitscan, "GoldWandPuff1");
	}

	// [BB] If the player hit a player with his attack, potentially give him a medal.
	PLAYER_CheckStruckPlayer ( self );

	S_Sound(self, CHAN_WEAPON, "weapons/wandhit", 1, ATTN_NORM);

	// [BC] If we're the server, tell clients that a weapon is being fired.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		SERVERCOMMANDS_WeaponSound( ULONG( player - players ), "weapons/wandhit", ULONG( player - players ), SVCF_SKIPTHISCLIENT );

	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_FireGoldWandPL2
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireGoldWandPL2)
{
	PARAM_ACTION_PROLOGUE;

	int i;
	DAngle angle;
	int damage;
	double vz;
	player_t *player;

	if (NULL == (player = self->player))
	{
		return 0;
	}

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return 0;
	}

	// [BC] If we're the client, just play the sound and get out.
	if ( NETWORK_InClientMode() )
	{
		S_Sound( player->mo, CHAN_WEAPON, "weapons/wandhit", 1, ATTN_NORM );
		return 0;
	}

	DAngle pitch = P_BulletSlope(self);

	vz = -GetDefaultByName("GoldWandFX2")->Speed * pitch.TanClamped();
	P_SpawnMissileAngle(self, PClass::FindActor("GoldWandFX2"), self->Angles.Yaw - (45. / 8), vz, true); // [BB] Inform clients
	P_SpawnMissileAngle(self, PClass::FindActor("GoldWandFX2"), self->Angles.Yaw + (45. / 8), vz, true); // [BB] Inform clients
	angle = self->Angles.Yaw - (45. / 8);
	for(i = 0; i < 5; i++)
	{
		damage = 1+(pr_fgw2()&7);
		P_LineAttack (self, angle, PLAYERMISSILERANGE, pitch, damage, NAME_None, "GoldWandPuff2");
		angle += ((45. / 8) * 2) / 4;
	}

	// [BC] Apply spread.
	if ( player->cheats2 & CF2_SPREAD )
	{
		P_SpawnMissileAngle( self, PClass::FindActor("GoldWandFX2"), self->Angles.Yaw - (45. / 8) + 15, vz, true); // [BB] Inform clients
		P_SpawnMissileAngle( self, PClass::FindActor("GoldWandFX2"), self->Angles.Yaw + (45. / 8) + 15, vz, true); // [BB] Inform clients
		angle = self->Angles.Yaw - (45. / 8) + 15;
		for ( i = 0; i < 5; i++ )
		{
			damage = 1+(pr_fgw2()&7);
			P_LineAttack (self, angle, PLAYERMISSILERANGE, pitch, damage, NAME_None, "GoldWandPuff2");
			angle += ((45. / 8) * 2) / 4;
		}

		P_SpawnMissileAngle( self, PClass::FindActor("GoldWandFX2"), self->Angles.Yaw - (45. / 8) - 15, vz, true); // [BB] Inform clients
		P_SpawnMissileAngle( self, PClass::FindActor("GoldWandFX2"), self->Angles.Yaw + (45. / 8) - 15, vz, true); // [BB] Inform clients
		angle = self->Angles.Yaw - (45. / 8) - 15;
		for ( i = 0; i < 5; i++ )
		{
			damage = 1+(pr_fgw2()&7);
			P_LineAttack (self, angle, PLAYERMISSILERANGE, pitch, damage, NAME_Hitscan, "GoldWandPuff2");
			angle += ((45. / 8) * 2) / 4;
		}
	}
	S_Sound (self, CHAN_WEAPON, "weapons/wandhit", 1, ATTN_NORM);

	// [BC] If we're the server, tell clients that a weapon is being fired.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		SERVERCOMMANDS_WeaponSound( ULONG( player - players ), "weapons/wandhit", ULONG( player - players ), SVCF_SKIPTHISCLIENT );

	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_FireCrossbowPL1
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireCrossbowPL1)
{
	PARAM_ACTION_PROLOGUE;

	player_t *player;

	if (NULL == (player = self->player))
	{
		return 0;
	}

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return 0;
	}
	// [BC] Weapons are handled by the server.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	// [BB] Spread
	P_SpawnPlayerMissileWithPossibleSpread (self, PClass::FindActor("CrossbowFX1"));
	P_SpawnPlayerMissileWithPossibleSpread (self, PClass::FindActor("CrossbowFX3"), self->Angles.Yaw - 4.5);
	P_SpawnPlayerMissileWithPossibleSpread (self, PClass::FindActor("CrossbowFX3"), self->Angles.Yaw + 4.5);
	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_FireCrossbowPL2
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireCrossbowPL2)
{
	PARAM_ACTION_PROLOGUE;

	player_t *player;

	if (NULL == (player = self->player))
	{
		return 0;
	}

	AWeapon *weapon = self->player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return 0;
	}

	// [BC] Weapons are handled by the server.
	if ( NETWORK_InClientMode() )
	{		
		return 0;
	}

	// [BB] Spread
	P_SpawnPlayerMissileWithPossibleSpread (self, PClass::FindActor("CrossbowFX2"));
	P_SpawnPlayerMissileWithPossibleSpread (self, PClass::FindActor("CrossbowFX2"), self->Angles.Yaw - 4.5);
	P_SpawnPlayerMissileWithPossibleSpread (self, PClass::FindActor("CrossbowFX2"), self->Angles.Yaw + 4.5);
	P_SpawnPlayerMissileWithPossibleSpread (self, PClass::FindActor("CrossbowFX3"), self->Angles.Yaw - 9.);
	P_SpawnPlayerMissileWithPossibleSpread (self, PClass::FindActor("CrossbowFX3"), self->Angles.Yaw + 9.);
	return 0;
}

//---------------------------------------------------------------------------
//
// PROC A_GauntletAttack
//
//---------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION_PARAMS(AActor, A_GauntletAttack)
{
	PARAM_ACTION_PROLOGUE;

	DAngle Angle;
	int damage;
	DAngle slope;
	int randVal;
	double dist;
	player_t *player;
	PClassActor *pufftype;
	FTranslatedLineTarget t;
	int actualdamage = 0;

	if (nullptr == (player = self->player))
	{
		return 0;
	}

	PARAM_INT(power);

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != nullptr)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return 0;

		player->GetPSprite(PSP_WEAPON)->x = ((pr_gatk() & 3) - 2);
		player->GetPSprite(PSP_WEAPON)->y = WEAPONTOP + (pr_gatk() & 3);
	}
	Angle = self->Angles.Yaw;
	if (power)
	{
		damage = pr_gatk.HitDice (2);
		dist = 4*MELEERANGE;
		Angle += pr_gatk.Random2() * (2.8125 / 256);
		pufftype = PClass::FindActor("GauntletPuff2");
	}
	else
	{
		damage = pr_gatk.HitDice (2);
		dist = SAWRANGE;
		Angle += pr_gatk.Random2() * (5.625 / 256);
		pufftype = PClass::FindActor("GauntletPuff1");
	}
	slope = P_AimLineAttack (self, Angle, dist);
	P_LineAttack (self, Angle, dist, slope, damage, NAME_Melee, pufftype, false, &t, &actualdamage);
	if (!t.linetarget)
	{
		if (pr_gatk() > 64)
		{
			player->extralight = !player->extralight;
		}
		S_Sound (self, CHAN_AUTO, "weapons/gauntletson", 1, ATTN_NORM);
		return 0;
	}
	randVal = pr_gatk();
	if (randVal < 64)
	{
		player->extralight = 0;
	}
	else if (randVal < 160)
	{
		player->extralight = 1;
	}
	else
	{
		player->extralight = 2;
	}

	// [EP] Clients should not execute this, let the server know what's needed.
	if ( NETWORK_InClientMode() == false )
	{
		if (power)
		{
			// [EP] Save the current health value for bandwidth.
			const int prevhealth = self->health;

			if (!(t.linetarget->flags5 & MF5_DONTDRAIN)) P_GiveBody (self, actualdamage>>1);
			S_Sound (self, CHAN_AUTO, "weapons/gauntletspowhit", 1, ATTN_NORM);

			// [EP] Inform the clients about the sound and the possible health change.
			if ( NETWORK_GetState() == NETSTATE_SERVER )
			{
				SERVERCOMMANDS_SoundActor( self, CHAN_AUTO, "weapons/gauntletspowhit", 1, ATTN_NORM );
				if ( prevhealth != self->health )
					 SERVERCOMMANDS_SetPlayerHealth( player - players );
			}
		}
		else
		{
			S_Sound (self, CHAN_AUTO, "weapons/gauntletshit", 1, ATTN_NORM);

			// [EP] Inform the clients about the sound.
			if ( NETWORK_GetState() == NETSTATE_SERVER )
				SERVERCOMMANDS_SoundActor( self, CHAN_AUTO, "weapons/gauntletshit", 1, ATTN_NORM );
		}
	}
	// turn to face target
	DAngle angle = t.angleFromSource;
	DAngle anglediff = deltaangle(self->Angles.Yaw, angle);

	if (anglediff < 0.0)
	{
		if (anglediff < -4.5)
			self->Angles.Yaw = angle + 90.0 / 21;
		else
			self->Angles.Yaw -= 4.5;
	}
	else
	{
		if (anglediff > 4.5)
			self->Angles.Yaw = angle - 90.0 / 21;
		else
			self->Angles.Yaw += 4.5;
	}
	self->flags |= MF_JUSTATTACKED;
	return 0;
}

// --- Mace -----------------------------------------------------------------

#define MAGIC_JUNK 1234

// Mace FX4 -----------------------------------------------------------------

class AMaceFX4 : public AActor
{
	DECLARE_CLASS (AMaceFX4, AActor)
public:
	int DoSpecialDamage (AActor *target, int damage, FName damagetype);
};

IMPLEMENT_CLASS (AMaceFX4)

int AMaceFX4::DoSpecialDamage (AActor *target, int damage, FName damagetype)
{
	if ((target->flags2 & MF2_BOSS) || (target->flags3 & MF3_DONTSQUASH) || target->IsTeammate (this->target))
	{ // Don't allow cheap boss kills and don't instagib teammates
		return damage;
	}
	else if (target->player)
	{ // Player specific checks
		if (target->player->mo->flags2 & MF2_INVULNERABLE)
		{ // Can't hurt invulnerable players
			return -1;
		}
		if (P_AutoUseChaosDevice (target->player))
		{ // Player was saved using chaos device
			return -1;
		}
	}
	return TELEFRAG_DAMAGE; // Something's gonna die
}

//----------------------------------------------------------------------------
//
// PROC A_FireMacePL1B
//
//----------------------------------------------------------------------------

void FireMacePL1B (AActor *actor)
{
	AActor *ball;
	player_t *player;

	if (NULL == (player = actor->player))
	{
		return;
	}

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
	}

	// [BC] Weapons are handled by the server.
	if ( NETWORK_InClientMode() )
	{
		return;
	}

	ball = Spawn("MaceFX2", actor->PosPlusZ(28 - actor->Floorclip), ALLOW_REPLACE);
	ball->Vel.Z = 2 - player->mo->Angles.Pitch.TanClamped();
	ball->target = actor;
	ball->Angles.Yaw = actor->Angles.Yaw;
	ball->AddZ(ball->Vel.Z);
	ball->VelFromAngle();
	ball->Vel += actor->Vel.XY()/2;
	S_Sound (ball, CHAN_BODY, "weapons/maceshoot", 1, ATTN_NORM);

	// [BC] If we're the server, spawn the ball and play the sound.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
	{
		SERVERCOMMANDS_SpawnMissileExact( ball );
		SERVERCOMMANDS_SoundActor( ball, CHAN_BODY, "weapons/maceshoot", 1, ATTN_NORM );
	}

	// [BC] Apply spread.
	if ( player->cheats2 & CF2_SPREAD )
	{
		ball = Spawn("MaceFX2", actor->PosPlusZ(28 - actor->Floorclip), ALLOW_REPLACE);
		ball->Vel.Z = 2 - player->mo->Angles.Pitch.TanClamped();
		ball->target = actor;
		ball->Angles.Yaw = actor->Angles.Yaw + 15;
		ball->AddZ(ball->Vel.Z);
		ball->VelFromAngle();
		ball->Vel += actor->Vel.XY() / 2;
		S_Sound (ball, CHAN_BODY, "weapons/maceshoot", 1, ATTN_NORM);

		// [BC] If we're the server, spawn the ball and play the sound.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		{
			SERVERCOMMANDS_SpawnMissileExact( ball );
			SERVERCOMMANDS_SoundActor( ball, CHAN_BODY, "weapons/maceshoot", 1, ATTN_NORM );
		}

		ball = Spawn("MaceFX2", actor->PosPlusZ(28 - actor->Floorclip), ALLOW_REPLACE);
		ball->Vel.Z = 2 - player->mo->Angles.Pitch.TanClamped();
		ball->target = actor;
		ball->Angles.Yaw = actor->Angles.Yaw - 15;
		ball->AddZ(ball->Vel.Z);
		ball->VelFromAngle();
		ball->Vel += actor->Vel.XY() / 2;
		S_Sound (ball, CHAN_BODY, "weapons/maceshoot", 1, ATTN_NORM);

		// [BC] If we're the server, spawn the ball and play the sound.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		{
			SERVERCOMMANDS_SpawnMissileExact( ball );
			SERVERCOMMANDS_SoundActor( ball, CHAN_BODY, "weapons/maceshoot", 1, ATTN_NORM );
		}
	}

	P_CheckMissileSpawn (ball, actor->radius);
}

//----------------------------------------------------------------------------
//
// PROC A_FireMacePL1
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireMacePL1)
{
	PARAM_ACTION_PROLOGUE;

	AActor *ball;
	player_t *player;

	if (nullptr == (player = self->player))
	{
		return 0;
	}

	if (pr_maceatk() < 28)
	{
		FireMacePL1B(self);
		return 0;
	}
	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != nullptr)
	{
		if (!weapon->DepleteAmmo(weapon->bAltFire))
			return 0;

		player->GetPSprite(PSP_WEAPON)->x = ((pr_maceatk() & 3) - 2);
		player->GetPSprite(PSP_WEAPON)->y = WEAPONTOP + (pr_maceatk() & 3);
	}

	// [BC] Weapons are handled by the server.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	ball = P_SpawnPlayerMissile(self, PClass::FindActor("MaceFX1"), self->Angles.Yaw + (((pr_maceatk() & 7) - 4) * (360. / 256)));
	if (ball)
	{
		ball->special1 = 16; // tics till dropoff
	}

	// [BC] Apply spread.
	if ( player->cheats2 & CF2_SPREAD )
	{
		ball = P_SpawnPlayerMissile(self, PClass::FindActor("MaceFX1"), self->Angles.Yaw + (((pr_maceatk() & 7) - 4) * (360. / 256)) + 15 );
		if (ball)
		{
			ball->special1 = 16; // tics till dropoff
		}

		ball = P_SpawnPlayerMissile(self, PClass::FindActor("MaceFX1"), self->Angles.Yaw + (((pr_maceatk() & 7) - 4) * (360. / 256)) - 15);
		if (ball)
		{
			ball->special1 = 16; // tics till dropoff
		}
	}
	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_MacePL1Check
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_MacePL1Check)
{
	PARAM_ACTION_PROLOGUE;

	// [BC] Let the server handle this.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	if (self->special1 == 0)
	{
		return 0;
	}
	self->special1 -= 4;
	if (self->special1 > 0)
	{
		return 0;
	}
	self->special1 = 0;
	self->flags &= ~MF_NOGRAVITY;
	self->Gravity = 1. / 8;;
	// [RH] Avoid some precision loss by scaling the velocity directly
#if 0
	// This is the original code, for reference.
	a.ngle_t angle = self->angle>>ANGLETOF.INESHIFT;
	self->velx = F.ixedMul(7*F.RACUNIT, f.inecosine[angle]);
	self->vely = F.ixedMul(7*F.RACUNIT, f.inesine[angle]);
#else
	double velscale = 7 / self->Vel.XY().Length();
	self->Vel.X *= velscale;
	self->Vel.Y *= velscale;
#endif
	self->Vel.Z *= 0.5;

	// [BC] If we're the server, tell clients to move the object.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
	{
		SERVERCOMMANDS_SetThingFlags( self, FLAGSET_FLAGS );
		SERVERCOMMANDS_SetThingFlags( self, FLAGSET_FLAGS2 );
		SERVERCOMMANDS_MoveThingExact( self, CM_X|CM_Y|CM_Z|CM_VELX|CM_VELY|CM_VELZ );
		SERVERCOMMANDS_SetThingGravity( self );
	}

	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_MaceBallImpact
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_MaceBallImpact)
{
	PARAM_ACTION_PROLOGUE;

	// [BC] Let the server handle this.
	if ( NETWORK_InClientMode() )
	{
		// We need to make sure the ball doesn't temporary go into it's death frame.
		if ( self->flags & MF_INBOUNCE )
			self->SetState (self->SpawnState);

		return 0;
	}

	if ((self->health != MAGIC_JUNK) && (self->flags & MF_INBOUNCE))
	{ // Bounce
		self->health = MAGIC_JUNK;
		self->Vel.Z *= 0.75;
		self->BounceFlags = BOUNCE_None;

		// [BC] If we're the server, tell clients to move the object.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		{
			SERVERCOMMANDS_SetThingFlags( self, FLAGSET_FLAGS2 );
			SERVERCOMMANDS_SetThingState( self, STATE_SPAWN );
			SERVERCOMMANDS_MoveThingExact( self, CM_X|CM_Y|CM_Z|CM_VELX|CM_VELY|CM_VELZ );
			SERVERCOMMANDS_SoundActor( self, CHAN_BODY, "weapons/macebounce", 1, ATTN_NORM );
		}

		self->SetState (self->SpawnState);
		S_Sound (self, CHAN_BODY, "weapons/macebounce", 1, ATTN_NORM);
	}
	else
	{ // Explode
		self->Vel.Zero();
		self->flags |= MF_NOGRAVITY;
		self->Gravity = 1;
		S_Sound (self, CHAN_BODY, "weapons/macehit", 1, ATTN_NORM);

		// [BC] If we're the server, tell clients to move the object.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		{
			SERVERCOMMANDS_SetThingFlags( self, FLAGSET_FLAGS );
			SERVERCOMMANDS_SetThingFlags( self, FLAGSET_FLAGS2 );
			SERVERCOMMANDS_MoveThingExact( self, CM_X|CM_Y|CM_Z|CM_VELX|CM_VELY|CM_VELZ );
			SERVERCOMMANDS_SetThingGravity( self );
			SERVERCOMMANDS_SoundActor( self, CHAN_BODY, "weapons/macebounce", 1, ATTN_NORM );
		}
	}
	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_MaceBallImpact2
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_MaceBallImpact2)
{
	PARAM_ACTION_PROLOGUE;

	AActor *tiny;

	// [BC] Let the server handle this.
	if ( NETWORK_InClientMode() )
	{
		// We need to make sure the ball doesn't temporary go into it's death frame.
		if ( self->flags & MF_INBOUNCE )
		{
			double floordist = self->Z() - self->floorz;
			double ceildist = self->ceilingz - self->Z();
			double vel;

			if (floordist <= ceildist)
			{
				vel = FIXED2DBL ( MulScale32 ( FLOAT2FIXED ( self->Vel.Z ), FLOAT2FIXED ( self->Sector->floorplane.fC() )) );
			}
			else
			{
				vel = FIXED2DBL ( MulScale32 ( FLOAT2FIXED ( self->Vel.Z ), FLOAT2FIXED ( self->Sector->ceilingplane.fC() )) );
			}
			if (vel >= 2)
				self->SetState (self->SpawnState);
		}

		return 0;
	}

	if ((self->Z() <= self->floorz) && P_HitFloor (self))
	{ // Landed in some sort of liquid
		// [BB]
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_DestroyThing( self );

		self->Destroy ();
		return 0;
	}
	if (self->flags & MF_INBOUNCE)
	{
		if (self->Vel.Z < 2)
		{
			goto boom;
		}

		// Bounce
		self->Vel.Z *= 0.75;
		self->SetState (self->SpawnState);

		// [BC] If we're the server, send the state change and move it.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_MoveThingExact( self, CM_X|CM_Y|CM_Z|CM_VELX|CM_VELY|CM_VELZ );

		tiny = Spawn("MaceFX3", self->Pos(), ALLOW_REPLACE);
		tiny->target = self->target;
		tiny->Angles.Yaw = self->Angles.Yaw + 90.;
		tiny->VelFromAngle(self->Vel.Z - 1.);
		tiny->Vel += { self->Vel.X * .5, self->Vel.Y * .5, self->Vel.Z };

		// [BC] If we're the server, spawn this missile.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_SpawnMissileExact( tiny );

		P_CheckMissileSpawn (tiny, self->radius);

		tiny = Spawn("MaceFX3", self->Pos(), ALLOW_REPLACE);
		tiny->target = self->target;
		tiny->Angles.Yaw = self->Angles.Yaw - 90.;
		tiny->VelFromAngle(self->Vel.Z - 1.);
		tiny->Vel += { self->Vel.X * .5, self->Vel.Y * .5, self->Vel.Z };

		// [BC] If we're the server, spawn this missile.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_SpawnMissileExact( tiny );

		P_CheckMissileSpawn (tiny, self->radius);
	}
	else
	{ // Explode
boom:
		self->Vel.Zero();
		self->flags |= MF_NOGRAVITY;
		self->BounceFlags = BOUNCE_None;
		self->Gravity = 1;

		// [BC] If we're the server, tell clients to move the object.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		{
			SERVERCOMMANDS_SetThingState( self, STATE_DEATH );
			SERVERCOMMANDS_MoveThingExact( self, CM_X|CM_Y|CM_Z|CM_VELX|CM_VELY|CM_VELZ );
			SERVERCOMMANDS_SetThingFlags( self, FLAGSET_FLAGS );
			SERVERCOMMANDS_SetThingFlags( self, FLAGSET_FLAGS2 );
			SERVERCOMMANDS_SetThingGravity( self );
		}
	}
	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_FireMacePL2
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireMacePL2)
{
	PARAM_ACTION_PROLOGUE;

	AActor *mo;
	player_t *player;
	FTranslatedLineTarget t;

	if (NULL == (player = self->player))
	{
		return 0;
	}

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return 0;
	}

	// [BC] If we're the client, play the sound and get out.
	if ( NETWORK_InClientMode() )
	{
		S_Sound( player->mo, CHAN_WEAPON, "weapons/maceshoot", 1, ATTN_NORM );
		return 0;
	}

	mo = P_SpawnPlayerMissile (self, 0,0,0, RUNTIME_CLASS(AMaceFX4), self->Angles.Yaw, &t);
	if (mo)
	{
		mo->Vel += self->Vel.XY();
		mo->Vel.Z = 2 - player->mo->Angles.Pitch.TanClamped();
		if (t.linetarget && !t.unlinked)
		{
			mo->tracer = t.linetarget;
		}
	}
	S_Sound (self, CHAN_WEAPON, "weapons/maceshoot", 1, ATTN_NORM);

	// [BC] If we're the server, play the sound.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
	{
		SERVERCOMMANDS_MoveThingExact( mo, CM_X|CM_Y|CM_Z|CM_VELX|CM_VELY|CM_VELZ );
		SERVERCOMMANDS_WeaponSound( ULONG( player - players ), "weapons/maceshoot", ULONG( player - players ), SVCF_SKIPTHISCLIENT );
	}

	if ( player->cheats2 & CF2_SPREAD )
	{
		mo = P_SpawnPlayerMissile(self, 0, 0, 0, RUNTIME_CLASS(AMaceFX4), self->Angles.Yaw + 15, &t);
		if (mo)
		{
			mo->Vel += self->Vel.XY();
			mo->Vel.Z = 2 - player->mo->Angles.Pitch.TanClamped();
			if (t.linetarget && !t.unlinked)
			{
				mo->tracer = t.linetarget;
			}

			// [BC] If we're the server, play the sound.
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
				SERVERCOMMANDS_MoveThingExact( mo, CM_X|CM_Y|CM_Z|CM_VELX|CM_VELY|CM_VELZ );
		}

		mo = P_SpawnPlayerMissile(self, 0, 0, 0, RUNTIME_CLASS(AMaceFX4), self->Angles.Yaw - 15, &t);
		if (mo)
		{
			mo->Vel += self->Vel.XY();
			mo->Vel.Z = 2 - player->mo->Angles.Pitch.TanClamped();
			if (t.linetarget && !t.unlinked)
			{
				mo->tracer = t.linetarget;
			}

			// [BC] If we're the server, play the sound.
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
				SERVERCOMMANDS_MoveThingExact( mo, CM_X|CM_Y|CM_Z|CM_VELX|CM_VELY|CM_VELZ );
		}
	}
	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_DeathBallImpact
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_DeathBallImpact)
{
	PARAM_ACTION_PROLOGUE;

	int i;
	AActor *target;
	DAngle angle = 0.;
	bool newAngle;
	FTranslatedLineTarget t;

	// [BC] Let the server handle this.
	if ( NETWORK_InClientMode() )
	{
		// We need to make sure the self doesn't temporary go into it's death frame.
		if ( self->flags & MF_INBOUNCE )
		{
			double floordist = self->Z() - self->floorz;
			double ceildist = self->ceilingz - self->Z();
			double vel;

			if (floordist <= ceildist)
			{
				vel = FIXED2DBL ( MulScale32 ( FLOAT2FIXED ( self->Vel.Z ), FLOAT2FIXED ( self->Sector->floorplane.fC() )) );
			}
			else
			{
				vel = FIXED2DBL ( MulScale32 ( FLOAT2FIXED ( self->Vel.Z ), FLOAT2FIXED ( self->Sector->ceilingplane.fC() )) );
			}
			if (vel >= 2)
				self->SetState (self->SpawnState);
		}

		return 0;
	}

	if ((self->Z() <= self->floorz) && P_HitFloor (self))
	{ // Landed in some sort of liquid
		self->Destroy ();
		return 0;
	}
	if (self->flags & MF_INBOUNCE)
	{
		if (self->Vel.Z < 2)
		{
			goto boom;
		}

		// Bounce
		newAngle = false;
		target = self->tracer;
		if (target)
		{
			if (!(target->flags&MF_SHOOTABLE))
			{ // Target died
				self->tracer = NULL;
			}
			else
			{ // Seek
				angle = self->AngleTo(target);
				newAngle = true;
			}
		}
		else
		{ // Find new target
			angle = 0.;
			for (i = 0; i < 16; i++)
			{
				P_AimLineAttack (self, angle, 640., &t, 0., ALF_NOFRIENDS|ALF_PORTALRESTRICT, NULL, self->target);
				if (t.linetarget && self->target != t.linetarget)
				{
					self->tracer = t.linetarget;
					angle = t.angleFromSource;
					newAngle = true;
					break;
				}
				angle += 22.5;
			}
		}
		if (newAngle)
		{
			self->Angles.Yaw = angle;
			self->VelFromAngle();
		}
		self->SetState (self->SpawnState);
		S_Sound (self, CHAN_BODY, "weapons/macestop", 1, ATTN_NORM);

		// [BC] If we're the server, send the state change and move it.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		{
			SERVERCOMMANDS_MoveThingExact( self, CM_X|CM_Y|CM_Z|CM_VELX|CM_VELY|CM_VELZ );
			SERVERCOMMANDS_SoundActor( self, CHAN_BODY, "weapons/macestop", 1, ATTN_NORM );
		}
	}
	else
	{ // Explode
boom:
		self->Vel.Zero();
		self->flags |= MF_NOGRAVITY;
		self->Gravity = 1;
		S_Sound (self, CHAN_BODY, "weapons/maceexplode", 1, ATTN_NORM);

		// [BC] If we're the server, do some stuff.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		{
			SERVERCOMMANDS_SetThingState( self, STATE_DEATH );
			SERVERCOMMANDS_MoveThing( self, CM_X|CM_Y|CM_Z|CM_VELX|CM_VELY|CM_VELZ );
			SERVERCOMMANDS_SetThingFlags( self, FLAGSET_FLAGS );
			SERVERCOMMANDS_SetThingFlags( self, FLAGSET_FLAGS2 );
			SERVERCOMMANDS_SetThingGravity( self );
			SERVERCOMMANDS_SoundActor( self, CHAN_BODY, "weapons/maceexplode", 1, ATTN_NORM );
		}
	}
	return 0;
}


// Blaster FX 1 -------------------------------------------------------------

//----------------------------------------------------------------------------
//
// Thinker for the ultra-fast blaster PL2 ripper-spawning missile.
//
//----------------------------------------------------------------------------

class ABlasterFX1 : public AFastProjectile
{
	DECLARE_CLASS(ABlasterFX1, AFastProjectile)
public:
	void Effect ();
	int DoSpecialDamage (AActor *target, int damage, FName damagetype);
};

int ABlasterFX1::DoSpecialDamage (AActor *target, int damage, FName damagetype)
{
	if (target->IsKindOf (PClass::FindClass ("Ironlich")))
	{ // Less damage to Ironlich bosses
		damage = pr_bfx1() & 1;
		if (!damage)
		{
			return -1;
		}
	}
	return damage;
}

void ABlasterFX1::Effect ()
{
	if (pr_bfx1t() < 64)
	{
		Spawn("BlasterSmoke", PosAtZ(MAX(Z() - 8., floorz)), ALLOW_REPLACE);
	}
}

IMPLEMENT_CLASS(ABlasterFX1)

// Ripper -------------------------------------------------------------------


class ARipper : public AActor
{
	DECLARE_CLASS (ARipper, AActor)
public:
	int DoSpecialDamage (AActor *target, int damage, FName damagetype);
};

IMPLEMENT_CLASS(ARipper)

int ARipper::DoSpecialDamage (AActor *target, int damage, FName damagetype)
{
	if (target->IsKindOf (PClass::FindClass ("Ironlich")))
	{ // Less damage to Ironlich bosses
		damage = pr_ripd() & 1;
		if (!damage)
		{
			return -1;
		}
	}
	return damage;
}

//----------------------------------------------------------------------------
//
// PROC A_FireBlasterPL1
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireBlasterPL1)
{
	PARAM_ACTION_PROLOGUE;

	DAngle angle;
	int damage;
	player_t *player;

	if (NULL == (player = self->player))
	{
		return 0;
	}

	AWeapon *weapon = self->player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return 0;
	}
	// [BC] If we're the client, just play the sound and get out.
	if ( NETWORK_InClientMode() )
	{
		S_Sound( player->mo, CHAN_WEAPON, "weapons/blastershoot", 1, ATTN_NORM );
		return 0;
	}

	DAngle pitch = P_BulletSlope(self);
	damage = pr_fb1.HitDice (4);
	angle = self->Angles.Yaw;
	if (player->refire)
	{
		angle += pr_fb1.Random2() * (5.625 / 256);
	}
	P_LineAttack (self, angle, PLAYERMISSILERANGE, pitch, damage, NAME_Hitscan, "BlasterPuff");

	// [BC] Apply spread.
	if ( player->cheats2 & CF2_SPREAD )
	{
		P_LineAttack( self, angle + 15., PLAYERMISSILERANGE, pitch, damage, NAME_Hitscan, "BlasterPuff");
		P_LineAttack( self, angle - 15., PLAYERMISSILERANGE, pitch, damage, NAME_Hitscan, "BlasterPuff");
	}

	// [BB] If the player hit a player with his attack, potentially give him a medal.
	PLAYER_CheckStruckPlayer ( self );

	S_Sound (self, CHAN_WEAPON, "weapons/blastershoot", 1, ATTN_NORM);

	// [BC] If we're the server, tell clients that a weapon is being fired.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		SERVERCOMMANDS_WeaponSound( ULONG( player - players ), "weapons/blastershoot", ULONG( player - players ), SVCF_SKIPTHISCLIENT );

	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_SpawnRippers
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_SpawnRippers)
{
	PARAM_ACTION_PROLOGUE;

	unsigned int i;
	DAngle angle;
	AActor *ripper;

	for(i = 0; i < 8; i++)
	{
		ripper = Spawn<ARipper> (self->Pos(), ALLOW_REPLACE);
		angle = i*45.;
		ripper->target = self->target;
		ripper->Angles.Yaw = angle;
		ripper->VelFromAngle();
		P_CheckMissileSpawn (ripper, self->radius);
	}
	return 0;
}

// --- Skull rod ------------------------------------------------------------


// Horn Rod FX 2 ------------------------------------------------------------

class AHornRodFX2 : public AActor
{
	DECLARE_CLASS (AHornRodFX2, AActor)
public:
	int DoSpecialDamage (AActor *target, int damage, FName damagetype);
};

IMPLEMENT_CLASS (AHornRodFX2)

int AHornRodFX2::DoSpecialDamage (AActor *target, int damage, FName damagetype)
{
	if (target->IsKindOf (PClass::FindClass("Sorcerer2")) && pr_hrfx2() < 96)
	{ // D'Sparil teleports away
		P_DSparilTeleport (target);
		return -1;
	}
	return damage;
}

// Rain pillar 1 ------------------------------------------------------------

class ARainPillar : public AActor
{
	DECLARE_CLASS (ARainPillar, AActor)
public:
	int DoSpecialDamage (AActor *target, int damage, FName damagetype);
};

IMPLEMENT_CLASS (ARainPillar)

int ARainPillar::DoSpecialDamage (AActor *target, int damage, FName damagetype)
{
	if (target->flags2 & MF2_BOSS)
	{ // Decrease damage for bosses
		damage = (pr_rp() & 7) + 1;
	}
	return damage;
}

// Rain tracker "inventory" item --------------------------------------------

class ARainTracker : public AInventory
{
	DECLARE_CLASS (ARainTracker, AInventory)
public:
	
	void Serialize(FSerializer &arc);
	TObjPtr<AActor> Rain1, Rain2;
};

IMPLEMENT_CLASS (ARainTracker)
	
void ARainTracker::Serialize(FSerializer &arc)
{
	Super::Serialize (arc);
	arc("rain1", Rain1)
		("rain2", Rain2);
}

//----------------------------------------------------------------------------
//
// PROC A_FireSkullRodPL1
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireSkullRodPL1)
{
	PARAM_ACTION_PROLOGUE;

	AActor *mo;
	player_t *player;

	if (NULL == (player = self->player))
	{
		return 0;
	}

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return 0;
	}

	// [BC] Weapons are handled by the server.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	mo = P_SpawnPlayerMissile (self, PClass::FindActor("HornRodFX1"));
	// Randomize the first frame
	if (mo && pr_fsr1() > 128)
	{
		mo->SetState (mo->state->GetNextState());
	}

	// [BC] Apply spread.
	if ( player->cheats2 & CF2_SPREAD )
	{
		mo = P_SpawnPlayerMissile( self, PClass::FindActor("HornRodFX1"), self->Angles.Yaw + 15 );
		// Randomize the first frame
		if (mo && pr_fsr1() > 128)
		{
			mo->SetState (mo->state->GetNextState());
		}

		mo = P_SpawnPlayerMissile( self, PClass::FindActor("HornRodFX1"), self->Angles.Yaw - 15);
		// Randomize the first frame
		if (mo && pr_fsr1() > 128)
		{
			mo->SetState (mo->state->GetNextState());
		}
	}

	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_FireSkullRodPL2
//
// The special2 field holds the player number that shot the rain missile.
// The special1 field holds the id of the rain sound.
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireSkullRodPL2)
{
	PARAM_ACTION_PROLOGUE;

	player_t *player;
	AActor *MissileActor;
	FTranslatedLineTarget t;

	if (NULL == (player = self->player))
	{
		return 0;
	}
	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return 0;
	}

	// [BC] Weapons are handled by the server.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	P_SpawnPlayerMissile (self, 0,0,0, RUNTIME_CLASS(AHornRodFX2), self->Angles.Yaw, &t, &MissileActor);
	// Use MissileActor instead of the return value from
	// P_SpawnPlayerMissile because we need to give info to the mobj
	// even if it exploded immediately.
	if (MissileActor != NULL)
	{
		MissileActor->special2 = (int)(player - players);

		// [BB] Set the special, otherwise the translation of the rain spawned later will be wrong.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_SetThingSpecial2( MissileActor );

		if (t.linetarget && !t.unlinked)
		{
			MissileActor->tracer = t.linetarget;
		}
		S_Sound (MissileActor, CHAN_WEAPON, "weapons/hornrodpowshoot", 1, ATTN_NORM);

		// [BC] If we're the server, play this sound.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_SoundActor( MissileActor, CHAN_WEAPON, "weapons/hornrodpowshoot", 1, ATTN_NORM );
	}

	// [BC] Apply spread.
	if ( player->cheats2 & CF2_SPREAD )
	{
		P_SpawnPlayerMissile (self, 0,0,0, RUNTIME_CLASS(AHornRodFX2), self->Angles.Yaw + 15, &t, &MissileActor);
		// Use MissileActor instead of the return value from
		// P_SpawnPlayerMissile because we need to give info to the mobj
		// even if it exploded immediately.
		if (MissileActor != NULL)
		{
			MissileActor->special2 = (int)(player - players);

			// [BB] Set the special, otherwise the translation of the rain spawned later will be wrong.
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
				SERVERCOMMANDS_SetThingSpecial2( MissileActor );

			if (t.linetarget && !t.unlinked)
			{
				MissileActor->tracer = t.linetarget;
			}
			S_Sound (MissileActor, CHAN_WEAPON, "weapons/hornrodpowshoot", 1, ATTN_NORM);

			// [BC] If we're the server, play this sound.
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
				SERVERCOMMANDS_SoundActor( MissileActor, CHAN_WEAPON, "weapons/hornrodpowshoot", 1, ATTN_NORM );
		}

		P_SpawnPlayerMissile (self, 0,0,0, RUNTIME_CLASS(AHornRodFX2), self->Angles.Yaw - 15, &t, &MissileActor);
		// Use MissileActor instead of the return value from
		// P_SpawnPlayerMissile because we need to give info to the mobj
		// even if it exploded immediately.
		if (MissileActor != NULL)
		{
			MissileActor->special2 = (int)(player - players);

			// [BB] Set the special, otherwise the translation of the rain spawned later will be wrong.
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
				SERVERCOMMANDS_SetThingSpecial2( MissileActor );

			if (t.linetarget && !t.unlinked)
			{
				MissileActor->tracer = t.linetarget;
			}
			S_Sound (MissileActor, CHAN_WEAPON, "weapons/hornrodpowshoot", 1, ATTN_NORM);

			// [BC] If we're the server, play this sound.
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
				SERVERCOMMANDS_SoundActor( MissileActor, CHAN_WEAPON, "weapons/hornrodpowshoot", 1, ATTN_NORM );
		}
	}
	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_AddPlayerRain
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_AddPlayerRain)
{
	PARAM_ACTION_PROLOGUE;

	ARainTracker *tracker;

	// [BC] Let the server spawn rain.
	if ( NETWORK_InClientMode() )
		return 0;

	if (self->target == NULL || self->target->health <= 0)
	{ // Shooter is dead or nonexistant
		return 0;
	}

	tracker = self->target->FindInventory<ARainTracker> ();

	// They player is only allowed two rainstorms at a time. Shooting more
	// than that will cause the oldest one to terminate.
	if (tracker != NULL)
	{
		if (tracker->Rain1 && tracker->Rain2)
		{ // Terminate an active rain
			if (tracker->Rain1->health < tracker->Rain2->health)
			{
				if (tracker->Rain1->health > 16)
				{
					tracker->Rain1->health = 16;

					// [Dusk] Sync the new health value to clients.
					if ( NETWORK_GetState() == NETSTATE_SERVER )
						SERVERCOMMANDS_SetThingHealth( tracker->Rain1 );
				}
				tracker->Rain1 = NULL;
			}
			else
			{
				if (tracker->Rain2->health > 16)
				{
					tracker->Rain2->health = 16;

					// [Dusk] Sync the new health value to clients.
					if ( NETWORK_GetState() == NETSTATE_SERVER )
						SERVERCOMMANDS_SetThingHealth( tracker->Rain2 );
				}
				tracker->Rain2 = NULL;
			}
		}
	}
	else
	{
		tracker = static_cast<ARainTracker *> (self->target->GiveInventoryType (RUNTIME_CLASS(ARainTracker)));
	}
	// Add rain mobj to list
	if (tracker->Rain1)
	{
		tracker->Rain2 = self;
	}
	else
	{
		tracker->Rain1 = self;
	}
	self->special1 = S_FindSound ("misc/rain");
	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_SkullRodStorm
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_SkullRodStorm)
{
	PARAM_ACTION_PROLOGUE;

	AActor *mo;
	ARainTracker *tracker;

	if (self->health-- == 0)
	{
		S_StopSound (self, CHAN_BODY);
		if (self->target == NULL)
		{ // Player left the game
			self->Destroy ();
			return 0;
		}
		tracker = self->target->FindInventory<ARainTracker> ();
		if (tracker != NULL)
		{
			if (tracker->Rain1 == self)
			{
				tracker->Rain1 = NULL;
			}
			else if (tracker->Rain2 == self)
			{
				tracker->Rain2 = NULL;
			}
		}
		self->Destroy ();
		return 0;
	}
	if (pr_storm() < 25)
	{ // Fudge rain frequency
		return 0;
	}
	double xo = ((pr_storm() & 127) - 64);
	double yo = ((pr_storm() & 127) - 64);
	DVector3 pos = self->Vec2OffsetZ(xo, yo, ONCEILINGZ);
	mo = Spawn<ARainPillar> (pos, ALLOW_REPLACE);
	// We used bouncecount to store the 3D floor index in A_HideInCeiling
	if (!mo) return 0;
	if (mo->Sector->PortalGroup != self->Sector->PortalGroup)
	{
		// spawning this through a portal will never work right so abort right away.
		mo->Destroy();
		return 0;
	}
	if (self->bouncecount >= 0 && (unsigned)self->bouncecount < self->Sector->e->XFloor.ffloors.Size())
		pos.Z = self->Sector->e->XFloor.ffloors[self->bouncecount]->bottom.plane->ZatPoint(mo);
	else
		pos.Z = self->Sector->ceilingplane.ZatPoint(mo);
	int moceiling = P_Find3DFloor(NULL, pos, false, false, pos.Z);
	if (moceiling >= 0) mo->SetZ(pos.Z - mo->Height);
	mo->Translation = ( NETWORK_GetState( ) != NETSTATE_SINGLE ) ?	TRANSLATION(TRANSLATION_RainPillar,self->special2) : 0;
	mo->target = self->target;
	mo->Vel.X = MinVel; // Force collision detection
	mo->Vel.Z = -mo->Speed;
	mo->special2 = self->special2; // Transfer player number
	P_CheckMissileSpawn (mo, self->radius);
	if (self->special1 != -1 && !S_IsActorPlayingSomething (self, CHAN_BODY, -1))
	{
		S_Sound (self, CHAN_BODY|CHAN_LOOP, self->special1, 1, ATTN_NORM);
	}
	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_RainImpact
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_RainImpact)
{
	PARAM_ACTION_PROLOGUE;
	if (self->Z() > self->floorz)
	{
		self->SetState (self->FindState("NotFloor"));
	}
	else if (pr_impact() < 40)
	{
		P_HitFloor (self);
	}
	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_HideInCeiling
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_HideInCeiling)
{
	PARAM_ACTION_PROLOGUE;

	// We use bouncecount to store the 3D floor index
	double foo;
	for (int i = self->Sector->e->XFloor.ffloors.Size() - 1; i >= 0; i--)
	{
		F3DFloor * rover = self->Sector->e->XFloor.ffloors[i];
		if (!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS)) continue;

		if ((foo = rover->bottom.plane->ZatPoint(self)) >= self->Top())
		{
			self->SetZ(foo + 4, false);
			self->bouncecount = i;
			return 0;
		}
	}
	self->bouncecount = -1;
	self->SetZ(self->ceilingz + 4, false);
	return 0;
}

// --- Phoenix Rod ----------------------------------------------------------

class APhoenixRod : public AWeapon
{
	DECLARE_CLASS (APhoenixRod, AWeapon)
public:
	
	void Serialize(FSerializer &arc)
	{
		Super::Serialize (arc);
		arc("flamecount", FlameCount);
	}
	int FlameCount;		// for flamethrower duration
};

class APhoenixRodPowered : public APhoenixRod
{
	DECLARE_CLASS (APhoenixRodPowered, APhoenixRod)
public:
	void EndPowerup ();
};

IMPLEMENT_CLASS (APhoenixRod)
IMPLEMENT_CLASS (APhoenixRodPowered)

void APhoenixRodPowered::EndPowerup ()
{
	DepleteAmmo (bAltFire);
	Owner->player->refire = 0;
	S_StopSound (Owner, CHAN_WEAPON);
	Owner->player->ReadyWeapon = SisterWeapon;
	P_SetPsprite(Owner->player, PSP_WEAPON, SisterWeapon->GetReadyState());
}

class APhoenixFX1 : public AActor
{
	DECLARE_CLASS (APhoenixFX1, AActor)
public:
	int DoSpecialDamage (AActor *target, int damage, FName damagetype);
};


IMPLEMENT_CLASS (APhoenixFX1)

int APhoenixFX1::DoSpecialDamage (AActor *target, int damage, FName damagetype)
{
	if (target->IsKindOf (PClass::FindClass("Sorcerer2")) && pr_hrfx2() < 96)
	{ // D'Sparil teleports away
		P_DSparilTeleport (target);
		return -1;
	}
	return damage;
}

// Phoenix FX 2 -------------------------------------------------------------

class APhoenixFX2 : public AActor
{
	DECLARE_CLASS (APhoenixFX2, AActor)
public:
	int DoSpecialDamage (AActor *target, int damage, FName damagetype);
};

IMPLEMENT_CLASS (APhoenixFX2)

int APhoenixFX2::DoSpecialDamage (AActor *target, int damage, FName damagetype)
{
	if (target->player && pr_pfx2 () < 128)
	{ // Freeze player for a bit
		target->reactiontime += 4;
	}
	return damage;
}

//----------------------------------------------------------------------------
//
// PROC A_FirePhoenixPL1
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FirePhoenixPL1)
{
	PARAM_ACTION_PROLOGUE;

	player_t *player;

	if (NULL == (player = self->player))
	{
		return 0;
	}

	AWeapon *weapon = self->player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return 0;
	}

	// [BC] Weapons are handled by the server.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	P_SpawnPlayerMissileWithPossibleSpread (self, RUNTIME_CLASS(APhoenixFX1)); // [BB] Spread
	self->Thrust(self->Angles.Yaw + 180, 4);

	// [BC] Push the player back even more if they are using spread.
	if ( player->cheats2 & CF2_SPREAD )
	{
		self->Thrust(self->Angles.Yaw + 180, 8);
	}

	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_PhoenixPuff
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_PhoenixPuff)
{
	PARAM_ACTION_PROLOGUE;

	AActor *puff;
	DAngle angle;

	//[RH] Heretic never sets the target for seeking
	//P_SeekerMissile (self, 5, 10);
	puff = Spawn("PhoenixPuff", self->Pos(), ALLOW_REPLACE);
	angle = self->Angles.Yaw + 90;
	puff->Vel = DVector3(angle.ToVector(1.3), 0);

	puff = Spawn("PhoenixPuff", self->Pos(), ALLOW_REPLACE);
	angle = self->Angles.Yaw - 90;
	puff->Vel = DVector3(angle.ToVector(1.3), 0);
	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_InitPhoenixPL2
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_InitPhoenixPL2)
{
	PARAM_ACTION_PROLOGUE;

	if (self->player != NULL)
	{
		APhoenixRod *flamethrower = static_cast<APhoenixRod *> (self->player->ReadyWeapon);
		if (flamethrower != NULL)
		{
			flamethrower->FlameCount = FLAME_THROWER_TICS;
		}
	}
	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_FirePhoenixPL2
//
// Flame thrower effect.
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FirePhoenixPL2)
{
	PARAM_ACTION_PROLOGUE;

	AActor *mo;

	double slope;
	FSoundID soundid;
	player_t *player;
	APhoenixRod *flamethrower;

	if (nullptr == (player = self->player))
	{
		return 0;
	}

	soundid = "weapons/phoenixpowshoot";

	flamethrower = static_cast<APhoenixRod *> (player->ReadyWeapon);
	if (flamethrower == nullptr || --flamethrower->FlameCount == 0)
	{ // Out of flame
		P_SetPsprite(player, PSP_WEAPON, flamethrower->FindState("Powerdown"));
		player->refire = 0;
		S_StopSound (self, CHAN_WEAPON);
		return 0;
	}
	// [BC] If we're the client, just play the sound and get out.
	if ( NETWORK_InClientMode() )
	{
		if (!player->refire || !S_IsActorPlayingSomething (player->mo, CHAN_WEAPON, -1))
		{
			S_Sound (player->mo, CHAN_WEAPON|CHAN_LOOP, soundid, 1, ATTN_NORM);
		}
		return 0;
	}


	slope = -self->Angles.Pitch.TanClamped();
	double xo = pr_fp2.Random2() / 128.;
	double yo = pr_fp2.Random2() / 128.;
	DVector3 pos = self->Vec3Offset(xo, yo, 26 + slope - self->Floorclip);

	slope += 0.1;
	mo = Spawn("PhoenixFX2", pos, ALLOW_REPLACE);
	mo->target = self;
	mo->Angles.Yaw = self->Angles.Yaw;
	mo->VelFromAngle();
	mo->Vel += self->Vel.XY();
	mo->Vel.Z = mo->Speed * slope;
	if (!player->refire || !S_IsActorPlayingSomething (self, CHAN_WEAPON, -1))
	{
		S_Sound (self, CHAN_WEAPON|CHAN_LOOP, soundid, 1, ATTN_NORM);
	}	

	// [BC] Apply spread.
	if ( player->cheats2 & CF2_SPREAD )
	{
		mo = Spawn("PhoenixFX2", pos, ALLOW_REPLACE);
		mo->target = self;
		mo->Angles.Yaw = self->Angles.Yaw + 15;
		mo->VelFromAngle();
		mo->Vel += self->Vel.XY();
		mo->Vel.Z = mo->Speed * slope;
		if (!player->refire || !S_IsActorPlayingSomething (self, CHAN_WEAPON, -1))
		{
			S_Sound (self, CHAN_WEAPON|CHAN_LOOP, soundid, 1, ATTN_NORM);
		}

		mo = Spawn("PhoenixFX2", pos, ALLOW_REPLACE);
		mo->target = self;
		mo->Angles.Yaw = self->Angles.Yaw - 15;
		mo->VelFromAngle();
		mo->Vel += self->Vel.XY();
		mo->Vel.Z = mo->Speed * slope;
		if (!player->refire || !S_IsActorPlayingSomething (self, CHAN_WEAPON, -1))
		{
			S_Sound (self, CHAN_WEAPON|CHAN_LOOP, soundid, 1, ATTN_NORM);
		}
	}

	// [BC] If we're the server, spawn this missile.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		SERVERCOMMANDS_SpawnMissile( mo );

	P_CheckMissileSpawn (mo, self->radius);
	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_ShutdownPhoenixPL2
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_ShutdownPhoenixPL2)
{
	PARAM_ACTION_PROLOGUE;

	player_t *player;

	if (NULL == (player = self->player))
	{
		return 0;
	}
	S_StopSound (self, CHAN_WEAPON);
	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return 0;
	}
	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_FlameEnd
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FlameEnd)
{
	PARAM_ACTION_PROLOGUE;

	self->Vel.Z += 1.5;
	return 0;
}

//----------------------------------------------------------------------------
//
// PROC A_FloatPuff
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FloatPuff)
{
	PARAM_ACTION_PROLOGUE;

	self->Vel.Z += 1.8;
	return 0;
}

