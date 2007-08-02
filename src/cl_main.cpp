//-----------------------------------------------------------------------------
//
// Skulltag Source
// Copyright (C) 2003 Brad Carney
// Copyright (C) 2007-2012 Skulltag Development Team
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 3. Neither the name of the Skulltag Development Team nor the names of its
//    contributors may be used to endorse or promote products derived from this
//    software without specific prior written permission.
// 4. Redistributions in any form must be accompanied by information on how to
//    obtain complete source code for the software and any accompanying
//    software that uses the software. The source code must either be included
//    in the distribution or be available for no more than the cost of
//    distribution plus a nominal fee, and must be freely redistributable
//    under reasonable conditions. For an executable file, complete source
//    code means the source code for all modules it contains. It does not
//    include source code for modules or files that typically accompany the
//    major components of the operating system on which the executable file
//    runs.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//
//
// Filename: cl_main.cpp
//
// Description: Contains variables and routines related to the client portion
// of the program.
//
//-----------------------------------------------------------------------------

#include "networkheaders.h"
#include "a_action.h"
#include "a_sharedglobal.h"
#include "a_doomglobal.h"
#include "announcer.h"
#include "cl_commands.h"
#include "cl_demo.h"
#include "cooperative.h"
#include "doomtype.h"
#include "doomstat.h"
#include "d_player.h"
#include "g_game.h"
#include "gamemode.h"
#include "d_net.h"
#include "p_local.h"
#include "s_sound.h"
#include "gi.h"
#include "i_net.h"
#include "i_system.h"
#include "c_dispatch.h"
#include "st_stuff.h"
#include "m_argv.h"
#include "m_cheat.h"
#include "cl_main.h"
#include "p_effect.h"
#include "p_lnspec.h"
#include "possession.h"
#include "c_console.h"
#include "s_sndseq.h"
#include "version.h"
#include "p_acs.h"
#include "p_enemy.h"
#include "survival.h"
#include "v_video.h"
#include "w_wad.h"
#include "deathmatch.h"
#include "duel.h"
#include "team.h"
#include "chat.h"
#include "announcer.h"
#include "network.h"
#include "sv_main.h"
#include "sbar.h"
#include "m_random.h"
#include "templates.h"
#include "stats.h"
#include "lastmanstanding.h"
#include "scoreboard.h"
#include "joinqueue.h"
#include "callvote.h"
#include "invasion.h"
#include "r_sky.h"

//*****************************************************************************
//	MISC CRAP THAT SHOULDN'T BE HERE BUT HAS TO BE BECAUSE OF SLOPPY CODING

void	G_PlayerReborn( int player );
//void	ChangeSpy (bool forward);
void	goOn (int position, bool keepFacing, bool secret, bool resetinv);
polyobj_t *GetPolyobj (int polyNum);
int		D_PlayerClassToInt (const char *classname);
bool	P_AdjustFloorCeil (AActor *thing);
void	ClientObituary (AActor *self, AActor *inflictor, AActor *attacker, LONG lMeansOfDeath);
void	P_CrouchMove(player_t * player, int direction);
bool	DoThingRaise( AActor *thing );
extern	bool	SpawningMapThing;

EXTERN_CVAR( Bool, telezoom )
EXTERN_CVAR( Bool, sv_cheats )
EXTERN_CVAR( Int, cl_bloodtype )
EXTERN_CVAR( Int, cl_pufftype )
EXTERN_CVAR( String, playerclass )
EXTERN_CVAR( Int, am_cheat )

extern char *g_szActorKeyLetter[NUMBER_OF_ACTOR_NAME_KEY_LETTERS];
extern char *g_szActorFullName[NUMBER_OF_ACTOR_NAME_KEY_LETTERS];

//*****************************************************************************
//	PROTOTYPES

// Protocol functions.
static	void	client_Header( BYTESTREAM_s *pByteStream );
static	void	client_ResetSequence( BYTESTREAM_s *pByteStream );
static	void	client_Ping( BYTESTREAM_s *pByteStream );
static	void	client_BeginSnapshot( BYTESTREAM_s *pByteStream );
static	void	client_EndSnapshot( BYTESTREAM_s *pByteStream );

// Player functions.
static	void	client_SpawnPlayer( BYTESTREAM_s *pByteStream );
static	void	client_MovePlayer( BYTESTREAM_s *pByteStream );
static	void	client_DamagePlayer( BYTESTREAM_s *pByteStream );
static	void	client_KillPlayer( BYTESTREAM_s *pByteStream );
static	void	client_SetPlayerHealth( BYTESTREAM_s *pByteStream );
static	void	client_UpdatePlayerArmorDisplay( BYTESTREAM_s *pByteStream );
static	void	client_SetPlayerState( BYTESTREAM_s *pByteStream );
static	void	client_SetPlayerUserInfo( BYTESTREAM_s *pByteStream );
static	void	client_SetPlayerFrags( BYTESTREAM_s *pByteStream );
static	void	client_SetPlayerPoints( BYTESTREAM_s *pByteStream );
static	void	client_SetPlayerWins( BYTESTREAM_s *pByteStream );
static	void	client_SetPlayerKillCount( BYTESTREAM_s *pByteStream );
static	void	client_SetPlayerChatStatus( BYTESTREAM_s *pByteStream );
static	void	client_SetPlayerLaggingStatus( BYTESTREAM_s *pByteStream );
static	void	client_SetPlayerReadyToGoOnStatus( BYTESTREAM_s *pByteStream );
static	void	client_SetPlayerTeam( BYTESTREAM_s *pByteStream );
static	void	client_SetPlayerCamera( BYTESTREAM_s *pByteStream );
static	void	client_UpdatePlayerPing( BYTESTREAM_s *pByteStream );
static	void	client_UpdatePlayerExtraData( BYTESTREAM_s *pByteStream );
static	void	client_UpdatePlayerPendingWeapon( BYTESTREAM_s *pByteStream );
static	void	client_DoInventoryUse( BYTESTREAM_s *pByteStream );
static	void	client_UpdatePlayerTime( BYTESTREAM_s *pByteStream );
static	void	client_MoveLocalPlayer( BYTESTREAM_s *pByteStream );
static	void	client_DisconnectPlayer( BYTESTREAM_s *pByteStream );
static	void	client_SetConsolePlayer( BYTESTREAM_s *pByteStream );
static	void	client_ConsolePlayerKicked( BYTESTREAM_s *pByteStream );
static	void	client_GivePlayerMedal( BYTESTREAM_s *pByteStream );
static	void	client_ResetAllPlayersFragcount( BYTESTREAM_s *pByteStream );
static	void	client_PlayerIsSpectator( BYTESTREAM_s *pByteStream );
static	void	client_PlayerSay( BYTESTREAM_s *pByteStream );
static	void	client_PlayerTaunt( BYTESTREAM_s *pByteStream );
static	void	client_PlayerRespawnInvulnerability( BYTESTREAM_s *pByteStream );
static	void	client_SetPlayerAmmoCapacity( BYTESTREAM_s *pByteStream );

// Thing functions.
static	void	client_SpawnThing( BYTESTREAM_s *pByteStream );
static	void	client_SpawnThingNoNetID( BYTESTREAM_s *pByteStream, bool bReceiveTranslation = false );
static	void	client_SpawnThingExact( BYTESTREAM_s *pByteStream );
static	void	client_SpawnThingExactNoNetID( BYTESTREAM_s *pByteStream );
static	void	client_MoveThing( BYTESTREAM_s *pByteStream );
static	void	client_MoveThingExact( BYTESTREAM_s *pByteStream );
static	void	client_DamageThing( BYTESTREAM_s *pByteStream );
static	void	client_KillThing( BYTESTREAM_s *pByteStream );
static	void	client_SetThingState( BYTESTREAM_s *pByteStream );
static	void	client_DestroyThing( BYTESTREAM_s *pByteStream );
static	void	client_SetThingAngle( BYTESTREAM_s *pByteStream );
static	void	client_SetThingTID( BYTESTREAM_s *pByteStream );
static	void	client_SetThingWaterLevel( BYTESTREAM_s *pByteStream );
static	void	client_SetThingFlags( BYTESTREAM_s *pByteStream );
static	void	client_SetThingArguments( BYTESTREAM_s *pByteStream );
static	void	client_SetThingTranslation( BYTESTREAM_s *pByteStream );
static	void	client_SetThingProperty( BYTESTREAM_s *pByteStream );
static	void	client_SetThingSound( BYTESTREAM_s *pByteStream );
static	void	client_SetThingSpecial2( BYTESTREAM_s *pByteStream );
static	void	client_SetWeaponAmmoGive( BYTESTREAM_s *pByteStream );
static	void	client_ThingIsCorpse( BYTESTREAM_s *pByteStream );
static	void	client_HideThing( BYTESTREAM_s *pByteStream );
static	void	client_TeleportThing( BYTESTREAM_s *pByteStream );
static	void	client_ThingActivate( BYTESTREAM_s *pByteStream );
static	void	client_ThingDeactivate( BYTESTREAM_s *pByteStream );
static	void	client_RespawnDoomThing( BYTESTREAM_s *pByteStream );
static	void	client_RespawnRavenThing( BYTESTREAM_s *pByteStream );

// Print commands.
static	void	client_Print( BYTESTREAM_s *pByteStream );
static	void	client_PrintMid( BYTESTREAM_s *pByteStream );
static	void	client_PrintMOTD( BYTESTREAM_s *pByteStream );
static	void	client_PrintHUDMessage( BYTESTREAM_s *pByteStream );
static	void	client_PrintHUDMessageFadeOut( BYTESTREAM_s *pByteStream );
static	void	client_PrintHUDMessageFadeInOut( BYTESTREAM_s *pByteStream );
static	void	client_PrintHUDMessageTypeOnFadeOut( BYTESTREAM_s *pByteStream );

// Game commands.
static	void	client_SetGameMode( BYTESTREAM_s *pByteStream );
static	void	client_SetGameSkill( BYTESTREAM_s *pByteStream );
static	void	client_SetGameDMFlags( BYTESTREAM_s *pByteStream );
static	void	client_SetGameModeLimits( BYTESTREAM_s *pByteStream );
static	void	client_SetGameEndLevelDelay( BYTESTREAM_s *pByteStream );
static	void	client_SetGameModeState( BYTESTREAM_s *pByteStream );
static	void	client_SetDuelNumDuels( BYTESTREAM_s *pByteStream );
static	void	client_SetLMSSpectatorSettings( BYTESTREAM_s *pByteStream );
static	void	client_SetLMSAllowedWeapons( BYTESTREAM_s *pByteStream );
static	void	client_SetInvasionNumMonstersLeft( BYTESTREAM_s *pByteStream );
static	void	client_SetInvasionWave( BYTESTREAM_s *pByteStream );
static	void	client_DoPossessionArtifactPickedUp( BYTESTREAM_s *pByteStream );
static	void	client_DoPossessionArtifactDropped( BYTESTREAM_s *pByteStream );
static	void	client_DoGameModeFight( BYTESTREAM_s *pByteStream );
static	void	client_DoGameModeCountdown( BYTESTREAM_s *pByteStream );
static	void	client_DoGameModeWinSequence( BYTESTREAM_s *pByteStream );

// Team commands.
static	void	client_SetTeamFrags( BYTESTREAM_s *pByteStream );
static	void	client_SetTeamScore( BYTESTREAM_s *pByteStream );
static	void	client_SetTeamWins( BYTESTREAM_s *pByteStream );
static	void	client_SetTeamReturnTicks( BYTESTREAM_s *pByteStream );
static	void	client_TeamFlagReturned( BYTESTREAM_s *pByteStream );

// Missile commands.
static	void	client_SpawnMissile( BYTESTREAM_s *pByteStream );
static	void	client_SpawnMissileExact( BYTESTREAM_s *pByteStream );
static	void	client_MissileExplode( BYTESTREAM_s *pByteStream );

// Weapon commands.
static	void	client_WeaponSound( BYTESTREAM_s *pByteStream );
static	void	client_WeaponChange( BYTESTREAM_s *pByteStream );
static	void	client_WeaponRailgun( BYTESTREAM_s *pByteStream );

// Sector commands.
static	void	client_SetSectorFloorPlane( BYTESTREAM_s *pByteStream );
static	void	client_SetSectorCeilingPlane( BYTESTREAM_s *pByteStream );
static	void	client_SetSectorLightLevel( BYTESTREAM_s *pByteStream );
static	void	client_SetSectorColor( BYTESTREAM_s *pByteStream );
static	void	client_SetSectorFade( BYTESTREAM_s *pByteStream );
static	void	client_SetSectorFlat( BYTESTREAM_s *pByteStream );
static	void	client_SetSectorPanning( BYTESTREAM_s *pByteStream );
static	void	client_SetSectorRotation( BYTESTREAM_s *pByteStream );
static	void	client_SetSectorScale( BYTESTREAM_s *pByteStream );
static	void	client_SetSectorFriction( BYTESTREAM_s *pByteStream );
static	void	client_SetSectorAngleYOffset( BYTESTREAM_s *pByteStream );
static	void	client_SetSectorGravity( BYTESTREAM_s *pByteStream );
static	void	client_StopSectorLightEffect( BYTESTREAM_s *pByteStream );
static	void	client_DestroyAllSectorMovers( BYTESTREAM_s *pByteStream );

// Sector light commands.
static	void	client_DoSectorLightFireFlicker( BYTESTREAM_s *pByteStream );
static	void	client_DoSectorLightFlicker( BYTESTREAM_s *pByteStream );
static	void	client_DoSectorLightLightFlash( BYTESTREAM_s *pByteStream );
static	void	client_DoSectorLightStrobe( BYTESTREAM_s *pByteStream );
static	void	client_DoSectorLightGlow( BYTESTREAM_s *pByteStream );
static	void	client_DoSectorLightGlow2( BYTESTREAM_s *pByteStream );
static	void	client_DoSectorLightPhased( BYTESTREAM_s *pByteStream );

// Line commands.
static	void	client_SetLineAlpha( BYTESTREAM_s *pByteStream );
static	void	client_SetLineTexture( BYTESTREAM_s *pByteStream );
static	void	client_SetLineBlocking( BYTESTREAM_s *pByteStream );

// Sound commands.
static	void	client_Sound( BYTESTREAM_s *pByteStream );
static	void	client_SoundID( BYTESTREAM_s *pByteStream );
static	void	client_SoundActor( BYTESTREAM_s *pByteStream );
static	void	client_SoundIDActor( BYTESTREAM_s *pByteStream );
static	void	client_SoundPoint( BYTESTREAM_s *pByteStream );
static	void	client_StartSectorSequence( BYTESTREAM_s *pByteStream );
static	void	client_StopSectorSequence( BYTESTREAM_s *pByteStream );

// Vote commands.
static	void	client_CallVote( BYTESTREAM_s *pByteStream );
static	void	client_PlayerVote( BYTESTREAM_s *pByteStream );
static	void	client_VoteEnded( BYTESTREAM_s *pByteStream );

// Map commands.
static	void	client_MapLoad( BYTESTREAM_s *pByteStream );
static	void	client_MapNew( BYTESTREAM_s *pByteStream );
static	void	client_MapExit( BYTESTREAM_s *pByteStream );
static	void	client_MapAuthenticate( BYTESTREAM_s *pByteStream );
static	void	client_SetMapTime( BYTESTREAM_s *pByteStream );
static	void	client_SetMapNumKilledMonsters( BYTESTREAM_s *pByteStream );
static	void	client_SetMapNumFoundItems( BYTESTREAM_s *pByteStream );
static	void	client_SetMapNumFoundSecrets( BYTESTREAM_s *pByteStream );
static	void	client_SetMapNumTotalMonsters( BYTESTREAM_s *pByteStream );
static	void	client_SetMapNumTotalItems( BYTESTREAM_s *pByteStream );
static	void	client_SetMapMusic( BYTESTREAM_s *pByteStream );
static	void	client_SetMapSky( BYTESTREAM_s *pByteStream );

// Inventory commands.
static	void	client_GiveInventory( BYTESTREAM_s *pByteStream );
static	void	client_TakeInventory( BYTESTREAM_s *pByteStream );
static	void	client_GivePowerup( BYTESTREAM_s *pByteStream );
static	void	client_DoInventoryPickup( BYTESTREAM_s *pByteStream );
static	void	client_DestroyAllInventory( BYTESTREAM_s *pByteStream );

// Door commands.
static	void	client_DoDoor( BYTESTREAM_s *pByteStream );
static	void	client_DestroyDoor( BYTESTREAM_s *pByteStream );
static	void	client_ChangeDoorDirection( BYTESTREAM_s *pByteStream );

// Floor commands.
static	void	client_DoFloor( BYTESTREAM_s *pByteStream );
static	void	client_DestroyFloor( BYTESTREAM_s *pByteStream );
static	void	client_ChangeFloorDirection( BYTESTREAM_s *pByteStream );
static	void	client_ChangeFloorType( BYTESTREAM_s *pByteStream );
static	void	client_ChangeFloorDestDist( BYTESTREAM_s *pByteStream );
static	void	client_StartFloorSound( BYTESTREAM_s *pByteStream );

// Ceiling commands.
static	void	client_DoCeiling( BYTESTREAM_s *pByteStream );
static	void	client_DestroyCeiling( BYTESTREAM_s *pByteStream );
static	void	client_ChangeCeilingDirection( BYTESTREAM_s *pByteStream );
static	void	client_ChangeCeilingSpeed( BYTESTREAM_s *pByteStream );
static	void	client_PlayCeilingSound( BYTESTREAM_s *pByteStream );

// Plat commands.
static	void	client_DoPlat( BYTESTREAM_s *pByteStream );
static	void	client_DestroyPlat( BYTESTREAM_s *pByteStream );
static	void	client_ChangePlatStatus( BYTESTREAM_s *pByteStream );
static	void	client_PlayPlatSound( BYTESTREAM_s *pByteStream );

// Elevator commands.
static	void	client_DoElevator( BYTESTREAM_s *pByteStream );
static	void	client_DestroyElevator( BYTESTREAM_s *pByteStream );
static	void	client_StartElevatorSound( BYTESTREAM_s *pByteStream );

// Pillar commands.
static	void	client_DoPillar( BYTESTREAM_s *pByteStream );
static	void	client_DestroyPillar( BYTESTREAM_s *pByteStream );

// Waggle commands.
static	void	client_DoWaggle( BYTESTREAM_s *pByteStream );
static	void	client_DestroyWaggle( BYTESTREAM_s *pByteStream );
static	void	client_UpdateWaggle( BYTESTREAM_s *pByteStream );

// Rotate poly commands.
static	void	client_DoRotatePoly( BYTESTREAM_s *pByteStream );
static	void	client_DestroyRotatePoly( BYTESTREAM_s *pByteStream );

// Move poly commands.
static	void	client_DoMovePoly( BYTESTREAM_s *pByteStream );
static	void	client_DestroyMovePoly( BYTESTREAM_s *pByteStream );

// Poly door commands.
static	void	client_DoPolyDoor( BYTESTREAM_s *pByteStream );
static	void	client_DestroyPolyDoor( BYTESTREAM_s *pByteStream );
static	void	client_SetPolyDoorSpeedPosition( BYTESTREAM_s *pByteStream );

// Generic polyobject commands.
static	void	client_PlayPolyobjSound( BYTESTREAM_s *pByteStream );
static	void	client_SetPolyobjPosition( BYTESTREAM_s *pByteStream );
static	void	client_SetPolyobjRotation( BYTESTREAM_s *pByteStream );

// Miscellaneous commands.
static	void	client_EarthQuake( BYTESTREAM_s *pByteStream );
static	void	client_SetQueuePosition( BYTESTREAM_s *pByteStream );
static	void	client_DoScroller( BYTESTREAM_s *pByteStream );
static	void	client_SetScroller( BYTESTREAM_s *pByteStream );
static	void	client_GenericCheat( BYTESTREAM_s *pByteStream );
static	void	client_SetCameraToTexture( BYTESTREAM_s *pByteStream );

//*****************************************************************************
//	VARIABLES

// Local network buffer for the client.
static	NETBUFFER_s			g_LocalBuffer;

// The address of the server we're connected to or trying to connect to.
static	NETADDRESS_s		g_AddressServer;

// The address of the last server we tried to connect to/we're connected to.
static	NETADDRESS_s		g_AddressLastConnected;

// Last time we heard from the server.
static	ULONG				g_ulLastServerTick;

// State of the client's connection to the server.
static	CONNECTIONSTATE_e	g_ConnectionState;

// Have we heard from the server recently?
static	bool				g_bServerLagging;

// What's the time of the last message the server got from us?
static	bool				g_bClientLagging;

// Used for item respawning client-side.
static	FRandom				g_RestorePositionSeed( "ClientRestorePos" );

// Is it okay to send user info, or is that currently disabled?
static	bool				g_bAllowSendingOfUserInfo = true;

// Number of bytes sent/received this tick; useful for stats.
static	LONG				g_lBytesSent;
static	LONG				g_lBytesReceived;
static	LONG				g_lMaxBytesSent;
static	LONG				g_lMaxBytesReceived;
static	LONG				g_lMaxBytesSentLastSecond;
static	LONG				g_lMaxBytesReceivedLastSecond;

// The name of the map we need to authenticate.
static	char				g_szMapName[8];

// Last console player update tick.
static	ULONG				g_ulLastConsolePlayerUpdateTick;

// This contains the last 256 packets we've received.
static	PACKETBUFFER_s		g_ReceivedPacketBuffer;

// This is the start position of each packet within that buffer.
static	LONG				g_lPacketBeginning[256];

// This is the sequences of the last 256 packets we've received.
static	LONG				g_lPacketSequence[256];

// This is the  size of the last 256 packets we've received.
static	LONG				g_lPacketSize[256];

// This is the index of the incoming packet.
static	BYTE				g_bPacketNum;

// This is the current position in the received packet buffer.
static	LONG				g_lCurrentPosition;

// This is the sequence of the last packet we parsed.
static	LONG				g_lLastParsedSequence;

// This is the highest sequence of any packet we've received.
static	LONG				g_lHighestReceivedSequence;

// Delay for sending a request missing packets.
static	LONG				g_lMissingPacketTicks;

// Debugging variables.
static	LONG				g_lLastCmd;
static	char				*g_pszHeaderNames[NUM_SERVER_COMMANDS] =
{
	"SVC_HEADER",
	"SVC_UNRELIABLEPACKET",
	"SVC_RESETSEQUENCE",
	"SVC_PING",
	"SVC_NOTHING",
	"SVC_BEGINSNAPSHOT",
	"SVC_ENDSNAPSHOT",
	"SVC_SPAWNPLAYER",
	"SVC_MOVEPLAYER",
	"SVC_DAMAGEPLAYER",
	"SVC_KILLPLAYER",
	"SVC_SETPLAYERHEALTH",
	"SVC_SETPLAYERSTATE",
	"SVC_SETPLAYERUSERINFO",
	"SVC_SETPLAYERFRAGS",
	"SVC_SETPLAYERPOINTS",
	"SVC_SETPLAYERWINS",
	"SVC_SETPLAYERKILLCOUNT",
	"SVC_SETPLAYERCHATSTATUS",
	"SVC_SETPLAYERLAGGINGSTATUS",
	"SVC_SETPLAYERREADYTOGOONSTATUS",
	"SVC_SETPLAYERTEAM",
	"SVC_SETPLAYERCAMERA",
	"SVC_UPDATEPLAYERPING",
	"SVC_UPDATEPLAYEREXTRADATA",
	"SVC_UPDATEPLAYERTIME",
	"SVC_MOVELOCALPLAYER",
	"SVC_DISCONNECTPLAYER",
	"SVC_SETCONSOLEPLAYER",
	"SVC_CONSOLEPLAYERKICKED",
	"SVC_GIVEPLAYERMEDAL",
	"SVC_RESETALLPLAYERSFRAGCOUNT",
	"SVC_PLAYERISSPECTATOR",
	"SVC_PLAYERSAY",
	"SVC_PLAYERTAUNT",
	"SVC_PLAYERRESPAWNINVULNERABILITY",
	"SVC_SPAWNTHING",
	"SVC_SPAWNTHINGNONETID",
	"SVC_SPAWNTHINGEXACT",
	"SVC_SPAWNTHINGEXACTNONETID",
	"SVC_MOVETHING",
	"SVC_DAMAGETHING",
	"SVC_KILLTHING",
	"SVC_SETTHINGSTATE",
	"SVC_DESTROYTHING",
	"SVC_SETTHINGANGLE",
	"SVC_SETTHINGWATERLEVEL",
	"SVC_SETTHINGFLAGS",
	"SVC_SETTHINGARGUMENTS",
	"SVC_SETTHINGTRANSLATION",
	"SVC_SETTHINGPROPERTY",
	"SVC_SETTHINGSOUND",
	"SVC_SETWEAPONAMMOGIVE",
	"SVC_THINGISCORPSE",
	"SVC_HIDETHING",
	"SVC_TELEPORTTHING",
	"SVC_THINGACTIVATE",
	"SVC_THINGDEACTIVATE",
	"SVC_RESPAWNDOOMTHING",
	"SVC_PRINT",
	"SVC_PRINTMID",
	"SVC_PRINTMOTD",
	"SVC_PRINTHUDMESSAGE",
	"SVC_PRINTHUDMESSAGEFADEOUT",
	"SVC_PRINTHUDMESSAGEFADEINOUT",
	"SVC_PRINTHUDMESSAGETYPEONFADEOUT",
	"SVC_SETGAMEMODE",
	"SVC_SETGAMESKILL",
	"SVC_SETGAMEDMFLAGS",
	"SVC_SETGAMEMODELIMITS",
	"SVC_SETGAMEENDLEVELDELAY",
	"SVC_SETGAMEMODESTATE",
	"SVC_SETDUELNUMDUELS",
	"SVC_SETLMSSPECTATORSETTINGS",
	"SVC_SETLMSALLOWEDWEAPONS",
	"SVC_SETINVASIONNUMMONSTERSLEFT",
	"SVC_SETINVASIONWAVE",
	"SVC_DOPOSSESSIONARTIFACTPICKEDUP",
	"SVC_DOPOSSESSIONARTIFACTDROPPED",
	"SVC_DOGAMEMODEFIGHT",
	"SVC_DOGAMEMODECOUNTDOWN",
	"SVC_DOGAMEMODEWINSEQUENCE",
	"SVC_SETTEAMFRAGS",
	"SVC_SETTEAMSCORE",
	"SVC_SETTEAMWINS",
	"SVC_SETTEAMRETURNTICKS",
	"SVC_TEAMFLAGRETURNED",
	"SVC_SPAWNMISSILE",
	"SVC_SPAWNMISSILEEXACT",
	"SVC_MISSILEEXPLODE",
	"SVC_WEAPONFIRE",
	"SVC_WEAPONCHANGE",
	"SVC_WEAPONRAILGUN",
	"SVC_SETSECTORFLOORPLANE",
	"SVC_SETSECTORCEILINGPLANE",
	"SVC_SETSECTORLIGHTLEVEL",
	"SVC_SETSECTORCOLOR",
	"SVC_SETSECTORFADE",
	"SVC_SETSECTORFLAT",
	"SVC_SETSECTORPANNING",
	"SVC_SETSECTORROTATION",
	"SVC_SETSECTORSCALE",
	"SVC_SETSECTORFRICTION",
	"SVC_SETSECTORANGLEYOFFSET",
	"SVC_SETSECTORGRAVITY",
	"SVC_STOPSECTORLIGHTEFFECT",
	"SVC_DESTROYALLSECTORMOVERS",
	"SVC_DOSECTORLIGHTFIREFLICKER",
	"SVC_DOSECTORLIGHTFLICKER",
	"SVC_DOSECTORLIGHTLIGHTFLASH",
	"SVC_DOSECTORLIGHTSTROBE",
	"SVC_DOSECTORLIGHTGLOW",
	"SVC_DOSECTORLIGHTGLOW2",
	"SVC_DOSECTORLIGHTPHASED",
	"SVC_SETLINEALPHA",
	"SVC_SETLINETEXTURE",
	"SVC_SETLINEBLOCKING",
	"SVC_SOUND",
	"SVC_SOUNDID",
	"SVC_SOUNDACTOR",
	"SVC_SOUNDIDACTOR",
	"SVC_SOUNDPOINT",
	"SVC_STARTSECTORSEQUENCE",
	"SVC_STOPSECTORSEQUENCE",
	"SVC_CALLVOTE",
	"SVC_PLAYERVOTE",
	"SVC_VOTEENDED",
	"SVC_MAPLOAD",
	"SVC_MAPNEW",
	"SVC_MAPEXIT",
	"SVC_MAPAUTHENTICATE",
	"SVC_SETMAPTIME",
	"SVC_SETMAPNUMKILLEDMONSTERS",
	"SVC_SETMAPNUMFOUNDITEMS",
	"SVC_SETMAPNUMFOUNDSECRETS",
	"SVC_SETMAPNUMTOTALMONSTERS",
	"SVC_SETMAPNUMTOTALITEMS",
	"SVC_SETMAPMUSIC",
	"SVC_SETMAPSKY",
	"SVC_GIVEINVENTORY",
	"SVC_TAKEINVENTORY",
	"SVC_GIVEPOWERUP",
	"SVC_DOINVENTORYPICKUP",
	"SVC_DESTROYALLINVENTORY",
	"SVC_DODOOR",
	"SVC_DESTROYDOOR",
	"SVC_CHANGEDOORDIRECTION",
	"SVC_DOFLOOR",
	"SVC_DESTROYFLOOR",
	"SVC_CHANGEFLOORDIRECTION",
	"SVC_CHANGEFLOORTYPE",
	"SVC_CHANGEFLOORDESTDIST",
	"SVC_STARTFLOORSOUND",
	"SVC_DOCEILING",
	"SVC_DESTROYCEILING",
	"SVC_CHANGECEILINGDIRECTION",
	"SVC_CHANGECEILINGSPEED",
	"SVC_PLAYCEILINGSOUND",
	"SVC_DOPLAT",
	"SVC_DESTROYPLAT",
	"SVC_CHANGEPLATSTATUS",
	"SVC_PLAYPLATSOUND",
	"SVC_DOELEVATOR",
	"SVC_DESTROYELEVATOR",
	"SVC_STARTELEVATORSOUND",
	"SVC_DOPILLAR",
	"SVC_DESTROYPILLAR",
	"SVC_DOWAGGLE",
	"SVC_DESTROYWAGGLE",
	"SVC_UPDATEWAGGLE",
	"SVC_DOROTATEPOLY",
	"SVC_DESTROYROTATEPOLY",
	"SVC_DOMOVEPOLY",
	"SVC_DESTROYMOVEPOLY",
	"SVC_DOPOLYDOOR",
	"SVC_DESTROYPOLYDOOR",
	"SVC_SETPOLYDOORSPEEDPOSITION",
	"SVC_PLAYPOLYOBJSOUND",
	"SVC_SETPOLYOBJPOSITION",
	"SVC_SETPOLYROTATION",
	"SVC_EARTHQUAKE",
	"SVC_SETQUEUEPOSITION",
	"SVC_DOSCROLLER",
	"SVC_GENERICCHEAT",
	"SVC_SETCAMERATOTEXTURE",
	"SVC_UPDATEPLAYERARMORDISPLAY",
	"SVC_UPDATEPLAYEREPENDINGWEAPON",
	"SVC_USEINVENTORY",
	"SVC_SETTHINGTID",
	"SVC_SETPLAYERAMMOCAPACITY",
	"SVC_SPAWNTHINGWITHTRANSNONETID",
	"SVC_RESPAWNRAVENTHING",
	"SVC_MOVETHINGEXACT",
	"SVC_SETTHINGSPECIAL2",
	"SVC_SETSCROLLER",


};

//*****************************************************************************
//	FUNCTIONS

// [BB] Some helper functions here to reduce the amunt of code duplications.
// TO-DO: Use them where suitably. As of now they are only used in
// client_SetPlayerAmmoCapacity
//*****************************************************************************
//
AInventory* FindPlayerInventory( ULONG ulPlayer, const PClass *pType )
{
	AInventory		*pInventory;
	// Try to find this object within the player's personal inventory.
	pInventory = players[ulPlayer].mo->FindInventory( pType );

	// If the player doesn't have this type, give it to him.
	if ( pInventory == NULL )
		pInventory = players[ulPlayer].mo->GiveInventoryType( pType );

	// If he still doesn't have the object after trying to give it to him... then YIKES!
	if ( pInventory == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "EnsurePlayerHasInventory: Failed to give inventory type, %s!\n", pType->TypeName.GetChars( ) );
#endif
	}
	return pInventory;
}

//*****************************************************************************
//
AInventory* FindPlayerInventory( ULONG ulPlayer, const char	*pszName )
{
	const PClass	*pType;
	pType = PClass::FindClass( pszName );
	if ( pType == NULL )
		return NULL;
	else
		return FindPlayerInventory( ulPlayer, pType );
}

//*****************************************************************************
//
void CLIENT_Construct( void )
{
    char		*pszPort;
	char		*pszIPAddress;
	char		*pszDemoName;
	ULONG		ulIdx;
	USHORT		usPort;
	UCVarValue	Val;

	// Start off as being disconnected.
	g_ConnectionState = CTS_DISCONNECTED;

	// Check if the user wants to use an alternate port for the server.
	pszPort = Args.CheckValue( "-port" );
    if ( pszPort )
    {
       usPort = atoi( pszPort );
       Printf( PRINT_HIGH, "Connecting using alternate port %i.\n", usPort );
    }
	else 
	   usPort = DEFAULT_CLIENT_PORT;

	// Set up a socket and network message buffer.
	NETWORK_Construct( usPort, true );

	NETWORK_InitBuffer( &g_LocalBuffer, MAX_UDP_PACKET * 8, BUFFERTYPE_WRITE );
	NETWORK_ClearBuffer( &g_LocalBuffer );

	// Initialize the stored packets buffer.
	g_ReceivedPacketBuffer.lMaxSize = MAX_UDP_PACKET * 256;
	memset( g_ReceivedPacketBuffer.abData, 0, MAX_UDP_PACKET * 256 );

	// Connect to a server right off the bat.
    pszIPAddress = Args.CheckValue( "-connect" );
    if ( pszIPAddress )
    {
		// Convert the given IP string into our server address.
		NETWORK_StringToAddress( pszIPAddress, &g_AddressServer );

		// If the user didn't specify a port, use the default one.
		if ( g_AddressServer.usPort == 0 )
			NETWORK_SetAddressPort( g_AddressServer, DEFAULT_SERVER_PORT );

		// If we try to reconnect, use this address.
		g_AddressLastConnected = g_AddressServer;

		// Put us in a "connection attempt" state.
		g_ConnectionState = CTS_TRYCONNECT;

		// Put the game in client mode.
		NETWORK_SetState( NETSTATE_CLIENT );

		// Make sure cheats are off.
		Val.Bool = false;
		sv_cheats.ForceSet( Val, CVAR_Bool );
		am_cheat = 0;

		// Make sure our visibility is normal.
		R_SetVisibility( 8.0f );

		for ( ulIdx = 0; ulIdx < MAXPLAYERS; ulIdx++ )
		{
			playeringame[ulIdx] = false;

			// Zero out all the player information.
			CLIENT_ResetPlayerData( &players[ulIdx] );
		}

		// If we've elected to record a demo, begin that process now.
		pszDemoName = Args.CheckValue( "-record" );
		if ( pszDemoName )
			CLIENTDEMO_BeginRecording( pszDemoName );
    }
	// User didn't specify an IP to connect to, so don't try to connect to anything.
	else
		g_ConnectionState = CTS_DISCONNECTED;
}

//*****************************************************************************
//
void CLIENT_Tick( void )
{
	// Reset the bytes in/out.
	g_lBytesSent = 0;
	g_lBytesReceived = 0;
	// Reset the max. every second.
	if (( gametic % TICRATE ) == 0 )
	{
		static	LONG	s_lLastGameticUpdate = 0;

		if ( gametic != s_lLastGameticUpdate )
		{
			g_lMaxBytesSentLastSecond = g_lMaxBytesSent;
			g_lMaxBytesReceivedLastSecond = g_lMaxBytesReceived;

			s_lLastGameticUpdate = gametic;
		}

		g_lMaxBytesSent = 0;
		g_lMaxBytesReceived = 0;
	}

	// Determine what to do based on our connection state.
	switch ( g_ConnectionState )
	{
	// Just chillin' at the full console. Nothing to do.
	case CTS_DISCONNECTED:

		break;
	// At the fullconsole, attempting to connect to a server.
	case CTS_TRYCONNECT:

		// Reset all this.
		g_bServerLagging = false;
		g_bClientLagging = false;

		// If we're not connected to a server, and have an IP specified, try to connect.
		if ( g_AddressServer.abIP[0] )
			CLIENT_SendConnectionSignal( );
		break;
	// A connection has been established with the server; now authenticate the level.
	case CTS_AUTHENTICATINGLEVEL:

		CLIENT_AttemptAuthentication( g_szMapName );
		break;
	// The level has been authenticated. Request level data and send user info.
	case CTS_CONNECTED:

		// Send data request and userinfo.
		CLIENT_AttemptConnection( );
		break;
	default:

		break;
	}
}

//*****************************************************************************
//
CONNECTIONSTATE_e CLIENT_GetConnectionState( void )
{
	return ( g_ConnectionState );
}

