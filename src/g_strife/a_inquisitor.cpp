/*
#include "actor.h"
#include "m_random.h"
#include "a_action.h"
#include "p_local.h"
#include "p_enemy.h"
#include "s_sound.h"
#include "thingdef/thingdef.h"
*/

static FRandom pr_inq ("Inquisitor");

DEFINE_ACTION_FUNCTION(AActor, A_InquisitorWalk)
{
	PARAM_ACTION_PROLOGUE;

	S_Sound (self, CHAN_BODY, "inquisitor/walk", 1, ATTN_NORM);
	A_Chase (stack, self);
	return 0;
}

bool InquisitorCheckDistance (AActor *self)
{
	if (self->reactiontime == 0 && P_CheckSight (self, self->target))
	{
		return self->Distance2D (self->target) < 264.;
	}
	return false;
}

DEFINE_ACTION_FUNCTION(AActor, A_InquisitorDecide)
{
	PARAM_ACTION_PROLOGUE;

	// [BC] This is handled server-side.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	if (self->target == NULL)
		return 0;

	A_FaceTarget (self);
	if (!InquisitorCheckDistance (self))
	{
		// [BC] Set the thing's state.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_SetThingFrame( self, self->FindState("Grenade") );

		self->SetState (self->FindState("Grenade"));
	}
	if (self->target->Z() != self->Z())
	{
		if (self->Top() + 54 < self->ceilingz)
		{
			// [BC] Set the thing's state.
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
				SERVERCOMMANDS_SetThingFrame( self, self->FindState("Jump") );

			self->SetState (self->FindState("Jump"));
		}
	}
	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, A_InquisitorAttack)
{
	PARAM_ACTION_PROLOGUE;

	AActor *proj;

	// [BC] This is handled server-side.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	if (self->target == NULL)
		return 0;

	A_FaceTarget (self);

	self->AddZ(32);
	self->Angles.Yaw -= 45./32;
	proj = P_SpawnMissileZAimed (self, self->Z(), self->target, PClass::FindActor("InquisitorShot"));
	if (proj != NULL)
	{
		proj->Vel.Z += 9;

		// [BC] Tell clients to spawn the missile.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_SpawnMissile( proj );
	}
	self->Angles.Yaw += 45./16;
	proj = P_SpawnMissileZAimed (self, self->Z(), self->target, PClass::FindActor("InquisitorShot"));
	if (proj != NULL)
	{
		proj->Vel.Z += 16;

		// [BC] Tell clients to spawn the missile.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_SpawnMissile( proj );
	}
	self->AddZ(-32);
	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, A_InquisitorJump)
{
	PARAM_ACTION_PROLOGUE;

	double dist;
	double speed;

	// [BC] This is handled server-side.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	if (self->target == NULL)
		return 0;

	S_Sound (self, CHAN_ITEM|CHAN_LOOP, "inquisitor/jump", 1, ATTN_NORM);
	self->AddZ(64);
	A_FaceTarget (self);
	speed = self->Speed * (2./3);
	self->VelFromAngle(speed);
	dist = self->DistanceBySpeed(self->target, speed);
	self->Vel.Z = (self->target->Z() - self->Z()) / dist;
	self->reactiontime = 60;
	self->flags |= MF_NOGRAVITY;

	// [BC] If we're the server, update the thing's position.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
	{
		SERVERCOMMANDS_MoveThingExact( self, CM_Z|CM_VELX|CM_VELY|CM_VELZ );

		// [CW] Also, set the flags to ensure the actor can fly.
		SERVERCOMMANDS_SetThingFlags( self, FLAGSET_FLAGS );
	}

	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, A_InquisitorCheckLand)
{
	PARAM_ACTION_PROLOGUE;

	// [BC] This is handled server-side.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	self->reactiontime--;
	if (self->reactiontime < 0 ||
		self->Vel.X == 0 ||
		self->Vel.Y == 0 ||
		self->Z() <= self->floorz)
	{
		// [BC] Set the thing's state.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_SetThingState( self, STATE_SEE );

		self->SetState (self->SeeState);
		self->reactiontime = 0;
		self->flags &= ~MF_NOGRAVITY;
		S_StopSound (self, CHAN_ITEM);
		return 0;
	}
	if (!S_IsActorPlayingSomething (self, CHAN_ITEM, -1))
	{
		S_Sound (self, CHAN_ITEM|CHAN_LOOP, "inquisitor/jump", 1, ATTN_NORM);
	}
	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, A_TossArm)
{
	PARAM_ACTION_PROLOGUE;

	AActor *foo = Spawn("InquisitorArm", self->PosPlusZ(24.), ALLOW_REPLACE);
	foo->Angles.Yaw = self->Angles.Yaw - 90. + pr_inq.Random2() * (360./1024.);
	foo->VelFromAngle(foo->Speed / 8);
	foo->Vel.Z = pr_inq() / 64.;
	return 0;
}

