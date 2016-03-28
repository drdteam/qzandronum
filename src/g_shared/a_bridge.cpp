#include "actor.h"
#include "info.h"
#include "gi.h"
#include "m_random.h"
#include "thingdef/thingdef.h"

// [Dusk]
#include "network.h"

static FRandom pr_orbit ("Orbit");


// Custom bridge --------------------------------------------------------
/*
	args[0]: Bridge radius, in mapunits
	args[1]: Bridge height, in mapunits
	args[2]: Amount of bridge balls (if 0: Doom bridge)
	args[3]: Rotation speed of bridge balls, in byte angle per seconds, sorta:
		Since an arg is only a byte, it can only go from 0 to 255, while ZDoom's
		BAM go from 0 to 65535. Plus, it needs to be able to go either way. So,
		up to 128, it goes counterclockwise; 129-255 is clockwise, substracting
		256 from it to get the angle. A few example values:
		  0: Hexen default
	     11:  15� / seconds
		 21:  30� / seconds
		 32:  45� / seconds
		 64:  90� / seconds
		128: 180� / seconds
		192: -90� / seconds
		223: -45� / seconds
		233: -30� / seconds
		244: -15� / seconds
		This value only matters if args[2] is not zero.
	args[4]: Rotation radius of bridge balls, in bridge radius %.
		If 0, use Hexen default: ORBIT_RADIUS, regardless of bridge radius.
		This value only matters if args[2] is not zero.
*/

class ACustomBridge : public AActor
{
	DECLARE_CLASS (ACustomBridge, AActor)
public:
	void BeginPlay ();
	void Destroy();
};

IMPLEMENT_CLASS(ACustomBridge)

void ACustomBridge::BeginPlay ()
{
	if (args[2]) // Hexen bridge if there are balls
	{
		SetState(SeeState);
		radius = args[0] ? args[0] << FRACBITS : 32 * FRACUNIT;
		height = args[1] ? args[1] << FRACBITS : 2 * FRACUNIT;
	}
	else // No balls? Then a Doom bridge.
	{
		radius = args[0] ? args[0] << FRACBITS : 36 * FRACUNIT;
		height = args[1] ? args[1] << FRACBITS : 4 * FRACUNIT;
		RenderStyle = STYLE_Normal;
	}
}

void ACustomBridge::Destroy()
{
	// Hexen originally just set a flag to make the bridge balls remove themselves in A_BridgeOrbit.
	// But this is not safe with custom bridge balls that do not necessarily call that function.
	// So the best course of action is to look for all bridge balls here and destroy them ourselves.
	
	TThinkerIterator<AActor> it;
	AActor *thing;
	
	while ((thing = it.Next()))
	{
		if (thing->target == this)
		{
			thing->Destroy();
		}
	}
	Super::Destroy();
}

// Action functions for the non-Doom bridge --------------------------------

#define ORBIT_RADIUS	15

// New bridge stuff
//	Parent
//		special1	true == removing from world
//
//	Child
//		target		pointer to center mobj
//		angle		angle of ball

DEFINE_ACTION_FUNCTION(AActor, A_BridgeOrbit)
{
	PARAM_ACTION_PROLOGUE;

	if (self->target == NULL)
	{ // Don't crash if somebody spawned this into the world
	  // independantly of a Bridge actor.
		return 0;
	}
	// Set default values
	// Every five tics, Hexen moved the ball 3/256th of a revolution.
	int rotationspeed  = ANGLE_45/32*3/5;
	int rotationradius = ORBIT_RADIUS * FRACUNIT;
	// If the bridge is custom, set non-default values if any.

	// Set angular speed; 1--128: counterclockwise rotation ~=1--180�; 129--255: clockwise rotation ~= 180--1�
	if (self->target->args[3] > 128) rotationspeed = ANGLE_45/32 * (self->target->args[3]-256) / TICRATE;
	else if (self->target->args[3] > 0) rotationspeed = ANGLE_45/32 * (self->target->args[3]) / TICRATE;
	// Set rotation radius
	if (self->target->args[4]) rotationradius = ((self->target->args[4] * self->target->radius) / 100);

	self->angle += rotationspeed;
	self->SetOrigin(self->target->Vec3Angle(rotationradius, self->angle, 0), true);
	self->floorz = self->target->floorz;
	self->ceilingz = self->target->ceilingz;
	return 0;
}


DEFINE_ACTION_FUNCTION_PARAMS(AActor, A_BridgeInit)
{
	PARAM_ACTION_PROLOGUE;
	PARAM_CLASS_OPT(balltype, AActor)	{ balltype = NULL; }

	angle_t startangle;
	AActor *ball;

	if (balltype == NULL)
	{
		balltype = PClass::FindActor("BridgeBall");
	}

	startangle = pr_orbit() << 24;

	// Spawn triad into world -- may be more than a triad now.
	int ballcount = self->args[2]==0 ? 3 : self->args[2];

	for (int i = 0; i < ballcount; i++)
	{
		ball = Spawn(balltype, self->Pos(), ALLOW_REPLACE);
		ball->angle = startangle + (ANGLE_45/32) * (256/ballcount) * i;
		ball->target = self;
		CALL_ACTION(A_BridgeOrbit, ball);

		// [Dusk] bridge balls should not be included in full updates
		// as the bridge thing will spawn them instead.
		if (NETWORK_GetState() == NETSTATE_SERVER)
			ball->ulNetworkFlags |= NETFL_ALLOWCLIENTSPAWN;
	}
	return 0;
}


// Invisible bridge --------------------------------------------------------

class AInvisibleBridge : public AActor
{
	DECLARE_CLASS (AInvisibleBridge, AActor)
public:
	void BeginPlay ();
};

IMPLEMENT_CLASS(AInvisibleBridge)

void AInvisibleBridge::BeginPlay ()
{
	Super::BeginPlay ();
	if (args[0])
		radius = args[0] << FRACBITS;
	if (args[1])
		height = args[1] << FRACBITS;
}