//*****************************************************************************
//
void CLIENT_SetConnectionState( CONNECTIONSTATE_e State )
{
	UCVarValue	Val;

	g_ConnectionState = State;
	switch ( g_ConnectionState )
	{
	case CTS_RECEIVINGSNAPSHOT:

		Val.Bool = false;
		cooperative.ForceSet( Val, CVAR_Bool );
		survival.ForceSet( Val, CVAR_Bool );
		invasion.ForceSet( Val, CVAR_Bool );

		deathmatch.ForceSet( Val, CVAR_Bool );
		teamplay.ForceSet( Val, CVAR_Bool );
		duel.ForceSet( Val, CVAR_Bool );
		terminator.ForceSet( Val, CVAR_Bool );
		lastmanstanding.ForceSet( Val, CVAR_Bool );
		teamlms.ForceSet( Val, CVAR_Bool );
		possession.ForceSet( Val, CVAR_Bool );
		teampossession.ForceSet( Val, CVAR_Bool );

		teamgame.ForceSet( Val, CVAR_Bool );
		ctf.ForceSet( Val, CVAR_Bool );
		oneflagctf.ForceSet( Val, CVAR_Bool );
		skulltag.ForceSet( Val, CVAR_Bool );

		sv_cheats.ForceSet( Val, CVAR_Bool );

		Val.Int = false;
		dmflags.ForceSet( Val, CVAR_Int );
		dmflags2.ForceSet( Val, CVAR_Int );
		compatflags.ForceSet( Val, CVAR_Int );

		gameskill.ForceSet( Val, CVAR_Int );
		botskill.ForceSet( Val, CVAR_Int );

		fraglimit.ForceSet( Val, CVAR_Int );
		timelimit.ForceSet( Val, CVAR_Int );
		pointlimit.ForceSet( Val, CVAR_Int );
		duellimit.ForceSet( Val, CVAR_Int );
		winlimit.ForceSet( Val, CVAR_Int );
		wavelimit.ForceSet( Val, CVAR_Int );
		break;
	}
}

//*****************************************************************************
//
NETBUFFER_s *CLIENT_GetLocalBuffer( void )
{
	return ( &g_LocalBuffer );
}

//*****************************************************************************
//
void CLIENT_SetLocalBuffer( NETBUFFER_s *pBuffer )
{
	g_LocalBuffer = *pBuffer;
}

//*****************************************************************************
//
ULONG CLIENT_GetLastServerTick( void )
{
	return ( g_ulLastServerTick );
}

//*****************************************************************************
//
void CLIENT_SetLastServerTick( ULONG ulTick )
{
	g_ulLastServerTick = ulTick;
}

//*****************************************************************************
//
ULONG CLIENT_GetLastConsolePlayerUpdateTick( void )
{
	return ( g_ulLastConsolePlayerUpdateTick );
}

//*****************************************************************************
//
void CLIENT_SetLastConsolePlayerUpdateTick( ULONG ulTick )
{
	g_ulLastConsolePlayerUpdateTick = ulTick;
}

//*****************************************************************************
//
bool CLIENT_GetServerLagging( void )
{
	return ( g_bServerLagging );
}

//*****************************************************************************
//
void CLIENT_SetServerLagging( bool bLagging )
{
	g_bServerLagging = bLagging;
}

//*****************************************************************************
//
bool CLIENT_GetClientLagging( void )
{
	return ( g_bClientLagging );
}

//*****************************************************************************
//
void CLIENT_SetClientLagging( bool bLagging )
{
	g_bClientLagging = bLagging;
}

//*****************************************************************************
//
NETADDRESS_s CLIENT_GetServerAddress( void )
{
	return ( g_AddressServer );
}

//*****************************************************************************
//
void CLIENT_SetServerAddress( NETADDRESS_s Address )
{
	g_AddressServer = Address;
}

//*****************************************************************************
//
ULONG CLIENT_GetBytesReceived( void )
{
	return ( g_lBytesReceived );
}

//*****************************************************************************
//
void CLIENT_AddBytesReceived( ULONG ulBytes )
{
	g_lBytesReceived += ulBytes;
	if ( g_lBytesReceived > g_lMaxBytesReceived )
		g_lMaxBytesReceived = g_lBytesReceived;
}

//*****************************************************************************
//
void CLIENT_SendConnectionSignal( void )
{
	ULONG	ulIdx;
	static	USHORT	s_usConnectionResendTimer = ( CONNECTION_RESEND_TIME / 2 );

	if ( s_usConnectionResendTimer == 0 )
	{
		s_usConnectionResendTimer = CONNECTION_RESEND_TIME;

		Printf( "Connecting to %s\n", NETWORK_AddressToString( g_AddressServer ));

		// Reset a bunch of stuff.
		NETWORK_ClearBuffer( &g_LocalBuffer );
		memset( g_ReceivedPacketBuffer.abData, 0, MAX_UDP_PACKET * 32 );
		for ( ulIdx = 0; ulIdx < 256; ulIdx++ )
		{
			g_lPacketBeginning[ulIdx] = 0;
			g_lPacketSequence[ulIdx] = -1;
			g_lPacketSize[ulIdx] = 0;
		}

		g_bPacketNum = 0;
		g_lCurrentPosition = 0;
		g_lLastParsedSequence = -1;
		g_lHighestReceivedSequence = -1;

		g_lMissingPacketTicks = 0;

		 // Send connection signal to the server.
		NETWORK_WriteByte( &g_LocalBuffer.ByteStream, CONNECT_CHALLENGE );
		NETWORK_WriteString( &g_LocalBuffer.ByteStream, DOTVERSIONSTR );
		NETWORK_WriteString( &g_LocalBuffer.ByteStream, cl_password.GetGenericRep( CVAR_String ).String );
		NETWORK_WriteByte( &g_LocalBuffer.ByteStream, cl_startasspectator );
		NETWORK_WriteByte( &g_LocalBuffer.ByteStream, cl_dontrestorefrags );
		NETWORK_WriteByte( &g_LocalBuffer.ByteStream, NETGAMEVERSION );

		g_lBytesSent += g_LocalBuffer.ulCurrentSize;
		if ( g_lBytesSent > g_lMaxBytesSent )
			g_lMaxBytesSent = g_lBytesSent;
		NETWORK_LaunchPacket( &g_LocalBuffer, g_AddressServer );
		NETWORK_ClearBuffer( &g_LocalBuffer );
	}

	s_usConnectionResendTimer--;
}

//*****************************************************************************
//
void CLIENT_AttemptAuthentication( char *pszMapName )
{
	ULONG	ulIdx;
	static	USHORT	s_usAuthenticationResendTimer = 0;

	if ( s_usAuthenticationResendTimer == 0 )
	{
		s_usAuthenticationResendTimer = GAMESTATE_RESEND_TIME;

		Printf( "Authenticating level...\n" );

		memset( g_lPacketSequence, -1, sizeof(g_lPacketSequence) );
		g_bPacketNum = 0;

		NETWORK_WriteByte( &g_LocalBuffer.ByteStream, CONNECT_AUTHENTICATING );

		// Send a checksum of our verticies, linedefs, sidedefs, and sectors.
		CLIENT_AuthenticateLevel( pszMapName );

		g_lBytesSent += g_LocalBuffer.ulCurrentSize;
		if ( g_lBytesSent > g_lMaxBytesSent )
			g_lMaxBytesSent = g_lBytesSent;
		NETWORK_LaunchPacket( &g_LocalBuffer, g_AddressServer );
		NETWORK_ClearBuffer( &g_LocalBuffer );
	}

	// Make sure all players are gone from the level.
	for ( ulIdx = 0; ulIdx < MAXPLAYERS; ulIdx++ )
	{
		playeringame[ulIdx] = false;

		// Zero out all the player information.
		CLIENT_ResetPlayerData( &players[ulIdx] );
	}

	s_usAuthenticationResendTimer--;
}

//*****************************************************************************
//
void CLIENT_AttemptConnection( void )
{
	ULONG	ulIdx;
	static	USHORT	s_usGetDataResendTimer = 0;

	if ( s_usGetDataResendTimer == 0 )
	{
		s_usGetDataResendTimer = GAMESTATE_RESEND_TIME;

		Printf( "Requesting gamestate...\n" );

		memset( g_lPacketSequence, -1, sizeof (g_lPacketSequence) );
		g_bPacketNum = 0;

		// Send them a message to get data from the server, along with our userinfo.
		NETWORK_WriteByte( &g_LocalBuffer.ByteStream, CONNECT_GETDATA );
		CLIENT_SendUserInfo( USERINFO_ALL );

		g_lBytesSent += g_LocalBuffer.ulCurrentSize;
		if ( g_lBytesSent > g_lMaxBytesSent )
			g_lMaxBytesSent = g_lBytesSent;
		NETWORK_LaunchPacket( &g_LocalBuffer, g_AddressServer );
		NETWORK_ClearBuffer( &g_LocalBuffer );
	}

	// Make sure all players are gone from the level.
	for ( ulIdx = 0; ulIdx < MAXPLAYERS; ulIdx++ )
	{
		playeringame[ulIdx] = false;

		// Zero out all the player information.
		CLIENT_ResetPlayerData( &players[ulIdx] );
	}

	s_usGetDataResendTimer--;
}

//*****************************************************************************
//
void CLIENT_QuitNetworkGame( void )
{
	ULONG	ulIdx;

	// Clear out the existing players.
	for ( ulIdx = 0; ulIdx < MAXPLAYERS; ulIdx++ )
	{
		playeringame[ulIdx] = false;

		// Zero out all the player information.
		CLIENT_ResetPlayerData( &players[ulIdx] );
	}

	// If we're connected in any way, send a disconnect signal.
	if ( g_ConnectionState != CTS_DISCONNECTED )
	{
		NETWORK_WriteByte( &g_LocalBuffer.ByteStream, CONNECT_QUIT );

		g_lBytesSent += g_LocalBuffer.ulCurrentSize;
		if ( g_lBytesSent > g_lMaxBytesSent )
			g_lMaxBytesSent = g_lBytesSent;
		NETWORK_LaunchPacket( &g_LocalBuffer, g_AddressServer );
		NETWORK_ClearBuffer( &g_LocalBuffer );
	}

	// Clear out our copy of the server address.
	memset( &g_AddressServer, 0, sizeof( g_AddressServer ));
	CLIENT_SetConnectionState( CTS_DISCONNECTED );

	// Go back to the full console.
	gameaction = ga_fullconsole;

	// View is no longer active.
	viewactive = false;

	g_lLastParsedSequence = -1;
	g_lHighestReceivedSequence = -1;

	g_lMissingPacketTicks = 0;

	// Set the network state back to single player.
	NETWORK_SetState( NETSTATE_SINGLE );

	// [BB] This prevents the status bar from showing up shortly, if you start a new game, while connected to a server.
	gamestate = GS_FULLCONSOLE;

	// If we're recording a demo, then finish it!
	if ( CLIENTDEMO_IsRecording( ))
		G_CheckDemoStatus( );
}

//*****************************************************************************
//
void CLIENT_QuitNetworkGame( char *pszError )
{
	Printf( "%s\n", pszError );
	CLIENT_QuitNetworkGame( );
}

//*****************************************************************************
//
void CLIENT_SendUserInfo( ULONG ulFlags )
{
	// Temporarily disable userinfo for when the player setup menu updates our userinfo. Then
	// we can just send all our userinfo in one big bulk, instead of each time it updates
	// a userinfo property.
	if ( g_bAllowSendingOfUserInfo == false )
		return;

	CLIENTCOMMANDS_UserInfo( ulFlags );
}

//*****************************************************************************
//
void CLIENT_SendCmd( void )
{		
	ticcmd_t	*cmd;
	ULONG		ulBits;

	if (( gametic < 1 ) || (( gamestate != GS_LEVEL ) && ( gamestate != GS_INTERMISSION )))
		return;

	if ( players[consoleplayer].mo == NULL )
		return;

	// If we're at intermission, and toggling our "ready to go" status, tell the server.
	if ( gamestate == GS_INTERMISSION )
	{
		if (( players[consoleplayer].cmd.ucmd.buttons ^ players[consoleplayer].oldbuttons ) &&
			(( players[consoleplayer].cmd.ucmd.buttons & players[consoleplayer].oldbuttons ) == players[consoleplayer].oldbuttons ))
		{
			CLIENTCOMMANDS_ReadyToGoOn( );
		}

		players[consoleplayer].oldbuttons = players[consoleplayer].cmd.ucmd.buttons;
	}

	// Don't send movement information if we're spectating!
	if ( players[consoleplayer].bSpectating )
	{
		if (( gametic % ( TICRATE * 2 )) == 0 )
		{
			// Send the gametic.
			CLIENTCOMMANDS_SpectateInfo( );
		}
	}

	// Not in a level or spectating; nothing to do!
	if (( gamestate != GS_LEVEL ) || ( players[consoleplayer].bSpectating ))
	{
		if ( NETWORK_CalcBufferSize( &g_LocalBuffer ))
		{
			g_lBytesSent += g_LocalBuffer.ulCurrentSize;
			if ( g_lBytesSent > g_lMaxBytesSent )
				g_lMaxBytesSent = g_lBytesSent;
			NETWORK_LaunchPacket( &g_LocalBuffer, g_AddressServer );
			NETWORK_ClearBuffer( &g_LocalBuffer );
		}
		return;
	}

	// Initialize the bits.
	ulBits = 0;

	// Send the move header and the gametic.
	NETWORK_WriteByte( &g_LocalBuffer.ByteStream, CLC_CLIENTMOVE );
	NETWORK_WriteLong( &g_LocalBuffer.ByteStream, gametic );

	// Decide what additional information needs to be sent.
	cmd = &players[consoleplayer].cmd;
	if ( cmd->ucmd.buttons )
		ulBits |= CLIENT_UPDATE_BUTTONS;
	if ( cmd->ucmd.forwardmove )
		ulBits |= CLIENT_UPDATE_FORWARDMOVE;
	if ( cmd->ucmd.sidemove )
		ulBits |= CLIENT_UPDATE_SIDEMOVE;
	if ( cmd->ucmd.upmove )
		ulBits |= CLIENT_UPDATE_UPMOVE;

	// Tell the server what information we'll be sending.
	NETWORK_WriteByte( &g_LocalBuffer.ByteStream, ulBits );

	// Send the necessary movement/steering information.
	if ( ulBits & CLIENT_UPDATE_BUTTONS )
	{
		if ( iwanttousecrouchingeventhoughitsretardedandunnecessaryanditsimplementationishorribleimeanverticallyshrinkingskinscomeonthatsinsanebutwhatevergoaheadandhaveyourcrouching == false )
			NETWORK_WriteByte( &g_LocalBuffer.ByteStream, cmd->ucmd.buttons & ~BT_DUCK );
		else
			NETWORK_WriteByte( &g_LocalBuffer.ByteStream, cmd->ucmd.buttons );
	}
	if ( ulBits & CLIENT_UPDATE_FORWARDMOVE )
		NETWORK_WriteShort( &g_LocalBuffer.ByteStream, cmd->ucmd.forwardmove );
	if ( ulBits & CLIENT_UPDATE_SIDEMOVE )
		NETWORK_WriteShort( &g_LocalBuffer.ByteStream, cmd->ucmd.sidemove );
	if ( ulBits & CLIENT_UPDATE_UPMOVE )
		NETWORK_WriteShort( &g_LocalBuffer.ByteStream, cmd->ucmd.upmove );

	NETWORK_WriteLong( &g_LocalBuffer.ByteStream, players[consoleplayer].mo->angle );
	NETWORK_WriteLong( &g_LocalBuffer.ByteStream, players[consoleplayer].mo->pitch );

	// Attack button.
	if ( cmd->ucmd.buttons & BT_ATTACK )
	{
		if ( players[consoleplayer].ReadyWeapon == NULL )
			NETWORK_WriteString( &g_LocalBuffer.ByteStream, "NULL" );
		else
			NETWORK_WriteString( &g_LocalBuffer.ByteStream, (char *)players[consoleplayer].ReadyWeapon->GetClass( )->TypeName.GetChars( ));
	}

	g_lBytesSent += g_LocalBuffer.ulCurrentSize;
	if ( g_lBytesSent > g_lMaxBytesSent )
		g_lMaxBytesSent = g_lBytesSent;
	NETWORK_LaunchPacket( &g_LocalBuffer, g_AddressServer );
	NETWORK_ClearBuffer( &g_LocalBuffer );
}

//*****************************************************************************
//

void client_GenerateAndSendMapLumpMD5Hash( MapData *Map, const LONG LumpNumber ){
	char		szChecksum[64];
	// Generate the checksum string.
	generateMapLumpMD5Hash( Map, LumpNumber, szChecksum );
	// Now, send the vertex checksum string.
	NETWORK_WriteString( &g_LocalBuffer.ByteStream, szChecksum );
}

//*****************************************************************************
//
void CLIENT_AuthenticateLevel( char *pszMapName )
{
	// [BB] Check if the wads contain the map at all. If not, don't send any checksums.
	MapData* map = P_OpenMapData( pszMapName );
	if(  map == NULL )
	{
		Printf( "map %s not found!\n", pszMapName );
		return;
	}
	else{
		// Generate and send checksums for the map lumps:
		// VERTICIES
		client_GenerateAndSendMapLumpMD5Hash( map, ML_VERTEXES );
		// LINEDEFS
		client_GenerateAndSendMapLumpMD5Hash( map, ML_LINEDEFS );
		// SIDEDEFS
		client_GenerateAndSendMapLumpMD5Hash( map, ML_SIDEDEFS );
		// SECTORS
		client_GenerateAndSendMapLumpMD5Hash( map, ML_SECTORS );
		delete map;
	}
}

//*****************************************************************************
//
bool CLIENT_ReadPacketHeader( BYTESTREAM_s *pByteStream )
{
	LONG	lIdx;
	LONG	lCommand;
	LONG	lSequence;

	// Read in the command. Since it's the first one in the packet, it should be
	// SVC_HEADER or SVC_UNRELIABLEPACKET.
	lCommand = NETWORK_ReadByte( pByteStream );

	// If this is an unreliable packet, just break out of here and begin parsing it. There's no
	// need to store it.
	if ( lCommand == SVC_UNRELIABLEPACKET )
		return ( true );
	else

	// Read in the sequence. This is the # of the packet the server has sent us.
	lSequence = NETWORK_ReadLong( pByteStream );
	if ( lCommand != SVC_HEADER )
		Printf( "CLIENT_ReadPacketHeader: WARNING! Expected SVC_HEADER or SVC_UNRELIABLEPACKET!\n" );

	// Check to see if we've already received this packet. If so, skip it.
	for ( lIdx = 0; lIdx < 256; lIdx++ )
	{
		if ( g_lPacketSequence[lIdx] == lSequence )
			return ( false );
	}

	// The end of the buffer has been reached.
	if (( g_lCurrentPosition + ( NETWORK_CalcBufferSize( NETWORK_GetNetworkMessageBuffer( )))) >= g_ReceivedPacketBuffer.lMaxSize )
		g_lCurrentPosition = 0;

	// Save a bunch of information about this incoming packet.
	g_lPacketBeginning[g_bPacketNum] = g_lCurrentPosition;
	g_lPacketSize[g_bPacketNum] = NETWORK_CalcBufferSize( NETWORK_GetNetworkMessageBuffer( ));
	g_lPacketSequence[g_bPacketNum] = lSequence;

	// Save the received packet.
	memcpy( g_ReceivedPacketBuffer.abData + g_lPacketBeginning[g_bPacketNum], NETWORK_GetNetworkMessageBuffer( )->ByteStream.pbStream, NETWORK_CalcBufferSize( NETWORK_GetNetworkMessageBuffer( )));
	g_lCurrentPosition += NETWORK_CalcBufferSize( NETWORK_GetNetworkMessageBuffer( ));

	if ( lSequence > g_lHighestReceivedSequence )
		g_lHighestReceivedSequence = lSequence;

	g_bPacketNum++;

	return ( false );
}

//*****************************************************************************
//
void CLIENT_ParsePacket( BYTESTREAM_s *pByteStream, bool bSequencedPacket )
{
	LONG	lCommand;

	while ( 1 )
	{  
		lCommand = NETWORK_ReadByte( pByteStream );

		// End of message.
		if ( lCommand == -1 )
			break;

#ifdef _DEBUG
		// Option to print commands for debugging purposes.
		if ( cl_showcommands )
			CLIENT_PrintCommand( lCommand );
#endif

		// Process this command.
		CLIENT_ProcessCommand( lCommand, pByteStream );

		g_lLastCmd = lCommand;
	}

	if ( debugfile )
		fprintf( debugfile, "End parsing packet.\n" );

	if ( bSequencedPacket )
		g_lLastParsedSequence++;
}

//*****************************************************************************
//
void CLIENT_PrintCommand( LONG lCommand )
{
	char	*pszString;

	if ( lCommand >= NUM_SERVER_COMMANDS )
	{
		switch ( lCommand )
		{
		case CONNECT_CHALLENGE:

			pszString = "CONNECT_CHALLENGE";
			break;
		case CONNECT_READY:

			pszString = "CONNECT_READY";
			break;
		case CONNECT_GETDATA:

			pszString = "CONNECT_GETDATA";
			break;
		case CONNECT_QUIT:

			pszString = "CONNECT_QUIT";
			break;
		case CONNECT_AUTHENTICATED:

			pszString = "CONNECT_AUTHENTICATED";
			break;
		case CONNECT_AUTHENTICATING:

			pszString = "CONNECT_AUTHENTICATING";
			break;
		case NETWORK_ERROR:

			pszString = "NETWORK_ERROR";
			break;
		}
	}
	else
	{
		if (( cl_showcommands >= 2 ) && ( lCommand == SVC_MOVELOCALPLAYER ))
			return;
		if (( cl_showcommands >= 3 ) && ( lCommand == SVC_MOVEPLAYER ))
			return;
		if (( cl_showcommands >= 4 ) && ( lCommand == SVC_UPDATEPLAYEREXTRADATA ))
			return;

		pszString = g_pszHeaderNames[lCommand];
	}

	Printf( "%s\n", pszString );

	if ( debugfile )
		fprintf( debugfile, "%s\n", pszString );
}

//*****************************************************************************
//
void CLIENT_ProcessCommand( LONG lCommand, BYTESTREAM_s *pByteStream )
{
	char	szString[128];

	switch ( lCommand )
	{
	case CONNECT_READY:

		// Print a status message.
		Printf( "Connected!\n" );

		// Read in the map name we now need to authenticate.
		sprintf( g_szMapName, "%s", NETWORK_ReadString( pByteStream ));

		// The next step is the authenticate the level.
		if ( CLIENTDEMO_IsPlaying( ) == false )
		{
			CLIENT_SetConnectionState( CTS_AUTHENTICATINGLEVEL );
			CLIENT_AttemptAuthentication( g_szMapName );
		}
		break;
	case CONNECT_AUTHENTICATED:

		// Print a status message.
		Printf( "Level authenticated!\n" );

		// We've been authenticated, and are waiting to receive a snapshot.
		if ( CLIENTDEMO_IsPlaying( ) == false )
		{
			CLIENT_SetConnectionState( CTS_CONNECTED );
			CLIENT_AttemptConnection( );
		}
		break;
	case NETWORK_ERROR:
		{
			char	szErrorString[256];
			ULONG	ulErrorCode;

			// Read in the error code.
			ulErrorCode = NETWORK_ReadByte( pByteStream );

			// Build the error string based on the error code.
			switch ( ulErrorCode )
			{
			case NETWORK_ERRORCODE_WRONGPASSWORD:

				sprintf( szErrorString, "Incorrect password." );
				break;
			case NETWORK_ERRORCODE_WRONGVERSION:

				sprintf( szErrorString, "Failed connect. Your version is different.\nThis server is using version: %s\nPlease check http://www.skulltag.com/ for updates.", NETWORK_ReadString( pByteStream ));
				break;
			case NETWORK_ERRORCODE_WRONGPROTOCOLVERSION:

				sprintf( szErrorString, "Failed connect. Your version uses outdated network code.\nPlease check http://www.skulltag.com/ for updates." );
				break;
			case NETWORK_ERRORCODE_BANNED:

				sprintf( szErrorString, "Couldn't connect. You have been banned from this server!" );
				break;
			case NETWORK_ERRORCODE_SERVERISFULL:

				sprintf( szErrorString, "Server is full." );
				break;
			case NETWORK_ERRORCODE_AUTHENTICATIONFAILED:

				sprintf( szErrorString, "Level authentication failed.\nPlease make sure you are using the exact same WAD(s) as the server, and try again." );
				break;
			case NETWORK_ERRORCODE_FAILEDTOSENDUSERINFO:

				sprintf( szErrorString, "Failed to send userinfo." );
				break;
			default:

				sprintf( szErrorString, "Unknown error code: %d!\n\nYour version may be different. Please check http://www.skulltag.com/ for updates.", ulErrorCode );
				break;
			}

			CLIENT_QuitNetworkGame( szErrorString );
		}
		return;
	case SVC_HEADER:

		client_Header( pByteStream );
		break;
	case SVC_RESETSEQUENCE:

		client_ResetSequence( pByteStream );
		break;
	case SVC_PING:

		client_Ping( pByteStream );
		break;
	case SVC_NOTHING:

		break;
	case SVC_BEGINSNAPSHOT:

		client_BeginSnapshot( pByteStream );
		break;
	case SVC_ENDSNAPSHOT:

		client_EndSnapshot( pByteStream );
		break;
	case SVC_SPAWNPLAYER:

		client_SpawnPlayer( pByteStream );
		break;
	case SVC_MOVEPLAYER:

		client_MovePlayer( pByteStream );
		break;
	case SVC_DAMAGEPLAYER:

		client_DamagePlayer( pByteStream );
		break;
	case SVC_KILLPLAYER:

		client_KillPlayer( pByteStream );
		break;
	case SVC_SETPLAYERHEALTH:

		client_SetPlayerHealth( pByteStream );
		break;
	case SVC_UPDATEPLAYERARMORDISPLAY:

		client_UpdatePlayerArmorDisplay( pByteStream );
		break;
	case SVC_SETPLAYERSTATE:

		client_SetPlayerState( pByteStream );
		break;
	case SVC_SETPLAYERUSERINFO:

		client_SetPlayerUserInfo( pByteStream );
		break;
	case SVC_SETPLAYERFRAGS:

		client_SetPlayerFrags( pByteStream );
		break;
	case SVC_SETPLAYERPOINTS:

		client_SetPlayerPoints( pByteStream );
		break;
	case SVC_SETPLAYERWINS:

		client_SetPlayerWins( pByteStream );
		break;
	case SVC_SETPLAYERKILLCOUNT:

		client_SetPlayerKillCount( pByteStream );
		break;
	case SVC_SETPLAYERCHATSTATUS:

		client_SetPlayerChatStatus( pByteStream );
		break;
	case SVC_SETPLAYERLAGGINGSTATUS:

		client_SetPlayerLaggingStatus( pByteStream );
		break;
	case SVC_SETPLAYERREADYTOGOONSTATUS:

		client_SetPlayerReadyToGoOnStatus( pByteStream );
		break;
	case SVC_SETPLAYERTEAM:

		client_SetPlayerTeam( pByteStream );
		break;
	case SVC_SETPLAYERCAMERA:

		client_SetPlayerCamera( pByteStream );
		break;
	case SVC_UPDATEPLAYERPING:

		client_UpdatePlayerPing( pByteStream );
		break;
	case SVC_UPDATEPLAYEREXTRADATA:

		client_UpdatePlayerExtraData( pByteStream );
		break;
	case SVC_UPDATEPLAYEREPENDINGWEAPON:

		client_UpdatePlayerPendingWeapon( pByteStream );
		break;
	case SVC_USEINVENTORY:

		client_DoInventoryUse( pByteStream );
		break;
	case SVC_UPDATEPLAYERTIME:

		client_UpdatePlayerTime( pByteStream );
		break;
	case SVC_MOVELOCALPLAYER:

		client_MoveLocalPlayer( pByteStream );
		break;
	case SVC_DISCONNECTPLAYER:

		client_DisconnectPlayer( pByteStream );
		break;
	case SVC_SETCONSOLEPLAYER:

		client_SetConsolePlayer( pByteStream );
		break;
	case SVC_CONSOLEPLAYERKICKED:

		client_ConsolePlayerKicked( pByteStream );
		break;
	case SVC_GIVEPLAYERMEDAL:

		client_GivePlayerMedal( pByteStream );
		break;
	case SVC_RESETALLPLAYERSFRAGCOUNT:

		client_ResetAllPlayersFragcount( pByteStream );
		break;
	case SVC_PLAYERISSPECTATOR:

		client_PlayerIsSpectator( pByteStream );
		break;
	case SVC_PLAYERSAY:

		client_PlayerSay( pByteStream );
		break;
	case SVC_PLAYERTAUNT:

		client_PlayerTaunt( pByteStream );
		break;
	case SVC_PLAYERRESPAWNINVULNERABILITY:

		client_PlayerRespawnInvulnerability( pByteStream );
		break;
	case SVC_SETPLAYERAMMOCAPACITY:

		client_SetPlayerAmmoCapacity( pByteStream );
		break;
	case SVC_SPAWNTHING:

		client_SpawnThing( pByteStream );
		break;
	case SVC_SPAWNTHINGNONETID:

		client_SpawnThingNoNetID( pByteStream );
		break;
	case SVC_SPAWNTHINGWITHTRANSNONETID:

		client_SpawnThingNoNetID( pByteStream, true );
		break;
	case SVC_SPAWNTHINGEXACT:

		client_SpawnThingExact( pByteStream );
		break;
	case SVC_SPAWNTHINGEXACTNONETID:

		client_SpawnThingExactNoNetID( pByteStream );
		break;
	case SVC_MOVETHING:

		client_MoveThing( pByteStream );
		break;
	case SVC_MOVETHINGEXACT:

		client_MoveThingExact( pByteStream );
		break;
	case SVC_DAMAGETHING:

		client_DamageThing( pByteStream );
		break;
	case SVC_KILLTHING:

		client_KillThing( pByteStream );
		break;
	case SVC_SETTHINGSTATE:

		client_SetThingState( pByteStream );
		break;
	case SVC_DESTROYTHING:

		client_DestroyThing( pByteStream );
		break;
	case SVC_SETTHINGANGLE:

		client_SetThingAngle( pByteStream );
		break;
	case SVC_SETTHINGTID:

		client_SetThingTID( pByteStream );
		break;
	case SVC_SETTHINGWATERLEVEL:

		client_SetThingWaterLevel( pByteStream );
		break;
	case SVC_SETTHINGFLAGS:

		client_SetThingFlags( pByteStream );
		break;
	case SVC_SETTHINGARGUMENTS:

		client_SetThingArguments( pByteStream );
		break;
	case SVC_SETTHINGTRANSLATION:

		client_SetThingTranslation( pByteStream );
		break;
	case SVC_SETTHINGPROPERTY:

		client_SetThingProperty( pByteStream );
		break;
	case SVC_SETTHINGSOUND:

		client_SetThingSound( pByteStream );
		break;
	case SVC_SETTHINGSPECIAL2:

		client_SetThingSpecial2( pByteStream );
		break;
	case SVC_SETWEAPONAMMOGIVE:

		client_SetWeaponAmmoGive( pByteStream );
		break;
	case SVC_THINGISCORPSE:

		client_ThingIsCorpse( pByteStream );
		break;
	case SVC_HIDETHING:

		client_HideThing( pByteStream );
		break;
	case SVC_TELEPORTTHING:

		client_TeleportThing( pByteStream );
		break;
	case SVC_THINGACTIVATE:

		client_ThingActivate( pByteStream );
		break;
	case SVC_THINGDEACTIVATE:

		client_ThingDeactivate( pByteStream );
		break;
	case SVC_RESPAWNDOOMTHING:

		client_RespawnDoomThing( pByteStream );
		break;
	case SVC_RESPAWNRAVENTHING:

		client_RespawnRavenThing( pByteStream );
		break;
	case SVC_PRINT:

		client_Print( pByteStream );
		break;
	case SVC_PRINTMID:

		client_PrintMid( pByteStream );
		break;
	case SVC_PRINTMOTD:

		client_PrintMOTD( pByteStream );
		break;
	case SVC_PRINTHUDMESSAGE:

		client_PrintHUDMessage( pByteStream );
		break;
	case SVC_PRINTHUDMESSAGEFADEOUT:

		client_PrintHUDMessageFadeOut( pByteStream );
		break;
	case SVC_PRINTHUDMESSAGEFADEINOUT:

		client_PrintHUDMessageFadeInOut( pByteStream );
		break;
	case SVC_PRINTHUDMESSAGETYPEONFADEOUT:

		client_PrintHUDMessageTypeOnFadeOut( pByteStream );
		break;
	case SVC_SETGAMEMODE:

		client_SetGameMode( pByteStream );
		break;
	case SVC_SETGAMESKILL:

		client_SetGameSkill( pByteStream );
		break;
	case SVC_SETGAMEDMFLAGS:

		client_SetGameDMFlags( pByteStream );
		break;
	case SVC_SETGAMEMODELIMITS:

		client_SetGameModeLimits( pByteStream );
		break;
	case SVC_SETGAMEENDLEVELDELAY:
		
		client_SetGameEndLevelDelay( pByteStream );
		break;
	case SVC_SETGAMEMODESTATE:

		client_SetGameModeState( pByteStream );
		break;
	case SVC_SETDUELNUMDUELS:

		client_SetDuelNumDuels( pByteStream );
		break;
	case SVC_SETLMSSPECTATORSETTINGS:

		client_SetLMSSpectatorSettings( pByteStream );
		break;
	case SVC_SETLMSALLOWEDWEAPONS:

		client_SetLMSAllowedWeapons( pByteStream );
		break;
	case SVC_SETINVASIONNUMMONSTERSLEFT:

		client_SetInvasionNumMonstersLeft( pByteStream );
		break;
	case SVC_SETINVASIONWAVE:

		client_SetInvasionWave( pByteStream );
		break;
	case SVC_DOPOSSESSIONARTIFACTPICKEDUP:

		client_DoPossessionArtifactPickedUp( pByteStream );
		break;
	case SVC_DOPOSSESSIONARTIFACTDROPPED:

		client_DoPossessionArtifactDropped( pByteStream );
		break;
	case SVC_DOGAMEMODEFIGHT:

		client_DoGameModeFight( pByteStream );
		break;
	case SVC_DOGAMEMODECOUNTDOWN:

		client_DoGameModeCountdown( pByteStream );
		break;
	case SVC_DOGAMEMODEWINSEQUENCE:

		client_DoGameModeWinSequence( pByteStream );
		break;
	case SVC_SETTEAMFRAGS:

		client_SetTeamFrags( pByteStream );
		break;
	case SVC_SETTEAMSCORE:

		client_SetTeamScore( pByteStream );
		break;
	case SVC_SETTEAMWINS:

		client_SetTeamWins( pByteStream );
		break;
	case SVC_SETTEAMRETURNTICKS:

		client_SetTeamReturnTicks( pByteStream );
		break;
	case SVC_TEAMFLAGRETURNED:

		client_TeamFlagReturned( pByteStream );
		break;
	case SVC_SPAWNMISSILE:

		client_SpawnMissile( pByteStream );
		break;
	case SVC_SPAWNMISSILEEXACT:

		client_SpawnMissileExact( pByteStream );
		break;
	case SVC_MISSILEEXPLODE:

		client_MissileExplode( pByteStream );
		break;
	case SVC_WEAPONSOUND:

		client_WeaponSound( pByteStream );
		break;
	case SVC_WEAPONCHANGE:

		client_WeaponChange( pByteStream );
		break;
	case SVC_WEAPONRAILGUN:

		client_WeaponRailgun( pByteStream );
		break;
	case SVC_SETSECTORFLOORPLANE:

		client_SetSectorFloorPlane( pByteStream );
		break;
	case SVC_SETSECTORCEILINGPLANE:

		client_SetSectorCeilingPlane( pByteStream );
		break;
	case SVC_SETSECTORLIGHTLEVEL:

		client_SetSectorLightLevel( pByteStream );
		break;
	case SVC_SETSECTORCOLOR:

		client_SetSectorColor( pByteStream );
		break;
	case SVC_SETSECTORFADE:

		client_SetSectorFade( pByteStream );
		break;
	case SVC_SETSECTORFLAT:

		client_SetSectorFlat( pByteStream );
		break;
	case SVC_SETSECTORPANNING:

		client_SetSectorPanning( pByteStream );
		break;
	case SVC_SETSECTORROTATION:

		client_SetSectorRotation( pByteStream );
		break;
	case SVC_SETSECTORSCALE:

		client_SetSectorScale( pByteStream );
		break;
	case SVC_SETSECTORFRICTION:

		client_SetSectorFriction( pByteStream );
		break;
	case SVC_SETSECTORANGLEYOFFSET:

		client_SetSectorAngleYOffset( pByteStream );
		break;
	case SVC_SETSECTORGRAVITY:

		client_SetSectorGravity( pByteStream );
		break;
	case SVC_STOPSECTORLIGHTEFFECT:

		client_StopSectorLightEffect( pByteStream );
		break;
	case SVC_DESTROYALLSECTORMOVERS:

		client_DestroyAllSectorMovers( pByteStream );
		break;
	case SVC_DOSECTORLIGHTFIREFLICKER:

		client_DoSectorLightFireFlicker( pByteStream );
		break;
	case SVC_DOSECTORLIGHTFLICKER:

		client_DoSectorLightFlicker( pByteStream );
		break;
	case SVC_DOSECTORLIGHTLIGHTFLASH:

		client_DoSectorLightLightFlash( pByteStream );
		break;
	case SVC_DOSECTORLIGHTSTROBE:

		client_DoSectorLightStrobe( pByteStream );
		break;
	case SVC_DOSECTORLIGHTGLOW:

		client_DoSectorLightGlow( pByteStream );
		break;
	case SVC_DOSECTORLIGHTGLOW2:

		client_DoSectorLightGlow2( pByteStream );
		break;
	case SVC_DOSECTORLIGHTPHASED:

		client_DoSectorLightPhased( pByteStream );
		break;
	case SVC_SETLINEALPHA:

		client_SetLineAlpha( pByteStream );
		break;
	case SVC_SETLINETEXTURE:

		client_SetLineTexture( pByteStream );
		break;
	case SVC_SETLINEBLOCKING:

		client_SetLineBlocking( pByteStream );
		break;
	case SVC_SOUND:

		client_Sound( pByteStream );
		break;
	case SVC_SOUNDID:

		client_SoundID( pByteStream );
		break;
	case SVC_SOUNDACTOR:

		client_SoundActor( pByteStream );
		break;
	case SVC_SOUNDIDACTOR:

		client_SoundIDActor( pByteStream );
		break;
	case SVC_SOUNDPOINT:

		client_SoundPoint( pByteStream );
		break;
	case SVC_STARTSECTORSEQUENCE:

		client_StartSectorSequence( pByteStream );
		break;
	case SVC_STOPSECTORSEQUENCE:

		client_StopSectorSequence( pByteStream );
		break;
	case SVC_CALLVOTE:

		client_CallVote( pByteStream );
		break;
	case SVC_PLAYERVOTE:

		client_PlayerVote( pByteStream );
		break;
	case SVC_VOTEENDED:

		client_VoteEnded( pByteStream );
		break;
	case SVC_MAPLOAD:

		client_MapLoad( pByteStream );
		break;
	case SVC_MAPNEW:

		client_MapNew( pByteStream );
		break;
	case SVC_MAPEXIT:

		client_MapExit( pByteStream );
		break;
	case SVC_MAPAUTHENTICATE:

		client_MapAuthenticate( pByteStream );
		break;
	case SVC_SETMAPTIME:

		client_SetMapTime( pByteStream );
		break;
	case SVC_SETMAPNUMKILLEDMONSTERS:

		client_SetMapNumKilledMonsters( pByteStream );
		break;
	case SVC_SETMAPNUMFOUNDITEMS:

		client_SetMapNumFoundItems( pByteStream );
		break;
	case SVC_SETMAPNUMFOUNDSECRETS:

		client_SetMapNumFoundSecrets( pByteStream );
		break;
	case SVC_SETMAPNUMTOTALMONSTERS:

		client_SetMapNumTotalMonsters( pByteStream );
		break;
	case SVC_SETMAPNUMTOTALITEMS:

		client_SetMapNumTotalItems( pByteStream );
		break;
	case SVC_SETMAPMUSIC:

		client_SetMapMusic( pByteStream );
		break;
	case SVC_SETMAPSKY:

		client_SetMapSky( pByteStream );
		break;
	case SVC_GIVEINVENTORY:

		client_GiveInventory( pByteStream );
		break;
	case SVC_TAKEINVENTORY:

		client_TakeInventory( pByteStream );
		break;
	case SVC_GIVEPOWERUP:

		client_GivePowerup( pByteStream );
		break;
	case SVC_DOINVENTORYPICKUP:

		client_DoInventoryPickup( pByteStream );
		break;
	case SVC_DESTROYALLINVENTORY:

		client_DestroyAllInventory( pByteStream );
		break;
	case SVC_DODOOR:

		client_DoDoor( pByteStream );
		break;
	case SVC_DESTROYDOOR:

		client_DestroyDoor( pByteStream );
		break;
	case SVC_CHANGEDOORDIRECTION:

		client_ChangeDoorDirection( pByteStream );
		break;
	case SVC_DOFLOOR:

		client_DoFloor( pByteStream );
		break;
	case SVC_DESTROYFLOOR:

		client_DestroyFloor( pByteStream );
		break;
	case SVC_CHANGEFLOORDIRECTION:

		client_ChangeFloorDirection( pByteStream );
		break;
	case SVC_CHANGEFLOORTYPE:

		client_ChangeFloorType( pByteStream );
		break;
	case SVC_CHANGEFLOORDESTDIST:

		client_ChangeFloorDestDist( pByteStream );
		break;
	case SVC_STARTFLOORSOUND:

		client_StartFloorSound( pByteStream );
		break;
	case SVC_DOCEILING:

		client_DoCeiling( pByteStream );
		break;
	case SVC_DESTROYCEILING:

		client_DestroyCeiling( pByteStream );
		break;
	case SVC_CHANGECEILINGDIRECTION:

		client_ChangeCeilingDirection( pByteStream );
		break;
	case SVC_CHANGECEILINGSPEED:

		client_ChangeCeilingSpeed( pByteStream );
		break;
	case SVC_PLAYCEILINGSOUND:

		client_PlayCeilingSound( pByteStream );
		break;
	case SVC_DOPLAT:

		client_DoPlat( pByteStream );
		break;
	case SVC_DESTROYPLAT:

		client_DestroyPlat( pByteStream );
		break;
	case SVC_CHANGEPLATSTATUS:

		client_ChangePlatStatus( pByteStream );
		break;
	case SVC_PLAYPLATSOUND:

		client_PlayPlatSound( pByteStream );
		break;
	case SVC_DOELEVATOR:

		client_DoElevator( pByteStream );
		break;
	case SVC_DESTROYELEVATOR:

		client_DestroyElevator( pByteStream );
		break;
	case SVC_STARTELEVATORSOUND:

		client_StartElevatorSound( pByteStream );
		break;
	case SVC_DOPILLAR:

		client_DoPillar( pByteStream );
		break;
	case SVC_DESTROYPILLAR:

		client_DestroyPillar( pByteStream );
		break;
	case SVC_DOWAGGLE:

		client_DoWaggle( pByteStream );
		break;
	case SVC_DESTROYWAGGLE:

		client_DestroyWaggle( pByteStream );
		break;
	case SVC_UPDATEWAGGLE:

		client_UpdateWaggle( pByteStream );
		break;
	case SVC_DOROTATEPOLY:

		client_DoRotatePoly( pByteStream );
		break;
	case SVC_DESTROYROTATEPOLY:

		client_DestroyRotatePoly( pByteStream );
		break;
	case SVC_DOMOVEPOLY:

		client_DoMovePoly( pByteStream );
		break;
	case SVC_DESTROYMOVEPOLY:

		client_DestroyMovePoly( pByteStream );
		break;
	case SVC_DOPOLYDOOR:

		client_DoPolyDoor( pByteStream );
		break;
	case SVC_DESTROYPOLYDOOR:

		client_DestroyPolyDoor( pByteStream );
		break;
	case SVC_SETPOLYDOORSPEEDPOSITION:

		client_SetPolyDoorSpeedPosition( pByteStream );
		break;
	case SVC_PLAYPOLYOBJSOUND:

		client_PlayPolyobjSound( pByteStream );
		break;
	case SVC_SETPOLYOBJPOSITION:

		client_SetPolyobjPosition( pByteStream );
		break;
	case SVC_SETPOLYOBJROTATION:

		client_SetPolyobjRotation( pByteStream );
		break;
	case SVC_EARTHQUAKE:

		client_EarthQuake( pByteStream );
		break;
	case SVC_SETQUEUEPOSITION:

		client_SetQueuePosition( pByteStream );
		break;
	case SVC_DOSCROLLER:

		client_DoScroller( pByteStream );
		break;
	case SVC_SETSCROLLER:

		client_SetScroller( pByteStream );
		break;
	case SVC_GENERICCHEAT:

		client_GenericCheat( pByteStream );
		break;
	case SVC_SETCAMERATOTEXTURE:

		client_SetCameraToTexture( pByteStream );
		break;
	default:

		sprintf( szString, "CLIENT_ParsePacket: Illegible server message: %d\nLast command: (%s)\n", lCommand, g_pszHeaderNames[g_lLastCmd] );
		CLIENT_QuitNetworkGame( szString );
		return;
	}
}

