/*
#include "actor.h"
#include "info.h"
#include "p_local.h"
#include "s_sound.h"
#include "a_action.h"
#include "m_random.h"
#include "a_hexenglobal.h"
#include "thingdef/thingdef.h"
*/

static FRandom pr_boom ("BishopBoom");
static FRandom pr_atk ("BishopAttack");
static FRandom pr_decide ("BishopDecide");
static FRandom pr_doblur ("BishopDoBlur");
static FRandom pr_sblur ("BishopSpawnBlur");
static FRandom pr_pain ("BishopPainBlur");

//============================================================================
//
// A_BishopAttack
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_BishopAttack)
{
	PARAM_ACTION_PROLOGUE;

	// [BB] This is server-side.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	if (!self->target)
	{
		return 0;
	}
	S_Sound (self, CHAN_BODY, self->AttackSound, 1, ATTN_NORM);

	// [BB] If we're the server, tell the clients to play the sound.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		SERVERCOMMANDS_SoundActor( self, CHAN_BODY, S_GetName(self->AttackSound), 1, ATTN_NORM );

	if (self->CheckMeleeRange())
	{
		int damage = pr_atk.HitDice (4);
		int newdam = P_DamageMobj (self->target, self, self, damage, NAME_Melee);
		P_TraceBleed (newdam > 0 ? newdam : damage, self->target, self);
		return 0;
	}
	self->special1 = (pr_atk() & 3) + 5;
	return 0;
}

//============================================================================
//
// A_BishopAttack2
//
//		Spawns one of a string of bishop missiles
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_BishopAttack2)
{
	PARAM_ACTION_PROLOGUE;

	AActor *mo;

	// [BB] This is server-side.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	if (!self->target || !self->special1)
	{
		self->special1 = 0;
		self->SetState (self->SeeState);

		// [BB] If we're the server, tell the clients of the state change.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_SetThingState( self, STATE_SEE );

		return 0;
	}
	mo = P_SpawnMissile (self, self->target, PClass::FindActor("BishopFX"), NULL, true); // [BB] Inform clients
	if (mo != NULL)
	{
		mo->tracer = self->target;
	}
	self->special1--;
	return 0;
}

//============================================================================
//
// A_BishopMissileWeave
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_BishopMissileWeave)
{
	PARAM_ACTION_PROLOGUE;

	A_Weave(self, 2, 2, 2*FRACUNIT, FRACUNIT);
	return 0;
}

//============================================================================
//
// A_BishopDecide
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_BishopDecide)
{
	PARAM_ACTION_PROLOGUE;

	// [BB] This is server-side.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	if (pr_decide() < 220)
	{
		return 0;
	}
	else
	{
		// [BB] If we're the server, tell the clients to update this thing's state.
		if ( ( NETWORK_GetState( ) == NETSTATE_SERVER ) )
			SERVERCOMMANDS_SetThingFrame( self, self->FindState ("Blur") );

		self->SetState (self->FindState ("Blur"));
	}
	return 0;
}

//============================================================================
//
// A_BishopDoBlur
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_BishopDoBlur)
{
	PARAM_ACTION_PROLOGUE;

	// [BB] This is server-side.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	self->special1 = (pr_doblur() & 3) + 3; // Random number of blurs
	if (pr_doblur() < 120)
	{
		P_ThrustMobj (self, self->angle + ANG90, 11*FRACUNIT);
	}
	else if (pr_doblur() > 125)
	{
		P_ThrustMobj (self, self->angle - ANG90, 11*FRACUNIT);
	}
	else
	{ // Thrust forward
		P_ThrustMobj (self, self->angle, 11*FRACUNIT);
	}
	S_Sound (self, CHAN_BODY, "BishopBlur", 1, ATTN_NORM);

	// [BB] If we're the server, update the thing's velocity and play the sound.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
	{
		SERVERCOMMANDS_MoveThingExact( self, CM_VELX|CM_VELY );
		SERVERCOMMANDS_SoundActor( self, CHAN_BODY, "BishopBlur", 1, ATTN_NORM );
	}

	return 0;
}

//============================================================================
//
// A_BishopSpawnBlur
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_BishopSpawnBlur)
{
	PARAM_ACTION_PROLOGUE;

	// [BB] This is server-side.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	AActor *mo;

	if (!--self->special1)
	{
		self->velx = 0;
		self->vely = 0;

		// [BB] If we're the server, update the thing's velocity.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_MoveThingExact( self, CM_VELX|CM_VELY );

		if (pr_sblur() > 96)
		{
			// [BB] If we're the server, tell the clients of the state change.
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
				SERVERCOMMANDS_SetThingState( self, STATE_SEE );

			self->SetState (self->SeeState);
		}
		else
		{
			// [BB] If we're the server, tell the clients of the state change.
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
				SERVERCOMMANDS_SetThingState( self, STATE_MISSILE );
			
			self->SetState (self->MissileState);
		}
	}
	mo = Spawn ("BishopBlur", self->Pos(), ALLOW_REPLACE);
	if (mo)
	{
		mo->angle = self->angle;

		// [BB] If we're the server, tell the clients to spawn the thing and set its angle.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		{
			SERVERCOMMANDS_SpawnThing( mo );
			SERVERCOMMANDS_SetThingAngle( mo );
		}
	}
	return 0;
}

//============================================================================
//
// A_BishopChase
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_BishopChase)
{
	PARAM_ACTION_PROLOGUE;

	// [BB] This is server-side. The z coordinate seems to go out of sync
	// on client and server, if you make this client side.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	fixed_t newz = self->Z() - finesine[self->special2 << BOBTOFINESHIFT] * 4;
	self->special2 = (self->special2 + 4) & 63;
	newz += finesine[self->special2 << BOBTOFINESHIFT] * 4;
	self->SetZ(newz);

	// [BB] If we're the server, update the thing's z coordinate.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		SERVERCOMMANDS_MoveThingExact( self, CM_Z );

	return 0;
}

//============================================================================
//
// A_BishopPuff
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_BishopPuff)
{
	PARAM_ACTION_PROLOGUE;

	AActor *mo;

	mo = Spawn ("BishopPuff", self->PosPlusZ(40*FRACUNIT), ALLOW_REPLACE);
	if (mo)
	{
		mo->velz = FRACUNIT/2;
	}
	return 0;
}

//============================================================================
//
// A_BishopPainBlur
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_BishopPainBlur)
{
	PARAM_ACTION_PROLOGUE;

	AActor *mo;

	// [BB] This is server-side.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	if (pr_pain() < 64)
	{
		// [BB] If we're the server, tell the clients to update this thing's state.
		if ( ( NETWORK_GetState( ) == NETSTATE_SERVER ) )
			SERVERCOMMANDS_SetThingFrame( self, self->FindState ("Blur") );

		self->SetState (self->FindState ("Blur"));
		return 0;
	}
	fixed_t xo = (pr_pain.Random2() << 12);
	fixed_t yo = (pr_pain.Random2() << 12);
	fixed_t zo = (pr_pain.Random2() << 11);
	mo = Spawn ("BishopPainBlur", self->Vec3Offset(xo, yo, zo), ALLOW_REPLACE);
	if (mo)
	{
		mo->angle = self->angle;

		// [BB] If we're the server, tell the clients to spawn the thing and set its angle.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		{
			SERVERCOMMANDS_SpawnThing( mo );
			SERVERCOMMANDS_SetThingAngle( mo );
		}
	}
	return 0;
}
