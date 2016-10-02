#include "info.h"
#include "a_pickups.h"
#include "a_artifacts.h"
#include "gstrings.h"
#include "p_local.h"
#include "s_sound.h"
#include "c_console.h"
#include "doomstat.h"
#include "v_font.h"
// [BB] New #includes.
#include "cl_demo.h"
#include "deathmatch.h"
#include "network.h"
#include "sv_commands.h"

IMPLEMENT_CLASS(PClassPuzzleItem)

void PClassPuzzleItem::DeriveData(PClass *newclass)
{
	Super::DeriveData(newclass);
	assert(newclass->IsKindOf(RUNTIME_CLASS(PClassPuzzleItem)));
	static_cast<PClassPuzzleItem *>(newclass)->PuzzFailMessage = PuzzFailMessage;
}

IMPLEMENT_CLASS(APuzzleItem)

bool APuzzleItem::HandlePickup (AInventory *item)
{
	// Can't carry more than 1 of each puzzle item in coop netplay
	if (( NETWORK_GetState( ) != NETSTATE_SINGLE ) && !deathmatch && item->GetClass() == GetClass())
	{
		return true;
	}
	return Super::HandlePickup (item);
}

bool APuzzleItem::Use (bool pickup)
{
	// [BC] Puzzle item usage is done server-side.
	// [Dusk] If we got here as the client, the puzzle item was used successfully.
	if ( NETWORK_InClientMode() )
		return true;

	if (P_UsePuzzleItem (Owner, PuzzleItemNumber))
	{
		return true;
	}
	// [RH] Always play the sound if the use fails.
	S_Sound (Owner, CHAN_VOICE, "*puzzfail", 1, ATTN_IDLE, true);	// [BC] Inform the clients.

	// [BB] The server has to generate the message in any case.
	if (Owner != NULL && ( Owner->CheckLocalView (consoleplayer) || ( NETWORK_GetState( ) == NETSTATE_SERVER ) ) )
	{
		FString message = GetClass()->PuzzFailMessage;
		if (message.IsNotEmpty() && message[0] == '$') message = GStrings[&message[1]];
		if (message.IsEmpty()) message = GStrings("TXT_USEPUZZLEFAILED");
		C_MidPrintBold (SmallFont, message);

		// [BB] If we're the server, print the message. This sends the message to all players.
		// Should be tweaked so that it only is shown to those who are watching through the
		// eyes of Owner-player.
		// [Dusk] printing the message to everybody is a little annoying - changed this to
		// print only to this player instead.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_PrintMid( message, true, Owner->player - players, SVCF_ONLYTHISCLIENT );
	}

	return false;
}

bool APuzzleItem::ShouldStay ()
{
	return ( NETWORK_GetState( ) != NETSTATE_SINGLE );
}