//*****************************************************************************
//
AActor* CLIENT_SpawnThing( char *pszName, fixed_t X, fixed_t Y, fixed_t Z, LONG lNetID )
{
	AActor			*pActor;
	const PClass	*pType;

	// Only spawn actors if we're actually in a level.
	if ( gamestate != GS_LEVEL )
		return NULL;

	// Some optimization. For some actors that are sent in bunches, to reduce the size,
	// just send some key letter that identifies the actor, instead of the full name.

	for( int i = 0; i < NUMBER_OF_ACTOR_NAME_KEY_LETTERS; i++ ){
		if ( stricmp( pszName, g_szActorKeyLetter[i] ) == 0 ){
			if ( cl_showspawnnames )
				Printf( "Key Letter: %s ", pszName );
			pszName = g_szActorFullName[i];
			break;
		}
	}

	// Potentially print the name, position, and network ID of the thing spawning.
	if ( cl_showspawnnames )
		Printf( "Name: %s: (%d, %d, %d), %d\n", pszName, X >> FRACBITS, Y >> FRACBITS, Z >> FRACBITS, lNetID );

	// If there's already an actor with the network ID of the thing we're spawning, kill it!
	pActor = NETWORK_FindThingByNetID( lNetID );
	if ( pActor )
	{
/*
		if ( pActor == players[consoleplayer].mo )
		{
#ifdef CLIENT_WARNING_MESSAGES
			Printf( "CLIENT_SpawnThing: WARNING! Tried to delete console player's body!\n" );
#endif
			return;
		}
*/
		pActor->Destroy( );
	}

	// Get the class of the thing we're trying to spawn.
	pType = PClass::FindClass( pszName );
	if ( pType == NULL )
	{
		Printf( "CLIENT_SpawnThing: Unknown actor type: %s!\n", pszName );
		return NULL;
	}

	// Handle sprite/particle display options.
	if ( stricmp( pszName, "blood" ) == 0 )
	{
		if ( cl_bloodtype >= 1 )
		{
			angle_t	Angle;

			Angle = ( M_Random( ) - 128 ) << 24;
			P_DrawSplash2( 32, X, Y, Z, Angle, 2, 0 );
		}

		// Just do particles.
		if ( cl_bloodtype == 2 )
			return NULL;
	}

	// Now that all checks have been done, spawn the actor.
	pActor = Spawn( pType, X, Y, Z, NO_REPLACE );
	if ( pActor )
	{
		pActor->lNetID = lNetID;
		if ( lNetID != -1 )
		{
			g_NetIDList[lNetID].bFree = false;
			g_NetIDList[lNetID].pActor = pActor;
		}

		pActor->SpawnPoint[0] = X >> FRACBITS;
		pActor->SpawnPoint[1] = Y >> FRACBITS;

		// Whenever blood spawns, its momz is always 2 * FRACUNIT.
		if ( stricmp( pszName, "blood" ) == 0 )
			pActor->momz = FRACUNIT*2;

		if ( invasion )
			pActor->ulInvasionWave = INVASION_GetCurrentWave( );
	}
	return pActor;
}

//*****************************************************************************
//
void CLIENT_SpawnMissile( char *pszName, fixed_t X, fixed_t Y, fixed_t Z, fixed_t MomX, fixed_t MomY, fixed_t MomZ, LONG lNetID, LONG lTargetNetID )
{
	AActor				*pActor;
	const PClass		*pType;

	// Only spawn missiles if we're actually in a level.
	if ( gamestate != GS_LEVEL )
		return;

	// Some optimization. For some actors that are sent in bunches, to reduce the size,
	// just send some key letter that identifies the actor, instead of the full name.
	if ( stricmp( pszName, "1" ) == 0 )
		pszName = "PlasmaBall";

	// Potentially print the name, position, and network ID of the thing spawning.
	if ( cl_showspawnnames )
		Printf( "Name: %s: (%d, %d, %d), %d\n", pszName, X >> FRACBITS, Y >> FRACBITS, Z >> FRACBITS, lNetID );

	// If there's already an actor with the network ID of the thing we're spawning, kill it!
	pActor = NETWORK_FindThingByNetID( lNetID );
	if ( pActor )
	{
/*
		if ( pActor == players[consoleplayer].mo )
		{
#ifdef CLIENT_WARNING_MESSAGES
			Printf( "CLIENT_SpawnMissile: WARNING! Tried to delete console player's body!\n" );
#endif
			return;
		}
*/
		pActor->Destroy( );
	}

	// Get the class of the thing we're trying to spawn.
	pType = PClass::FindClass( pszName );
	if ( pType == NULL )
	{
		Printf( "CLIENT_SpawnMissile: Unknown actor type: %s!\n", pszName );
		return;
	}

	// Now that all checks have been done, spawn the actor.
	pActor = Spawn( pType, X, Y, Z, NO_REPLACE );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "CLIENT_SpawnMissile: Failed to spawn missile: %d\n", lNetID );
#endif
		return;
	}

	// Set the thing's momentum.
	pActor->momx = MomX;
	pActor->momy = MomY;
	pActor->momz = MomZ;

	// Derive the thing's angle from its momentum.
	pActor->angle = R_PointToAngle2( 0, 0, MomX, MomY );

	pActor->lNetID = lNetID;
	if ( lNetID != -1 )
	{
		g_NetIDList[lNetID].bFree = false;
		g_NetIDList[lNetID].pActor = pActor;
	}

	// Play the seesound if this missile has one.
	if ( pActor->SeeSound )
		S_SoundID( pActor, CHAN_VOICE, pActor->SeeSound, 1, ATTN_NORM );

	pActor->target = NETWORK_FindThingByNetID( lTargetNetID );
/*
	// Move a bit forward.
	pActor->x += pActor->momx >> 1;
	pActor->y += pActor->momy >> 1;
	pActor->z += pActor->momz >> 1;
*/
}

//*****************************************************************************
//
void CLIENT_MoveThing( AActor *pActor, fixed_t X, fixed_t Y, fixed_t Z )
{
   if (( pActor == NULL ) || ( gamestate != GS_LEVEL ))
	   return;
   
   pActor->SetOrigin( X, Y, Z );

   // This is needed to restore tmfloorz.
   P_AdjustFloorCeil( pActor );
}

//*****************************************************************************
//
void CLIENT_RestoreSpecialPosition( AActor *pActor )
{
	// Move item back to its original location
	fixed_t _x, _y;
	sector_t *sec;

	_x = pActor->SpawnPoint[0] << FRACBITS;
	_y = pActor->SpawnPoint[1] << FRACBITS;
	sec = R_PointInSubsector (_x, _y)->sector;

	pActor->SetOrigin( _x, _y, sec->floorplane.ZatPoint( _x, _y ));
	P_CheckPosition( pActor, _x, _y );

	if ( pActor->flags & MF_SPAWNCEILING )
	{
		pActor->z = pActor->ceilingz - pActor->height - ( pActor->SpawnPoint[2] << FRACBITS );
	}
	else if ( pActor->flags2 & MF2_SPAWNFLOAT )
	{
		fixed_t space = pActor->ceilingz - pActor->height - pActor->floorz;
		if (space > 48*FRACUNIT)
		{
			space -= 40*FRACUNIT;
			pActor->z = ((space * g_RestorePositionSeed())>>8) + pActor->floorz + 40*FRACUNIT;
		}
		else
		{
			pActor->z = pActor->floorz;
		}
	}
	else
	{
		pActor->z = (pActor->SpawnPoint[2] << FRACBITS) + pActor->floorz;
		if (pActor->flags2 & MF2_FLOATBOB)
		{
			pActor->z += FloatBobOffsets[(pActor->FloatBobPhase + level.time) & 63];
		}
	}
}

//*****************************************************************************
//
void CLIENT_RestoreSpecialDoomThing( AActor *pActor, bool bFog )
{
	// Change an actor that's been put into a hidden for respawning back to its spawn state
	// (respawning it). The reason we need to duplicate this function (this same code basically
	// exists in A_RestoreSpecialDoomThing( )) is because normally, when items are touched and
	// we want to respawn them, we put them into a hidden state, with the first frame being 1050
	// ticks long. Once that frame is finished, it calls A_ResotoreSpecialDoomThing and Position.
	// We don't want that to happen on the client end, because we don't want items to respawn on
	// the client end. However, there really is no good way to do that. If we don't call the
	// function while in client mode, items won't respawn at all. We could do it with external
	// variables, but that's just not clean. We could just not tick actors, but we definitely 
	// don't want that. Anyway, the solution we're going to use here is to break out of restore
	// special doomthing( ) if we're in client mode to avoid client-side respawning, and just
	// call a different function all together when the server wants us to respawn an item that
	// does basically the same thing.

	// Make the item visible and touchable again.
	pActor->renderflags &= ~RF_INVISIBLE;
	pActor->flags |= MF_SPECIAL;

	if (( pActor->GetDefault( )->flags & MF_NOGRAVITY ) == false )
		pActor->flags &= ~MF_NOGRAVITY;

	// Should this item even respawn?
	if ( static_cast<AInventory *>( pActor )->DoRespawn( ))
	{
		// Put the actor back to its spawn state.
		pActor->SetState( pActor->SpawnState );

		// Play the spawn sound, and make a little fog.
		if ( bFog )
		{
			S_Sound( pActor, CHAN_VOICE, "misc/spawn", 1, ATTN_IDLE );
			Spawn( "ItemFog", pActor->x, pActor->y, pActor->z, ALLOW_REPLACE );
		}
	}
}

//*****************************************************************************
//
void CLIENT_WaitForServer( void )
{
	if ( players[consoleplayer].bSpectating )
	{
		// If the last time we heard from the server exceeds five seconds, we're lagging!
		if ((( gametic - CLIENTDEMO_GetGameticOffset( ) - g_ulLastServerTick ) >= ( TICRATE * 5 )) &&
			(( gametic - CLIENTDEMO_GetGameticOffset( )) > ( TICRATE * 5 )))
		{
			g_bServerLagging = true;
		}
	}
	else
	{
		// If the last time we heard from the server exceeds one second, we're lagging!
		if ((( gametic - CLIENTDEMO_GetGameticOffset( ) - g_ulLastServerTick ) >= TICRATE ) &&
			(( gametic - CLIENTDEMO_GetGameticOffset( )) > TICRATE ))
		{
			g_bServerLagging = true;
		}
	}
}

//*****************************************************************************
//
void CLIENT_RemoveCorpses( void )
{
	AActor	*pActor;
	ULONG	ulCorpseCount;

	// Allow infinite corpses.
	if ( cl_maxcorpses == 0 )
		return;

	// Initialize the number of corpses.
	ulCorpseCount = 0;

	TThinkerIterator<AActor> iterator;
	while (( pActor = iterator.Next( )))
	{
		if (( pActor->IsKindOf( RUNTIME_CLASS( APlayerPawn )) == false ) ||
			( pActor->player ) ||
			( pActor->health > 0 ))
		{
			continue;
		}

		ulCorpseCount++;
		if ( ulCorpseCount >= cl_maxcorpses )
		{
/*
			if ( pActor == players[consoleplayer].mo )
			{
#ifdef CLIENT_WARNING_MESSAGES
				Printf( "CLIENT_RemoveCorpses: WARNING! Tried to delete console player's body!\n" );
#endif
				continue;
			}
*/
			pActor->Destroy( );
		}
	}
}

//*****************************************************************************
//
sector_t *CLIENT_FindSectorByID( ULONG ulID )
{
	if ( ulID >= numsectors )
		return ( NULL );

	return ( &sectors[ulID] );
}

//*****************************************************************************
//
bool CLIENT_IsValidPlayer( ULONG ulPlayer )
{
	// If the player index is out of range, or this player is not in the game, then the
	// player index is not valid.
	if (( ulPlayer >= MAXPLAYERS ) || ( playeringame[ulPlayer] == false ))
		return ( false );

	return ( true );
}

//*****************************************************************************
//
void CLIENT_AllowSendingOfUserInfo( bool bAllow )
{
	g_bAllowSendingOfUserInfo = bAllow;
}

//*****************************************************************************
//
void CLIENT_ResetPlayerData( player_s *pPlayer )
{
	pPlayer->mo = NULL;
	pPlayer->playerstate = 0;
	pPlayer->cls = 0;
	pPlayer->DesiredFOV = 0;
	pPlayer->FOV = 0;
	pPlayer->viewz = 0;
	pPlayer->viewheight = 0;
	pPlayer->deltaviewheight = 0;
	pPlayer->bob = 0;
	pPlayer->momx = 0;
	pPlayer->momy = 0;
	pPlayer->centering = 0;
	pPlayer->turnticks = 0;
	pPlayer->oldbuttons = 0;
	pPlayer->attackdown = 0;
	pPlayer->health = 0;
	pPlayer->inventorytics = 0;
	pPlayer->CurrentPlayerClass = 0;
	pPlayer->pieces = 0;
	pPlayer->backpack = 0;
	pPlayer->fragcount = 0;
	pPlayer->ReadyWeapon = 0;
	pPlayer->PendingWeapon = 0;
	pPlayer->cheats = 0;
	pPlayer->Powers = 0;
	pPlayer->refire = 0;
	pPlayer->killcount = 0;
	pPlayer->itemcount = 0;
	pPlayer->secretcount = 0;
	pPlayer->damagecount = 0;
	pPlayer->bonuscount = 0;
	pPlayer->hazardcount = 0;
	pPlayer->poisoncount = 0;
	pPlayer->poisoner = 0;
	pPlayer->attacker = 0;
	pPlayer->extralight = 0;
	pPlayer->morphTics = 0;
	pPlayer->PremorphWeapon = 0;
	pPlayer->chickenPeck = 0;
	pPlayer->jumpTics = 0;
	pPlayer->respawn_time = 0;
	pPlayer->camera = 0;
	pPlayer->air_finished = 0;
	pPlayer->accuracy = 0;
	pPlayer->stamina = 0;
	pPlayer->BlendR = 0;
	pPlayer->BlendG = 0;
	pPlayer->BlendB = 0;
	pPlayer->BlendA = 0;
// 		pPlayer->LogText(),
	pPlayer->crouching = 0;
	pPlayer->crouchdir = 0;
	pPlayer->crouchfactor = 0;
	pPlayer->crouchoffset = 0;
	pPlayer->crouchviewdelta = 0;
	pPlayer->bOnTeam = 0;
	pPlayer->ulTeam = 0;
	pPlayer->lPointCount = 0;
	pPlayer->ulDeathCount = 0;
	pPlayer->ulLastFragTick = 0;
	pPlayer->ulLastExcellentTick = 0;
	pPlayer->ulLastBFGFragTick = 0;
	pPlayer->ulConsecutiveHits = 0;
	pPlayer->ulConsecutiveRailgunHits = 0;
	pPlayer->ulFragsWithoutDeath = 0;
	pPlayer->ulDeathsWithoutFrag = 0;
	pPlayer->bChatting = 0;
	pPlayer->bSpectating = 0;
	pPlayer->bDeadSpectator = 0;
	pPlayer->bStruckPlayer = 0;
	pPlayer->ulRailgunShots = 0;
	pPlayer->pIcon = 0;
	pPlayer->lMaxHealthBonus = 0;
	pPlayer->lMaxArmorBonus = 0;
	pPlayer->ulWins = 0;
	pPlayer->pSkullBot = 0;
	pPlayer->bIsBot = 0;
	pPlayer->ulPing = 0;
	pPlayer->bReadyToGoOn = 0;
	pPlayer->bSpawnOkay = 0;
	pPlayer->SpawnX = 0;
	pPlayer->SpawnY = 0;
	pPlayer->SpawnAngle = 0;
	pPlayer->OldPendingWeapon = 0;
	pPlayer->bLagging = 0;
	pPlayer->bSpawnTelefragged = 0;
	pPlayer->ulTime = 0;

	memset( &pPlayer->cmd, 0, sizeof( pPlayer->cmd ));
	if (( pPlayer - players ) != consoleplayer )
		memset( &pPlayer->userinfo, 0, sizeof( pPlayer->userinfo ));
	memset( pPlayer->psprites, 0, sizeof( pPlayer->psprites ));

	memset( &pPlayer->ulMedalCount, 0, sizeof( ULONG ) * NUM_MEDALS );
	memset( &pPlayer->ServerXYZ, 0, sizeof( fixed_t ) * 3 );
	memset( &pPlayer->ServerXYZMom, 0, sizeof( fixed_t ) * 3 );
}

//*****************************************************************************
//
LONG CLIENT_AdjustDoorDirection( LONG lDirection )
{
	lDirection -= 1;

	// Not a valid door direction.
	if (( lDirection < -1 ) || ( lDirection > 2 ))
		return ( INT_MAX );

	return ( lDirection );
}

//*****************************************************************************
//
LONG CLIENT_AdjustFloorDirection( LONG lDirection )
{
	lDirection -= 1;

	// Not a valid door direction.
	if (( lDirection < -1 ) || ( lDirection > 1 ))
		return ( INT_MAX );

	return ( lDirection );
}

//*****************************************************************************
//
LONG CLIENT_AdjustCeilingDirection( LONG lDirection )
{
	lDirection -= 1;

	// Not a valid door direction.
	if (( lDirection < -1 ) || ( lDirection > 1 ))
		return ( INT_MAX );

	return ( lDirection );
}

//*****************************************************************************
//
LONG CLIENT_AdjustElevatorDirection( LONG lDirection )
{
	lDirection -= 1;

	// Not a valid door direction.
	if (( lDirection < -1 ) || ( lDirection > 1 ))
		return ( INT_MAX );

	return ( lDirection );
}

//*****************************************************************************
//
void CLIENT_CheckForMissingPackets( void )
{
	ULONG	ulIdx;
	ULONG	ulIdx2;

	// We already told the server we're missing packets a little bit ago. No need
	// to do it again.
	if ( g_lMissingPacketTicks > 0 )
	{
		g_lMissingPacketTicks--;
		return;
	}

	if ( g_lLastParsedSequence != g_lHighestReceivedSequence )
	{
		NETWORK_WriteByte( &g_LocalBuffer.ByteStream, CLC_MISSINGPACKET );

		// Now, go through and figure out what packets we're missing. Request these from the server.
		for ( ulIdx = g_lLastParsedSequence + 1; ulIdx <= g_lHighestReceivedSequence - 1; ulIdx++ )
		{
			for ( ulIdx2 = 0; ulIdx2 < 256; ulIdx2++ )
			{
				// We've found this packet! No need to tell the server we're missing it.
				if ( g_lPacketSequence[ulIdx2] == ulIdx )
					break;
			}

			// If we didn't find the packet, tell the server we're missing it.
			if ( ulIdx2 == 256 )
				NETWORK_WriteLong( &g_LocalBuffer.ByteStream, ulIdx );
		}

		// When we're done, write -1 to indicate that we're finished.
		NETWORK_WriteLong( &g_LocalBuffer.ByteStream, -1 );
	}

	// Don't send out a request for the missing packets for another 1/4 second.
	g_lMissingPacketTicks = ( TICRATE / 4 );
}

//*****************************************************************************
//
bool CLIENT_GetNextPacket( void )
{
	ULONG	ulIdx;

	// Find the next packet in the sequence.
	for ( ulIdx = 0; ulIdx < 256; ulIdx++ )
	{
		// Found it!
		if ( g_lPacketSequence[ulIdx] == ( g_lLastParsedSequence + 1 ))
		{
			memset( NETWORK_GetNetworkMessageBuffer( )->pbData, -1, MAX_UDP_PACKET );
			memcpy( NETWORK_GetNetworkMessageBuffer( )->pbData, g_ReceivedPacketBuffer.abData + g_lPacketBeginning[ulIdx], g_lPacketSize[ulIdx] );
			NETWORK_GetNetworkMessageBuffer( )->ulCurrentSize = g_lPacketSize[ulIdx];
			NETWORK_GetNetworkMessageBuffer( )->ByteStream.pbStream = NETWORK_GetNetworkMessageBuffer( )->pbData;
			NETWORK_GetNetworkMessageBuffer( )->ByteStream.pbStreamEnd = NETWORK_GetNetworkMessageBuffer( )->ByteStream.pbStream + g_lPacketSize[ulIdx];

			return ( true );
		}
	}

	// We didn't find it!
	return ( false );
}

//*****************************************************************************
//*****************************************************************************
//
static void client_Header( BYTESTREAM_s *pByteStream )
{
	LONG	lSequence;

	// Read in the sequence. This is the # of the packet the server has sent us.
	// This function shouldn't ever be called since the packet header is parsed seperately.
	lSequence = NETWORK_ReadLong( pByteStream );
//	Printf( "client_Header: Received packet %d\n", lSequence );
}

//*****************************************************************************
//
static void client_ResetSequence( BYTESTREAM_s *pByteStream )
{
//	Printf( "SEQUENCE RESET! g_lLastParsedSequence = %d\n", g_lLastParsedSequence );
	g_lLastParsedSequence = g_lHighestReceivedSequence - 2;
}

//*****************************************************************************
//
static void client_Ping( BYTESTREAM_s *pByteStream )
{
	ULONG	ulTime;

	// Read in the time on the server end. We send this back to them, and then the server can
	// tell how many milliseconds passed since it sent the ping message to us.
	ulTime = NETWORK_ReadLong( pByteStream );

	// Send back the server's time.
	CLIENTCOMMANDS_Pong( ulTime );
}

//*****************************************************************************
//
static void client_BeginSnapshot( BYTESTREAM_s *pByteStream )
{
	// Display in the console that we're receiving a snapshot.
	Printf( "Receiving snapshot...\n" );

	// Set the client connection state to receiving a snapshot. This disables things like
	// announcer sounds.
	CLIENT_SetConnectionState( CTS_RECEIVINGSNAPSHOT );
}

//*****************************************************************************
//
static void client_EndSnapshot( BYTESTREAM_s *pByteStream )
{
	// We're all done! Set the new client connection state to active.
	CLIENT_SetConnectionState( CTS_ACTIVE );

	// Make the view active.
	viewactive = true;
}

//*****************************************************************************
//
static void client_SpawnPlayer( BYTESTREAM_s *pByteStream )
{
	ULONG			ulPlayer;
	player_t		*pPlayer;
	APlayerPawn		*pActor;
	LONG			lID;
	fixed_t			X;
	fixed_t			Y;
	fixed_t			Z;
	angle_t			Angle;
	bool			bSpectating;
	bool			bDeadSpectator;
	bool			bIsBot;
	LONG			lPlayerState;
	LONG			lState;
	LONG			lPlayerClass;
	LONG			lSkin;
	bool			bWasWatchingPlayer;
	APlayerPawn		*pOldActor;

	// Which player is being spawned?
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the player's state prior to being spawned. This determines whether or not we
	// should wipe his weapons, etc.
	lPlayerState = NETWORK_ReadByte( pByteStream );

	// Is the player a bot?
	bIsBot = !!NETWORK_ReadByte( pByteStream );

	// State of the player?
	lState = NETWORK_ReadByte( pByteStream );

	// Is he spectating?
	bSpectating = !!NETWORK_ReadByte( pByteStream );

	// Is he a dead spectator?
	bDeadSpectator = !!NETWORK_ReadByte( pByteStream );

	// Network ID of the player's body.
	lID = NETWORK_ReadShort( pByteStream );

	// Angle of the player.
	Angle = NETWORK_ReadLong( pByteStream );

	// XYZ position of the player.
	X = NETWORK_ReadLong( pByteStream );
	Y = NETWORK_ReadLong( pByteStream );
	Z = NETWORK_ReadLong( pByteStream );

	lPlayerClass = NETWORK_ReadByte( pByteStream );

	// Invalid player ID or not in a level.
	if (( ulPlayer >= MAXPLAYERS ) || ( gamestate != GS_LEVEL ))
		return;

	// If there's already an actor with this net ID, kill it!
	if ( g_NetIDList[lID].pActor != NULL )
	{
/*
		if ( g_NetIDList[lID].pActor == players[consoleplayer].mo )
		{
#ifdef CLIENT_WARNING_MESSAGES
			Printf( "client_SpawnPlayer: WARNING! Tried to delete console player's body!\n" );
#endif
			return;
		}
*/
		g_NetIDList[lID].pActor->Destroy( );
		g_NetIDList[lID].pActor = NULL;
		g_NetIDList[lID].bFree = true;
	}

	// This player is now in the game!
	playeringame[ulPlayer] = true;
	pPlayer = &players[ulPlayer];

	// Kill the player's old icon if necessary.
	if ( pPlayer->pIcon )
	{
		pPlayer->pIcon->Destroy( );
		pPlayer->pIcon = NULL;
	}

	// First, disassociate the player's corpse.
	bWasWatchingPlayer = false;
	pOldActor = pPlayer->mo;
	if ( pOldActor )
	{
		// If the player's old body is not in a death state, put the body into the last
		// frame of its death state.
		if ( pOldActor->health > 0 )
		{
			FState	*pDeadState;
			FState	*pBaseState;

			A_NoBlocking( pOldActor );

			// Put him in the last frame of his death state.
			pDeadState = pOldActor->DeathState;
			do
			{
				pBaseState = pDeadState;
				if ( pDeadState != NULL )
				{
					pOldActor->SetStateNF( pDeadState );
					pDeadState = pDeadState->GetNextState( );
				}
			} while (( pDeadState != NULL ) && ( pBaseState == pDeadState + 1 ));
		}

		// Check to see if the console player is spectating through this player's eyes.
		// If so, we need to reattach his camera to it when the player respawns.
		if ( pOldActor->CheckLocalView( consoleplayer ))
			bWasWatchingPlayer = true;

		if (( lPlayerState == PST_REBORN ) ||
			( lPlayerState == PST_REBORNNOINVENTORY ) ||
			( lPlayerState == PST_ENTER ) ||
			( lPlayerState == PST_ENTERNOINVENTORY ))
		{
			pOldActor->player = NULL;
			pOldActor->id = -1;
		}
		else
		{
			pOldActor->Destroy( );
			pOldActor->player = NULL;
			pOldActor = NULL;
		}
	}

	// Set up the player class.
	pPlayer->CurrentPlayerClass = lPlayerClass;
	pPlayer->cls = PlayerClasses[pPlayer->CurrentPlayerClass].Type;

	// Spawn the body.
	pActor = static_cast<APlayerPawn *>( Spawn( pPlayer->cls, X, Y, ONFLOORZ, NO_REPLACE ));

	pPlayer->mo = pActor;
	pActor->player = pPlayer;
	pPlayer->playerstate = lState;

	// If we were watching through this player's eyes, reattach the camera.
	if ( bWasWatchingPlayer )
		players[consoleplayer].camera = pPlayer->mo;

	// Set the network ID.
	pPlayer->mo->lNetID = lID;
	if ( lID != -1 )
	{
		g_NetIDList[lID].bFree = false;
		g_NetIDList[lID].pActor = pPlayer->mo;
	}

	if (( lPlayerState == PST_REBORN ) ||
		( lPlayerState == PST_REBORNNOINVENTORY ) ||
		( lPlayerState == PST_ENTER ) ||
		( lPlayerState == PST_ENTERNOINVENTORY ))
	{
		G_PlayerReborn( ulPlayer );
	}

	// Give all cards in death match mode.
	if ( deathmatch )
		pPlayer->mo->GiveDeathmatchInventory( );
	// [BC] Don't filter coop inventory in teamgame mode.
	// Special inventory handling for respawning in coop.
	else if (( teamgame == false ) &&
			 ( lPlayerState == PST_REBORN ) &&
			 ( pOldActor ))
	{
		pPlayer->mo->FilterCoopRespawnInventory( pOldActor );
	}

	// Also, destroy all of the old actor's inventory.
	if ( pOldActor )
		pOldActor->DestroyAllInventory( );

	// Set the spectator variables after G_PlayerReborn so our data doesn't get lost.
	pPlayer->bSpectating = bSpectating;
	pPlayer->bDeadSpectator = bDeadSpectator;

	// If this is the console player, and he's spawned as a regular player, he's definitely not
	// in line anymore!
	if ((( pPlayer - players ) == consoleplayer ) && ( pPlayer->bSpectating == false ))
		JOINQUEUE_SetClientPositionInLine( -1 );

	// Set the player's bot status.
	pPlayer->bIsBot = bIsBot;

	// [RH] set color translations for player sprites
	pActor->Translation = TRANSLATION( TRANSLATION_Players, ulPlayer );
	pActor->angle = Angle;
	pActor->pitch = pActor->roll = 0;
	pActor->health = pPlayer->health;
	pActor->lFixedColormap = 0;

	//Added by MC: Identification (number in the players[MAXPLAYERS] array)
    pActor->id = ulPlayer;

	// [RH] Set player sprite based on skin
	// [BC] Handle cl_skins here.
	if ( cl_skins <= 0 )
	{
		lSkin = R_FindSkin( "base", pPlayer->CurrentPlayerClass );
		pActor->flags4 |= MF4_NOSKIN;
	}
	else if ( cl_skins >= 2 )
	{
		if ( skins[pPlayer->userinfo.skin].bCheat )
		{
			lSkin = R_FindSkin( "base", pPlayer->CurrentPlayerClass );
			pActor->flags4 |= MF4_NOSKIN;
		}
		else
			lSkin = pPlayer->userinfo.skin;
	}
	else
		lSkin = pPlayer->userinfo.skin;

	if (( lSkin < 0 ) || ( lSkin >= numskins ))
		lSkin = R_FindSkin( "base", pPlayer->CurrentPlayerClass );

	pActor->sprite = skins[lSkin].sprite;
	pActor->scaleX = pActor->scaleY = skins[lSkin].Scale;

	pPlayer->DesiredFOV = pPlayer->FOV = 90.f;
	pPlayer->camera = pActor;
	pPlayer->playerstate = PST_LIVE;
	pPlayer->refire = 0;
	pPlayer->damagecount = 0;
	pPlayer->bonuscount = 0;
	pPlayer->morphTics = 0;
	pPlayer->extralight = 0;
	pPlayer->fixedcolormap = 0;
	pPlayer->viewheight = pPlayer->mo->ViewHeight;

	pPlayer->attacker = NULL;
	pPlayer->BlendR = pPlayer->BlendG = pPlayer->BlendB = pPlayer->BlendA = 0.f;
	pPlayer->cheats = 0;
	pPlayer->Uncrouch( );

	// killough 10/98: initialize bobbing to 0.
	pPlayer->momx = 0;
	pPlayer->momy = 0;

	// If the console player is being respawned, place the camera back in his own body.
	if ( ulPlayer == consoleplayer )
		players[consoleplayer].camera = players[consoleplayer].mo;

	// setup gun psprite
	P_SetupPsprites (pPlayer);

	// If this console player is looking through this player's eyes, attach the status
	// bar to this player.
	if (( StatusBar ) && ( players[ulPlayer].mo->CheckLocalView( consoleplayer )))
		StatusBar->AttachToPlayer( pPlayer );

	// If the player is a spectator, set some properties.
	if ( pPlayer->bSpectating )
	{
		pPlayer->mo->flags2 |= (MF2_CANNOTPUSH);
		pPlayer->mo->flags &= ~(MF_SOLID|MF_SHOOTABLE|MF_PICKUP);
		pPlayer->mo->flags2 &= ~(MF2_PASSMOBJ);
		pPlayer->mo->RenderStyle = STYLE_None;

		// Make me flat!
		pPlayer->mo->height = 0;
		pPlayer->cheats |= CF_NOTARGET;

		// Reset a bunch of other stuff.
		pPlayer->extralight = 0;
		pPlayer->fixedcolormap = 0;
		pPlayer->damagecount = 0;
		pPlayer->bonuscount = 0;
		pPlayer->poisoncount = 0;
		pPlayer->inventorytics = 0;

		// Don't lag anymore if we're a spectator.
		if ( ulPlayer == consoleplayer )
			g_bClientLagging = false;
	}
	else
	{
		// Spawn the respawn fog.
		unsigned an = pActor->angle >> ANGLETOFINESHIFT;
		Spawn( "TeleportFog", pActor->x + 20 * finecosine[an],
			pActor->y + 20 * finesine[an],
			pActor->z + TELEFOGHEIGHT, ALLOW_REPLACE );
	}

	pPlayer->playerstate = PST_LIVE;
	
	// If this is the consoleplayer, set the realorigin and ServerXYZMom.
	if ( ulPlayer == consoleplayer )
	{
		pPlayer->ServerXYZ[0] = pPlayer->mo->x;
		pPlayer->ServerXYZ[1] = pPlayer->mo->y;
		pPlayer->ServerXYZ[2] = pPlayer->mo->z;

		pPlayer->ServerXYZMom[0] = 0;
		pPlayer->ServerXYZMom[1] = 0;
		pPlayer->ServerXYZMom[2] = 0;
	}

	// Refresh the HUD because this is potentially a new player.
	SCOREBOARD_RefreshHUD( );
}

