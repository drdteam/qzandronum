/*
#include "actor.h"
#include "info.h"
#include "p_enemy.h"
#include "p_local.h"
#include "a_action.h"
#include "m_random.h"
#include "s_sound.h"
#include "thingdef/thingdef.h"
*/

static FRandom pr_dragonseek ("DragonSeek");
static FRandom pr_dragonflight ("DragonFlight");
static FRandom pr_dragonflap ("DragonFlap");
static FRandom pr_dragonfx2 ("DragonFX2");

DECLARE_ACTION(A_DragonFlight)

//============================================================================
//
// DragonSeek
//
//============================================================================

static void DragonSeek (AActor *actor, DAngle thresh, DAngle turnMax)
{
	int dir;
	double dist;
	DAngle delta;
	AActor *target;
	int i;
	angle_t bestAngle;
	angle_t angleToSpot, angleToTarget;
	AActor *mo;

	// [BB] Let the server do this.
	if ( NETWORK_InClientMode() )
	{
		return;
	}

	target = actor->tracer;
	if(target == NULL)
	{
		return;
	}
	dir = P_FaceMobj (actor, target, &delta);
	if (delta > thresh)
	{
		delta /= 2;
		if (delta > turnMax)
		{
			delta = turnMax;
		}
	}
	if (dir)
	{ // Turn clockwise
		actor->Angles.Yaw += delta;
	}
	else
	{ // Turn counter clockwise
		actor->Angles.Yaw -= delta;
	}
	actor->VelFromAngle();

	dist = actor->DistanceBySpeed(target, actor->Speed);
	if (actor->Top() < target->Z() ||
		target->Top() < actor->Z())
	{
		actor->Vel.Z = (target->Z() - actor->Z()) / dist;
	}
	// [BB] If we're the server, update the thing's velocity and angle.
	// Unfortunately there are sync issues, if we don't also update the actual position.
	// Is there a way to fix this without sending the position?
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		SERVERCOMMANDS_MoveThingExact( actor, CM_X|CM_Y|CM_Z|CM_ANGLE|CM_VELX|CM_VELY|CM_VELZ );

	if (target->flags&MF_SHOOTABLE && pr_dragonseek() < 64)
	{ // attack the destination mobj if it's attackable
		AActor *oldTarget;

		if (absangle(actor->_f_angle() - actor->__f_AngleTo(target)) < ANGLE_45/2)
		{
			oldTarget = actor->target;
			actor->target = target;
			if (actor->CheckMeleeRange ())
			{
				int damage = pr_dragonseek.HitDice (10);
				int newdam = P_DamageMobj (actor->target, actor, actor, damage, NAME_Melee);
				P_TraceBleed (newdam > 0 ? newdam : damage, actor->target, actor);
				S_Sound (actor, CHAN_WEAPON, actor->AttackSound, 1, ATTN_NORM, true);	// [BB] Inform the clients.;
			}
			else if (pr_dragonseek() < 128 && P_CheckMissileRange(actor))
			{
				P_SpawnMissile(actor, target, PClass::FindActor("DragonFireball"), NULL, true); // [BB] Inform clients
				S_Sound (actor, CHAN_WEAPON, actor->AttackSound, 1, ATTN_NORM, true);	// [BB] Inform the clients.
			}
			actor->target = oldTarget;
		}
	}
	if (dist < 4)
	{ // Hit the target thing
		if (actor->target && pr_dragonseek() < 200)
		{
			AActor *bestActor = NULL;
			bestAngle = ANGLE_MAX;
			angleToTarget = actor->__f_AngleTo(actor->target);
			for (i = 0; i < 5; i++)
			{
				if (!target->args[i])
				{
					continue;
				}
				FActorIterator iterator (target->args[i]);
				mo = iterator.Next ();
				if (mo == NULL)
				{
					continue;
				}
				angleToSpot = actor->__f_AngleTo(mo);
				if (absangle(angleToSpot-angleToTarget) < bestAngle)
				{
					bestAngle = absangle(angleToSpot-angleToTarget);
					bestActor = mo;
				}
			}
			if (bestActor != NULL)
			{
				actor->tracer = bestActor;
			}
		}
		else
		{
			// [RH] Don't lock up if the dragon doesn't have any
			// targets defined
			for (i = 0; i < 5; ++i)
			{
				if (target->args[i] != 0)
				{
					break;
				}
			}
			if (i < 5)
			{
				do
				{
					i = (pr_dragonseek()>>2)%5;
				} while(!target->args[i]);
				FActorIterator iterator (target->args[i]);
				actor->tracer = iterator.Next ();
			}
		}
	}
}

