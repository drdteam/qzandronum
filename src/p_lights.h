#ifndef __P_LIGHTS__
#define __P_LIGHTS__

#include "dthinker.h"

// [BB] Moved here from p_lights.cpp
class DFireFlicker : public DLighting
{
	DECLARE_CLASS(DFireFlicker, DLighting)
public:
	DFireFlicker(sector_t *sector);
	DFireFlicker(sector_t *sector, int upper, int lower);
	void		Serialize(FSerializer &arc);
	void		Tick();

	// [BC] Create this object for this new client entering the game.
	void	UpdateToClient( ULONG ulClient );
protected:
	int 		m_Count;
	int 		m_MaxLight;
	int 		m_MinLight;
private:
	DFireFlicker();
};

class DFlicker : public DLighting
{
	DECLARE_CLASS(DFlicker, DLighting)
public:
	DFlicker(sector_t *sector, int upper, int lower);
	void		Serialize(FSerializer &arc);
	void		Tick();

	// [BC] Create this object for this new client entering the game.
	void	UpdateToClient( ULONG ulClient );
protected:
	int 		m_Count;
	int 		m_MaxLight;
	int 		m_MinLight;
private:
	DFlicker();
};

class DLightFlash : public DLighting
{
	DECLARE_CLASS(DLightFlash, DLighting)
public:
	DLightFlash(sector_t *sector);
	DLightFlash(sector_t *sector, int min, int max);
	void		Serialize(FSerializer &arc);
	void		Tick();

	// [BC] Create this object for this new client entering the game.
	void	UpdateToClient( ULONG ulClient );

	// [BC] Access function(s).
	void		SetCount( LONG lCount );
protected:
	int 		m_Count;
	int 		m_MaxLight;
	int 		m_MinLight;
	int 		m_MaxTime;
	int 		m_MinTime;
private:
	DLightFlash();
};

class DStrobe : public DLighting
{
	DECLARE_CLASS(DStrobe, DLighting)
public:
	DStrobe(sector_t *sector, int utics, int ltics, bool inSync);
	DStrobe(sector_t *sector, int upper, int lower, int utics, int ltics);
	void		Serialize(FSerializer &arc);
	void		Tick();

	// [BC] Create this object for this new client entering the game.
	void	UpdateToClient( ULONG ulClient );

	// [BC] Access function(s).
	void		SetCount( LONG lCount );
protected:
	int 		m_Count;
	int 		m_MinLight;
	int 		m_MaxLight;
	int 		m_DarkTime;
	int 		m_BrightTime;
private:
	DStrobe();
};

class DGlow : public DLighting
{
	DECLARE_CLASS(DGlow, DLighting)
public:
	DGlow(sector_t *sector);
	void		Serialize(FSerializer &arc);
	void		Tick();

	// [BC] Create this object for this new client entering the game.
	void		UpdateToClient( ULONG ulClient );
protected:
	int 		m_MinLight;
	int 		m_MaxLight;
	int 		m_Direction;
private:
	DGlow();
};

// [RH] Glow from Light_Glow and Light_Fade specials
class DGlow2 : public DLighting
{
	DECLARE_CLASS(DGlow2, DLighting)
public:
	DGlow2(sector_t *sector, int start, int end, int tics, bool oneshot);
	void		Serialize(FSerializer &arc);
	void		Tick();

	// [BC] Create this object for this new client entering the game.
	void		UpdateToClient( ULONG ulClient );

	// [BC] Access function(s).
	void		SetTics( LONG lTics );
protected:
	int			m_Start;
	int			m_End;
	int			m_MaxTics;
	int			m_Tics;
	bool		m_OneShot;
private:
	DGlow2();
};

// [RH] Phased light thinker
class DPhased : public DLighting
{
	DECLARE_CLASS(DPhased, DLighting)
public:
	DPhased(sector_t *sector);
	DPhased(sector_t *sector, int baselevel, int phase);
	void		Serialize(FSerializer &arc);
	void		Tick();

	// [BC] Create this object for this new client entering the game.
	void		UpdateToClient( ULONG ulClient );
protected:
	BYTE		m_BaseLevel;
	BYTE		m_Phase;
private:
	DPhased();
	DPhased(sector_t *sector, int baselevel);
	int PhaseHelper(sector_t *sector, int index, int light, sector_t *prev);
};

#endif