//*****************************************************************************
//
static void client_MovePlayer( BYTESTREAM_s *pByteStream )
{
	ULONG		ulPlayer;
	bool		bVisible;
	fixed_t		X;
	fixed_t		Y;
	fixed_t		Z;
	angle_t		Angle;
	fixed_t		MomX;
	fixed_t		MomY;
	fixed_t		MomZ;
	bool		bCrouching;

	// Read in the player number.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Is this player visible? If not, there's no other information to read in.
	bVisible = !!NETWORK_ReadByte( pByteStream );

	// The server only sends position, angle, etc. information if the player is actually
	// visible to us.
	if ( bVisible )
	{
		// Read in the player's XYZ position.
		X = NETWORK_ReadShort( pByteStream ) << FRACBITS;
		Y = NETWORK_ReadShort( pByteStream ) << FRACBITS;
		Z = NETWORK_ReadShort( pByteStream ) << FRACBITS;

		// Read in the player's angle.
		Angle = NETWORK_ReadLong( pByteStream );

		// Read in the player's XYZ momentum.
		MomX = NETWORK_ReadShort( pByteStream ) << FRACBITS;
		MomY = NETWORK_ReadShort( pByteStream ) << FRACBITS;
		MomZ = NETWORK_ReadShort( pByteStream ) << FRACBITS;

		// Read in whether or not the player's crouching.
		bCrouching = !!NETWORK_ReadByte( pByteStream );
	}

	// Check to make sure everything is valid. If not, break out.
	if (( CLIENT_IsValidPlayer( ulPlayer ) == false ) || ( players[ulPlayer].mo == NULL ) || ( gamestate != GS_LEVEL ))
		return;

	// If we're not allowed to know the player's location, then just make him invisible.
	if ( bVisible == false )
	{
		players[ulPlayer].mo->renderflags |= RF_INVISIBLE;

		// Don't move the player since the server didn't send any useful position information.
		return;
	}
	else
		players[ulPlayer].mo->renderflags &= ~RF_INVISIBLE;

	// Set the player's XYZ position.
	players[ulPlayer].mo->x = X;
	players[ulPlayer].mo->y = Y;
	players[ulPlayer].mo->z = Z;

	// Set the player's angle.
	players[ulPlayer].mo->angle = Angle;

	// Set the player's XYZ momentum.
	players[ulPlayer].mo->momx = MomX;
	players[ulPlayer].mo->momy = MomY;
	players[ulPlayer].mo->momz = MomZ;

	// Is the player crouching?
	players[ulPlayer].crouchdir = ( bCrouching ) ? 1 : -1;

	if (( players[ulPlayer].crouchdir == 1 ) &&
		( players[ulPlayer].crouchfactor < FRACUNIT ) &&
		(( players[ulPlayer].mo->z + players[ulPlayer].mo->height ) < players[ulPlayer].mo->ceilingz ))
	{
		P_CrouchMove( &players[ulPlayer], 1 );
	}
	else if (( players[ulPlayer].crouchdir == -1 ) &&
		( players[ulPlayer].crouchfactor > FRACUNIT/2 ))
	{
		P_CrouchMove( &players[ulPlayer], -1 );
	}
}

//*****************************************************************************
//
static void client_DamagePlayer( BYTESTREAM_s *pByteStream )
{
	ULONG		ulPlayer;
	LONG		lHealth;
	LONG		lArmor;
	LONG		lDamage;
	ABasicArmor	*pArmor;

	// Read in the player being damaged.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the new health and armor values.
	lHealth = NETWORK_ReadShort( pByteStream );
	lArmor = NETWORK_ReadShort( pByteStream );

	// Level not loaded, ignore...
	if ( gamestate != GS_LEVEL )
		return;

	// Check to make sure everything is valid. If not, break out.
	if (( CLIENT_IsValidPlayer( ulPlayer ) == false ) || ( players[ulPlayer].mo == NULL ))
		return;

	// Calculate the amount of damage being taken based on the old health value, and the
	// new health value.
	lDamage = players[ulPlayer].health - lHealth;

	// Do the damage.
//	P_DamageMobj( players[ulPlayer].mo, NULL, NULL, lDamage, 0, 0 );

	// Set the new health value.
	players[ulPlayer].mo->health = players[ulPlayer].health = lHealth;

	pArmor = players[ulPlayer].mo->FindInventory<ABasicArmor>( );
	if ( pArmor )
		pArmor->Amount = lArmor;

	// Set the damagecount, for blood on the screen.
	players[ulPlayer].damagecount += lDamage;
	if ( players[ulPlayer].damagecount > 100 )
		players[ulPlayer].damagecount = 100;
	if ( players[ulPlayer].damagecount < 0 )
		players[ulPlayer].damagecount = 0;

	if ( players[ulPlayer].mo->CheckLocalView( consoleplayer ))
	{
		if ( lDamage > 100 )
			lDamage = 100;

		I_Tactile( 40,10,40 + lDamage * 2 );
	}

	// Also, make sure they get put into the pain state.
	players[ulPlayer].mo->SetState( players[ulPlayer].mo->PainState );
}

//*****************************************************************************
//
static void client_KillPlayer( BYTESTREAM_s *pByteStream )
{
	ULONG		ulPlayer;
	LONG		lSourceID;
	LONG		lInflictorID;
	LONG		lHealth;
	LONG		lMOD;
	char		*pszString;
	AActor		*pSource;
	AActor		*pInflictor;
	AWeapon		*pWeapon;
	ULONG		ulIdx;
	ULONG		ulSourcePlayer;

	// Read in the player who's dying.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the actor that killed the player.
	lSourceID = NETWORK_ReadShort( pByteStream );

	// Read in the network ID of the inflictor.
	lInflictorID = NETWORK_ReadShort( pByteStream );

	// Read in how much health they currently have (for gibs).
	lHealth = NETWORK_ReadShort( pByteStream );

	// Read in the means of death.
	lMOD = NETWORK_ReadByte( pByteStream );

	// Read in the player who did the killing's ready weapon so we can properly do obituary
	// messages.
	pszString = NETWORK_ReadString( pByteStream );

	// Check to make sure everything is valid. If not, break out.
	if (( CLIENT_IsValidPlayer( ulPlayer ) == false ) || ( players[ulPlayer].mo == NULL ))
		return;

	// Find the actor associated with the source. It's okay if this actor does not exist.
	if ( lSourceID != -1 )
		pSource = NETWORK_FindThingByNetID( lSourceID );
	else
		pSource = NULL;

	// Find the actor associated with the inflictor. It's okay if this actor does not exist.
	if ( lInflictorID != -1 )
		pInflictor = NETWORK_FindThingByNetID( lInflictorID );
	else
		pInflictor = NULL;

	// Set the player's new health.
	players[ulPlayer].health = players[ulPlayer].mo->health = lHealth;

	// Kill the player.
	players[ulPlayer].mo->Die( NULL, NULL );

	// Free the player's body's network ID.
	if ( players[ulPlayer].mo->lNetID != -1 )
	{
		g_NetIDList[players[ulPlayer].mo->lNetID].bFree = true;
		g_NetIDList[players[ulPlayer].mo->lNetID].pActor = NULL;

		players[ulPlayer].mo->lNetID = -1;
	}

	// If health on the status bar is less than 0%, make it 0%.
	if ( players[ulPlayer].health <= 0 )
		players[ulPlayer].health = 0;

	// Potentially get rid of some corpses. This isn't necessarily client-only.
	CLIENT_RemoveCorpses( );

	ulSourcePlayer = MAXPLAYERS;
	for ( ulIdx = 0; ulIdx < MAXPLAYERS; ulIdx++ )
	{
		if (( playeringame[ulIdx] == false ) ||
			( players[ulIdx].mo == NULL ))
		{
			continue;
		}

		if ( players[ulIdx].mo == pSource )
		{
			ulSourcePlayer = ulIdx;
			break;
		}
	}

	if (( deathmatch || teamgame ) &&
		( cl_showlargefragmessages ) &&
		( ulSourcePlayer < MAXPLAYERS ) &&
		( ulPlayer != ulSourcePlayer ) &&
		( lMOD != MOD_SPAWNTELEFRAG ) &&
		((( duel ) && (( DUEL_GetState( ) == DS_WINSEQUENCE ) || ( DUEL_GetState( ) == DS_COUNTDOWN ))) == false ) &&
		((( lastmanstanding || teamlms ) && (( LASTMANSTANDING_GetState( ) == LMSS_WINSEQUENCE ) || ( LASTMANSTANDING_GetState( ) == LMSS_COUNTDOWN ))) == false ))
	{
		if ((( deathmatch == false ) || (( fraglimit == 0 ) || ( players[ulSourcePlayer].fragcount < fraglimit ))) &&
			(( lastmanstanding == false ) || (( winlimit == 0 ) || ( players[ulSourcePlayer].ulWins < winlimit ))) &&
			(( teamlms == false ) || (( winlimit == 0 ) || ( TEAM_GetWinCount( players[ulSourcePlayer].ulTeam ) < winlimit ))))
		{
			// Display a large "You were fragged by <name>." message in the middle of the screen.
			if ( ulPlayer == consoleplayer )
				SCOREBOARD_DisplayFraggedMessage( &players[ulSourcePlayer] );
			// Display a large "You fragged <name>!" message in the middle of the screen.
			else if ( ulSourcePlayer == consoleplayer )
				SCOREBOARD_DisplayFragMessage( &players[ulPlayer] );
		}
	}

	if (( ulSourcePlayer < MAXPLAYERS ) && ( players[ulSourcePlayer].mo ))
	{
		pWeapon = static_cast<AWeapon *>( players[ulSourcePlayer].mo->FindInventory( PClass::FindClass( pszString )));
		if ( pWeapon == NULL )
			pWeapon = static_cast<AWeapon *>( players[ulSourcePlayer].mo->GiveInventoryType( PClass::FindClass( pszString )));

		if ( pWeapon )
			players[ulSourcePlayer].ReadyWeapon = pWeapon;
	}

	// Finally, print the obituary string.
	ClientObituary( players[ulPlayer].mo, pInflictor, pSource, lMOD );
/*
	if ( ulSourcePlayer < MAXPLAYERS )
		ClientObituary( players[ulPlayer].mo, pInflictor, players[ulSourcePlayer].mo, lMOD );
	else
		ClientObituary( players[ulPlayer].mo, pInflictor, NULL, lMOD );
*/
	// Refresh the HUD, since this could affect the number of players left in an LMS game.
	SCOREBOARD_RefreshHUD( );
}

//*****************************************************************************
//
static void client_SetPlayerHealth( BYTESTREAM_s *pByteStream )
{
	LONG	lHealth;
	ULONG	ulPlayer;

	// Read in the player whose health is being altered.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the health;
	lHealth = NETWORK_ReadShort( pByteStream );

	// If this is an invalid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	players[ulPlayer].health = lHealth;
	if ( players[ulPlayer].mo )
		players[ulPlayer].mo->health = lHealth;
}

//*****************************************************************************
//
static void client_UpdatePlayerArmorDisplay( BYTESTREAM_s *pByteStream )
{
	ULONG		ulPlayer;
	LONG		lArmorAmount;
	char		*pszArmorIconName;
	AInventory	*pArmor;

	// Read in the player whose armor display is updated.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the armor amount and icon;
	lArmorAmount = NETWORK_ReadShort( pByteStream );
	pszArmorIconName = NETWORK_ReadString( pByteStream );

	// If this is an invalid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	pArmor = players[ulPlayer].mo ? players[ulPlayer].mo->FindInventory<ABasicArmor>( ) : NULL;
	if ( pArmor != NULL )
	{
		pArmor->Amount = lArmorAmount;
		pArmor->Icon = TexMan.GetTexture( pszArmorIconName, 0 );
	}
}

//*****************************************************************************
//
static void client_SetPlayerState( BYTESTREAM_s *pByteStream )
{
	ULONG		ulPlayer;
	ULONG		ulState;

	// Read in the player whose state is being updated.
	ulPlayer = NETWORK_ReadByte( pByteStream );
	
	// Read in the state to update him to.
	ulState = NETWORK_ReadByte( pByteStream );

	// If this isn't a valid player, break out.
	if (( CLIENT_IsValidPlayer( ulPlayer ) == false ) || ( players[ulPlayer].mo == NULL ))
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetPlayerState: No player object for player: %d\n", ulPlayer );
#endif
		return;
	}

	// If the player is dead, then we shouldn't have to update his state.
	if ( players[ulPlayer].mo->health <= 0 )
		return;

	// Finally, change the player's state to whatever the server told us it was.
	switch( ulState )
	{
	case STATE_PLAYER_IDLE:

		players[ulPlayer].mo->PlayIdle( );
		break;
	case STATE_PLAYER_SEE:

		players[ulPlayer].mo->SetState( players[ulPlayer].mo->SpawnState );
		players[ulPlayer].mo->PlayRunning( );
		break;
	case STATE_PLAYER_ATTACK:

		players[ulPlayer].mo->PlayAttacking( );
		break;
	case STATE_PLAYER_ATTACK2:

		players[ulPlayer].mo->PlayAttacking2( );
		break;
	default:

#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetPlayerState: Unknown state: %d\n", ulState );
#endif
		break;
	}
}

//*****************************************************************************
//
static void client_SetPlayerUserInfo( BYTESTREAM_s *pByteStream )
{
	ULONG		ulIdx;
    player_t	*pPlayer;
	ULONG		ulPlayer;
	ULONG		ulFlags;
	char		szName[MAXPLAYERNAME+1];
	LONG		lGender;
	LONG		lColor;
	char		*pszSkin;
	LONG		lRailgunTrailColor;
	LONG		lHandicap;
	LONG		lClass;
	LONG		lSkin;

	// Read in the player whose userinfo is being sent to us.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in what userinfo entries are going to be updated.
	ulFlags = NETWORK_ReadShort( pByteStream );

	// Read in the player's name.
	if ( ulFlags & USERINFO_NAME )
		sprintf( szName, NETWORK_ReadString( pByteStream ));

	// Read in the player's gender.
	if ( ulFlags & USERINFO_GENDER )
		lGender = NETWORK_ReadByte( pByteStream );

	// Read in the player's color.
	if ( ulFlags & USERINFO_COLOR )
		lColor = NETWORK_ReadLong( pByteStream );

	// Read in the player's railgun trail color.
	if ( ulFlags & USERINFO_RAILCOLOR )
		lRailgunTrailColor = NETWORK_ReadByte( pByteStream );

	// Read in the player's skin.
	if ( ulFlags & USERINFO_SKIN )
		pszSkin = NETWORK_ReadString( pByteStream );

	// Read in the player's handicap.
	if ( ulFlags & USERINFO_HANDICAP )
		lHandicap = NETWORK_ReadByte( pByteStream );

	// Read in the player's class.
	if ( ulFlags & USERINFO_PLAYERCLASS )
		lClass = NETWORK_ReadByte( pByteStream );

	// If this isn't a valid player, break out.
	// We actually send the player's userinfo before he gets spawned, thus putting him in
	// the game. Therefore, this call won't work unless the way the server sends the data
	// changes.
//	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
//		return;
	if ( ulPlayer >= MAXPLAYERS )
		return;

	// Now that everything's been read in, actually set the player's userinfo properties.
	// Player's name.
    pPlayer = &players[ulPlayer];
	if ( ulFlags & USERINFO_NAME )
	{
		if ( strlen( szName ) > MAXPLAYERNAME )
			szName[MAXPLAYERNAME] = '\0';
		strcpy( pPlayer->userinfo.netname, szName );

		// Remove % signs from names.
		for ( ulIdx = 0; ulIdx < strlen( pPlayer->userinfo.netname ); ulIdx++ )
		{
			if ( pPlayer->userinfo.netname[ulIdx] == '%' )
				pPlayer->userinfo.netname[ulIdx] = ' ';
		}
	}

    // Other info.
	if ( ulFlags & USERINFO_GENDER )
		pPlayer->userinfo.gender = lGender;
	if ( ulFlags & USERINFO_COLOR )
	    pPlayer->userinfo.color = lColor;
	if ( ulFlags & USERINFO_RAILCOLOR )
		pPlayer->userinfo.lRailgunTrailColor = lRailgunTrailColor;

	// Make sure the skin is valid.
	if ( ulFlags & USERINFO_SKIN )
	{
		pPlayer->userinfo.skin = R_FindSkin( pszSkin, pPlayer->CurrentPlayerClass );

		// [BC] Handle cl_skins here.
		if ( cl_skins <= 0 )
		{
			lSkin = R_FindSkin( "base", pPlayer->CurrentPlayerClass );
			if ( pPlayer->mo )
				pPlayer->mo->flags4 |= MF4_NOSKIN;
		}
		else if ( cl_skins >= 2 )
		{
			if ( skins[pPlayer->userinfo.skin].bCheat )
			{
				lSkin = R_FindSkin( "base", pPlayer->CurrentPlayerClass );
				if ( pPlayer->mo )
					pPlayer->mo->flags4 |= MF4_NOSKIN;
			}
			else
				lSkin = pPlayer->userinfo.skin;
		}
		else
			lSkin = pPlayer->userinfo.skin;

		if (( lSkin < 0 ) || ( lSkin >= numskins ))
			lSkin = R_FindSkin( "base", pPlayer->CurrentPlayerClass );

		if ( pPlayer->mo )
		{
			pPlayer->mo->sprite = skins[lSkin].sprite;
			pPlayer->mo->scaleX = pPlayer->mo->scaleY = skins[lSkin].Scale;
		}
	}

	// Read in the player's handicap.
	if ( ulFlags & USERINFO_HANDICAP )
	{
		pPlayer->userinfo.lHandicap = lHandicap;
		if ( pPlayer->userinfo.lHandicap < 0 )
			pPlayer->userinfo.lHandicap = 0;
		else if ( pPlayer->userinfo.lHandicap > deh.MaxSoulsphere )
			pPlayer->userinfo.lHandicap = deh.MaxSoulsphere;
	}

	// Read in the player's class.
	if (( (gameinfo.gametype == GAME_Hexen) || (PlayerClasses.Size() > 1) ) && ( ulFlags & USERINFO_PLAYERCLASS ))
		pPlayer->userinfo.PlayerClass = lClass;

	// Build translation tables, always gotta do this!
	R_BuildPlayerTranslation( ulPlayer );
}

//*****************************************************************************
//
static void client_SetPlayerFrags( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	LONG	lFragCount;

	// Read in the player whose frags are being updated.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the number of points he's supposed to get.
	lFragCount = NETWORK_ReadShort( pByteStream );

	// If this isn't a valid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	if (( g_ConnectionState == CTS_ACTIVE ) &&
		( deathmatch ) &&
		(( duel == false ) || ( DUEL_GetState( ) == DS_INDUEL )) &&
		((( lastmanstanding == false ) && ( teamlms == false )) || ( LASTMANSTANDING_GetState( ) == LMSS_INPROGRESS )) &&
		( teamplay == false ) &&
		( lastmanstanding == false ) &&
		( teamlms == false ) &&
		( possession == false ) &&
		( teampossession == false ))
	{
		ANNOUNCER_PlayFragSounds( ulPlayer, players[ulPlayer].fragcount, lFragCount );
	}

	// Finally, set the player's frag count, and refresh the HUD.
	players[ulPlayer].fragcount = lFragCount;
	SCOREBOARD_RefreshHUD( );
}

//*****************************************************************************
//
static void client_SetPlayerPoints( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	LONG	lPointCount;

	// Read in the player whose frags are being updated.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the number of points he's supposed to get.
	lPointCount = NETWORK_ReadShort( pByteStream );

	// If this isn't a valid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;
/*
	if (( g_ConnectionState == CTS_ACTIVE ) &&
		( possession ))
	{
		ANNOUNCER_PlayScoreSounds( ulPlayer, players[ulPlayer].lPointCount, lPointCount );
	}
*/
	// Finally, set the player's point count, and refresh the HUD.
	players[ulPlayer].lPointCount = lPointCount;
	SCOREBOARD_RefreshHUD( );
}

//*****************************************************************************
//
static void client_SetPlayerWins( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	LONG	lWins;

	// Read in the player whose wins are being updated.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the number of wins he's supposed to have.
	lWins = NETWORK_ReadByte( pByteStream );

	// If this is an invalid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	// Finally, set the player's win count, and refresh the HUD.
	players[ulPlayer].ulWins = lWins;
	SCOREBOARD_RefreshHUD( );
}

//*****************************************************************************
//
static void client_SetPlayerKillCount( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	LONG	lKillCount;

	// Read in the player whose kill count being updated.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the number of kills he's supposed to have.
	lKillCount = NETWORK_ReadShort( pByteStream );

	// If this is an invalid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	// Finally, set the player's kill count, and refresh the HUD.
	players[ulPlayer].killcount = lKillCount;
	SCOREBOARD_RefreshHUD( );
}

//*****************************************************************************
//
void client_SetPlayerChatStatus( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	bool	bChatting;

	// Read in the player whose chat status is being altered.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Is the player chatting, or not?
	bChatting = !!NETWORK_ReadByte( pByteStream );

	// If this is an invalid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	// Finally, set the player's chat status.
	players[ulPlayer].bChatting = bChatting;
}

//*****************************************************************************
//
void client_SetPlayerLaggingStatus( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	bool	bLagging;

	// Read in the player whose chat status is being altered.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Is the player lagging, or not?
	bLagging = !!NETWORK_ReadByte( pByteStream );

	// If this is an invalid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	// Finally, set the player's lagging status.
	players[ulPlayer].bLagging = bLagging;
}

//*****************************************************************************
//
static void client_SetPlayerReadyToGoOnStatus( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	bool	bReadyToGoOn;

	// Read in the player whose "ready to go on" status is being altered.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in whether or not he's ready.
	bReadyToGoOn = !!NETWORK_ReadByte( pByteStream );

	// If this is an invalid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	// Finally, set the player's "ready to go on" status.
	players[ulPlayer].bReadyToGoOn = bReadyToGoOn;
}

//*****************************************************************************
//
static void client_SetPlayerTeam( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	ULONG	ulTeam;

	// Read in the player having his team set.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the player's team.
	ulTeam = NETWORK_ReadByte( pByteStream );

	// If this is an invalid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	// Update the player's team.
	PLAYER_SetTeam( &players[ulPlayer], ulTeam, false );
}

//*****************************************************************************
//
static void client_SetPlayerCamera( BYTESTREAM_s *pByteStream )
{
	AActor	*pCamera;
	LONG	lID;
	bool	bRevertPleaseStatus;

	// Read in the network ID of the camera.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the "revert please" status.
	bRevertPleaseStatus = !!NETWORK_ReadByte( pByteStream );

	// Find the camera by the network ID.
	pCamera = NETWORK_FindThingByNetID( lID );
	if ( pCamera == NULL )
	{
		players[consoleplayer].camera = players[consoleplayer].mo;
		players[consoleplayer].cheats &= ~CF_REVERTPLEASE;
		return;
	}

	// Finally, change the player's camera.
	players[consoleplayer].camera = pCamera;
	if ( bRevertPleaseStatus == false )
		players[consoleplayer].cheats &= ~CF_REVERTPLEASE;
	else
		players[consoleplayer].cheats |= CF_REVERTPLEASE;
}

//*****************************************************************************
//
static void client_UpdatePlayerPing( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	ULONG	ulPing;

	// Read in the player whose ping is being updated.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the player's ping.
	ulPing = NETWORK_ReadShort( pByteStream );

	// If this is an invalid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	// Finally, set the player's ping.
	players[ulPlayer].ulPing = ulPing;
}

//*****************************************************************************
//
static void client_UpdatePlayerExtraData( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
//	ULONG	ulPendingWeapon;
//	ULONG	ulReadyWeapon;
	LONG	lPitch;
	ULONG	ulWaterLevel;
	ULONG	ulButtons;
	LONG	lViewZ;
	LONG	lBob;

	// Read in the player who's info is about to be updated.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the player's pitch.
	lPitch = NETWORK_ReadLong( pByteStream );

	// Read in the player's water level.
	ulWaterLevel = NETWORK_ReadByte( pByteStream );

	// Read in the player's buttons.
	ulButtons = NETWORK_ReadByte( pByteStream );

	// Read in the view and weapon bob.
	lViewZ = NETWORK_ReadLong( pByteStream );
	lBob = NETWORK_ReadLong( pByteStream );

	// If the player doesn't exist, get out!
	if (( players[ulPlayer].mo == NULL ) || ( playeringame[ulPlayer] == false ))
		return;

	// [BB] If the spectated player uses the GL renderer and we are using software,
	// the viewangle has to be limited.	We don't care about cl_disallowfullpitch here.
	if ( !currentrenderer )
	{
		if (lPitch < -ANGLE_1*32)
			lPitch = -ANGLE_1*32;
		if (lPitch > ANGLE_1*56)
			lPitch = ANGLE_1*56;
	}
	players[ulPlayer].mo->pitch = lPitch;
	players[ulPlayer].mo->waterlevel = ulWaterLevel;
	players[ulPlayer].cmd.ucmd.buttons = ulButtons;
//	players[ulPlayer].momx = lMomX;
//	players[ulPlayer].momy = lMomY;
	players[ulPlayer].viewz = lViewZ;
	players[ulPlayer].bob = lBob;
}

//*****************************************************************************
//
static void client_UpdatePlayerPendingWeapon( BYTESTREAM_s *pByteStream )
{
	ULONG			ulPlayer;
	const char		*pszPendingWeapon;
	const PClass	*pType = NULL;
	AWeapon			*pWeapon = NULL;


	// Read in the player who's info is about to be updated.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the name of the weapon.
	pszPendingWeapon = NETWORK_ReadString( pByteStream );

	// Some optimization. For standard Doom weapons, to reduce the size of the string
	// that's sent out, just send some key character that identifies the weapon, instead
	// of the full name.
	convertWeaponKeyLetterToFullString( pszPendingWeapon );

	// If the player doesn't exist, get out!
	if (( players[ulPlayer].mo == NULL ) || ( playeringame[ulPlayer] == false ))
		return;

	if ( strcmp (pszPendingWeapon, "NULL") && pszPendingWeapon[0] != '\0' )
	{
		pType = PClass::FindClass( pszPendingWeapon );
		if ( ( pType != NULL ) && ( pType->IsDescendantOf( RUNTIME_CLASS( AWeapon ))) )
		{
			// If we dont have this weapon already, we do now!
			AWeapon *pWeapon = static_cast<AWeapon *>( players[ulPlayer].mo->FindInventory( pType ));
			if ( pWeapon == NULL )
				pWeapon = static_cast<AWeapon *>( players[ulPlayer].mo->GiveInventoryType( pType ));

			// If he still doesn't have the object after trying to give it to him... then YIKES!
			if ( pWeapon == NULL )
			{
#ifdef CLIENT_WARNING_MESSAGES
				Printf( "client_WeaponChange: Failed to give inventory type, %s!\n", pszName );
#endif
				return;
			}
			players[ulPlayer].PendingWeapon = pWeapon;
		}
	}
}

//*****************************************************************************
//
static void client_DoInventoryUse( BYTESTREAM_s *pByteStream )
{
	// Read the name of the inventory item we shall use.
	const char *pszString = NETWORK_ReadString( pByteStream );

	// Check to make sure everything is valid. If not, break out.
	if ( players[consoleplayer].mo == NULL )
		return;

	AInventory *item = players[consoleplayer].mo->FindInventory (PClass::FindClass (pszString));

	if (item != NULL)
	{
		players[consoleplayer].mo->UseInventory (item);
	}
}

//*****************************************************************************
//
static void client_UpdatePlayerTime( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	ULONG	ulTime;

	// Read in the player.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the time.
	ulTime = NETWORK_ReadShort( pByteStream );

	// If this is an invalid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	players[ulPlayer].ulTime = ulTime * ( TICRATE * 60 );
}

//*****************************************************************************
//
static void client_MoveLocalPlayer( BYTESTREAM_s *pByteStream )
{
	player_s	*pPlayer;
	ULONG		ulClientTicOnServerEnd;
	fixed_t		X;
	fixed_t		Y;
	fixed_t		Z;
	fixed_t		MomX;
	fixed_t		MomY;
	fixed_t		MomZ;
	ULONG		ulWaterLevel;

	pPlayer = &players[consoleplayer];
	
	// Read in the last tick that we sent to the server.
	ulClientTicOnServerEnd = NETWORK_ReadLong( pByteStream );

	// Get XYZ.
	X = NETWORK_ReadLong( pByteStream );
	Y = NETWORK_ReadLong( pByteStream );
	Z = NETWORK_ReadLong( pByteStream );

	// Get XYZ momentum.
	MomX = NETWORK_ReadLong( pByteStream );
	MomY = NETWORK_ReadLong( pByteStream );
	MomZ = NETWORK_ReadLong( pByteStream );

	// Get waterlevel.
	ulWaterLevel = NETWORK_ReadByte( pByteStream );

	// No player object to update.
	if ( pPlayer->mo == NULL )
		return;

	// "ulClientTicOnServerEnd" is the gametic of the last time we sent a movement command.
	CLIENT_SetLastConsolePlayerUpdateTick( ulClientTicOnServerEnd );

	// If the last time the server heard from us exceeds one second, the client is lagging!
	if (( gametic - CLIENTDEMO_GetGameticOffset( ) - ulClientTicOnServerEnd >= TICRATE ) && (( gametic + CLIENTDEMO_GetGameticOffset( )) > TICRATE ))
		g_bClientLagging = true;
	else
		g_bClientLagging = false;

	// If the player is dead, simply ignore this (remember, this could be parsed from an
	// out-of-order packet, since it's sent unreliably)!
	if ( pPlayer->playerstate == PST_DEAD )
		return;

	// Now that everything's check out, update stuff.
	pPlayer->ServerXYZ[0] = X;
	pPlayer->ServerXYZ[1] = Y;
	pPlayer->ServerXYZ[2] = Z;

	pPlayer->ServerXYZMom[0] = MomX;
	pPlayer->ServerXYZMom[1] = MomY;
	pPlayer->ServerXYZMom[2] = MomZ;

	if ( pPlayer->bSpectating == false )
		pPlayer->mo->waterlevel = ulWaterLevel;
}

//*****************************************************************************
//
void client_DisconnectPlayer( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;

	// Read in the player who's being disconnected (could be us!).
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// If this is an invalid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	// If we were a spectator and looking through this player's eyes, revert them.
	if ( players[ulPlayer].mo->CheckLocalView( consoleplayer ))
	{
		players[consoleplayer].camera = players[consoleplayer].mo;
		S_UpdateSounds( players[consoleplayer].camera );
		StatusBar->AttachToPlayer( &players[consoleplayer] );
	}

	// Create a little disconnect particle effect thingamabobber!
	P_DisconnectEffect( players[ulPlayer].mo );

	// Destroy the actor associated with the player.
	if ( players[ulPlayer].mo )
	{
		players[ulPlayer].mo->Destroy( );
		players[ulPlayer].mo = NULL;
	}

	playeringame[ulPlayer] = false;

	// Zero out all the player information.
	CLIENT_ResetPlayerData( &players[ulPlayer] );

	// Refresh the HUD because this affects the number of players in the game.
	SCOREBOARD_RefreshHUD( );
}

//*****************************************************************************
//
static void client_SetConsolePlayer( BYTESTREAM_s *pByteStream )
{
	LONG	lConsolePlayer;

	// Read in what our local player index is.
	lConsolePlayer = NETWORK_ReadByte( pByteStream );

	// If this index is invalid, break out.
	if ( lConsolePlayer >= MAXPLAYERS )
		return;

	// In a client demo, don't lose the userinfo we gave to our console player.
	if ( CLIENTDEMO_IsPlaying( ))
		memcpy( &players[lConsolePlayer].userinfo, &players[consoleplayer].userinfo, sizeof( userinfo_t ));

	// Otherwise, since it's valid, set our local player index to this.
	consoleplayer = lConsolePlayer;

	// Finally, apply our local userinfo to this player slot.
	D_SetupUserInfo( );
}

//*****************************************************************************
//
static void client_ConsolePlayerKicked( BYTESTREAM_s *pByteStream )
{
	// Set the connection state to "disconnected" before calling CLIENT_QuitNetworkGame( ),
	// so that we don't send a disconnect signal to the server.
	CLIENT_SetConnectionState( CTS_DISCONNECTED );

	// End the network game.
	CLIENT_QuitNetworkGame( );

	// Let the player know he's been kicked.
	Printf( "You have been kicked from the server.\n" );
}

//*****************************************************************************
//
static void client_GivePlayerMedal( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	ULONG	ulMedal;

	// Read in the player to award the medal to.
	ulPlayer = NETWORK_ReadByte( pByteStream );
	
	// Read in the medal to be awarded.
	ulMedal = NETWORK_ReadByte( pByteStream );

	// If this is an invalid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	// Award the medal.
	MEDAL_GiveMedal( ulPlayer, ulMedal );
}

//*****************************************************************************
//
static void client_ResetAllPlayersFragcount( BYTESTREAM_s *pByteStream )
{
	// This function pretty much takes care of everything we need to do!
	PLAYER_ResetAllPlayersFragcount( );
}

//*****************************************************************************
//
static void client_PlayerIsSpectator( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	bool	bDeadSpectator;

	// Read in the player who's going to be spectating.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in if he's becoming a dead spectator.
	bDeadSpectator = !!NETWORK_ReadByte( pByteStream );

	// If this is an invalid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	// Make the player a spectator.
	PLAYER_SetSpectator( &players[ulPlayer], false, bDeadSpectator );

	// If we were a spectator and looking through this player's eyes, revert them.
	if ( players[ulPlayer].mo->CheckLocalView( consoleplayer ))
	{
		players[consoleplayer].camera = players[consoleplayer].mo;
//		ChangeSpy( true );
	}

	// Don't lag anymore if we're a spectator.
	if ( ulPlayer == consoleplayer )
		g_bClientLagging = false;
}

//*****************************************************************************
//
static void client_PlayerSay( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	ULONG	ulMode;
	char	*pszString;

	// Read in the player who's supposed to be talking.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the chat mode. Could be global, team-only, etc.
	ulMode = NETWORK_ReadByte( pByteStream );

	// Read in the actual chat string.
	pszString = NETWORK_ReadString( pByteStream );

	// If ulPlayer == MAXPLAYERS, that means the server is talking.
	if ( ulPlayer != MAXPLAYERS )
	{
		// If this is an invalid player, break out.
		if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
			return;
	}

	// Finally, print out the chat string.
	CHAT_PrintChatString( ulPlayer, ulMode, pszString );
}

//*****************************************************************************
//
static void client_PlayerTaunt( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;

	// Read in the player index who's taunting.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Don't taunt if we're not in a level!
	if ( gamestate != GS_LEVEL )
		return;

	// If this is an invalid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	if (( players[ulPlayer].bSpectating ) ||
		( players[ulPlayer].health <= 0 ) ||
		( players[ulPlayer].mo == NULL ) ||
		( i_compatflags & COMPATF_DISABLETAUNTS ))
	{
		return;
	}

	// Play the taunt sound!
	if ( cl_taunts )
		S_Sound( players[ulPlayer].mo, CHAN_VOICE, "*taunt", 1, ATTN_NORM );
}