//============================================================================
//
// A_DragonInitFlight
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_DragonInitFlight)
{
	PARAM_ACTION_PROLOGUE;

	// [BB] Let the server do this.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	FActorIterator iterator (self->tid);

	do
	{ // find the first tid identical to the dragon's tid
		self->tracer = iterator.Next ();
		if (self->tracer == NULL)
		{
			// [BB] If we're the server, tell the clients to update the thing's state.
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
				SERVERCOMMANDS_SetThingState( self, STATE_SPAWN );

			self->SetState (self->SpawnState);
			return 0;
		}
	} while (self->tracer == self);
	self->RemoveFromHash ();
	return 0;
}

//============================================================================
//
// A_DragonFlight
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_DragonFlight)
{
	PARAM_ACTION_PROLOGUE;

	angle_t angle;

	// [BB] Let the server do this.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	DragonSeek (self, 4., 8.);
	if (self->target)
	{
		if(!(self->target->flags&MF_SHOOTABLE))
		{ // target died
			self->target = NULL;
			return 0;
		}
		angle = self->__f_AngleTo(self->target);
		if (absangle(self->_f_angle()-angle) < ANGLE_45/2 && self->CheckMeleeRange())
		{
			int damage = pr_dragonflight.HitDice (8);
			int newdam = P_DamageMobj (self->target, self, self, damage, NAME_Melee);
			P_TraceBleed (newdam > 0 ? newdam : damage, self->target, self);
			S_Sound (self, CHAN_WEAPON, self->AttackSound, 1, ATTN_NORM, true);	// [BB] Inform the clients.
		}
		else if (absangle(self->_f_angle()-angle) <= ANGLE_1*20)
		{
			// [BB] If we're the server, tell the clients to update the thing's state.
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			{
				SERVERCOMMANDS_SetThingState( self, STATE_MISSILE );
			}

			self->SetState (self->MissileState);
			S_Sound (self, CHAN_WEAPON, self->AttackSound, 1, ATTN_NORM, true);	// [BB] Inform the clients.
		}
	}
	else
	{
		P_LookForPlayers (self, true, NULL);
	}
	return 0;
}

//============================================================================
//
// A_DragonFlap
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_DragonFlap)
{
	PARAM_ACTION_PROLOGUE;

	CALL_ACTION(A_DragonFlight, self);
	if (pr_dragonflap() < 240)
	{
		S_Sound (self, CHAN_BODY, "DragonWingflap", 1, ATTN_NORM);
	}
	else
	{
		self->PlayActiveSound ();
	}
	return 0;
}

//============================================================================
//
// A_DragonAttack
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_DragonAttack)
{
	PARAM_ACTION_PROLOGUE;

	// [BB] Let the server do this.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	P_SpawnMissile (self, self->target, PClass::FindActor("DragonFireball"), NULL, true); // [BB] Inform clients
	return 0;
}

//============================================================================
//
// A_DragonFX2
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_DragonFX2)
{
	PARAM_ACTION_PROLOGUE;

	AActor *mo;
	int i;
	int delay;

	delay = 16+(pr_dragonfx2()>>3);
	for (i = 1+(pr_dragonfx2()&3); i; i--)
	{
		fixed_t xo = ((pr_dragonfx2() - 128) << 14);
		fixed_t yo = ((pr_dragonfx2() - 128) << 14);
		fixed_t zo = ((pr_dragonfx2() - 128) << 12);

		mo = Spawn ("DragonExplosion", self->Vec3Offset(xo, yo, zo), ALLOW_REPLACE);
		if (mo)
		{
			mo->tics = delay+(pr_dragonfx2()&3)*i*2;
			mo->target = self->target;
		}
	}
	return 0;
}

//============================================================================
//
// A_DragonPain
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_DragonPain)
{
	PARAM_ACTION_PROLOGUE;

	CALL_ACTION(A_Pain, self);

	// [BB] Let the server do this.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	if (!self->tracer)
	{ // no destination spot yet
		// [BB] If we're the server, tell the clients to update the thing's state.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_SetThingState( self, STATE_SEE );

		self->SetState (self->SeeState);
	}
	return 0;
}

//============================================================================
//
// A_DragonCheckCrash
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_DragonCheckCrash)
{
	PARAM_ACTION_PROLOGUE;

	if (self->Z() <= self->floorz)
	{
		self->SetState (self->FindState ("Crash"));
	}
	return 0;
}
