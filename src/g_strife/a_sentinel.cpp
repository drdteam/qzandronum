/*
#include "actor.h"
#include "p_enemy.h"
#include "a_action.h"
#include "p_local.h"
#include "m_random.h"
#include "thingdef/thingdef.h"
*/

static FRandom pr_sentinelrefire ("SentinelRefire");

DEFINE_ACTION_FUNCTION(AActor, A_SentinelBob)
{
	fixed_t minz, maxz;

	// [CW] This is handled by the server.
	if ( NETWORK_InClientMode() )
		return;

	if (self->flags & MF_INFLOAT)
	{
		self->velz = 0;

		// [CW] Moving the actor is server side.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_MoveThing( self, CM_MOMZ );

		return;
	}
	if (self->threshold != 0)
		return;

	maxz =  self->ceilingz - self->height - 16*FRACUNIT;
	minz = self->floorz + 96*FRACUNIT;
	if (minz > maxz)
	{
		minz = maxz;
	}
	if (minz < self->Z())
	{
		self->velz -= FRACUNIT;
	}
	else
	{
		self->velz += FRACUNIT;
	}
	self->reactiontime = (minz >= self->Z()) ? 4 : 0;

	// [CW] Moving the actor is server side.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		SERVERCOMMANDS_MoveThingExact( self, CM_MOMZ );
}

DEFINE_ACTION_FUNCTION(AActor, A_SentinelAttack)
{
	AActor *missile = NULL, *trail;

	// [BB] Without a target the P_SpawnMissileZAimed call will crash.
	if (!self->target)
	{
		return;
	}

	// [CW] If we aren't a client, spawn the missile.
	if ( NETWORK_InClientMode() == false )
	{
		missile = P_SpawnMissileZAimed (self, self->Z() + 32*FRACUNIT, self->target, PClass::FindClass("SentinelFX2"));

		// [CW] Spawn the missile server side.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_SpawnMissile( missile );
	}

	if (missile != NULL && (missile->velx | missile->vely) != 0)
	{
		for (int i = 8; i > 1; --i)
		{
			trail = Spawn("SentinelFX1",
				self->Vec3Angle(missile->radius*i, missile->angle, (missile->velz / 4 * i)), ALLOW_REPLACE);
			if (trail != NULL)
			{
				trail->target = self;
				trail->velx = missile->velx;
				trail->vely = missile->vely;
				trail->velz = missile->velz;
				P_CheckMissileSpawn (trail, self->radius);
			}
		}
		missile->AddZ(missile->velz >> 2);
	}
}

DEFINE_ACTION_FUNCTION(AActor, A_SentinelRefire)
{
	A_FaceTarget (self);

	// [CW] Clients may not do this.
	if ( NETWORK_InClientMode() )
		return;

	if (pr_sentinelrefire() >= 30)
	{
		if (self->target == NULL ||
			self->target->health <= 0 ||
			!P_CheckSight (self, self->target, SF_SEEPASTBLOCKEVERYTHING|SF_SEEPASTSHOOTABLELINES) ||
			P_HitFriend(self) ||
			(self->MissileState == NULL && !self->CheckMeleeRange()) ||
			pr_sentinelrefire() < 40)
		{
			// [CW] Tell clients to set the frame.
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
				SERVERCOMMANDS_SetThingFrame( self, self->SeeState );

			self->SetState (self->SeeState);
		}
	}
}