//*****************************************************************************
//
static void client_PlayerRespawnInvulnerability( BYTESTREAM_s *pByteStream )
{
	ULONG		ulPlayer;
	AActor		*pActor;
	AInventory	*pInventory;

	// Read in the player index who has respawn invulnerability.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Don't taunt if we're not in a level!
	if ( gamestate != GS_LEVEL )
		return;

	// If this is an invalid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	// We can't apply the effect to the player's body if the player's body doesn't exist!
	pActor = players[ulPlayer].mo;
	if ( pActor == NULL )
		return;

	// First, we need to adjust the blend color, so the player's screen doesn't go white.
	pInventory = pActor->FindInventory( RUNTIME_CLASS( APowerInvulnerable ));
	if ( pInventory == NULL )
		return;

	static_cast<APowerup *>( pInventory )->BlendColor = 0;

	// Apply respawn invulnerability effect.
	switch ( cl_respawninvuleffect )
	{
	case 1:

		pActor->RenderStyle = STYLE_Translucent;
		pActor->effects |= FX_VISIBILITYFLICKER;
		break;
	case 2:

		pActor->effects |= FX_RESPAWNINVUL;
		break;
	}
}

//*****************************************************************************
//
static void client_SetPlayerAmmoCapacity( BYTESTREAM_s *pByteStream )
{
	ULONG			ulPlayer;
	char			*pszName;
	LONG			lMaxAmount;
	AInventory		*pAmmo;

	// Read in the player ID.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the name of the type of item to give.
	pszName = NETWORK_ReadString( pByteStream );

	// Read in the amount of this inventory type the player has.
	lMaxAmount = NETWORK_ReadShort( pByteStream );

	// Check to make sure everything is valid. If not, break out.
	if (( CLIENT_IsValidPlayer( ulPlayer ) == false ) || ( players[ulPlayer].mo == NULL ))
		return;

	pAmmo = FindPlayerInventory( ulPlayer, pszName );

	if ( pAmmo == NULL )
		return;

	if ( !(pAmmo->GetClass()->IsDescendantOf (RUNTIME_CLASS(AAmmo))) )
		return;

	// Set the new maximum amount of the inventory object.
	pAmmo->MaxAmount = lMaxAmount;

	// Since an item displayed on the HUD may have been given, refresh the HUD.
	SCOREBOARD_RefreshHUD( );
}
//*****************************************************************************
//
static void client_SpawnThing( BYTESTREAM_s *pByteStream )
{
	fixed_t			X;
	fixed_t			Y;
	fixed_t			Z;
	char			*pszName;
	LONG			lID;

	// Read in the XYZ location of the item.
	X = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	Y = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	Z = NETWORK_ReadShort( pByteStream ) << FRACBITS;

	// Read in the name of the item.
	pszName = NETWORK_ReadString( pByteStream );

	// Read in the network ID of the item.
	lID = NETWORK_ReadShort( pByteStream );

	// Finally, spawn the thing.
	CLIENT_SpawnThing( pszName, X, Y, Z, lID );
}

//*****************************************************************************
//
static void client_SpawnThingNoNetID( BYTESTREAM_s *pByteStream, bool bReceiveTranslation )
{
	fixed_t			X;
	fixed_t			Y;
	fixed_t			Z;
	char			*pszName;

	// Read in the XYZ location of the item.
	X = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	Y = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	Z = NETWORK_ReadShort( pByteStream ) << FRACBITS;

	// Read in the name of the item.
	pszName = NETWORK_ReadString( pByteStream );

	// Finally, spawn the thing.
	AActor *pActor = CLIENT_SpawnThing( pszName, X, Y, Z, -1 );

	// [BB] If we are supposed to set the translation, read in the translation
	// and set it, if we sucessfully spawned the actor.
	if( bReceiveTranslation )
	{
		LONG lTranslation = NETWORK_ReadLong( pByteStream );
		if( pActor )
			pActor->Translation = lTranslation;
	}
}

//*****************************************************************************
//
static void client_SpawnThingExact( BYTESTREAM_s *pByteStream )
{
	fixed_t			X;
	fixed_t			Y;
	fixed_t			Z;
	char			*pszName;
	LONG			lID;

	// Read in the XYZ location of the item.
	X = NETWORK_ReadLong( pByteStream );
	Y = NETWORK_ReadLong( pByteStream );
	Z = NETWORK_ReadLong( pByteStream );

	// Read in the name of the item.
	pszName = NETWORK_ReadString( pByteStream );

	// Read in the network ID of the item.
	lID = NETWORK_ReadShort( pByteStream );

	// Finally, spawn the thing.
	CLIENT_SpawnThing( pszName, X, Y, Z, lID );
}

//*****************************************************************************
//
static void client_SpawnThingExactNoNetID( BYTESTREAM_s *pByteStream )
{
	fixed_t			X;
	fixed_t			Y;
	fixed_t			Z;
	char			*pszName;

	// Read in the XYZ location of the item.
	X = NETWORK_ReadLong( pByteStream );
	Y = NETWORK_ReadLong( pByteStream );
	Z = NETWORK_ReadLong( pByteStream );

	// Read in the name of the item.
	pszName = NETWORK_ReadString( pByteStream );

	// Finally, spawn the thing.
	CLIENT_SpawnThing( pszName, X, Y, Z, -1 );
}

//*****************************************************************************
//
static void client_MoveThing( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	LONG	lBits;
	AActor	*pActor;
	fixed_t	X;
	fixed_t	Y;
	fixed_t	Z;

	// Read in the network ID of the thing to update.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the data that will be updated.
	lBits = NETWORK_ReadShort( pByteStream );

	// Try to find the corresponding actor.
	pActor = NETWORK_FindThingByNetID( lID );

	if (( pActor == NULL ) || gamestate != GS_LEVEL )
	{
		// No thing up update; skip the rest of the message.
		if ( lBits & CM_X ) NETWORK_ReadShort( pByteStream );
		if ( lBits & CM_Y ) NETWORK_ReadShort( pByteStream );
		if ( lBits & CM_Z ) NETWORK_ReadShort( pByteStream );
		if ( lBits & CM_ANGLE ) NETWORK_ReadLong( pByteStream );
		if ( lBits & CM_MOMX ) NETWORK_ReadShort( pByteStream );
		if ( lBits & CM_MOMY ) NETWORK_ReadShort( pByteStream );
		if ( lBits & CM_MOMZ ) NETWORK_ReadShort( pByteStream );

		return;
	}

	X = pActor->x;
	Y = pActor->y;
	Z = pActor->z;

	// Read in the position data.
	if ( lBits & CM_X )
		X = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	if ( lBits & CM_Y )
		Y = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	if ( lBits & CM_Z )
		Z = NETWORK_ReadShort( pByteStream ) << FRACBITS;

	// Update the thing's position.
	CLIENT_MoveThing( pActor, X, Y, Z );

	// Read in the angle data.
	if ( lBits & CM_ANGLE )
		pActor->angle = NETWORK_ReadLong( pByteStream );

	// Read in the momentum data.
	if ( lBits & CM_MOMX )
		pActor->momx = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	if ( lBits & CM_MOMY )
		pActor->momy = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	if ( lBits & CM_MOMZ )
		pActor->momz = NETWORK_ReadShort( pByteStream ) << FRACBITS;
}

//*****************************************************************************
//
static void client_MoveThingExact( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	LONG	lBits;
	AActor	*pActor;
	fixed_t	X;
	fixed_t	Y;
	fixed_t	Z;

	// Read in the network ID of the thing to update.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the data that will be updated.
	lBits = NETWORK_ReadShort( pByteStream );

	// Try to find the corresponding actor.
	pActor = NETWORK_FindThingByNetID( lID );

	if (( pActor == NULL ) || gamestate != GS_LEVEL )
	{
		// No thing up update; skip the rest of the message.
		if ( lBits & CM_X ) NETWORK_ReadLong( pByteStream );
		if ( lBits & CM_Y ) NETWORK_ReadLong( pByteStream );
		if ( lBits & CM_Z ) NETWORK_ReadLong( pByteStream );
		if ( lBits & CM_ANGLE ) NETWORK_ReadLong( pByteStream );
		if ( lBits & CM_MOMX ) NETWORK_ReadLong( pByteStream );
		if ( lBits & CM_MOMY ) NETWORK_ReadLong( pByteStream );
		if ( lBits & CM_MOMZ ) NETWORK_ReadLong( pByteStream );

		return;
	}

	X = pActor->x;
	Y = pActor->y;
	Z = pActor->z;

	// Read in the position data.
	if ( lBits & CM_X )
		X = NETWORK_ReadLong( pByteStream );
	if ( lBits & CM_Y )
		Y = NETWORK_ReadLong( pByteStream );
	if ( lBits & CM_Z )
		Z = NETWORK_ReadLong( pByteStream );

	// Update the thing's position.
	CLIENT_MoveThing( pActor, X, Y, Z );

	// Read in the angle data.
	if ( lBits & CM_ANGLE )
		pActor->angle = NETWORK_ReadLong( pByteStream );

	// Read in the momentum data.
	if ( lBits & CM_MOMX )
		pActor->momx = NETWORK_ReadLong( pByteStream );
	if ( lBits & CM_MOMY )
		pActor->momy = NETWORK_ReadLong( pByteStream );
	if ( lBits & CM_MOMZ )
		pActor->momz = NETWORK_ReadLong( pByteStream );
}

//*****************************************************************************
//
static void client_DamageThing( BYTESTREAM_s *pByteStream )
{
	LONG		lID;
	AActor		*pActor;

	// Read in ID of the thing being damaged.
	lID = NETWORK_ReadShort( pByteStream );

	// Not in a level; nothing to do!
	if ( gamestate != GS_LEVEL )
		return;

	pActor = NETWORK_FindThingByNetID( lID );

	// Nothing to damage.
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DamageThing: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// Damage the thing.
	P_DamageMobj( pActor, NULL, NULL, 0, MOD_UNKNOWN );
}

//*****************************************************************************
//
static void client_KillThing( BYTESTREAM_s *pByteStream )
{
	AActor	*pActor;
	LONG	lID;
	LONG	lHealth;

	// Read in the network ID of the thing that died.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the thing's health.
	lHealth = NETWORK_ReadShort( pByteStream );

	// Level not loaded; ingore.
	if ( gamestate != GS_LEVEL )
		return;

	// Find the actor that matches the given network ID.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_KillThing: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// Set the thing's health. This should enable the proper death state to play.
	pActor->health = lHealth;

	// Kill the thing.
	pActor->Die( NULL, NULL );
}

//*****************************************************************************
//
static void client_SetThingState( BYTESTREAM_s *pByteStream )
{
	AActor		*pActor;
	LONG		lID;
	LONG		lState;
	FState		*pNewState;

	// Read in the network ID for the object to have its state changed.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the state.
	lState = NETWORK_ReadByte( pByteStream );

	// Find the actor associated with the ID.
	pActor = NETWORK_FindThingByNetID( lID );

	// Not in a level; nothing to do (shouldn't happen!)
	if ( gamestate != GS_LEVEL )
		return;
	
	// Couldn't find the actor.
	if ( pActor == NULL )
	{
		// There should probably be the potential for a warning message here.
		return;
	}
/*
	switch ( lState )
	{
	case STATE_SPAWN:

		Printf( "STATE_SPAWN\n" );
		break;
	case STATE_SEE:

		Printf( "STATE_SEE\n" );
		break;
	case STATE_PAIN:

		Printf( "STATE_PAIN\n" );
		break;
	case STATE_MELEE:

		Printf( "STATE_MELEE\n" );
		break;
	case STATE_MISSILE:

		Printf( "STATE_MISSILE\n" );
		break;
	case STATE_DEATH:

		Printf( "STATE_DEATH\n" );
		break;
	case STATE_XDEATH:

		Printf( "STATE_XDEATH\n" );
		break;
	case STATE_RAISE:

		Printf( "STATE_RAISE\n" );
		break;
	case STATE_HEAL:

		Printf( "STATE_HEAL\n" );
		break;
	}
*/
	switch ( lState )
	{
	case STATE_SEE:

		pNewState = pActor->SeeState;
		break;
	case STATE_SPAWN:

		pNewState = pActor->SpawnState;
		break;
	case STATE_PAIN:

		pNewState = pActor->PainState;
		break;
	case STATE_MELEE:

		pNewState = pActor->MeleeState;
		break;
	case STATE_MISSILE:

		pNewState = pActor->MissileState;
		break;
	case STATE_DEATH:

		pNewState = pActor->DeathState;
		break;
	case STATE_XDEATH:

		pNewState = pActor->XDeathState;
		break;
	case STATE_RAISE:

		// When an actor raises, we need to do a whole bunch of other stuff.
		DoThingRaise( pActor );
		return;
	case STATE_HEAL:

		if ( pActor->HealState )
		{
			pNewState = pActor->HealState;
			S_Sound( pActor, CHAN_BODY, "vile/raise", 1, ATTN_IDLE );

			// Since the arch-vile is reviving a monster at this point, increment the total number
			// of monsters remaining on the level.
			level.total_monsters++;
		}
		else if ( pActor->IsKindOf( RUNTIME_CLASS( AArchvile )))
		{
			pNewState = &AArchvile::States[25];	// S_VILE_HEAL
			S_Sound( pActor, CHAN_BODY, "vile/raise", 1, ATTN_IDLE );

			// Since the arch-vile is reviving a monster at this point, increment the total number
			// of monsters remaining on the level.
			level.total_monsters++;
		}
		else
			return;

		break;
	default:

#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetThingState: Unknown state: %d\n", lState );
#endif
		return; 
	}

	// Set the angle.
//	pActor->angle = Angle;
	pActor->SetState( pNewState );
//	pActor->SetStateNF( pNewState );
}

//*****************************************************************************
//
static void client_DestroyThing( BYTESTREAM_s *pByteStream )
{
	AActor	*pActor;
	LONG	lID;

	// Read the actor's network ID.
	lID = NETWORK_ReadShort( pByteStream );

	// Find the actor based on the net ID.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DestroyThing: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// Destroy the thing.
	pActor->Destroy( );
}

//*****************************************************************************
//
static void client_SetThingAngle( BYTESTREAM_s *pByteStream )
{
	AActor		*pActor;
	LONG		lID;
	fixed_t		Angle;

	// Read in the thing's network ID.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the thing's new angle.
	Angle = NETWORK_ReadLong( pByteStream );

	// Now try to find the thing.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetThingAngle: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// Finally, set the angle.
	pActor->angle = Angle;
}

//*****************************************************************************
//
static void client_SetThingTID( BYTESTREAM_s *pByteStream )
{
	AActor		*pActor;
	LONG		lID;
	LONG		lTid;

	// Read in the thing's network ID.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the thing's new angle.
	lTid = NETWORK_ReadShort( pByteStream );

	// Now try to find the thing.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetThingTID: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// Finally, set the tid.
	pActor->tid = lTid;
	pActor->AddToHash();
}

//*****************************************************************************
//
static void client_SetThingWaterLevel( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	AActor	*pActor;
	LONG	lWaterLevel;

	// Get the ID of the actor whose water level is being updated.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the water level.
	lWaterLevel = NETWORK_ReadByte( pByteStream );

	// Now try to find the corresponding actor.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetThingWaterLevel: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	pActor->waterlevel = lWaterLevel;
}

//*****************************************************************************
//
static void client_SetThingFlags( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	AActor	*pActor;
	ULONG	ulFlagSet;
	ULONG	ulFlags;

	// Get the ID of the actor whose flags are being updated.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the which flags are being updated.
	ulFlagSet = NETWORK_ReadByte( pByteStream );

	// Read in the flags.
	ulFlags = NETWORK_ReadLong( pByteStream );

	// Now try to find the corresponding actor.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetThingFlags: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	switch ( ulFlagSet )
	{
	case FLAGSET_FLAGS:

		pActor->flags = ulFlags;
		break;
	case FLAGSET_FLAGS2:

		pActor->flags2 = ulFlags;
		break;
	case FLAGSET_FLAGS3:

		pActor->flags3 = ulFlags;
		break;
	case FLAGSET_FLAGS4:

		pActor->flags4 = ulFlags;
		break;
	case FLAGSET_FLAGS5:

		pActor->flags5 = ulFlags;
		break;
	case FLAGSET_FLAGSST:

		pActor->ulSTFlags = ulFlags;
		break;
	}
}

//*****************************************************************************
//
static void client_SetThingArguments( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	ULONG	ulArgs0;
	ULONG	ulArgs1;
	ULONG	ulArgs2;
	ULONG	ulArgs3;
	ULONG	ulArgs4;
	AActor	*pActor;

	// Get the ID of the actor whose arguments are being updated.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the actor's arguments.
	ulArgs0 = NETWORK_ReadByte( pByteStream );
	ulArgs1 = NETWORK_ReadByte( pByteStream );
	ulArgs2 = NETWORK_ReadByte( pByteStream );
	ulArgs3 = NETWORK_ReadByte( pByteStream );
	ulArgs4 = NETWORK_ReadByte( pByteStream );

	// Now try to find the corresponding actor.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetThingArguments: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// Finally, set the thing's arguments.
	pActor->args[0] = ulArgs0;
	pActor->args[1] = ulArgs1;
	pActor->args[2] = ulArgs2;
	pActor->args[3] = ulArgs3;
	pActor->args[4] = ulArgs4;

	// Gross hack for invisible bridges, since they set their height/radius
	// based on their args the instant they're spawned.
	if ( pActor->GetClass( ) == PClass::FindClass( "InvisibleBridge" ))
		pActor->BeginPlay( );
}

//*****************************************************************************
//
static void client_SetThingTranslation( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	LONG	lTranslation;
	AActor	*pActor;

	// Get the ID of the actor whose translation is being updated.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the actor's translation.
	lTranslation = NETWORK_ReadLong( pByteStream );

	// Now try to find the corresponding actor.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetThingTranslation: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// Finally, set the thing's translation.
	pActor->Translation = lTranslation;
}

//*****************************************************************************
//

// These are directly pasted from p_acs.cpp. This list needs to be updated whenever
// that changes.
#define APROP_Health		0
#define APROP_Speed			1
#define APROP_Damage		2
#define APROP_Alpha			3
#define APROP_RenderStyle	4
#define APROP_Ambush		10
#define APROP_Invulnerable	11
#define APROP_JumpZ			12	// [GRB]
#define APROP_ChaseGoal		13
#define APROP_Frightened	14
#define APROP_SeeSound		5	// Sounds can only be set, not gotten
#define APROP_AttackSound	6
#define APROP_PainSound		7
#define APROP_DeathSound	8
#define APROP_ActiveSound	9

static void client_SetThingProperty( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	ULONG	ulProperty;
	ULONG	ulPropertyValue;
	AActor	*pActor;

	// Get the ID of the actor whose translation is being updated.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in which property is being updated.
	ulProperty = NETWORK_ReadByte( pByteStream );

	// Read in the actor's property.
	ulPropertyValue = NETWORK_ReadLong( pByteStream );

	// Now try to find the corresponding actor.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetThingProperty: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// Set one of the actor's properties, depending on what was read in.
	switch ( ulProperty )
	{
	case APROP_Speed:

		pActor->Speed = ulPropertyValue;
		break;
	case APROP_Alpha:

		pActor->alpha = ulPropertyValue;
		break;
	case APROP_RenderStyle:

		pActor->RenderStyle = ulPropertyValue;
		break;
	case APROP_JumpZ:

		if ( pActor->IsKindOf( RUNTIME_CLASS( APlayerPawn )))
			static_cast<APlayerPawn *>( pActor )->JumpZ = ulPropertyValue;
		break;
	default:

		Printf( "client_SetThingProperty: Unknown property, %d!\n", ulProperty );
		return;
	}
}

//*****************************************************************************
//
static void client_SetThingSound( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	ULONG	ulSound;
	char	*pszSound;
	AActor	*pActor;

	// Get the ID of the actor whose translation is being updated.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in which sound is being updated.
	ulSound = NETWORK_ReadByte( pByteStream );

	// Read in the actor's new sound.
	pszSound = NETWORK_ReadString( pByteStream );

	// Now try to find the corresponding actor.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetThingSound: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// Set one of the actor's sounds, depending on what was read in.
	switch ( ulSound )
	{
	case ACTORSOUND_SEESOUND:

		pActor->SeeSound = S_FindSound( pszSound );
		break;
	case ACTORSOUND_ATTACKSOUND:

		pActor->AttackSound = S_FindSound( pszSound );
		break;
	case ACTORSOUND_PAINSOUND:

		pActor->PainSound = S_FindSound( pszSound );
		break;
	case ACTORSOUND_DEATHSOUND:

		pActor->DeathSound = S_FindSound( pszSound );
		break;
	case ACTORSOUND_ACTIVESOUND:

		pActor->ActiveSound = S_FindSound( pszSound );
		break;
	default:

		Printf( "client_SetThingSound: Unknown sound, %d!\n", ulSound );
		return;
	}
}

//*****************************************************************************
//
static void client_SetThingSpecial2( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	LONG	lSpecial2;
	AActor	*pActor;

	// Get the ID of the actor whose special2 is being updated.
	lID = NETWORK_ReadShort( pByteStream );

	// Get the actor's special2.
	lSpecial2 = NETWORK_ReadShort( pByteStream );

	// Now try to find the corresponding actor.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetThingSpecial2: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// Set one of the actor's special2.
	pActor->special2 = lSpecial2;
}

//*****************************************************************************
//
static void client_SetWeaponAmmoGive( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	AActor	*pActor;
	ULONG	ulAmmoGive1;
	ULONG	ulAmmoGive2;

	// Get the ID of the actor whose water level is being updated.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the amount of ammo type 1 this weapon gives.
	ulAmmoGive1 = NETWORK_ReadShort( pByteStream );

	// Read in the amount of ammo type 2 this weapon gives.
	ulAmmoGive2 = NETWORK_ReadShort( pByteStream );

	// Now try to find the corresponding actor.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetThingWaterLevel: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// If this actor isn't a weapon, break out.
	if ( pActor->IsKindOf( RUNTIME_CLASS( AWeapon )) == false )
		return;

	// Finally, actually set the amount of ammo this weapon gives us.
	static_cast<AWeapon *>( pActor )->AmmoGive1 = ulAmmoGive1;
	static_cast<AWeapon *>( pActor )->AmmoGive2 = ulAmmoGive2;
}

//*****************************************************************************
//
static void client_ThingIsCorpse( BYTESTREAM_s *pByteStream )
{
	AActor	*pActor;
	LONG	lID;
	FState	*pDeadState;
	FState	*pBaseState;
	bool	bIsMonster;

	// Read in the network ID of the thing to make dead.
	lID = NETWORK_ReadShort( pByteStream );

	// Is this thing a monster?
	bIsMonster = !!NETWORK_ReadByte( pByteStream );

	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_ThingIsCorpse: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	A_NoBlocking( pActor );	// [RH] Use this instead of A_PainDie

	// Do some other stuff done in AActor::Die.
	pActor->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SKULLFLY|MF_NOGRAVITY);
	pActor->flags |= MF_CORPSE|MF_DROPOFF;
	pActor->height >>= 2;

	// Set the thing to the last frame of its death state.
	pDeadState = pActor->DeathState;
	do
	{
		pBaseState = pDeadState;
		if ( pDeadState != NULL )
		{
			pActor->SetStateNF( pDeadState );
			pDeadState = pDeadState->GetNextState( );
		}
	} while (( pDeadState != NULL ) && ( pBaseState == pDeadState + 1 ));

	if ( bIsMonster )
		level.killed_monsters++;

	// If this is a player, put the player in his dead state.
	if ( pActor->player )
		pActor->player->playerstate = PST_DEAD;
}

//*****************************************************************************
//
static void client_HideThing( BYTESTREAM_s *pByteStream )
{
	AActor	*pActor;
	LONG	lID;

	// Read in the network ID of the thing to hide.
	lID = NETWORK_ReadShort( pByteStream );

	// Didn't find it.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_HideThing: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// Put the item in a hidden state.
	static_cast<AInventory *>( pActor )->HideIndefinitely( );
}

//*****************************************************************************
//
static void client_TeleportThing( BYTESTREAM_s *pByteStream )
{
	LONG		lID;
	fixed_t		NewX;
	fixed_t		NewY;
	fixed_t		NewZ;
	fixed_t		NewMomX;
	fixed_t		NewMomY;
	fixed_t		NewMomZ;
	LONG		lNewReactionTime;
	angle_t		NewAngle;
	bool		bSourceFog;
	bool		bDestFog;
	bool		bTeleZoom;
	angle_t		Angle;
	AActor		*pActor;

	// Read in the network ID of the thing being teleported.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the thing's new position.
	NewX = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	NewY = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	NewZ = NETWORK_ReadShort( pByteStream ) << FRACBITS;

	// Read in the thing's new momentum.
	NewMomX = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	NewMomY = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	NewMomZ = NETWORK_ReadShort( pByteStream ) << FRACBITS;

	// Read in the thing's new reaction time.
	lNewReactionTime = NETWORK_ReadShort( pByteStream );

	// Read in the thing's new angle.
	NewAngle = NETWORK_ReadLong( pByteStream );

	// Should we spawn a teleport fog at the spot the thing is teleporting from?
	// What about the spot the thing is teleporting to?
	bSourceFog = !!NETWORK_ReadByte( pByteStream );
	bDestFog = !!NETWORK_ReadByte( pByteStream );

	// Should be do the teleport zoom?
	bTeleZoom = !!NETWORK_ReadByte( pByteStream );

	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
		return;

	// Move the player to his new position.
//	P_TeleportMove( pActor, NewX, NewY, NewZ, false );

	// Spawn teleport fog at the source.
	if ( bSourceFog )
		Spawn<ATeleportFog>( pActor->x, pActor->y, pActor->z + (( pActor->flags & MF_MISSILE ) ? 0 : TELEFOGHEIGHT ), ALLOW_REPLACE );

	// Set the thing's new position.
	CLIENT_MoveThing( pActor, NewX, NewY, NewZ );

	// Spawn a teleport fog at the destination.
	if ( bDestFog )
	{
		// Spawn the fog slightly in front of the thing's destination.
		Angle = NewAngle >> ANGLETOFINESHIFT;

		Spawn<ATeleportFog>( pActor->x + ( 20 * finecosine[Angle] ),
			pActor->y + ( 20 * finesine[Angle] ),
			pActor->z + (( pActor->flags & MF_MISSILE ) ? 0 : TELEFOGHEIGHT ),
			ALLOW_REPLACE );
	}

	// Set the thing's new momentum.
	pActor->momx = NewMomX;
	pActor->momy = NewMomY;
	pActor->momz = NewMomZ;

	// Also, if this is a player, set his bobbing appropriately.
	if ( pActor->player )
	{
		pActor->player->momx = NewMomX;
		pActor->player->momy = NewMomY;
	}

	// Reset the thing's new reaction time.
	pActor->reactiontime = lNewReactionTime;

	// Set the thing's new angle.
	pActor->angle = NewAngle;

	// User variable to do a weird zoom thingy when you teleport.
	if (( bTeleZoom ) && ( telezoom ) && ( pActor->player ))
		pActor->player->FOV = MIN( 175.f, pActor->player->DesiredFOV + 45.f );
}

//*****************************************************************************
//
static void client_ThingActivate( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	AActor	*pActor;
	AActor	*pActivator;

	// Get the ID of the actor to activate.
	lID = NETWORK_ReadShort( pByteStream );

	// Now try to find the corresponding actor.
	pActor = NETWORK_FindThingByNetID( lID );

	// Get the ID of the activator.
	lID = NETWORK_ReadShort( pByteStream );
	if ( lID == -1 )
		pActivator = NULL;
	else
		pActivator = NETWORK_FindThingByNetID( lID );

	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_ThingActivate: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// Finally, activate the actor.
	pActor->Activate( pActivator );
}

//*****************************************************************************
//
static void client_ThingDeactivate( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	AActor	*pActor;
	AActor	*pActivator;

	// Get the ID of the actor to deactivate.
	lID = NETWORK_ReadShort( pByteStream );

	// Now try to find the corresponding actor.
	pActor = NETWORK_FindThingByNetID( lID );

	// Get the ID of the activator.
	lID = NETWORK_ReadShort( pByteStream );
	if ( lID == -1 )
		pActivator = NULL;
	else
		pActivator = NETWORK_FindThingByNetID( lID );

	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_ThingDeactivate: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// Finally, deactivate the actor.
	pActor->Deactivate( pActivator );
}

//*****************************************************************************
//
static void client_RespawnDoomThing( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	bool	bFog;
	AActor	*pActor;

	// Read in the thing's network ID.
	lID = NETWORK_ReadShort( pByteStream );

	// Should a fog be spawned when the item respawns?
	bFog = !!NETWORK_ReadByte( pByteStream );

	// Nothing to do if the level isn't loaded!
	if ( gamestate != GS_LEVEL )
		return;

	pActor = NETWORK_FindThingByNetID( lID );

	// Couldn't find a matching actor. Ignore...
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_RespawnDoomThing: Couldn't find thing: %d\n", sID );
#endif
		return; 
	}

	// Finally, respawn the item.
	CLIENT_RestoreSpecialPosition( pActor );
	CLIENT_RestoreSpecialDoomThing( pActor, bFog );
}

//*****************************************************************************
//
static void client_RespawnRavenThing( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	AActor	*pActor;

	// Read in the thing's network ID.
	lID = NETWORK_ReadShort( pByteStream );

	// Nothing to do if the level isn't loaded!
	if ( gamestate != GS_LEVEL )
		return;

	pActor = NETWORK_FindThingByNetID( lID );

	// Couldn't find a matching actor. Ignore...
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_RespawnSpecialThing1: Couldn't find thing: %d\n", sID );
#endif
		return; 
	}

	pActor->renderflags &= ~RF_INVISIBLE;
	S_Sound( pActor, CHAN_VOICE, "misc/spawn", 1, ATTN_IDLE );

	pActor->SetState( &AInventory::States[6] );
}

//*****************************************************************************
//
static void client_Print( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPrintLevel;
	char	*pszString;

	// Read in the print level.
	ulPrintLevel = NETWORK_ReadByte( pByteStream );

	// Read in the string to be printed.
	pszString = NETWORK_ReadString( pByteStream );

	// Print out the message.
	Printf( ulPrintLevel, pszString );
    
	// If the server is saying something to us, play the chat sound.
	if (( strnicmp( "<server>", pszString, 8 ) == 0 ) || ( strnicmp( "* <server>", pszString, 10 ) == 0 ))
		S_Sound( CHAN_VOICE, gameinfo.chatSound, 1, ATTN_NONE );
	// If a player is saying something to us, play the chat sound.
	else if ( ulPrintLevel == PRINT_CHAT || ulPrintLevel == PRINT_TEAMCHAT )
		S_Sound( CHAN_VOICE, gameinfo.chatSound, 1, ATTN_NONE );
}

//*****************************************************************************
//
static void client_PrintMid( BYTESTREAM_s *pByteStream )
{
	char	*pszString;

	// Read in the string that's supposed to be printed.
	pszString = NETWORK_ReadString( pByteStream );

	// Print the message.
	C_MidPrint( pszString );
}

//*****************************************************************************
//

// :(. This is needed so that the MOTD can be printed in the color the user wishes to print
// mid-screen messages in.
extern	int PrintColors[7];
static void client_PrintMOTD( BYTESTREAM_s *pByteStream )
{
	char				*pszMOTD;
	DHUDMessageFadeOut	*pMsg;

	// Read in the Message of the Day string.
	pszMOTD = NETWORK_ReadString( pByteStream );

	if ( pszMOTD )
	{
		char szBuffer[1024];

		// Add pretty colors/formatting!
		V_ColorizeString( pszMOTD );

		sprintf( szBuffer, TEXTCOLOR_RED
			"\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36"
			"\36\36\36\36\36\36\36\36\36\36\36\36\37" TEXTCOLOR_TAN
			"\n\n%s\n" TEXTCOLOR_RED
			"\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36"
			"\36\36\36\36\36\36\36\36\36\36\36\36\37" TEXTCOLOR_NORMAL "\n\n" ,
			pszMOTD );

		// Add this message to the console window.
		AddToConsole( -1, szBuffer );

		// We cannot create the message if there's no status bar to attach it to.
		if ( StatusBar == NULL )
			return;

		pMsg = new DHUDMessageFadeOut( pszMOTD,
			1.5f,
			0.375f,
			0,
			0,
			(EColorRange)PrintColors[5],
			cl_motdtime,
			0.35f );

		StatusBar->AttachMessage( pMsg, 'MOTD' );
	}
}

//*****************************************************************************
//
static void client_PrintHUDMessage( BYTESTREAM_s *pByteStream )
{
	char		szString[256];
	float		fX;
	float		fY;
	LONG		lHUDWidth;
	LONG		lHUDHeight;
	LONG		lColor;
	float		fHoldTime;
	char		*pszFont;
	LONG		lID;
	DHUDMessage	*pMsg;
	FFont		*pOldFont;

	// Read in the string.
	sprintf( szString, NETWORK_ReadString( pByteStream ));

	// Read in the XY.
	fX = NETWORK_ReadFloat( pByteStream );
	fY = NETWORK_ReadFloat( pByteStream );

	// Read in the HUD size.
	lHUDWidth = NETWORK_ReadShort( pByteStream );
	lHUDHeight = NETWORK_ReadShort( pByteStream );

	// Read in the color.
	lColor = NETWORK_ReadByte( pByteStream );

	// Read in the hold time.
	fHoldTime = NETWORK_ReadFloat( pByteStream );

	// Read in the font being used.
	pszFont = NETWORK_ReadString( pByteStream );

	// Read in the ID.
	lID = NETWORK_ReadLong( pByteStream );

	// We cannot create the message if there's no status bar to attach it to.
	if ( StatusBar == NULL )
		return;

	pOldFont = screen->Font;
	if ( V_GetFont( pszFont ))
		screen->SetFont( V_GetFont( pszFont ));

	// Create the message.
	pMsg = new DHUDMessage( szString,
		fX,
		fY,
		lHUDWidth,
		lHUDHeight,
		(EColorRange)lColor,
		fHoldTime );

	// Now attach the message.
	StatusBar->AttachMessage( pMsg, lID ? 0xff000000|lID : 0 );

	// Finally, revert to the old font we were using.
	screen->SetFont( pOldFont );
}

//*****************************************************************************
//
static void client_PrintHUDMessageFadeOut( BYTESTREAM_s *pByteStream )
{
	char				szString[256];
	float				fX;
	float				fY;
	LONG				lHUDWidth;
	LONG				lHUDHeight;
	LONG				lColor;
	float				fHoldTime;
	float				fFadeOutTime;
	char				*pszFont;
	LONG				lID;
	DHUDMessageFadeOut	*pMsg;
	FFont				*pOldFont;

	// Read in the string.
	sprintf( szString, NETWORK_ReadString( pByteStream ));

	// Read in the XY.
	fX = NETWORK_ReadFloat( pByteStream );
	fY = NETWORK_ReadFloat( pByteStream );

	// Read in the HUD size.
	lHUDWidth = NETWORK_ReadShort( pByteStream );
	lHUDHeight = NETWORK_ReadShort( pByteStream );

	// Read in the color.
	lColor = NETWORK_ReadByte( pByteStream );

	// Read in the hold time.
	fHoldTime = NETWORK_ReadFloat( pByteStream );

	// Read in the fade time.
	fFadeOutTime = NETWORK_ReadFloat( pByteStream );

	// Read in the font being used.
	pszFont = NETWORK_ReadString( pByteStream );

	// Read in the ID.
	lID = NETWORK_ReadLong( pByteStream );

	// We cannot create the message if there's no status bar to attach it to.
	if ( StatusBar == NULL )
		return;

	pOldFont = screen->Font;
	if ( V_GetFont( pszFont ))
		screen->SetFont( V_GetFont( pszFont ));

	// Create the message.
	pMsg = new DHUDMessageFadeOut( szString,
		fX,
		fY,
		lHUDWidth,
		lHUDHeight,
		(EColorRange)lColor,
		fHoldTime,
		fFadeOutTime );

	// Now attach the message.
	StatusBar->AttachMessage( pMsg, lID ? 0xff000000|lID : 0 );

	// Finally, revert to the old font we were using.
	screen->SetFont( pOldFont );
}

//*****************************************************************************
//
static void client_PrintHUDMessageFadeInOut( BYTESTREAM_s *pByteStream )
{
	char					szString[256];
	float					fX;
	float					fY;
	LONG					lHUDWidth;
	LONG					lHUDHeight;
	LONG					lColor;
	float					fHoldTime;
	float					fFadeInTime;
	float					fFadeOutTime;
	char					*pszFont;
	LONG					lID;
	DHUDMessageFadeInOut	*pMsg;
	FFont					*pOldFont;

	// Read in the string.
	sprintf( szString, NETWORK_ReadString( pByteStream ));

	// Read in the XY.
	fX = NETWORK_ReadFloat( pByteStream );
	fY = NETWORK_ReadFloat( pByteStream );

	// Read in the HUD size.
	lHUDWidth = NETWORK_ReadShort( pByteStream );
	lHUDHeight = NETWORK_ReadShort( pByteStream );

	// Read in the color.
	lColor = NETWORK_ReadByte( pByteStream );

	// Read in the hold time.
	fHoldTime = NETWORK_ReadFloat( pByteStream );

	// Read in the fade in time.
	fFadeInTime = NETWORK_ReadFloat( pByteStream );

	// Read in the fade out time.
	fFadeOutTime = NETWORK_ReadFloat( pByteStream );

	// Read in the font being used.
	pszFont = NETWORK_ReadString( pByteStream );

	// Read in the ID.
	lID = NETWORK_ReadLong( pByteStream );

	// We cannot create the message if there's no status bar to attach it to.
	if ( StatusBar == NULL )
		return;

	pOldFont = screen->Font;
	if ( V_GetFont( pszFont ))
		screen->SetFont( V_GetFont( pszFont ));

	// Create the message.
	pMsg = new DHUDMessageFadeInOut( szString,
		fX,
		fY,
		lHUDWidth,
		lHUDHeight,
		(EColorRange)lColor,
		fHoldTime,
		fFadeInTime,
		fFadeOutTime );

	// Now attach the message.
	StatusBar->AttachMessage( pMsg, lID ? 0xff000000|lID : 0 );

	// Finally, revert to the old font we were using.
	screen->SetFont( pOldFont );
}

