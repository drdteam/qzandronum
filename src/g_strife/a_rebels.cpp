/*
#include "actor.h"
#include "m_random.h"
#include "a_action.h"
#include "p_local.h"
#include "p_enemy.h"
#include "s_sound.h"
#include "gi.h"
#include "a_sharedglobal.h"
#include "a_strifeglobal.h"
#include "thingdef/thingdef.h"
#include "doomstat.h"
*/

static FRandom pr_shootgun ("ShootGun");

//============================================================================
//
// A_ShootGun
//
//============================================================================

DEFINE_ACTION_FUNCTION(AActor, A_ShootGun)
{
	PARAM_ACTION_PROLOGUE;

	int pitch;

	if (self->target == NULL)
		return 0;

	// [BC] Play this sound to clients.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		SERVERCOMMANDS_SoundActor( self, CHAN_WEAPON, "monsters/rifle", 1, ATTN_NORM );

	S_Sound (self, CHAN_WEAPON, "monsters/rifle", 1, ATTN_NORM);
	A_FaceTarget (self);
	pitch = P_AimLineAttack (self, self->angle, MISSILERANGE);
	P_LineAttack (self, self->angle + (pr_shootgun.Random2() << 19),
		MISSILERANGE, pitch,
		3*(pr_shootgun() % 5 + 1), NAME_Hitscan, NAME_StrifePuff);
	return 0;
}

// Teleporter Beacon --------------------------------------------------------

class ATeleporterBeacon : public AInventory
{
	DECLARE_CLASS (ATeleporterBeacon, AInventory)
public:
	bool Use (bool pickup);
};

IMPLEMENT_CLASS (ATeleporterBeacon)

bool ATeleporterBeacon::Use (bool pickup)
{
	AInventory *drop;

	// [BC] This is handled server-side.
	if ( NETWORK_InClientMode() )
	{
		return ( true );
	}

	// Increase the amount by one so that when DropInventory decrements it,
	// the actor will have the same number of beacons that he started with.
	// When we return to UseInventory, it will take care of decrementing
	// Amount again and disposing of this item if there are no more.
	Amount++;
	drop = Owner->DropInventory (this);
	if (drop == NULL)
	{
		Amount--;
		return false;
	}
	else
	{
		drop->SetState(drop->FindState(NAME_Drop));
		drop->target = Owner;
		return true;
	}
}

DEFINE_ACTION_FUNCTION(AActor, A_Beacon)
{
	PARAM_ACTION_PROLOGUE;

	AActor *owner = self->target;
	AActor *rebel;
	angle_t an;
	// [BC]
	AActor	*pFog;

	// [BC] This is handled server-side.
	if ( NETWORK_InClientMode() )
	{
		return 0;
	}

	rebel = Spawn("Rebel1", self->X(), self->Y(), self->floorz, ALLOW_REPLACE);
	if (!P_TryMove (rebel, rebel->X(), rebel->Y(), true))
	{
		rebel->Destroy ();
		return 0;
	}
	// Once the rebels start teleporting in, you can't pick up the beacon anymore.
	self->flags &= ~MF_SPECIAL;
	static_cast<AInventory *>(self)->DropTime = 0;
	// Set up the new rebel.
	rebel->threshold = BASETHRESHOLD;
	rebel->target = NULL;
	rebel->flags4 |= MF4_INCOMBAT;
	rebel->LastHeard = owner;	// Make sure the rebels look for targets
	if (deathmatch)
	{
		rebel->health *= 2;
	}
	if (owner != NULL)
	{
		// Rebels are the same color as their owner (but only in multiplayer)
		// [BB] Changed "multiplayer" check
		if ( NETWORK_GetState( ) != NETSTATE_SINGLE )
		{
			rebel->Translation = owner->Translation;
		}
		rebel->SetFriendPlayer(owner->player);
		// Set the rebel's target to whatever last hurt the player, so long as it's not
		// one of the player's other rebels.
		if (owner->target != NULL && !rebel->IsFriend (owner->target))
		{
			rebel->target = owner->target;
		}
	}

	// [BC] Spawn the rebel, and put him in the see state.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
	{
		SERVERCOMMANDS_SpawnThing( rebel );
		SERVERCOMMANDS_SetThingState( rebel, STATE_SEE );
	}

	rebel->SetState (rebel->SeeState);
	rebel->angle = self->angle;
	an = self->angle >> ANGLETOFINESHIFT;
	pFog = Spawn<ATeleportFog> (rebel->Vec3Offset(20*finecosine[an], 20*finesine[an], TELEFOGHEIGHT), ALLOW_REPLACE);
	// [BC] Spawn the teleport fog.
	if (( NETWORK_GetState( ) == NETSTATE_SERVER ) &&
		( pFog ))
	{
		SERVERCOMMANDS_SpawnThing( pFog );
	}

	if (--self->health < 0)
	{
		// [BB] Tell clients to set the thing's state.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_SetThingState( self, STATE_DEATH );

		self->SetState(self->FindState(NAME_Death));
	}
	return 0;
}
