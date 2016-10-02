#ifndef __P_SCROLL__
#define __P_SCROLL__

#include "dthinker.h"

// [BB] Moved here from p_scroll.cpp
class DScroller : public DThinker
{
	DECLARE_CLASS (DScroller, DThinker)
	HAS_OBJECT_POINTERS
public:
	
	DScroller (EScroll type, double dx, double dy, int control, int affectee, int accel, EScrollPos scrollpos = EScrollPos::scw_all);
	DScroller (double dx, double dy, const line_t *l, int control, int accel, EScrollPos scrollpos = EScrollPos::scw_all);
	void Destroy();

	void Serialize(FSerializer &arc);
	void Tick ();

	bool AffectsWall (int wallnum) const { return m_Type == EScroll::sc_side && m_Affectee == wallnum; }
	int GetWallNum () const { return m_Type == EScroll::sc_side ? m_Affectee : -1; }
	void SetRate (double dx, double dy) { m_dx = dx; m_dy = dy; }
	bool IsType (EScroll type) const { return type == m_Type; }
	int GetAffectee () const { return m_Affectee; }
	EScrollPos GetScrollParts() const { return m_Parts; }

	// [BC] Create this object for this new client entering the game.
	void UpdateToClient(ULONG ulClient);

protected:
	EScroll m_Type;		// Type of scroll effect
	double m_dx, m_dy;		// (dx,dy) scroll speeds
	int m_Affectee;			// Number of affected sidedef, sector, tag, or whatever
	int m_Control;			// Control sector (-1 if none) used to control scrolling
	double m_LastHeight;	// Last known height of control sector
	double m_vdx, m_vdy;	// Accumulated velocity if accelerative
	int m_Accel;			// Whether it's accelerative
	EScrollPos m_Parts;			// Which parts of a sidedef are being scrolled?
	TObjPtr<DInterpolation> m_Interpolations[3];

private:
	DScroller ()
	{
	}
};

#endif