//*****************************************************************************
//
static void client_PrintHUDMessageTypeOnFadeOut( BYTESTREAM_s *pByteStream )
{
	char						szString[256];
	float						fX;
	float						fY;
	LONG						lHUDWidth;
	LONG						lHUDHeight;
	LONG						lColor;
	float						fTypeOnTime;
	float						fHoldTime;
	float						fFadeOutTime;
	char						*pszFont;
	LONG						lID;
	DHUDMessageTypeOnFadeOut	*pMsg;
	FFont						*pOldFont;

	// Read in the string.
	sprintf( szString, NETWORK_ReadString( pByteStream ));

	// Read in the XY.
	fX = NETWORK_ReadFloat( pByteStream );
	fY = NETWORK_ReadFloat( pByteStream );

	// Read in the HUD size.
	lHUDWidth = NETWORK_ReadShort( pByteStream );
	lHUDHeight = NETWORK_ReadShort( pByteStream );

	// Read in the color.
	lColor = NETWORK_ReadByte( pByteStream );

	// Read in the type on time.
	fTypeOnTime = NETWORK_ReadFloat( pByteStream );

	// Read in the hold time.
	fHoldTime = NETWORK_ReadFloat( pByteStream );

	// Read in the fade out time.
	fFadeOutTime = NETWORK_ReadFloat( pByteStream );

	// Read in the font being used.
	pszFont = NETWORK_ReadString( pByteStream );

	// Read in the ID.
	lID = NETWORK_ReadLong( pByteStream );

	// We cannot create the message if there's no status bar to attach it to.
	if ( StatusBar == NULL )
		return;

	pOldFont = screen->Font;
	if ( V_GetFont( pszFont ))
		screen->SetFont( V_GetFont( pszFont ));

	// Create the message.
	pMsg = new DHUDMessageTypeOnFadeOut( szString,
		fX,
		fY,
		lHUDWidth,
		lHUDHeight,
		(EColorRange)lColor,
		fTypeOnTime,
		fHoldTime,
		fFadeOutTime );

	// Now attach the message.
	StatusBar->AttachMessage( pMsg, lID ? 0xff000000|lID : 0 );

	// Finally, revert to the old font we were using.
	screen->SetFont( pOldFont );
}

//*****************************************************************************
//
static void client_SetGameMode( BYTESTREAM_s *pByteStream )
{
	UCVarValue	Value;

	Value.Bool = true;
	switch ( NETWORK_ReadByte( pByteStream ))
	{
	case GAMEMODE_COOPERATIVE:

		cooperative.ForceSet( Value, CVAR_Bool );
		break;
	case GAMEMODE_SURVIVAL:

		survival.ForceSet( Value, CVAR_Bool );
		break;
	case GAMEMODE_INVASION:

		invasion.ForceSet( Value, CVAR_Bool );
		break;
	case GAMEMODE_DEATHMATCH:

		deathmatch.ForceSet( Value, CVAR_Bool );
		break;
	case GAMEMODE_TEAMPLAY:

		teamplay.ForceSet( Value, CVAR_Bool );
		break;
	case GAMEMODE_DUEL:

		duel.ForceSet( Value, CVAR_Bool );
		break;
	case GAMEMODE_TERMINATOR:

		terminator.ForceSet( Value, CVAR_Bool );
		break;
	case GAMEMODE_LASTMANSTANDING:

		lastmanstanding.ForceSet( Value, CVAR_Bool );
		break;
	case GAMEMODE_TEAMLMS:

		teamlms.ForceSet( Value, CVAR_Bool );
		break;
	case GAMEMODE_POSSESSION:

		possession.ForceSet( Value, CVAR_Bool );
		break;
	case GAMEMODE_TEAMPOSSESSION:

		teampossession.ForceSet( Value, CVAR_Bool );
		break;
	case GAMEMODE_TEAMGAME:

		teamgame.ForceSet( Value, CVAR_Bool );
		break;
	case GAMEMODE_CTF:

		ctf.ForceSet( Value, CVAR_Bool );
		break;
	case GAMEMODE_ONEFLAGCTF:

		oneflagctf.ForceSet( Value, CVAR_Bool );
		break;
	case GAMEMODE_SKULLTAG:

		skulltag.ForceSet( Value, CVAR_Bool );
		break;
	}

	Value.Bool = !!NETWORK_ReadByte( pByteStream );
	instagib.ForceSet( Value, CVAR_Bool );

	Value.Bool = !!NETWORK_ReadByte( pByteStream );
	buckshot.ForceSet( Value, CVAR_Bool );
}

//*****************************************************************************
//
static void client_SetGameSkill( BYTESTREAM_s *pByteStream )
{
	UCVarValue	Value;

	// Read in the gameskill setting, and set gameskill to this setting.
	Value.Int = NETWORK_ReadByte( pByteStream );
	gameskill.ForceSet( Value, CVAR_Int );

	// Do the same for botskill.
	Value.Int = NETWORK_ReadByte( pByteStream );
	botskill.ForceSet( Value, CVAR_Int );
}

//*****************************************************************************
//
static void client_SetGameDMFlags( BYTESTREAM_s *pByteStream )
{
	UCVarValue	Value;

	// Read in the dmflags value, and set it to this value.
	Value.Int = NETWORK_ReadLong( pByteStream );
	dmflags.ForceSet( Value, CVAR_Int );

	// Do the same for dmflags2.
	Value.Int = NETWORK_ReadLong( pByteStream );
	dmflags2.ForceSet( Value, CVAR_Int );

	// ... and compatflags.
	Value.Int = NETWORK_ReadLong( pByteStream );
	compatflags.ForceSet( Value, CVAR_Int );
}

//*****************************************************************************
//
static void client_SetGameModeLimits( BYTESTREAM_s *pByteStream )
{
	UCVarValue	Value;

	// Read in, and set the value for fraglimit.
#ifdef STAY_NETWORK_COMPATIBLE
	Value.Int = NETWORK_ReadByte( pByteStream );
#else
	Value.Int = NETWORK_ReadShort( pByteStream );
#endif
	fraglimit.ForceSet( Value, CVAR_Int );

	// Read in, and set the value for timelimit.
	Value.Float = NETWORK_ReadFloat( pByteStream );
	timelimit.ForceSet( Value, CVAR_Float );

	// Read in, and set the value for pointlimit.
	Value.Int = NETWORK_ReadShort( pByteStream );
	pointlimit.ForceSet( Value, CVAR_Int );

	// Read in, and set the value for duellimit.
	Value.Int = NETWORK_ReadByte( pByteStream );
	duellimit.ForceSet( Value, CVAR_Int );

	// Read in, and set the value for winlimit.
	Value.Int = NETWORK_ReadByte( pByteStream );
	winlimit.ForceSet( Value, CVAR_Int );

	// Read in, and set the value for wavelimit.
	Value.Int = NETWORK_ReadByte( pByteStream );
	wavelimit.ForceSet( Value, CVAR_Int );

	// Read in, and set the value for sv_cheats.
	Value.Int = NETWORK_ReadByte( pByteStream );
	sv_cheats.ForceSet( Value, CVAR_Int );
}

//*****************************************************************************
//
static void client_SetGameEndLevelDelay( BYTESTREAM_s *pByteStream )
{
	ULONG	ulDelay;

	ulDelay = NETWORK_ReadShort( pByteStream );

	GAME_SetEndLevelDelay( ulDelay );
}

//*****************************************************************************
//
static void client_SetGameModeState( BYTESTREAM_s *pByteStream )
{
	ULONG	ulModeState;

	ulModeState = NETWORK_ReadByte( pByteStream );

	if ( duel )
		DUEL_SetState( (DUELSTATE_e)ulModeState );
	else if ( lastmanstanding || teamlms )
		LASTMANSTANDING_SetState( (LMSSTATE_e)ulModeState );
	else if ( possession || teampossession )
		POSSESSION_SetState( (PSNSTATE_e)ulModeState );
	else if ( survival )
		SURVIVAL_SetState( (SURVIVALSTATE_e)ulModeState );
	else if ( invasion )
		INVASION_SetState( (INVASIONSTATE_e)ulModeState );
}

//*****************************************************************************
//
static void client_SetDuelNumDuels( BYTESTREAM_s *pByteStream )
{
	ULONG	ulNumDuels;

	// Read in the number of duels that have occured.
	ulNumDuels = NETWORK_ReadByte( pByteStream );

	DUEL_SetNumDuels( ulNumDuels );
}

//*****************************************************************************
//
static void client_SetLMSSpectatorSettings( BYTESTREAM_s *pByteStream )
{
	UCVarValue	Value;

	Value.Int = NETWORK_ReadLong( pByteStream );
	lmsspectatorsettings.ForceSet( Value, CVAR_Int );
}

//*****************************************************************************
//
static void client_SetLMSAllowedWeapons( BYTESTREAM_s *pByteStream )
{
	UCVarValue	Value;

	Value.Int = NETWORK_ReadLong( pByteStream );
	lmsallowedweapons.ForceSet( Value, CVAR_Int );
}

//*****************************************************************************
//
static void client_SetInvasionNumMonstersLeft( BYTESTREAM_s *pByteStream )
{
	ULONG	ulNumMonstersLeft;
	ULONG	ulNumArchVilesLeft;

	// Read in the number of monsters left.
	ulNumMonstersLeft = NETWORK_ReadShort( pByteStream );

	// Read in the number of arch-viles left.
	ulNumArchVilesLeft = NETWORK_ReadShort( pByteStream );

	// Set the number of monsters/archies left.
	INVASION_SetNumMonstersLeft( ulNumMonstersLeft );
	INVASION_SetNumArchVilesLeft( ulNumArchVilesLeft );
}

//*****************************************************************************
//
static void client_SetInvasionWave( BYTESTREAM_s *pByteStream )
{
	ULONG	ulWave;

	// Read in the current wave we're on.
	ulWave = NETWORK_ReadByte( pByteStream );

	// Set the current wave in the invasion module.
	INVASION_SetCurrentWave( ulWave );
}

//*****************************************************************************
//
static void client_DoPossessionArtifactPickedUp( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	ULONG	ulTicks;

	// Read in the player who picked up the possession artifact.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in how many ticks remain until the player potentially scores a point.
	ulTicks = NETWORK_ReadShort( pByteStream );

	// If this is an invalid player, break out.
	if ( CLIENT_IsValidPlayer( ulPlayer ) == false )
		return;

	// If we're not playing possession, there's no need to do this.
	if (( possession == false ) && ( teampossession == false ))
		return;

	// Finally, call the function that handles the picking up of the artifact.
	POSSESSION_ArtifactPickedUp( &players[ulPlayer], ulTicks );
}

//*****************************************************************************
//
static void client_DoPossessionArtifactDropped( BYTESTREAM_s *pByteStream )
{
	// If we're not playing possession, there's no need to do this.
	if (( possession == false ) && ( teampossession == false ))
		return;

	// Simply call this function.
	POSSESSION_ArtifactDropped( );
}

//*****************************************************************************
//
static void client_DoGameModeFight( BYTESTREAM_s *pByteStream )
{
	ULONG	ulWave;

	// What wave are we starting? (invasion only).
	ulWave = NETWORK_ReadByte( pByteStream );

	// Play fight sound, and draw gfx.
	if ( duel )
		DUEL_DoFight( );
	else if ( lastmanstanding || teamlms )
		LASTMANSTANDING_DoFight( );
	else if ( possession || teampossession )
		POSSESSION_DoFight( );
	else if ( survival )
		SURVIVAL_DoFight( );
	else if ( invasion )
		INVASION_BeginWave( ulWave );
}

//*****************************************************************************
//
static void client_DoGameModeCountdown( BYTESTREAM_s *pByteStream )
{
	ULONG	ulTicks;

	ulTicks = NETWORK_ReadShort( pByteStream );

	// Begin the countdown.
	if ( duel )
		DUEL_StartCountdown( ulTicks );
	else if ( lastmanstanding || teamlms )
		LASTMANSTANDING_StartCountdown( ulTicks );
	else if ( possession || teampossession )
		POSSESSION_StartCountdown( ulTicks );
	else if ( survival )
		SURVIVAL_StartCountdown( ulTicks );
	else if ( invasion )
		INVASION_StartCountdown( ulTicks );
}

//*****************************************************************************
//
static void client_DoGameModeWinSequence( BYTESTREAM_s *pByteStream )
{
	ULONG	ulWinner;

	ulWinner = NETWORK_ReadByte( pByteStream );

	// Begin the win sequence.
	if ( duel )
		DUEL_DoWinSequence( ulWinner );
	else if ( lastmanstanding || teamlms )
	{
		if ( lastmanstanding && ( ulWinner == consoleplayer ))
			ANNOUNCER_PlayEntry( cl_announcer, "YouWin" );
		else if ( teamlms && players[consoleplayer].bOnTeam && ( ulWinner == players[consoleplayer].ulTeam ))
			ANNOUNCER_PlayEntry( cl_announcer, "YouWin" );

		LASTMANSTANDING_DoWinSequence( ulWinner );
	}
//	else if ( possession || teampossession )
//		POSSESSION_DoWinSequence( ulWinner );
	else if ( invasion )
		INVASION_DoWaveComplete( );
}

//*****************************************************************************
//
static void client_SetTeamFrags( BYTESTREAM_s *pByteStream )
{
	ULONG	ulTeam;
	LONG	lFragCount;
	bool	bAnnounce;

	// Read in the team.
	ulTeam = NETWORK_ReadByte( pByteStream );

	// Read in the fragcount.
	lFragCount = NETWORK_ReadShort( pByteStream );

	// Announce a lead change... but don't do it if we're receiving a snapshot of the level!
	bAnnounce = !!NETWORK_ReadByte( pByteStream );
	if ( g_ConnectionState != CTS_ACTIVE )
		bAnnounce = false;

	// Finally, set the team's fragcount.
	TEAM_SetFragCount( ulTeam, lFragCount, bAnnounce );
}

//*****************************************************************************
//
static void client_SetTeamScore( BYTESTREAM_s *pByteStream )
{
	ULONG	ulTeam;
	LONG	lScore;
	bool	bAnnounce;

	// Read in the team having its score updated.
	ulTeam = NETWORK_ReadByte( pByteStream );

	// Read in the team's new score.
	lScore = NETWORK_ReadShort( pByteStream );

	// Should it be announced?
	bAnnounce = !!NETWORK_ReadByte( pByteStream );
	
	// Don't announce the score change if we're receiving a snapshot of the level!
	if ( g_ConnectionState != CTS_ACTIVE )
		bAnnounce = false;

	TEAM_SetScore( ulTeam, lScore, bAnnounce );

	SCOREBOARD_RefreshHUD( );
}

//*****************************************************************************
//
static void client_SetTeamWins( BYTESTREAM_s *pByteStream )
{
	ULONG	ulTeamIdx;
	LONG	lWinCount;
	bool	bAnnounce;

	// Read in the team.
	ulTeamIdx = NETWORK_ReadByte( pByteStream );

	// Read in the wins.
	lWinCount = NETWORK_ReadShort( pByteStream );

	// Read in whether or not it should be announced.	
	bAnnounce = !!NETWORK_ReadByte( pByteStream );

	// Don't announce if we're receiving a snapshot of the level!
	if ( g_ConnectionState != CTS_ACTIVE )
		bAnnounce = false;

	// Finally, set the team's win count.
	TEAM_SetWinCount( ulTeamIdx, lWinCount, bAnnounce );
}

//*****************************************************************************
//
static void client_SetTeamReturnTicks( BYTESTREAM_s *pByteStream )
{
	ULONG	ulTeam;
	ULONG	ulTicks;

	// Read in the team having its return ticks altered.
	ulTeam = NETWORK_ReadByte( pByteStream );

	// Read in the return ticks value.
	ulTicks = NETWORK_ReadShort( pByteStream );

	// Finally, set the return ticks for the given team.
	TEAM_SetReturnTicks( ulTeam, ulTicks );
}

//*****************************************************************************
//
static void client_TeamFlagReturned( BYTESTREAM_s *pByteStream )
{
	ULONG	ulTeam;

	// Read in the team that the flag has been returned for.
	ulTeam = NETWORK_ReadByte( pByteStream );

	// Finally, just call this function that does all the dirty work.
	TEAM_ExecuteReturnRoutine( ulTeam, NULL );
}

//*****************************************************************************
//
static void client_SpawnMissile( BYTESTREAM_s *pByteStream )
{
	char				*pszName;
	fixed_t				X;
	fixed_t				Y;
	fixed_t				Z;
	fixed_t				MomX;
	fixed_t				MomY;
	fixed_t				MomZ;
	LONG				lID;
	LONG				lTargetID;

	// Read in the XYZ location of the missile.
	X = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	Y = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	Z = NETWORK_ReadShort( pByteStream ) << FRACBITS;

	// Read in the XYZ momentum of the missile.
	MomX = NETWORK_ReadLong( pByteStream );
	MomY = NETWORK_ReadLong( pByteStream );
	MomZ = NETWORK_ReadLong( pByteStream );

	// Read in the name of the missile.
	pszName = NETWORK_ReadString( pByteStream );

	// Read in the network ID of the missile, and its target.
	lID = NETWORK_ReadShort( pByteStream );
	lTargetID = NETWORK_ReadShort( pByteStream );

	// Finally, spawn the missile.
	CLIENT_SpawnMissile( pszName, X, Y, Z, MomX, MomY, MomZ, lID, lTargetID );
}

//*****************************************************************************
//
static void client_SpawnMissileExact( BYTESTREAM_s *pByteStream )
{
	char				*pszName;
	fixed_t				X;
	fixed_t				Y;
	fixed_t				Z;
	fixed_t				MomX;
	fixed_t				MomY;
	fixed_t				MomZ;
	LONG				lID;
	LONG				lTargetID;

	// Read in the XYZ location of the missile.
	X = NETWORK_ReadLong( pByteStream );
	Y = NETWORK_ReadLong( pByteStream );
	Z = NETWORK_ReadLong( pByteStream );

	// Read in the XYZ momentum of the missile.
	MomX = NETWORK_ReadLong( pByteStream );
	MomY = NETWORK_ReadLong( pByteStream );
	MomZ = NETWORK_ReadLong( pByteStream );

	// Read in the name of the missile.
	pszName = NETWORK_ReadString( pByteStream );

	// Read in the network ID of the missile, and its target.
	lID = NETWORK_ReadShort( pByteStream );
	lTargetID = NETWORK_ReadShort( pByteStream );

	// Finally, spawn the missile.
	CLIENT_SpawnMissile( pszName, X, Y, Z, MomX, MomY, MomZ, lID, lTargetID );
}

//*****************************************************************************
//
static void client_MissileExplode( BYTESTREAM_s *pByteStream )
{
	AActor		*pActor;
	LONG		lLine;
	line_t		*pLine;
	LONG		lID;
	fixed_t		X;
	fixed_t		Y;
	fixed_t		Z;
	bool		bNeedExplode = true;
	FState		*pDeadState;
	FState		*pState;

	// Read in the network ID of the exploding missile.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the line that the missile struck.
	lLine = NETWORK_ReadShort( pByteStream );
	if ( lLine >= 0 && lLine < numlines )
		pLine = &lines[lLine];
	else
		pLine = NULL;

	// Read in the XYZ of the explosion point.
	X = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	Y = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	Z = NETWORK_ReadShort( pByteStream ) << FRACBITS;

	// Find the actor associated with the given network ID.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
//		Printf( "client_MissileExplode: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// Move the new actor to the position.
	CLIENT_MoveThing( pActor, X, Y, Z );

	// See if any of these death frames match our current frame. If they do,
	// don't explode the missile.
	pDeadState = pActor->DeathState;
	while ( pDeadState != NULL )
	{
		if ( pActor->state == pDeadState )
		{
			bNeedExplode = false;
			break;
		}

		pState = pDeadState;
		pDeadState = pDeadState->GetNextState( );

		// If the state loops back to the beginning of the death state, or to itself, break out.
		if (( pDeadState == pActor->DeathState ) || ( pState == pDeadState ))
			break;
	}

	// Blow it up!
	if ( bNeedExplode )
		P_ExplodeMissile( pActor, pLine, NULL );
}

//*****************************************************************************
//
static void client_WeaponSound( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	char	*pszSound;

	// Read in the player who's creating a weapon sound.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the sound that's being played.
	pszSound = NETWORK_ReadString( pByteStream );

	// Check to make sure everything is valid. If not, break out.
	if (( CLIENT_IsValidPlayer( ulPlayer ) == false ) || ( players[ulPlayer].mo == NULL ))
		return;

	// Finally, play the sound.
	S_Sound( players[ulPlayer].mo, CHAN_WEAPON, pszSound, 1, ATTN_NORM );
}

//*****************************************************************************
//
static void client_WeaponChange( BYTESTREAM_s *pByteStream )
{
	const PClass	*pType;
	const char		*pszString;
	AWeapon			*pWeapon;

	// Read in the name of the weapon we're supposed to switch to.
	pszString = NETWORK_ReadString( pByteStream );

	// Some optimization. For standard Doom weapons, to reduce the size of the string
	// that's sent out, just send some key character that identifies the weapon, instead
	// of the full name.
	convertWeaponKeyLetterToFullString( pszString );

	if ( players[consoleplayer].mo == NULL )
		return;

	// If it's an invalid class, or not a weapon, don't switch.
	pType = PClass::FindClass( pszString );
	if (( pType == NULL ) || !( pType->IsDescendantOf( RUNTIME_CLASS( AWeapon ))))
		return;

	// If we dont have this weapon already, we do now!
	pWeapon = static_cast<AWeapon *>( players[consoleplayer].mo->FindInventory( pType ));
	if ( pWeapon == NULL )
		pWeapon = static_cast<AWeapon *>( players[consoleplayer].mo->GiveInventoryType( pType ));

	// If he still doesn't have the object after trying to give it to him... then YIKES!
	if ( pWeapon == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_WeaponChange: Failed to give inventory type, %s!\n", pszName );
#endif
		return;
	}

	// Bring the weapon up if necessary.
	if ( players[consoleplayer].ReadyWeapon != pWeapon )
		players[consoleplayer].PendingWeapon = pWeapon;
}

//*****************************************************************************
//
static void client_WeaponRailgun( BYTESTREAM_s *pByteStream )
{
	LONG		lID;
	FVector3	Start;
	FVector3	End;
	LONG		lColor1;
	LONG		lColor2;
	float		fMaxDiff;
	bool		bSilent;
	AActor		*pActor;

	// Read in the network ID of the source actor.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the XYZ position of the start of the trail.
	Start.X = NETWORK_ReadFloat( pByteStream );
	Start.Y = NETWORK_ReadFloat( pByteStream );
	Start.Z = NETWORK_ReadFloat( pByteStream );

	// Read in the XYZ position of the end of the trail.
	End.X = NETWORK_ReadFloat( pByteStream );
	End.Y = NETWORK_ReadFloat( pByteStream );
	End.Z = NETWORK_ReadFloat( pByteStream );

	// Read in the colors of the trail.
	lColor1 = NETWORK_ReadLong( pByteStream );
	lColor2 = NETWORK_ReadLong( pByteStream );

	// Read in maxdiff (whatever that is).
	fMaxDiff = NETWORK_ReadFloat( pByteStream );

	// Read in whether or not the trail should make a nosie.
	bSilent = !!NETWORK_ReadByte( pByteStream );

	// Find the actor associated with the given network ID.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_WeaponRailgun: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	P_DrawRailTrail( pActor, Start, End, lColor1, lColor2, fMaxDiff, bSilent );
}

//*****************************************************************************
//
static void client_SetSectorFloorPlane( BYTESTREAM_s *pByteStream )
{
	LONG		lSectorID;
	LONG		lHeight;
	sector_t	*pSector;
	LONG		lDelta;
	LONG		lLastPos;

	// Read in the sector network ID.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the height.
	lHeight = NETWORK_ReadShort( pByteStream ) << FRACBITS;

	// Find the sector associated with this network ID.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetSectorFloorPlane: Couldn't find sector: %d\n", ulSectorIdx );
#endif
		return;
	}

	// Calculate the change in floor height.
	lDelta = lHeight - pSector->floorplane.d;

	// Store the original height position.
	lLastPos = pSector->floorplane.d;

	// Change the height.
	pSector->floorplane.ChangeHeight( -lDelta );

	// Finally, adjust textures.
	pSector->floortexz += pSector->floorplane.HeightDiff( lLastPos );
}

//*****************************************************************************
//
static void client_SetSectorCeilingPlane( BYTESTREAM_s *pByteStream )
{
	LONG		lSectorID;
	LONG		lHeight;
	sector_t	*pSector;
	LONG		lDelta;
	LONG		lLastPos;

	// Read in the sector network ID.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the height.
	lHeight = NETWORK_ReadShort( pByteStream ) << FRACBITS;

	// Find the sector associated with this network ID.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetSectorCeilingPlane: Couldn't find sector: %d\n", ulSectorIdx );
#endif
		return;
	}

	// Calculate the change in ceiling height.
	lDelta = lHeight - pSector->ceilingplane.d;

	// Store the original height position.
	lLastPos = pSector->ceilingplane.d;

	// Change the height.
	pSector->ceilingplane.ChangeHeight( lDelta );

	// Finally, adjust textures.
	pSector->ceilingtexz += pSector->ceilingplane.HeightDiff( lLastPos );
}

//*****************************************************************************
//
static void client_SetSectorLightLevel( BYTESTREAM_s *pByteStream )
{
	LONG		lSectorID;
	LONG		lLightLevel;
	sector_t	*pSector;

	// Read in the sector network ID.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the new light level.
	lLightLevel = NETWORK_ReadByte( pByteStream );

	// Find the sector associated with this network ID.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetSectorLightLevel: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Finally, set the light level.
	pSector->lightlevel = lLightLevel;
}

//*****************************************************************************
//
static void client_SetSectorColor( BYTESTREAM_s *pByteStream )
{
	LONG		lSectorID;
	LONG		lR;
	LONG		lG;
	LONG		lB;
	LONG		lDesaturate;
	sector_t	*pSector;
	PalEntry	Color;

	// Read in the sector to have its panning altered.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the RGB and desaturate.
	lR = NETWORK_ReadByte( pByteStream );
	lG = NETWORK_ReadByte( pByteStream );
	lB = NETWORK_ReadByte( pByteStream );
	lDesaturate = NETWORK_ReadByte( pByteStream );

	// Now find the sector.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetSectorColor: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Finally, set the color.
	Color = PalEntry( lR, lG, lB );
	pSector->ColorMap = GetSpecialLights( Color, pSector->ColorMap->Fade, lDesaturate );
}

//*****************************************************************************
//
static void client_SetSectorFade( BYTESTREAM_s *pByteStream )
{
	LONG		lSectorID;
	LONG		lR;
	LONG		lG;
	LONG		lB;
	sector_t	*pSector;
	PalEntry	Fade;

	// Read in the sector to have its panning altered.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the RGB.
	lR = NETWORK_ReadByte( pByteStream );
	lG = NETWORK_ReadByte( pByteStream );
	lB = NETWORK_ReadByte( pByteStream );

	// Now find the sector.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetSectorFade: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Finally, set the fade.
	Fade = PalEntry( lR, lG, lB );
	pSector->ColorMap = GetSpecialLights( pSector->ColorMap->Color, Fade, pSector->ColorMap->Desaturate );
}

//*****************************************************************************
//
static void client_SetSectorFlat( BYTESTREAM_s *pByteStream )
{
	sector_t		*pSector;
	LONG			lSectorID;
	char			szCeilingFlatName[16];
	char			*pszFloorFlatName;
	LONG			lFlatLump;

	// Read in the sector ID.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the ceiling flat name.
	sprintf( szCeilingFlatName, NETWORK_ReadString( pByteStream ));

	// Read in the floor flat name.
	pszFloorFlatName = NETWORK_ReadString( pByteStream );

	// Not in a level. Nothing to do!
	if ( gamestate != GS_LEVEL )
		return;

	// Find the sector associated with this network ID.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetSectorFlat: Couldn't find sector: %d\n", lID );
#endif
		return; 
	}

	lFlatLump = TexMan.GetTexture( szCeilingFlatName, FTexture::TEX_Flat );
	pSector->ceilingpic = lFlatLump;

	lFlatLump = TexMan.GetTexture( pszFloorFlatName, FTexture::TEX_Flat );
	pSector->floorpic = lFlatLump;
}

//*****************************************************************************
//
static void client_SetSectorPanning( BYTESTREAM_s *pByteStream )
{
	LONG		lSectorID;
	LONG		lCeilingXOffset;
	LONG		lCeilingYOffset;
	LONG		lFloorXOffset;
	LONG		lFloorYOffset;
	sector_t	*pSector;

	// Read in the sector to have its panning altered.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read it's ceiling X offset.
	lCeilingXOffset = NETWORK_ReadByte( pByteStream );

	// Read it's ceiling Y offset.
	lCeilingYOffset = NETWORK_ReadByte( pByteStream );

	// Read it's floor X offset.
	lFloorXOffset = NETWORK_ReadByte( pByteStream );

	// Read it's floor Y offset.
	lFloorYOffset = NETWORK_ReadByte( pByteStream );

	// Now find the sector.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetSectorPanning: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Finally, set the offsets.
	pSector->ceiling_xoffs = lCeilingXOffset * FRACUNIT;
	pSector->ceiling_yoffs = lCeilingYOffset * FRACUNIT;

	pSector->floor_xoffs = lFloorXOffset * FRACUNIT;
	pSector->floor_yoffs = lFloorYOffset * FRACUNIT;
}

//*****************************************************************************
//
static void client_SetSectorRotation( BYTESTREAM_s *pByteStream )
{
	LONG		lSectorID;
	LONG		lCeilingRotation;
	LONG		lFloorRotation;
	sector_t	*pSector;
	PalEntry	Color;

	// Read in the sector to have its panning altered.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the ceiling and floor rotation.
	lCeilingRotation = NETWORK_ReadShort( pByteStream );
	lFloorRotation = NETWORK_ReadShort( pByteStream );

	// Now find the sector.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetSectorRotation: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Finally, set the rotation.
	pSector->ceiling_angle = ( lCeilingRotation * ANGLE_1 );
	pSector->floor_angle = ( lFloorRotation * ANGLE_1 );
}

//*****************************************************************************
//
static void client_SetSectorScale( BYTESTREAM_s *pByteStream )
{
	LONG		lSectorID;
	LONG		lCeilingXScale;
	LONG		lCeilingYScale;
	LONG		lFloorXScale;
	LONG		lFloorYScale;
	sector_t	*pSector;

	// Read in the sector to have its panning altered.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the ceiling and floor scale.
	lCeilingXScale = NETWORK_ReadShort( pByteStream ) * FRACBITS;
	lCeilingYScale = NETWORK_ReadShort( pByteStream ) * FRACBITS;
	lFloorXScale = NETWORK_ReadShort( pByteStream ) * FRACBITS;
	lFloorYScale = NETWORK_ReadShort( pByteStream ) * FRACBITS;

	// Now find the sector.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetSectorScale: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Finally, set the scale.
	pSector->ceiling_xscale = lCeilingXScale;
	pSector->ceiling_yscale = lCeilingYScale;
	pSector->floor_xscale = lFloorXScale;
	pSector->floor_yscale = lFloorYScale;
}

//*****************************************************************************
//
static void client_SetSectorFriction( BYTESTREAM_s *pByteStream )
{
	LONG		lSectorID;
	LONG		lFriction;
	LONG		lMoveFactor;
	sector_t	*pSector;

	// Read in the sector to have its friction altered.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the ceiling and floor scale.
	lFriction = NETWORK_ReadLong( pByteStream );
	lMoveFactor = NETWORK_ReadLong( pByteStream );

	// Now find the sector.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetSectorScale: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Set the friction.
	pSector->friction = lFriction;
	pSector->movefactor = lMoveFactor;

	// I'm not sure if we need to do this, but let's do it anyway.
	if ( lFriction == ORIG_FRICTION )
		pSector->special &= ~FRICTION_MASK;
	else
		pSector->special |= FRICTION_MASK;
}

//*****************************************************************************
//
static void client_SetSectorAngleYOffset( BYTESTREAM_s *pByteStream )
{
	LONG		lSectorID;
	LONG		lCeilingAngle;
	LONG		lCeilingYOffset;
	LONG		lFloorAngle;
	LONG		lFloorYOffset;
	sector_t	*pSector;

	// Read in the sector to have its friction altered.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the sector's ceiling and floor angle and y-offset.
	lCeilingAngle = NETWORK_ReadLong( pByteStream );
	lCeilingYOffset = NETWORK_ReadLong( pByteStream );
	lFloorAngle = NETWORK_ReadLong( pByteStream );
	lFloorYOffset = NETWORK_ReadLong( pByteStream );

	// Now find the sector.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetSectorScale: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Set the sector's angle and y-offset.
	pSector->base_ceiling_angle = lCeilingAngle;
	pSector->base_ceiling_yoffs = lCeilingYOffset;
	pSector->base_floor_angle = lFloorAngle;
	pSector->base_floor_yoffs = lFloorYOffset;
}

//*****************************************************************************
//
static void client_SetSectorGravity( BYTESTREAM_s *pByteStream )
{
	LONG		lSectorID;
	float		fGravity;
	sector_t	*pSector;

	// Read in the sector to have its friction altered.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the sector's gravity.
	fGravity = NETWORK_ReadFloat( pByteStream );

	// Now find the sector.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetSectorScale: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Set the sector's gravity.
	pSector->gravity = fGravity;
}

//*****************************************************************************
//
static void client_StopSectorLightEffect( BYTESTREAM_s *pByteStream )
{
	LONG							lSectorID;
	sector_t						*pSector;
	TThinkerIterator<DLighting>		Iterator;
	DLighting						*pEffect;

	// Read in the sector to have its light effect stopped.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Now find the sector.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_StopSectorLightEffect: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Finally, delete any effects this sector has.
	while (( pEffect = Iterator.Next( )) != NULL )
	{
		if ( pEffect->GetSector( ) == pSector )
		{
			pEffect->Destroy( );
			return;
		}
	}
}

//*****************************************************************************
//
static void client_DestroyAllSectorMovers( BYTESTREAM_s *pByteStream )
{
	ULONG	ulIdx;

	for ( ulIdx = 0; ulIdx < (ULONG)numsectors; ulIdx++ )
	{
		if ( sectors[ulIdx].ceilingdata )
			sectors[ulIdx].ceilingdata->Destroy( );
		if ( sectors[ulIdx].floordata )
			sectors[ulIdx].floordata->Destroy( );
	}
}

//*****************************************************************************
//
static void client_DoSectorLightFireFlicker( BYTESTREAM_s *pByteStream )
{
	LONG			lSectorID;
	sector_t		*pSector;
	LONG			lMaxLight;
	LONG			lMinLight;
	DFireFlicker	*pFireFlicker;

	// Read in the sector the light effect is attached to.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the sector's light level when the light effect is in its bright phase.
	lMaxLight = NETWORK_ReadByte( pByteStream );

	// Read in the sector's light level when the light effect is in its dark phase.
	lMinLight = NETWORK_ReadByte( pByteStream );

	// Now find the sector.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DoSectorLightFireFlicker: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Create the light effect.
	pFireFlicker = new DFireFlicker( pSector, lMaxLight, lMinLight );
}

//*****************************************************************************
//
static void client_DoSectorLightFlicker( BYTESTREAM_s *pByteStream )
{
	LONG			lSectorID;
	sector_t		*pSector;
	LONG			lMaxLight;
	LONG			lMinLight;
	DFlicker		*pFlicker;

	// Read in the sector the light effect is attached to.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the sector's light level when the light effect is in its bright phase.
	lMaxLight = NETWORK_ReadByte( pByteStream );

	// Read in the sector's light level when the light effect is in its dark phase.
	lMinLight = NETWORK_ReadByte( pByteStream );

	// Now find the sector.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DoSectorLightFireFlicker: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Create the light effect.
	pFlicker = new DFlicker( pSector, lMaxLight, lMinLight );
}

//*****************************************************************************
//
static void client_DoSectorLightLightFlash( BYTESTREAM_s *pByteStream )
{
	LONG			lSectorID;
	sector_t		*pSector;
	LONG			lMaxLight;
	LONG			lMinLight;
	DLightFlash		*pLightFlash;

	// Read in the sector the light effect is attached to.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the sector's light level when the light effect is in its bright phase.
	lMaxLight = NETWORK_ReadByte( pByteStream );

	// Read in the sector's light level when the light effect is in its dark phase.
	lMinLight = NETWORK_ReadByte( pByteStream );

	// Now find the sector.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DoSectorLightLightFlash: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Create the light effect.
	pLightFlash = new DLightFlash( pSector, lMinLight, lMaxLight );
}

//*****************************************************************************
//
static void client_DoSectorLightStrobe( BYTESTREAM_s *pByteStream )
{
	LONG			lSectorID;
	sector_t		*pSector;
	LONG			lDarkTime;
	LONG			lBrightTime;
	LONG			lMaxLight;
	LONG			lMinLight;
	LONG			lCount;
	DStrobe			*pStrobe;

	// Read in the sector the light effect is attached to.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in how long the effect stays in its dark phase.
	lDarkTime = NETWORK_ReadShort( pByteStream );

	// Read in how long the effect stays in its bright phase.
	lBrightTime = NETWORK_ReadShort( pByteStream );

	// Read in the sector's light level when the light effect is in its bright phase.
	lMaxLight = NETWORK_ReadByte( pByteStream );

	// Read in the sector's light level when the light effect is in its dark phase.
	lMinLight = NETWORK_ReadByte( pByteStream );

	// Read in the amount of time left until the light changes from bright to dark, or vice
	// versa.
	lCount = NETWORK_ReadByte( pByteStream );

	// Now find the sector.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DoSectorLightStrobe: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Create the light effect.
	pStrobe = new DStrobe( pSector, lMaxLight, lMinLight, lBrightTime, lDarkTime );
	pStrobe->SetCount( lCount );
}

//*****************************************************************************
//
static void client_DoSectorLightGlow( BYTESTREAM_s *pByteStream )
{
	LONG			lSectorID;
	sector_t		*pSector;
	DGlow			*pGlow;

	// Read in the sector the light effect is attached to.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Now find the sector.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DoSectorLightGlow: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Create the light effect.
	pGlow = new DGlow( pSector );
}

//*****************************************************************************
//
static void client_DoSectorLightGlow2( BYTESTREAM_s *pByteStream )
{
	LONG			lSectorID;
	sector_t		*pSector;
	LONG			lStart;
	LONG			lEnd;
	LONG			lTics;
	LONG			lMaxTics;
	bool			bOneShot;
	DGlow2			*pGlow2;

	// Read in the sector the light effect is attached to.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the start light level of the effect.
	lStart = NETWORK_ReadByte( pByteStream );

	// Read in the end light level of the effect.
	lEnd = NETWORK_ReadByte( pByteStream );

	// Read in the current progression of the effect.
	lTics = NETWORK_ReadShort( pByteStream );

	// Read in how many tics it takes to get from start to end.
	lMaxTics = NETWORK_ReadShort( pByteStream );

	// Read in whether or not the glow loops, or ends after one shot.
	bOneShot = !!NETWORK_ReadByte( pByteStream );

	// Now find the sector.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DoSectorLightStrobe: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Create the light effect.
	pGlow2 = new DGlow2( pSector, lStart, lEnd, lMaxTics, bOneShot );
	pGlow2->SetTics( lTics );
}

//*****************************************************************************
//
static void client_DoSectorLightPhased( BYTESTREAM_s *pByteStream )
{
	LONG			lSectorID;
	sector_t		*pSector;
	LONG			lBaseLevel;
	LONG			lPhase;
	DPhased			*pPhased;

	// Read in the sector the light effect is attached to.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the effect's base level parameter.
	lBaseLevel = NETWORK_ReadByte( pByteStream );

	// Read in the sector's phase parameter.
	lPhase = NETWORK_ReadByte( pByteStream );

	// Now find the sector.
	pSector = CLIENT_FindSectorByID( lSectorID );
	if ( pSector == NULL )
	{ 
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DoSectorLightFireFlicker: Cannot find sector: %d\n", lSectorID );
#endif
		return; 
	}

	// Create the light effect.
	pPhased = new DPhased( pSector, lBaseLevel, lPhase );
}

//*****************************************************************************
//
static void client_SetLineAlpha( BYTESTREAM_s *pByteStream )
{
	line_t	*pLine;
	ULONG	ulLineIdx;
	ULONG	ulAlpha;

	// Read in the line to have its alpha altered.
	ulLineIdx = NETWORK_ReadShort( pByteStream );

	// Read in the new alpha.
	ulAlpha = NETWORK_ReadByte( pByteStream );

	pLine = &lines[ulLineIdx];
	if (( pLine == NULL ) || ( ulLineIdx >= numlines ))
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetLineAlpha: Couldn't find line: %d\n", ulLineIdx );
#endif
		return;
	}

	// Finally, set the alpha.
	pLine->alpha = ulAlpha;
}

//*****************************************************************************
//
static void client_SetLineTexture( BYTESTREAM_s *pByteStream )
{
	line_t		*pLine;
	side_t		*pSide;
	ULONG		ulLineIdx;
	char		*pszTextureName;
	ULONG		ulSide;
	ULONG		ulPosition;
	LONG		lTexture;

	// Read in the line to have its alpha altered.
	ulLineIdx = NETWORK_ReadShort( pByteStream );

	// Read in the new texture name.
	pszTextureName = NETWORK_ReadString( pByteStream );

	// Read in the side.
	ulSide = !!NETWORK_ReadByte( pByteStream );

	// Read in the position.
	ulPosition = NETWORK_ReadByte( pByteStream );

	pLine = &lines[ulLineIdx];
	if (( pLine == NULL ) || ( ulLineIdx >= numlines ))
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetLineTexture: Couldn't find line: %d\n", ulLineIdx );
#endif
		return;
	}

	lTexture = TexMan.CheckForTexture( pszTextureName, FTexture::TEX_Wall );

	if ( lTexture < 0 )
		return;

	if ( pLine->sidenum[ulSide] == NO_INDEX )
		return;

	pSide = &sides[pLine->sidenum[ulSide]];

	switch ( ulPosition )
	{
	case 0 /*TEXTURE_TOP*/:
			
		pSide->toptexture = lTexture;
		break;
	case 1 /*TEXTURE_MIDDLE*/:

		pSide->midtexture = lTexture;
		break;
	case 2 /*TEXTURE_BOTTOM*/:

		pSide->bottomtexture = lTexture;
		break;
	default:

		break;
	}
}

//*****************************************************************************
//
static void client_SetLineBlocking( BYTESTREAM_s *pByteStream )
{
	LONG	lLine;
	LONG	lBlockFlags;

	// Read in the line ID.
	lLine = NETWORK_ReadShort( pByteStream );

	// Read in the blocking flags.
	lBlockFlags = NETWORK_ReadLong( pByteStream );

	// Invalid line ID.
	if (( lLine >= numlines ) || ( lLine < 0 ))
		return;

	lines[lLine].flags &= ~(ML_BLOCKING|ML_BLOCKPLAYERS|ML_BLOCKEVERYTHING|ML_RAILING);
	lines[lLine].flags |= lBlockFlags;
}

//*****************************************************************************
//
static void client_Sound( BYTESTREAM_s *pByteStream )
{
	char	*pszSoundString;
	LONG	lChannel;
	LONG	lVolume;
	LONG	lAttenuation;

	// Read in the channel.
	lChannel = NETWORK_ReadByte( pByteStream );

	// Read in the name of the sound to play.
	pszSoundString = NETWORK_ReadString( pByteStream );

	// Read in the volume.
	lVolume = NETWORK_ReadByte( pByteStream );
	if ( lVolume > 127 )
		lVolume = 127;

	// Read in the attenuation.
	lAttenuation = NETWORK_ReadByte( pByteStream );

	// Finally, play the sound.
	S_Sound( lChannel, pszSoundString, (float)lVolume / 127.f, lAttenuation );
}

//*****************************************************************************
//
static void client_SoundID( BYTESTREAM_s *pByteStream )
{
	fixed_t	SoundPoint[3];
	LONG	lSoundID;
	LONG	lChannel;
	LONG	lVolume;
	LONG	lAttenuation;

	// Read in the coordinates of the sound.
	SoundPoint[0] = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	SoundPoint[1] = NETWORK_ReadShort( pByteStream ) << FRACBITS;

	// Read in the sound ID.
//	lSoundID = NETWORK_ReadShort( pByteStream );
	lSoundID = NETWORK_ReadShort( pByteStream );

	// Read in the channel.
	lChannel = NETWORK_ReadByte( pByteStream );

	// Read in the volume.
	lVolume = NETWORK_ReadByte( pByteStream );
	if ( lVolume > 127 )
		lVolume = 127;

	// Read in the attenuation.
	lAttenuation = NETWORK_ReadByte( pByteStream );

	// Finally, play the sound.
	S_SoundID( SoundPoint, lChannel, lSoundID, (float)lVolume / 127.f, lAttenuation );
}

//*****************************************************************************
//
static void client_SoundActor( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	char	*pszSoundString;
	LONG	lChannel;
	LONG	lVolume;
	LONG	lAttenuation;
	AActor	*pActor;

	// Read in the spot ID.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the channel.
	lChannel = NETWORK_ReadByte( pByteStream );

	// Read in the name of the sound to play.
	pszSoundString = NETWORK_ReadString( pByteStream );

	// Read in the volume.
	lVolume = NETWORK_ReadByte( pByteStream );
	if ( lVolume > 127 )
		lVolume = 127;

	// Read in the attenuation.
	lAttenuation = NETWORK_ReadByte( pByteStream );

	// Find the actor from the ID.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SoundActor: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// Finally, play the sound.
	S_Sound( pActor, lChannel, pszSoundString, (float)lVolume / 127.f, lAttenuation );
}

//*****************************************************************************
//
static void client_SoundIDActor( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	LONG	lSoundID;
	LONG	lChannel;
	LONG	lVolume;
	LONG	lAttenuation;
	AActor	*pActor;

	// Read in the spot ID.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the channel.
	lChannel = NETWORK_ReadByte( pByteStream );

	// Read in the sound ID.
	lSoundID = NETWORK_ReadShort( pByteStream );

	// Read in the volume.
	lVolume = NETWORK_ReadByte( pByteStream );
	if ( lVolume > 127 )
		lVolume = 127;

	// Read in the attenuation.
	lAttenuation = NETWORK_ReadByte( pByteStream );

	// Find the actor from the ID.
	pActor = NETWORK_FindThingByNetID( lID );
	if ( pActor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SoundIDActor: Couldn't find thing: %d\n", lID );
#endif
		return;
	}

	// Finally, play the sound.
	S_SoundID( pActor, lChannel, lSoundID, (float)lVolume / 127.f, lAttenuation );
}

//*****************************************************************************
//
static void client_SoundPoint( BYTESTREAM_s *pByteStream )
{
	char	*pszSoundString;
	LONG	lChannel;
	LONG	lVolume;
	LONG	lAttenuation;
	fixed_t	X;
	fixed_t	Y;

	// Read in the XY of the sound.
	X = NETWORK_ReadShort( pByteStream ) << FRACBITS;
	Y = NETWORK_ReadShort( pByteStream ) << FRACBITS;

	// Read in the channel.
	lChannel = NETWORK_ReadByte( pByteStream );

	// Read in the name of the sound to play.
	pszSoundString = NETWORK_ReadString( pByteStream );

	// Read in the volume.
	lVolume = NETWORK_ReadByte( pByteStream );
	if ( lVolume > 127 )
		lVolume = 127;

	// Read in the attenuation.
	lAttenuation = NETWORK_ReadByte( pByteStream );

	// Finally, play the sound.
	S_Sound( X, Y, CHAN_BODY, pszSoundString, (float)lVolume / 127.f, lAttenuation );
}

//*****************************************************************************
//
static void client_StartSectorSequence( BYTESTREAM_s *pByteStream )
{
	LONG		lSectorID;
	char		*pszSequence;
	sector_t	*pSector;

	// Read in the sector ID.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the sound sequence to play.
	pszSequence = NETWORK_ReadString( pByteStream );

	// Make sure the sector ID is valid.
	if (( lSectorID >= 0 ) && ( lSectorID < numsectors ))
		pSector = &sectors[lSectorID];
	else
		return;

	// Finally, play the given sound sequence for this sector.
	SN_StartSequence( pSector, pszSequence, 0 );
}

//*****************************************************************************
//
static void client_StopSectorSequence( BYTESTREAM_s *pByteStream )
{
	LONG		lSectorID;
	sector_t	*pSector;

	// Read in the sector ID.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Make sure the sector ID is valid.
	if (( lSectorID >= 0 ) && ( lSectorID < numsectors ))
		pSector = &sectors[lSectorID];
	else
		return;

	// Finally, stop the sound sequence for this sector.
	SN_StopSequence( pSector );
}

//*****************************************************************************
//
static void client_CallVote( BYTESTREAM_s *pByteStream )
{
	char	szCommand[128];
	char	*pszParameters;
	ULONG	ulVoteCaller;

	// Read in the vote starter.
	ulVoteCaller = NETWORK_ReadByte( pByteStream );

	// Read in the command.
	sprintf( szCommand, NETWORK_ReadString( pByteStream ));

	// Read in the parameters.
	pszParameters = NETWORK_ReadString( pByteStream );

	// Begin the vote!
	CALLVOTE_BeginVote( szCommand, pszParameters, ulVoteCaller );
}

//*****************************************************************************
//
static void client_PlayerVote( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	bool	bYes;

	// Read in the player making the vote.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Did the player vote yes?
	bYes = !!NETWORK_ReadByte( pByteStream );

	if ( bYes )
		CALLVOTE_VoteYes( ulPlayer );
	else
		CALLVOTE_VoteNo( ulPlayer );
}

//*****************************************************************************
//
static void client_VoteEnded( BYTESTREAM_s *pByteStream )
{
	bool	bPassed;

	// Did the vote pass?
	bPassed = !!NETWORK_ReadByte( pByteStream );

	CALLVOTE_EndVote( bPassed );
}

//*****************************************************************************
//
static void client_MapLoad( BYTESTREAM_s *pByteStream )
{
	bool	bPlaying;
	char	*pszMap;
	
	// Read in the lumpname of the map we're about to load.
	pszMap = NETWORK_ReadString( pByteStream );

	// Check to see if we have the map.
	if ( P_CheckIfMapExists( pszMap ))
	{
		// Save our demo recording status since G_InitNew resets it.
		bPlaying = CLIENTDEMO_IsPlaying( );

		// Start new level.
		G_InitNew( pszMap, false );

		// Restore our demo recording status.
		CLIENTDEMO_SetPlaying( bPlaying );

		// [BB] viewactive is set in G_InitNew
		// For right now, the view is not active.
		//viewactive = false;

		// Kill the console.
		C_HideConsole( );
	}
	else
		Printf( "client_MapLoad: Unknown map: %s\n", pszMap );
}

//*****************************************************************************
//
static void client_MapNew( BYTESTREAM_s *pByteStream )
{
	char	*pszMapName;

	// Read in the new mapname the server is switching the level to.
	pszMapName = NETWORK_ReadString( pByteStream );

	// Clear out our local buffer.
	NETWORK_ClearBuffer( &g_LocalBuffer );

	// Back to the full console.
	gameaction = ga_fullconsole;

	// Also, the view is no longer active.
	viewactive = false;

	Printf( "Connecting to %s\n%s\n", NETWORK_AddressToString( g_AddressServer ), pszMapName );

	// Update the connection state, and begin trying to reconnect.
	CLIENT_SetConnectionState( CTS_TRYCONNECT );
	CLIENT_AttemptConnection( );
}

//*****************************************************************************
//
static void client_MapExit( BYTESTREAM_s *pByteStream )
{
	LONG	lPos;

	lPos = NETWORK_ReadByte( pByteStream );

	// Never loaded a level.
	if ( gamestate == GS_FULLCONSOLE )
		return;

	// Ingore if we get this twice (could happen).
	if ( gamestate != GS_INTERMISSION )
		goOn( lPos, true, false, false );
}

//*****************************************************************************
//
static void client_MapAuthenticate( BYTESTREAM_s *pByteStream )
{
	char	*pszMapName;

	pszMapName = NETWORK_ReadString( pByteStream );

	// Nothing to do in demo mode.
	if ( CLIENTDEMO_IsPlaying( ))
		return;

	NETWORK_WriteByte( &g_LocalBuffer.ByteStream, CLC_AUTHENTICATELEVEL );

	// Send a checksum of our verticies, linedefs, sidedefs, and sectors.
	CLIENT_AuthenticateLevel( pszMapName );

	g_lBytesSent += g_LocalBuffer.ulCurrentSize;
	if ( g_lBytesSent > g_lMaxBytesSent )
		g_lMaxBytesSent = g_lBytesSent;
	NETWORK_LaunchPacket( &g_LocalBuffer, g_AddressServer );
	NETWORK_ClearBuffer( &g_LocalBuffer );
}

//*****************************************************************************
//
static void client_SetMapTime( BYTESTREAM_s *pByteStream )
{
	level.time = NETWORK_ReadLong( pByteStream );
}

//*****************************************************************************
//
static void client_SetMapNumKilledMonsters( BYTESTREAM_s *pByteStream )
{
	level.killed_monsters = NETWORK_ReadShort( pByteStream );
}

//*****************************************************************************
//
static void client_SetMapNumFoundItems( BYTESTREAM_s *pByteStream )
{
	level.found_items = NETWORK_ReadShort( pByteStream );
}

//*****************************************************************************
//
static void client_SetMapNumFoundSecrets( BYTESTREAM_s *pByteStream )
{
	level.found_secrets = NETWORK_ReadShort( pByteStream );
}

//*****************************************************************************
//
static void client_SetMapNumTotalMonsters( BYTESTREAM_s *pByteStream )
{
	level.total_monsters = NETWORK_ReadShort( pByteStream );
}

//*****************************************************************************
//
static void client_SetMapNumTotalItems( BYTESTREAM_s *pByteStream )
{
	level.total_items = NETWORK_ReadShort( pByteStream );
}

//*****************************************************************************
//
static void client_SetMapMusic( BYTESTREAM_s *pByteStream )
{
	char	*pszMusicString;

	// Read in the music string.
	pszMusicString = NETWORK_ReadString( pByteStream );

	// Change the music.
	S_ChangeMusic( pszMusicString );
}

//*****************************************************************************
//
static void client_SetMapSky( BYTESTREAM_s *pByteStream )
{
	char	*pszSky1;
	char	*pszSky2;

	// Read in the texture name of the first sky.
	pszSky1 = NETWORK_ReadString( pByteStream );

	if ( pszSky1 != NULL )
	{
		strncpy( level.skypic1, pszSky1, 8 );
		sky1texture = TexMan.GetTexture( pszSky1, FTexture::TEX_Wall, FTextureManager::TEXMAN_Overridable );
	}

	// Read in the texture name of the second sky.
	pszSky2 = NETWORK_ReadString( pByteStream );

	if ( pszSky2 != NULL )
	{
		strncpy( level.skypic2, pszSky2, 8 );
		sky2texture = TexMan.GetTexture( pszSky2, FTexture::TEX_Wall, FTextureManager::TEXMAN_Overridable );
	}

	// Set some other sky properties.
	R_InitSkyMap( );
}

//*****************************************************************************
//
static void client_GiveInventory( BYTESTREAM_s *pByteStream )
{
	const PClass	*pType;
	ULONG			ulPlayer;
	char			*pszName;
	LONG			lAmount;
	AInventory		*pInventory;

	// Read in the player ID.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the name of the type of item to give.
	pszName = NETWORK_ReadString( pByteStream );

	// Read in the amount of this inventory type the player has.
	lAmount = NETWORK_ReadShort( pByteStream );

	// Check to make sure everything is valid. If not, break out.
	if (( CLIENT_IsValidPlayer( ulPlayer ) == false ) || ( players[ulPlayer].mo == NULL ))
		return;

	pType = PClass::FindClass( pszName );
	if ( pType == NULL )
		return;

	// Try to find this object within the player's personal inventory.
	pInventory = players[ulPlayer].mo->FindInventory( pType );

	// If the player doesn't have this type, give it to him.
	if ( pInventory == NULL )
		pInventory = players[ulPlayer].mo->GiveInventoryType( pType );

	// If he still doesn't have the object after trying to give it to him... then YIKES!
	if ( pInventory == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_TakeInventory: Failed to give inventory type, %s!\n", pszName );
#endif
		return;
	}

	// Set the new amount of the inventory object.
	pInventory->Amount = lAmount;
	if ( pInventory->Amount <= 0 )
	{
		// We can't actually destroy ammo, since it's vital for weapons.
		if ( pInventory->GetClass( )->ParentClass == RUNTIME_CLASS( AAmmo ))
			pInventory->Amount = 0;
		// But, we can destroy everything else.
		else
			pInventory->Destroy( );
	}

	// Since an item displayed on the HUD may have been given, refresh the HUD.
	SCOREBOARD_RefreshHUD( );
}

//*****************************************************************************
//
static void client_TakeInventory( BYTESTREAM_s *pByteStream )
{
	const PClass	*pType;
	ULONG			ulPlayer;
	char			*pszName;
	LONG			lAmount;
	AInventory		*pInventory;

	// Read in the player ID.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the name of the type of item to take away.
	pszName = NETWORK_ReadString( pByteStream );

	// Read in the new amount of this inventory type the player has.
	lAmount = NETWORK_ReadShort( pByteStream );

	// Check to make sure everything is valid. If not, break out.
	if (( CLIENT_IsValidPlayer( ulPlayer ) == false ) || ( players[ulPlayer].mo == NULL ))
		return;

	pType = PClass::FindClass( pszName );
	if ( pType == NULL )
		return;

	// Try to find this object within the player's personal inventory.
	pInventory = players[ulPlayer].mo->FindInventory( pType );

	// If the player doesn't have this type, give it to him.
	if ( pInventory == NULL )
		pInventory = players[ulPlayer].mo->GiveInventoryType( pType );

	// If he still doesn't have the object after trying to give it to him... then YIKES!
	if ( pInventory == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_TakeInventory: Failed to give inventory type, %s!\n", pszName );
#endif
		return;
	}

	// Set the new amount of the inventory object.
	pInventory->Amount = lAmount;
	if ( pInventory->Amount <= 0 )
	{
		// We can't actually destroy ammo, since it's vital for weapons.
		if ( pInventory->GetClass( )->ParentClass == RUNTIME_CLASS( AAmmo ))
			pInventory->Amount = 0;
		// But, we can destroy everything else.
		else
			pInventory->Destroy( );
	}

	// Since an item displayed on the HUD may have been taken away, refresh the HUD.
	SCOREBOARD_RefreshHUD( );
}

//*****************************************************************************
//
static void client_GivePowerup( BYTESTREAM_s *pByteStream )
{
	const PClass	*pType;
	ULONG			ulPlayer;
	char			*pszName;
	LONG			lAmount;
	LONG			lEffectTics;
	AInventory		*pInventory;

	// Read in the player ID.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the name of the type of item to give.
	pszName = NETWORK_ReadString( pByteStream );

	// Read in the amount of this inventory type the player has.
	lAmount = NETWORK_ReadShort( pByteStream );

	// Read in the amount of time left on this powerup.
	lEffectTics = NETWORK_ReadShort( pByteStream );

	// Check to make sure everything is valid. If not, break out.
	if (( CLIENT_IsValidPlayer( ulPlayer ) == false ) || ( players[ulPlayer].mo == NULL ))
		return;

	pType = PClass::FindClass( pszName );
	if ( pType == NULL )
		return;

	// If this isn't a powerup, just quit.
	if ( pType->IsDescendantOf( RUNTIME_CLASS( APowerup )) == false )
		return;

	// Try to find this object within the player's personal inventory.
	pInventory = players[ulPlayer].mo->FindInventory( pType );

	// If the player doesn't have this type, give it to him.
	if ( pInventory == NULL )
		pInventory = players[ulPlayer].mo->GiveInventoryType( pType );

	// If he still doesn't have the object after trying to give it to him... then YIKES!
	if ( pInventory == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_TakeInventory: Failed to give inventory type, %s!\n", pszName );
#endif
		return;
	}

	// Set the new amount of the inventory object.
	pInventory->Amount = lAmount;
	if ( pInventory->Amount <= 0 )
	{
		pInventory->Destroy( );
		pInventory = NULL;
	}

	if ( pInventory )
		static_cast<APowerup *>( pInventory )->EffectTics = lEffectTics;

	// Since an item displayed on the HUD may have been given, refresh the HUD.
	SCOREBOARD_RefreshHUD( );
}

//*****************************************************************************
//
static void client_DoInventoryPickup( BYTESTREAM_s *pByteStream )
{
	ULONG			ulPlayer;
	char			szClassName[64];
	char			*pszPickupMessage;
	AInventory		*pInventory;

	static LONG			s_lLastMessageTic = 0;
	static char			s_szLastMessage[256];

	// Read in the player ID.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the class name of the item.
	sprintf( szClassName, NETWORK_ReadString( pByteStream ));

	// Read in the pickup message.
	pszPickupMessage = NETWORK_ReadString( pByteStream );

	// Check to make sure everything is valid. If not, break out.
	if (( CLIENT_IsValidPlayer( ulPlayer ) == false ) || ( players[ulPlayer].mo == NULL ))
		return;

	// If the player doesn't have this inventory item, break out.
	pInventory = static_cast<AInventory *>( Spawn( PClass::FindClass( szClassName ), 0, 0, 0, NO_REPLACE ));
	if ( pInventory == NULL )
		return;

	// Print out the pickup message.
	if (( players[ulPlayer].mo->CheckLocalView( consoleplayer )) &&
		(( s_lLastMessageTic != gametic ) || ( stricmp( s_szLastMessage, pszPickupMessage ) != 0 )))
	{
		s_lLastMessageTic = gametic;
		strcpy( s_szLastMessage, pszPickupMessage );

		// This code is from PrintPickupMessage().
		if ( pszPickupMessage != NULL )
		{
			if ( pszPickupMessage[0] == '$' )
				pszPickupMessage = (char *)GStrings( pszPickupMessage + 1 );

			Printf( PRINT_LOW, "%s\n", pszPickupMessage );
		}

		StatusBar->FlashCrosshair( );
	}

	// Play the inventory pickup sound and blend the screen.
	pInventory->PlayPickupSound( players[ulPlayer].mo );
	players[ulPlayer].bonuscount = BONUSADD;

	// Play the announcer pickup entry as well.
	if ( players[ulPlayer].mo->CheckLocalView( consoleplayer ))
		ANNOUNCER_PlayEntry( cl_announcer, pInventory->PickupAnnouncerEntry( ));

	// Finally, destroy the temporarily spawned inventory item.
	pInventory->Destroy( );
}

//*****************************************************************************
//
static void client_DestroyAllInventory( BYTESTREAM_s *pByteStream )
{
	ULONG			ulPlayer;

	// Read in the player ID.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Check to make sure everything is valid. If not, break out.
	if (( CLIENT_IsValidPlayer( ulPlayer ) == false ) || ( players[ulPlayer].mo == NULL ))
		return;

	// Finally, destroy the player's inventory.
	players[ulPlayer].mo->DestroyAllInventory( );
}

//*****************************************************************************
//
static void client_DoDoor( BYTESTREAM_s *pByteStream )
{
	LONG			lSectorID;
	sector_t		*pSector;
	LONG			lSpeed;
	LONG			lDirection;
	LONG			lLightTag;
	LONG			lDoorID;
	DDoor			*pDoor;

	// Read in the sector ID.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the speed.
	lSpeed = NETWORK_ReadLong( pByteStream );

	// Read in the direction.
	lDirection = NETWORK_ReadByte( pByteStream );

	// Read in the delay.
	lLightTag = NETWORK_ReadShort( pByteStream );

	// Read in the door ID.
	lDoorID = NETWORK_ReadShort( pByteStream );

	// Make sure the sector ID is valid.
	if (( lSectorID >= 0 ) && ( lSectorID < numsectors ))
		pSector = &sectors[lSectorID];
	else
		return;

	// Since we still want to receive direction as a byte, but -1 can't be represented in byte
	// form, adjust the value into something that can be represented.
	lDirection = CLIENT_AdjustDoorDirection( lDirection );
	if ( lDirection == INT_MAX )
		return;

	// If door already has a thinker, we can't spawn a new door on it.
	if ( pSector->ceilingdata )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DoDoor: WARNING! Door's sector already has a ceiling mover attached to it!" );
#endif
		return;
	}

	// Create the new door.
	if ( pDoor = new DDoor( pSector, DDoor::doorRaise, lSpeed, 0, lLightTag, g_ConnectionState != CTS_ACTIVE ))
	{
		pDoor->SetID( lDoorID );
		pDoor->SetDirection( lDirection );
	}
}

//*****************************************************************************
//
static void client_DestroyDoor( BYTESTREAM_s *pByteStream )
{
	DDoor	*pDoor;
	LONG	lDoorID;

	// Read in the door ID.
	lDoorID = NETWORK_ReadShort( pByteStream );

	pDoor = P_GetDoorByID( lDoorID );
	if ( pDoor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DestroyDoor: Couldn't find door with ID: %d!\n", lDoorID );
#endif
		return;
	}

	pDoor->Destroy( );
}

//*****************************************************************************
//
static void client_ChangeDoorDirection( BYTESTREAM_s *pByteStream )
{
	DDoor	*pDoor;
	LONG	lDoorID;
	LONG	lDirection;

	// Read in the door ID.
	lDoorID = NETWORK_ReadShort( pByteStream );

	// Read in the new direction the door should move in.
	lDirection = NETWORK_ReadByte( pByteStream );

	// Since we still want to receive direction as a byte, but -1 can't be represented in byte
	// form, adjust the value into something that can be represented.
	lDirection = CLIENT_AdjustDoorDirection( lDirection );
	if ( lDirection == INT_MAX )
		return;

	pDoor = P_GetDoorByID( lDoorID );
	if ( pDoor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_ChangeDoorDirection: Couldn't find door with ID: %d!\n", lDoorID );
#endif
		return;
	}

	pDoor->SetDirection( lDirection );

	// Don't play a sound if the door is now motionless!
	if ( lDirection != 0 )
		pDoor->DoorSound( lDirection == 1 );
}

//*****************************************************************************
//
static void client_DoFloor( BYTESTREAM_s *pByteStream )
{
	LONG			lType;
	LONG			lDirection;
	LONG			FloorDestDist;
	LONG			lSpeed;
	LONG			lSectorID;
	LONG			lFloorID;
	sector_t		*pSector;
	DFloor			*pFloor;

	// Read in the type of floor.
	lType = NETWORK_ReadByte( pByteStream );

	// Read in the sector ID.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the direction of the floor.
	lDirection = NETWORK_ReadByte( pByteStream );

	// Read in the speed of the floor.
	lSpeed = NETWORK_ReadLong( pByteStream );

	// Read in the floor's destination height.
	FloorDestDist = NETWORK_ReadLong( pByteStream );

	// Read in the floor's network ID.
	lFloorID = NETWORK_ReadShort( pByteStream );

	// Since we still want to receive direction as a byte, but -1 can't be represented in byte
	// form, adjust the value into something that can be represented.
	lDirection = CLIENT_AdjustFloorDirection( lDirection );
	if ( lDirection == INT_MAX )
		return;

	// Invalid sector.
	if (( lSectorID >= numsectors ) || ( lSectorID < 0 ))
		return;

	pSector = &sectors[lSectorID];

	// If the sector already has activity, don't override it.
	if ( pSector->floordata )
		return;

	pFloor = new DFloor( pSector );
	pFloor->SetType( (DFloor::EFloor)lType );
	pFloor->SetDirection( lDirection );
	pFloor->SetFloorDestDist( FloorDestDist );
	pFloor->SetSpeed( lSpeed );
	pFloor->SetID( lFloorID );
}

//*****************************************************************************
//
static void client_DestroyFloor( BYTESTREAM_s *pByteStream )
{
	DFloor		*pFloor;
	LONG		lFloorID;

	// Read in the floor ID.
	lFloorID = NETWORK_ReadShort( pByteStream );

	pFloor = P_GetFloorByID( lFloorID );
	if ( pFloor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_ChangeFloorType: Couldn't find ceiling with ID: %d!\n", lFloorID );
#endif
		return;
	}

	SN_StopSequence( pFloor->GetSector( ));
	pFloor->Destroy( );
}

//*****************************************************************************
//
static void client_ChangeFloorDirection( BYTESTREAM_s *pByteStream )
{
	DFloor		*pFloor;
	LONG		lFloorID;
	LONG		lDirection;

	// Read in the floor ID.
	lFloorID = NETWORK_ReadShort( pByteStream );

	// Read in the new floor direction.
	lDirection = NETWORK_ReadByte( pByteStream );

	// Since we still want to receive direction as a byte, but -1 can't be represented in byte
	// form, adjust the value into something that can be represented.
	lDirection = CLIENT_AdjustFloorDirection( lDirection );
	if ( lDirection == INT_MAX )
		return;

	pFloor = P_GetFloorByID( lFloorID );
	if ( pFloor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_ChangeFloorType: Couldn't find ceiling with ID: %d!\n", lFloorID );
#endif
		return;
	}

	pFloor->SetDirection( lDirection );
}

//*****************************************************************************
//
static void client_ChangeFloorType( BYTESTREAM_s *pByteStream )
{
	DFloor		*pFloor;
	LONG		lFloorID;
	LONG		lType;

	// Read in the floor ID.
	lFloorID = NETWORK_ReadShort( pByteStream );

	// Read in the new type of floor this is.
	lType = NETWORK_ReadByte( pByteStream );

	pFloor = P_GetFloorByID( lFloorID );
	if ( pFloor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_ChangeFloorType: Couldn't find ceiling with ID: %d!\n", lFloorID );
#endif
		return;
	}

	pFloor->SetType( (DFloor::EFloor)lType );
}

//*****************************************************************************
//
static void client_ChangeFloorDestDist( BYTESTREAM_s *pByteStream )
{
	DFloor		*pFloor;
	LONG		lFloorID;
	fixed_t		DestDist;

	// Read in the floor ID.
	lFloorID = NETWORK_ReadShort( pByteStream );

	// Read in the new floor destination distance.
	DestDist = NETWORK_ReadLong( pByteStream );

	pFloor = P_GetFloorByID( lFloorID );
	if ( pFloor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_ChangeFloorType: Couldn't find ceiling with ID: %d!\n", lFloorID );
#endif
		return;
	}

	pFloor->SetFloorDestDist( DestDist );
}

//*****************************************************************************
//
static void client_StartFloorSound( BYTESTREAM_s *pByteStream )
{
	DFloor		*pFloor;
	LONG		lFloorID;

	// Read in the floor ID.
	lFloorID = NETWORK_ReadShort( pByteStream );

	pFloor = P_GetFloorByID( lFloorID );
	if ( pFloor == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_StartFloorSound: Couldn't find ceiling with ID: %d!\n", lFloorID );
#endif
		return;
	}

	// Finally, start playing the floor's sound sequence.
	pFloor->StartFloorSound( );
}

//*****************************************************************************
//
static void client_DoCeiling( BYTESTREAM_s *pByteStream )
{
	LONG			lType;
	fixed_t			BottomHeight;
	fixed_t			TopHeight;
	LONG			lSpeed;
	LONG			lCrush;
	LONG			lSilent;
	LONG			lDirection;
	LONG			lSectorID;
	LONG			lCeilingID;
	sector_t		*pSector;
	DCeiling		*pCeiling;

	// Read in the type of ceiling this is.
	lType = NETWORK_ReadByte( pByteStream );

	// Read in the sector this ceiling is attached to.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the direction this ceiling is moving in.
	lDirection = NETWORK_ReadByte( pByteStream );

	// Read in the lowest distance the ceiling can travel before it stops.
	BottomHeight = NETWORK_ReadLong( pByteStream );

	// Read in the highest distance the ceiling can travel before it stops.
	TopHeight = NETWORK_ReadLong( pByteStream );

	// Read in the speed of the ceiling.
	lSpeed = NETWORK_ReadLong( pByteStream );

	// Does this ceiling damage those who get squashed by it?
	lCrush = NETWORK_ReadByte( pByteStream );

	// Does this ceiling make noise?
	lSilent = NETWORK_ReadByte( pByteStream );

	// Read in the network ID of the ceiling.
	lCeilingID = NETWORK_ReadShort( pByteStream );

	// Since we still want to receive direction as a byte, but -1 can't be represented in byte
	// form, adjust the value into something that can be represented.
	lDirection = CLIENT_AdjustCeilingDirection( lDirection );
	if ( lDirection == INT_MAX )
		return;

	// Also, adjust the value of "crush".
	if ( lCrush == 0 )
		lCrush = -1;

	// Invalid sector.
	if (( lSectorID >= numsectors ) || ( lSectorID < 0 ))
		return;

	pSector = &sectors[lSectorID];

	pCeiling = new DCeiling( pSector, lSpeed, 0, lSilent );
	pCeiling->SetBottomHeight( BottomHeight );
	pCeiling->SetTopHeight( TopHeight );
	pCeiling->SetCrush( lCrush );
	pCeiling->SetDirection( lDirection );
	pCeiling->SetID( lCeilingID );
}

//*****************************************************************************
//
static void client_DestroyCeiling( BYTESTREAM_s *pByteStream )
{
	DCeiling	*pCeiling;
	LONG		lCeilingID;

	// Read in the ceiling ID.
	lCeilingID = NETWORK_ReadShort( pByteStream );

	pCeiling = P_GetCeilingByID( lCeilingID );
	if ( pCeiling == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DestroyCeiling: Couldn't find ceiling with ID: %d!\n", lCeilingID );
#endif
		return;
	}

	SN_StopSequence( pCeiling->GetSector( ));
	pCeiling->Destroy( );
}

//*****************************************************************************
//
static void client_ChangeCeilingDirection( BYTESTREAM_s *pByteStream )
{
	DCeiling	*pCeiling;
	LONG		lCeilingID;
	LONG		lDirection;

	// Read in the ceiling ID.
	lCeilingID = NETWORK_ReadShort( pByteStream );

	// Read in the new ceiling direction.
	lDirection = NETWORK_ReadByte( pByteStream );

	// Since we still want to receive direction as a byte, but -1 can't be represented in byte
	// form, adjust the value into something that can be represented.
	lDirection = CLIENT_AdjustCeilingDirection( lDirection );
	if ( lDirection == INT_MAX )
		return;

	pCeiling = P_GetCeilingByID( lCeilingID );
	if ( pCeiling == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_ChangeCeilingDirection: Couldn't find ceiling with ID: %d!\n", lCeilingID );
#endif
		return;
	}

	// Finally, set the new ceiling direction.
	pCeiling->SetDirection( lDirection );
}

//*****************************************************************************
//
static void client_ChangeCeilingSpeed( BYTESTREAM_s *pByteStream )
{
	DCeiling	*pCeiling;
	LONG		lCeilingID;
	LONG		lSpeed;

	// Read in the ceiling ID.
	lCeilingID = NETWORK_ReadShort( pByteStream );

	// Read in the new ceiling speed.
	lSpeed = NETWORK_ReadLong( pByteStream );

	pCeiling = P_GetCeilingByID( lCeilingID );
	if ( pCeiling == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_ChangeCeilingSpeed: Couldn't find ceiling with ID: %d!\n", lCeilingID );
#endif
		return;
	}

	pCeiling->SetSpeed( lSpeed );
}

//*****************************************************************************
//
static void client_PlayCeilingSound( BYTESTREAM_s *pByteStream )
{
	DCeiling	*pCeiling;
	LONG		lCeilingID;

	// Read in the ceiling ID.
	lCeilingID = NETWORK_ReadShort( pByteStream );

	pCeiling = P_GetCeilingByID( lCeilingID );
	if ( pCeiling == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_PlayCeilingSound: Couldn't find ceiling with ID: %d!\n", lCeilingID );
#endif
		return;
	}

	pCeiling->PlayCeilingSound( );
}

//*****************************************************************************
//
static void client_DoPlat( BYTESTREAM_s *pByteStream )
{
	LONG			lType;
	LONG			lStatus;
	fixed_t			High;
	fixed_t			Low;
	LONG			lSpeed;
	LONG			lSectorID;
	LONG			lPlatID;
	sector_t		*pSector;
	DPlat			*pPlat;

	// Read in the type of plat.
	lType = NETWORK_ReadByte( pByteStream );

	// Read in the sector ID.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the plat status (moving up, down, etc.).
	lStatus = NETWORK_ReadByte( pByteStream );

	// Read in the high range of the plat.
	High = NETWORK_ReadLong( pByteStream );

	// Read in the low range of the plat.
	Low = NETWORK_ReadLong( pByteStream );

	// Read in the speed.
	lSpeed = NETWORK_ReadLong( pByteStream );

	// Read in the plat ID.
	lPlatID = NETWORK_ReadShort( pByteStream );

	// Invalid sector.
	if (( lSectorID >= numsectors ) || ( lSectorID < 0 ))
		return;

	pSector = &sectors[lSectorID];

	// Create the plat, and set all its attributes that were read in.
	pPlat = new DPlat( pSector );
	pPlat->SetType( (DPlat::EPlatType)lType );
	pPlat->SetStatus( lStatus );
	pPlat->SetHigh( High );
	pPlat->SetLow( Low );
	pPlat->SetSpeed( lSpeed );
	pPlat->SetID( lPlatID );

	// Now, set other properties that don't really matter.
	pPlat->SetCrush( -1 );
	pPlat->SetTag( 0 );

	// Just set the delay to 0. The server will tell us when it should move again.
	pPlat->SetDelay( 0 );
}

//*****************************************************************************
//
static void client_DestroyPlat( BYTESTREAM_s *pByteStream )
{
	DPlat	*pPlat;
	LONG	lPlatID;

	// Read in the plat ID.
	lPlatID = NETWORK_ReadShort( pByteStream );

	pPlat = P_GetPlatByID( lPlatID );
	if ( pPlat == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DestroyPlat: Couldn't find plat with ID: %d!\n", lPlatID );
#endif
		return;
	}

	pPlat->Destroy( );
}

//*****************************************************************************
//
static void client_ChangePlatStatus( BYTESTREAM_s *pByteStream )
{
	DPlat	*pPlat;
	LONG	lPlatID;
	LONG	lStatus;

	// Read in the plat ID.
	lPlatID = NETWORK_ReadShort( pByteStream );

	// Read in the direction (aka status).
	lStatus = NETWORK_ReadByte( pByteStream );

	pPlat = P_GetPlatByID( lPlatID );
	if ( pPlat == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_ChangePlatStatus: Couldn't find plat with ID: %d!\n", lPlatID );
#endif
		return;
	}

	pPlat->SetStatus( lStatus );
}

//*****************************************************************************
//
static void client_PlayPlatSound( BYTESTREAM_s *pByteStream )
{
	DPlat	*pPlat;
	LONG	lPlatID;
	LONG	lSoundType;

	// Read in the plat ID.
	lPlatID = NETWORK_ReadShort( pByteStream );

	// Read in the type of sound to be played.
	lSoundType = NETWORK_ReadByte( pByteStream );

	pPlat = P_GetPlatByID( lPlatID );
	if ( pPlat == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_PlayPlatSound: Couldn't find plat with ID: %d!\n", lPlatID );
#endif
		return;
	}

	switch ( lSoundType )
	{
	case 0:

		SN_StopSequence( pPlat->GetSector( ));
		break;
	case 1:

		pPlat->PlayPlatSound( "Platform" );
		break;
	case 2:

		SN_StartSequence( pPlat->GetSector( ), "Silence", 0 );
		break;
	case 3:

		pPlat->PlayPlatSound( "Floor" );
		break;
	}
}

//*****************************************************************************
//
static void client_DoElevator( BYTESTREAM_s *pByteStream )
{
	LONG			lType;
	ULONG			lSectorID;
	LONG			lSpeed;
	LONG			lDirection;
	LONG			lFloorDestDist;
	LONG			lCeilingDestDist;
	LONG			lElevatorID;
	sector_t		*pSector;
	DElevator		*pElevator;

	// Read in the type of elevator.
	lType = NETWORK_ReadByte( pByteStream );

	// Read in the sector ID.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the speed.
	lSpeed = NETWORK_ReadLong( pByteStream );

	// Read in the direction.
	lDirection = NETWORK_ReadByte( pByteStream );

	// Read in the floor's destination distance.
	lFloorDestDist = NETWORK_ReadLong( pByteStream );

	// Read in the ceiling's destination distance.
	lCeilingDestDist = NETWORK_ReadLong( pByteStream );

	// Read in the elevator ID.
	lElevatorID = NETWORK_ReadShort( pByteStream );

	// Invalid sector.
	if (( lSectorID >= numsectors ) || ( lSectorID < 0 ))
		return;

	// Since we still want to receive direction as a byte, but -1 can't be represented in byte
	// form, adjust the value into something that can be represented.
	lDirection = CLIENT_AdjustElevatorDirection( lDirection );
	if ( lDirection == INT_MAX )
		return;

	pSector = &sectors[lSectorID];

	// Create the elevator, and set all its attributes that were read in.
	pElevator = new DElevator( pSector );
	pElevator->SetType( (DElevator::EElevator)lType );
	pElevator->SetSpeed( lSpeed );
	pElevator->SetDirection( lDirection );
	pElevator->SetFloorDestDist( lFloorDestDist );
	pElevator->SetCeilingDestDist( lCeilingDestDist );
	pElevator->SetID( lElevatorID );
}

//*****************************************************************************
//
static void client_DestroyElevator( BYTESTREAM_s *pByteStream )
{
	LONG		lElevatorID;
	DElevator	*pElevator;

	// Read in the elevator ID.
	lElevatorID = NETWORK_ReadShort( pByteStream );

	pElevator = P_GetElevatorByID( lElevatorID );
	if ( pElevator == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DestroyElevator: Couldn't find elevator with ID: %d!\n", lElevatorID );
#endif
		return;
	}

	pElevator->GetSector( )->floordata = NULL;
	pElevator->GetSector( )->ceilingdata = NULL;
	stopinterpolation( INTERP_SectorFloor, pElevator->GetSector( ));
	stopinterpolation( INTERP_SectorCeiling, pElevator->GetSector( ));

	// Finally, destroy the elevator.
	pElevator->Destroy( );
}

//*****************************************************************************
//
static void client_StartElevatorSound( BYTESTREAM_s *pByteStream )
{
	LONG		lElevatorID;
	DElevator	*pElevator;

	// Read in the elevator ID.
	lElevatorID = NETWORK_ReadShort( pByteStream );

	pElevator = P_GetElevatorByID( lElevatorID );
	if ( pElevator == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_StartElevatorSound: Couldn't find elevator with ID: %d!\n", lElevatorID );
#endif
		return;
	}

	// Finally, start the elevator sound.
	pElevator->StartFloorSound( );
}

//*****************************************************************************
//
static void client_DoPillar( BYTESTREAM_s *pByteStream )
{
	LONG			lType;
	ULONG			lSectorID;
	LONG			lFloorSpeed;
	LONG			lCeilingSpeed;
	LONG			lFloorTarget;
	LONG			lCeilingTarget;
	LONG			lPillarID;
	sector_t		*pSector;
	DPillar			*pPillar;

	// Read in the type of pillar.
	lType = NETWORK_ReadByte( pByteStream );

	// Read in the sector ID.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the speeds.
	lFloorSpeed = NETWORK_ReadLong( pByteStream );
	lCeilingSpeed = NETWORK_ReadLong( pByteStream );

	// Read in the targets.
	lFloorTarget = NETWORK_ReadLong( pByteStream );
	lCeilingTarget = NETWORK_ReadLong( pByteStream );

	// Read in the pillar ID.
	lPillarID = NETWORK_ReadShort( pByteStream );

	// Invalid sector.
	if (( lSectorID >= numsectors ) || ( lSectorID < 0 ))
		return;

	pSector = &sectors[lSectorID];

	// Create the pillar, and set all its attributes that were read in.
	pPillar = new DPillar( pSector );
	pPillar->SetType( (DPillar::EPillar)lType );
	pPillar->SetFloorSpeed( lFloorSpeed );
	pPillar->SetCeilingSpeed( lCeilingSpeed );
	pPillar->SetFloorTarget( lFloorTarget );
	pPillar->SetCeilingTarget( lCeilingTarget );
	pPillar->SetID( lPillarID );

	// Begin playing the sound sequence for the pillar.
	if ( pSector->seqType >= 0 )
		SN_StartSequence( pSector, pSector->seqType, SEQ_PLATFORM, 0 );
	else
		SN_StartSequence( pSector, "Floor", 0 );
}

//*****************************************************************************
//
static void client_DestroyPillar( BYTESTREAM_s *pByteStream )
{
	LONG		lPillarID;
	DPillar		*pPillar;

	// Read in the elevator ID.
	lPillarID = NETWORK_ReadShort( pByteStream );

	pPillar = P_GetPillarByID( lPillarID );
	if ( pPillar == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DestroyPillar: Couldn't find pillar with ID: %d!\n", lPillarID );
#endif
		return;
	}

	// Finally, destroy the pillar.
	pPillar->Destroy( );
}

//*****************************************************************************
//
static void client_DoWaggle( BYTESTREAM_s *pByteStream )
{
	bool			bCeiling;
	ULONG			lSectorID;
	LONG			lOriginalDistance;
	LONG			lAccumulator;
	LONG			lAccelerationDelta;
	LONG			lTargetScale;
	LONG			lScale;
	LONG			lScaleDelta;
	LONG			lTicker;
	LONG			lState;
	LONG			lWaggleID;
	sector_t		*pSector;
	DWaggleBase		*pWaggle;

	// Read in whether or not this is a ceiling waggle.
	bCeiling = !!NETWORK_ReadByte( pByteStream );

	// Read in the sector ID.
	lSectorID = NETWORK_ReadShort( pByteStream );

	// Read in the waggle's attributes.
	lOriginalDistance = NETWORK_ReadLong( pByteStream );
	lAccumulator = NETWORK_ReadLong( pByteStream );
	lAccelerationDelta = NETWORK_ReadLong( pByteStream );
	lTargetScale = NETWORK_ReadLong( pByteStream );
	lScale = NETWORK_ReadLong( pByteStream );
	lScaleDelta = NETWORK_ReadLong( pByteStream );
	lTicker = NETWORK_ReadLong( pByteStream );

	// Read in the state the waggle is in.
	lState = NETWORK_ReadByte( pByteStream );

	// Read in the waggle ID.
	lWaggleID = NETWORK_ReadShort( pByteStream );

	// Invalid sector.
	if (( lSectorID >= numsectors ) || ( lSectorID < 0 ))
		return;

	pSector = &sectors[lSectorID];

	// Create the waggle, and set all its attributes that were read in.
	if ( bCeiling )
		pWaggle = static_cast<DWaggleBase *>( new DCeilingWaggle( pSector ));
	else
		pWaggle = static_cast<DWaggleBase *>( new DFloorWaggle( pSector ));
	pWaggle->SetOriginalDistance( lOriginalDistance );
	pWaggle->SetAccumulator( lAccumulator );
	pWaggle->SetAccelerationDelta( lAccelerationDelta );
	pWaggle->SetTargetScale( lTargetScale );
	pWaggle->SetScale( lScale );
	pWaggle->SetScaleDelta( lScaleDelta );
	pWaggle->SetTicker( lTicker );
	pWaggle->SetState( lState );
	pWaggle->SetID( lWaggleID );
}

//*****************************************************************************
//
static void client_DestroyWaggle( BYTESTREAM_s *pByteStream )
{
	LONG			lWaggleID;
	DWaggleBase		*pWaggle;

	// Read in the waggle ID.
	lWaggleID = NETWORK_ReadShort( pByteStream );

	pWaggle = P_GetWaggleByID( lWaggleID );
	if ( pWaggle == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DestroyWaggle: Couldn't find waggle with ID: %d!\n", lWaggleID );
#endif
		return;
	}

	// Finally, destroy the waggle.
	pWaggle->Destroy( );
}

//*****************************************************************************
//
static void client_UpdateWaggle( BYTESTREAM_s *pByteStream )
{
	LONG			lWaggleID;
	LONG			lAccumulator;
	DWaggleBase		*pWaggle;

	// Read in the waggle ID.
	lWaggleID = NETWORK_ReadShort( pByteStream );

	// Read in the waggle's accumulator.
	lAccumulator = NETWORK_ReadLong( pByteStream );

	pWaggle = P_GetWaggleByID( lWaggleID );
	if ( pWaggle == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DestroyWaggle: Couldn't find waggle with ID: %d!\n", lWaggleID );
#endif
		return;
	}

	// Finally, update the waggle's accumulator.
	pWaggle->SetAccumulator( lAccumulator );
}

//*****************************************************************************
//
static void client_DoRotatePoly( BYTESTREAM_s *pByteStream )
{
	LONG			lSpeed;
	LONG			lPolyNum;
	polyobj_t		*pPoly;
	DRotatePoly		*pRotatePoly;

	// Read in the speed.
	lSpeed = NETWORK_ReadLong( pByteStream );

	// Read in the polyobject ID.
	lPolyNum = NETWORK_ReadShort( pByteStream );

	// Make sure the polyobj exists before we try to work with it.
	pPoly = GetPolyobj( lPolyNum );
	if ( pPoly == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DoRotatePoly: Invalid polyobj number: %d\n", lPolyNum );
#endif
		return;
	}

	// Create the polyobject.
	pRotatePoly = new DRotatePoly( lPolyNum );
	pRotatePoly->SetSpeed( lSpeed );

	// Attach the new polyobject to this ID.
	pPoly->specialdata = pRotatePoly;

	// Also, start the sound sequence associated with this polyobject.
	SN_StartSequence( pPoly, pPoly->seqType, SEQ_DOOR, 0 );
}

//*****************************************************************************
//
static void client_DestroyRotatePoly( BYTESTREAM_s *pByteStream )
{
	LONG							lID;
	DRotatePoly						*pPoly;
	DRotatePoly						*pTempPoly;
	TThinkerIterator<DRotatePoly>	Iterator;

	// Read in the DRotatePoly ID.
	lID = NETWORK_ReadShort( pByteStream );

	// Try to find the object from the ID. If it exists, destroy it.
	pPoly = NULL;
	while ( pTempPoly = Iterator.Next( ))
	{
		if ( pTempPoly->GetPolyObj( ) == lID )
		{
			pPoly = pTempPoly;
			break;
		}
	}

	if ( pPoly )
		pPoly->Destroy( );
}

//*****************************************************************************
//
static void client_DoMovePoly( BYTESTREAM_s *pByteStream )
{
	LONG			lXSpeed;
	LONG			lYSpeed;
	LONG			lPolyNum;
	polyobj_t		*pPoly;
	DMovePoly		*pMovePoly;

	// Read in the speed.
	lXSpeed = NETWORK_ReadLong( pByteStream );
	lYSpeed = NETWORK_ReadLong( pByteStream );

	// Read in the polyobject ID.
	lPolyNum = NETWORK_ReadShort( pByteStream );

	// Make sure the polyobj exists before we try to work with it.
	pPoly = GetPolyobj( lPolyNum );
	if ( pPoly == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DoRotatePoly: Invalid polyobj number: %d\n", lPolyNum );
#endif
		return;
	}

	// Create the polyobject.
	pMovePoly = new DMovePoly( lPolyNum );
	pMovePoly->SetXSpeed( lXSpeed );
	pMovePoly->SetYSpeed( lYSpeed );

	// Attach the new polyobject to this ID.
	pPoly->specialdata = pMovePoly;

	// Also, start the sound sequence associated with this polyobject.
	SN_StartSequence( pPoly, pPoly->seqType, SEQ_DOOR, 0 );
}

//*****************************************************************************
//
static void client_DestroyMovePoly( BYTESTREAM_s *pByteStream )
{
	LONG							lID;
	DMovePoly						*pPoly;
	DMovePoly						*pTempPoly;
	TThinkerIterator<DMovePoly>		Iterator;

	// Read in the DMovePoly ID.
	lID = NETWORK_ReadShort( pByteStream );

	// Try to find the object from the ID. If it exists, destroy it.
	pPoly = NULL;
	while ( pTempPoly = Iterator.Next( ))
	{
		if ( pTempPoly->GetPolyObj( ) == lID )
		{
			pPoly = pTempPoly;
			break;
		}
	}

	if ( pPoly )
		pPoly->Destroy( );
}

//*****************************************************************************
//
static void client_DoPolyDoor( BYTESTREAM_s *pByteStream )
{
	LONG			lType;
	LONG			lXSpeed;
	LONG			lYSpeed;
	LONG			lSpeed;
	LONG			lPolyNum;
	polyobj_t		*pPoly;
	DPolyDoor		*pPolyDoor;

	// Read in the type of poly door (swing or slide).
	lType = NETWORK_ReadByte( pByteStream );

	// Read in the speed.
	lXSpeed = NETWORK_ReadLong( pByteStream );
	lYSpeed = NETWORK_ReadLong( pByteStream );
	lSpeed = NETWORK_ReadLong( pByteStream );

	// Read in the polyobject ID.
	lPolyNum = NETWORK_ReadShort( pByteStream );

	// Make sure the polyobj exists before we try to work with it.
	pPoly = GetPolyobj( lPolyNum );
	if ( pPoly == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DoPolyDoor: Invalid polyobj number: %d\n", lPolyNum );
#endif
		return;
	}

	// Create the polyobject.
	pPolyDoor = new DPolyDoor( lPolyNum, (podoortype_t)lType );
	pPolyDoor->SetXSpeed( lXSpeed );
	pPolyDoor->SetYSpeed( lYSpeed );
	pPolyDoor->SetSpeed( lSpeed );

	// Attach the new polyobject to this ID.
	pPoly->specialdata = pPolyDoor;
}

//*****************************************************************************
//
static void client_DestroyPolyDoor( BYTESTREAM_s *pByteStream )
{
	LONG							lID;
	DPolyDoor						*pPoly;
	DPolyDoor						*pTempPoly;
	TThinkerIterator<DPolyDoor>		Iterator;

	// Read in the DPolyDoor ID.
	lID = NETWORK_ReadShort( pByteStream );

	// Try to find the object from the ID. If it exists, destroy it.
	pPoly = NULL;
	while ( pTempPoly = Iterator.Next( ))
	{
		if ( pTempPoly->GetPolyObj( ) == lID )
		{
			pPoly = pTempPoly;
			break;
		}
	}

	if ( pPoly )
		pPoly->Destroy( );
}

//*****************************************************************************
//
static void client_SetPolyDoorSpeedPosition( BYTESTREAM_s *pByteStream )
{
	LONG			lPolyID;
	LONG			lXSpeed;
	LONG			lYSpeed;
	LONG			lX;
	LONG			lY;
	polyobj_t		*pPoly;
	LONG			lDeltaX;
	LONG			lDeltaY;

	// Read in the polyobject ID.
	lPolyID = NETWORK_ReadShort( pByteStream );

	// Read in the polyobject x/yspeed.
	lXSpeed = NETWORK_ReadLong( pByteStream );
	lYSpeed = NETWORK_ReadLong( pByteStream );

	// Read in the polyobject X/.
	lX = NETWORK_ReadLong( pByteStream );
	lY = NETWORK_ReadLong( pByteStream );

	pPoly = GetPolyobj( lPolyID );
	if ( pPoly == NULL )
		return;

	lDeltaX = lX - pPoly->startSpot[0];
	lDeltaY = lY - pPoly->startSpot[1];

	PO_MovePolyobj( lPolyID, lDeltaX, lDeltaY );
	
	if ( pPoly->specialdata == NULL )
		return;

	static_cast<DPolyDoor *>( pPoly->specialdata )->SetXSpeed( lXSpeed );
	static_cast<DPolyDoor *>( pPoly->specialdata )->SetYSpeed( lYSpeed );
}

//*****************************************************************************
//
static void client_PlayPolyobjSound( BYTESTREAM_s *pByteStream )
{
	LONG						lID;
	NETWORK_POLYOBJSOUND_e		Sound;
	polyobj_t	*pPoly;

	// Read in the polyobject ID.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the sound to be played.
	Sound = (NETWORK_POLYOBJSOUND_e)NETWORK_ReadByte( pByteStream );

	pPoly = GetPolyobj( lID );
	if ( pPoly == NULL )
		return;

	switch ( Sound )
	{
	case POLYSOUND_STOPSEQUENCE:

		SN_StopSequence( pPoly );
		break;
	case POLYSOUND_SEQ_DOOR:

		SN_StartSequence( pPoly, pPoly->seqType, SEQ_DOOR, 0 );
		break;
	}
}

//*****************************************************************************
//
static void client_SetPolyobjPosition( BYTESTREAM_s *pByteStream )
{
	LONG			lPolyNum;
	polyobj_t		*pPoly;
	LONG			lX;
	LONG			lY;
	LONG			lDeltaX;
	LONG			lDeltaY;

	// Read in the polyobject number.
	lPolyNum = NETWORK_ReadShort( pByteStream );

	// Read in the XY position of the polyobj.
	lX = NETWORK_ReadLong( pByteStream );
	lY = NETWORK_ReadLong( pByteStream );

	// Get the polyobject from the index given.
	pPoly = GetPolyobj( lPolyNum );
	if ( pPoly == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetPolyobjPosition: Invalid polyobj number: %d\n", lPolyNum );
#endif
		return;
	}

	lDeltaX = lX - pPoly->startSpot[0];
	lDeltaY = lY - pPoly->startSpot[1];

//	Printf( "DeltaX: %d\nDeltaY: %d\n", lDeltaX, lDeltaY );

	// Finally, set the polyobject action.
	PO_MovePolyobj( lPolyNum, lDeltaX, lDeltaY );
}

//*****************************************************************************
//
static void client_SetPolyobjRotation( BYTESTREAM_s *pByteStream )
{
	LONG			lPolyNum;
	polyobj_t		*pPoly;
	LONG			lAngle;
	LONG			lDeltaAngle;

	// Read in the polyobject number.
	lPolyNum = NETWORK_ReadShort( pByteStream );

	// Read in the angle of the polyobj.
	lAngle = NETWORK_ReadLong( pByteStream );

	// Make sure the polyobj exists before we try to work with it.
	pPoly = GetPolyobj( lPolyNum );
	if ( pPoly == NULL )
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_SetPolyobjRotation: Invalid polyobj number: %d\n", lPolyNum );
#endif
		return;
	}

	lDeltaAngle = lAngle - pPoly->angle;

	// Finally, set the polyobject action.
	PO_RotatePolyobj( lPolyNum, lDeltaAngle );
}

//*****************************************************************************
//
static void client_EarthQuake( BYTESTREAM_s *pByteStream )
{
	AActor	*pCenter;
	LONG	lID;
	LONG	lIntensity;
	LONG	lDuration;
	LONG	lTremorRadius;

	// Read in the center's network ID.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the intensity of the quake.
	lIntensity = NETWORK_ReadByte( pByteStream );

	// Read in the duration of the quake.
	lDuration = NETWORK_ReadByte( pByteStream );

	// Read in the tremor radius of the quake.
	lTremorRadius = NETWORK_ReadByte( pByteStream );

	// Find the actor that represents the center of the quake based on the network
	// ID sent. If we can't find the actor, then the quake has no center.
	pCenter = NETWORK_FindThingByNetID( lID );
	if ( pCenter == NULL )
		return;

	// Create the earthquake. Since this is client-side, damage is always 0.
	new DEarthquake( pCenter, lIntensity, lDuration, 0, lTremorRadius );
}

//*****************************************************************************
//
static void client_SetQueuePosition( BYTESTREAM_s *pByteStream )
{
	LONG	lPosition;

	// Read in our position in the join queue.
	lPosition = NETWORK_ReadByte( pByteStream );

	if ( lPosition == MAXPLAYERS )
		Printf( "Join queue full!\n" );
	else
		Printf( "Your position in line is: %d\n", lPosition + 1 );

	// Update the joinqueue module with our position in line.
	JOINQUEUE_SetClientPositionInLine( lPosition );
}

//*****************************************************************************
//
static void client_DoScroller( BYTESTREAM_s *pByteStream )
{
	DScroller::EScrollType	Type;
	fixed_t					dX;
	fixed_t					dY;
	LONG					lSector;

	// Read in the type of scroller.
	Type = (DScroller::EScrollType)NETWORK_ReadByte( pByteStream );

	// Read in the X speed.
	dX = NETWORK_ReadLong( pByteStream );

	// Read in the Y speed.
	dY = NETWORK_ReadLong( pByteStream );

	// Read in the sector being scrolled.
	lSector = NETWORK_ReadShort( pByteStream );

	// Check to make sure what we've read in is valid.
	if (( Type != DScroller::sc_floor ) && ( Type != DScroller::sc_ceiling ) &&
		( Type != DScroller::sc_carry ) && ( Type != DScroller::sc_carry_ceiling ))
	{
		Printf( "client_DoScroller: Unknown type: %d!\n", (LONG)Type );
		return;
	}

	if (( lSector < 0 ) || ( lSector >= numsectors ))
	{
#ifdef CLIENT_WARNING_MESSAGES
		Printf( "client_DoScroller: Invalid sector ID: %d!\n", (LONG)lSector );
#endif
		return;
	}

	// Finally, create the scroller.
	new DScroller( Type, dX, dY, -1, lSector, 0 );
}

//*****************************************************************************
//
void SetScroller (int tag, DScroller::EScrollType type, fixed_t dx, fixed_t dy);

static void client_SetScroller( BYTESTREAM_s *pByteStream )
{
	DScroller::EScrollType	Type;
	fixed_t					dX;
	fixed_t					dY;
	LONG					lTag;

	// Read in the type of scroller.
	Type = (DScroller::EScrollType)NETWORK_ReadByte( pByteStream );

	// Read in the X speed.
	dX = NETWORK_ReadLong( pByteStream );

	// Read in the Y speed.
	dY = NETWORK_ReadLong( pByteStream );

	// Read in the sector being scrolled.
	lTag = NETWORK_ReadShort( pByteStream );

	// Check to make sure what we've read in is valid.
	if (( Type != DScroller::sc_floor ) && ( Type != DScroller::sc_ceiling ) &&
		( Type != DScroller::sc_carry ) && ( Type != DScroller::sc_carry_ceiling ))
	{
		Printf( "client_SetScroller: Unknown type: %d!\n", (LONG)Type );
		return;
	}

	// Finally, create or update the scroller.
	SetScroller (lTag, Type, dX, dY );
}

//*****************************************************************************
//
static void client_GenericCheat( BYTESTREAM_s *pByteStream )
{
	ULONG	ulPlayer;
	ULONG	ulCheat;

	// Read in the player who's doing the cheat.
	ulPlayer = NETWORK_ReadByte( pByteStream );

	// Read in the cheat.
	ulCheat = NETWORK_ReadByte( pByteStream );

	if ( playeringame[ulPlayer] == false )
		return;

	// Finally, do the cheat.
	cht_DoCheat( &players[ulPlayer], ulCheat );
}

//*****************************************************************************
//
static void client_SetCameraToTexture( BYTESTREAM_s *pByteStream )
{
	LONG	lID;
	char	*pszTexture;
	LONG	lFOV;
	AActor	*pCamera;
	LONG	lPicNum;

	// Read in the ID of the camera.
	lID = NETWORK_ReadShort( pByteStream );

	// Read in the name of the texture.
	pszTexture = NETWORK_ReadString( pByteStream );

	// Read in the FOV of the camera.
	lFOV = NETWORK_ReadByte( pByteStream );

	// Find the actor that represents the camera. If we can't find the actor, then
	// break out.
	pCamera = NETWORK_FindThingByNetID( lID );
	if ( pCamera == NULL )
		return;

	lPicNum = TexMan.CheckForTexture( pszTexture, FTexture::TEX_Wall, FTextureManager::TEXMAN_Overridable );
	if ( lPicNum < 0 )
	{
		Printf( "client_SetCameraToTexture: %s is not a texture\n", pszTexture );
		return;
	}

	FCanvasTextureInfo::Add( pCamera, lPicNum, lFOV );
}

//*****************************************************************************
//	CONSOLE COMMANDS

CCMD( connect )
{
	char		*pszDemoName;
	UCVarValue	Val;

	// Servers can't connect to other servers!
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		return;

	// No IP specified.
	if ( argv.argc( ) <= 1 )
	{
		Printf( "Usage: connect <server IP>\n" );
		return;
	}

	// Potentially disconnect from the current server.
	CLIENT_QuitNetworkGame( );

	// Put the game in client mode.
	NETWORK_SetState( NETSTATE_CLIENT );

	// Make sure cheats are off.
	Val.Bool = false;
	sv_cheats.ForceSet( Val, CVAR_Bool );
	am_cheat = 0;

	// Make sure our visibility is normal.
	R_SetVisibility( 8.0f );

	// Create a server IP from the given string.
	NETWORK_StringToAddress( argv[1], &g_AddressServer );

	// If the user didn't specify a port, use the default port.
	if ( g_AddressServer.usPort == 0 )
		NETWORK_SetAddressPort( g_AddressServer, DEFAULT_SERVER_PORT );

	g_AddressLastConnected = g_AddressServer;

	// Put the game in the full console.
	gameaction = ga_fullconsole;

	// Send out a connection signal.
	CLIENT_SendConnectionSignal( );

	// Update the connection state.
	CLIENT_SetConnectionState( CTS_TRYCONNECT );

	// If we've elected to record a demo, begin that process now.
	pszDemoName = Args.CheckValue( "-record" );
	if (( gamestate == GS_STARTUP ) && ( pszDemoName ))
		CLIENTDEMO_BeginRecording( pszDemoName );
}

//*****************************************************************************
//
CCMD( disconnect )
{
	// Nothing to do if we're not in client mode!
	if ( NETWORK_GetState( ) != NETSTATE_CLIENT )
		return;

	// Send disconnect signal, and end game.
	CLIENT_QuitNetworkGame( );
}

//*****************************************************************************
//
#ifdef	_DEBUG
CCMD( timeout )
{
	ULONG	ulIdx;

	// Nothing to do if we're not in client mode!
	if ( NETWORK_GetState( ) != NETSTATE_CLIENT )
		return;

	// Clear out the existing players.
	for ( ulIdx = 0; ulIdx < MAXPLAYERS; ulIdx++ )
	{
		// Zero out all the player information.
		playeringame[ulIdx] = false;
		CLIENT_ResetPlayerData( &players[ulIdx] );
	}
/*
	// If we're connected in any way, send a disconnect signal.
	if ( g_ConnectionState != CTS_DISCONNECTED )
	{
		NETWORK_WriteByte( &g_LocalBuffer, CONNECT_QUIT );
		g_lBytesSent += g_LocalBuffer.cursize;
		if ( g_lBytesSent > g_lMaxBytesSent )
			g_lMaxBytesSent = g_lBytesSent;
		NETWORK_LaunchPacket( g_LocalBuffer, g_AddressServer );
		NETWORK_ClearBuffer( &g_LocalBuffer );
	}
*/
	// Clear out our copy of the server address.
	memset( &g_AddressServer, 0, sizeof( g_AddressServer ));
	CLIENT_SetConnectionState( CTS_DISCONNECTED );

	// Go back to the full console.
	gameaction = ga_fullconsole;
}
#endif
//*****************************************************************************
//
CCMD( reconnect )
{
	UCVarValue	Val;

	// If we're in the middle of a game, we first need to disconnect from the server.
	if ( g_ConnectionState != CTS_DISCONNECTED )
		CLIENT_QuitNetworkGame( );
	
	// Store the address of the server we were on.
	if ( g_AddressLastConnected.abIP[0] == 0 )
	{
		Printf( "Unknown IP for last server. Use \"connect <server ip>\".\n" );
		return;
	}

	// Put the game in client mode.
	NETWORK_SetState( NETSTATE_CLIENT );

	// Make sure cheats are off.
	Val.Bool = false;
	sv_cheats.ForceSet( Val, CVAR_Bool );
	am_cheat = 0;

	// Make sure our visibility is normal.
	R_SetVisibility( 8.0f );

	// Set the address of the server we're trying to connect to to the previously connected to server.
	g_AddressServer = g_AddressLastConnected;

	// Put the game in the full console.
	gameaction = ga_fullconsole;

	// Send out a connection signal.
	CLIENT_SendConnectionSignal( );

	// Update the connection state.
	CLIENT_SetConnectionState( CTS_TRYCONNECT );
}

//*****************************************************************************
//
CCMD( rcon )
{
	char		szString[1024];
	char		szAppend[256];

	if ( g_ConnectionState != CTS_ACTIVE )
		return;

	if ( argv.argc( ) > 1 )
	{
		LONG	lLast;
		ULONG	ulIdx;
		ULONG	ulIdx2;
		bool	bHasSpace;

		memset( szString, 0, 1024 );
		memset( szAppend, 0, 256 );
		
		lLast = argv.argc( );

		// Since we don't want "rcon" to be part of our string, start at 1.
		for ( ulIdx = 1; ulIdx < lLast; ulIdx++ )
		{
			memset( szAppend, 0, 256 );

			bHasSpace = false;
			for ( ulIdx2 = 0; ulIdx2 < strlen( argv[ulIdx] ); ulIdx2++ )
			{
				if ( argv[ulIdx][ulIdx2] == ' ' )
				{
					bHasSpace = true;
					break;
				}
			}

			if ( bHasSpace )
				strcat( szAppend, "\"" );
			strcat( szAppend, argv[ulIdx] );
			strcat( szString, szAppend );
			if ( bHasSpace )
				strcat( szString, "\"" );
			if (( ulIdx + 1 ) < lLast )
				strcat( szString, " " );
		}
	
		// Alright, enviorment is correct, the string has been built.
		// SEND IT!
		CLIENTCOMMANDS_RCONCommand( szString );
	}
	else
		Printf( "Usage: rcon <command>\n" );
}

//*****************************************************************************
//
CCMD( send_password )
{
	if ( argv.argc( ) <= 1 )
	{
		Printf( "Usage: send_password <password>\n" );
		return;
	}

	if ( g_ConnectionState == CTS_ACTIVE )
		CLIENTCOMMANDS_RequestRCON( argv[1] );
}

//*****************************************************************************
//	CONSOLE VARIABLES

CVAR( Bool, cl_predict_players, true, CVAR_ARCHIVE )
CVAR( Int, cl_maxcorpses, 32, CVAR_ARCHIVE )
CVAR( Float, cl_motdtime, 5.0, CVAR_ARCHIVE )
CVAR( Bool, cl_taunts, true, CVAR_ARCHIVE )
CVAR( Int, cl_showcommands, 0, CVAR_ARCHIVE )
CVAR( Int, cl_showspawnnames, 0, CVAR_ARCHIVE )
CVAR( Bool, cl_startasspectator, true, CVAR_ARCHIVE )
CVAR( Bool, cl_dontrestorefrags, false, CVAR_ARCHIVE )
CVAR( String, cl_password, "password", CVAR_ARCHIVE )
CVAR( String, cl_joinpassword, "password", CVAR_ARCHIVE )
CVAR( Bool, cl_hitscandecalhack, true, CVAR_ARCHIVE )

//*****************************************************************************
//	STATISTICS

ADD_STAT( nettraffic )
{
	FString	Out;

	Out.Format( "%3d bytes sent (%3d max last second), %3d (%3d) received", 
		g_lBytesSent,
		g_lMaxBytesSentLastSecond,
		g_lBytesReceived,
		g_lMaxBytesReceivedLastSecond );

	return ( Out );
}
/*
// [BC] TEMPORARY
ADD_STAT( momentum )
{
	FString	Out;

	Out.Format( "X: %3d     Y: %3d", players[consoleplayer].momx, players[consoleplayer].momy );

	return ( Out );
}
*/

CCMD( ingame )
{
	ULONG	ulIdx;

	Printf( "Consoleplayer: %d\n", consoleplayer + 1 );
	for ( ulIdx = 0; ulIdx < MAXPLAYERS; ulIdx++ )
	{
		if ( playeringame[ulIdx] )
			Printf( "Player %d\n", ulIdx + 1 );
	}
}

