#ifndef __P_PUSHER__
#define __P_PUSHER__

#include "dthinker.h"

// [BB] Moved here from p_pusher.cpp
class DPusher : public DThinker
{
	DECLARE_CLASS (DPusher, DThinker)
	HAS_OBJECT_POINTERS
public:
	enum EPusher
	{
		p_push,
		p_pull,
		p_wind,
		p_current
	};

	DPusher ();
	DPusher (EPusher type, line_t *l, int magnitude, int angle, AActor *source, int affectee);
	void Serialize(FSerializer &arc);
	int CheckForSectorMatch (EPusher type, int tag);
	void ChangeValues (int magnitude, int angle)
	{
		// [BB] Save the original input angle value. This makes it easier to inform the clients about this pusher.
		m_Angle = angle;

		DAngle ang = angle * (360. / 256.);
		m_PushVec = ang.ToVector(magnitude);
		m_Magnitude = magnitude;
	}

	void Tick ();

	// [BB] Create this object for this new client entering the game.
	void UpdateToClient(ULONG ulClient);

protected:
	EPusher m_Type;
	TObjPtr<AActor> m_Source;// Point source if point pusher
	DVector2 m_PushVec;
	double m_Magnitude;		// Vector strength for point pusher
	double m_Radius;		// Effective radius for point pusher
	int m_Affectee;			// Number of affected sector

	// [BB]
	line_t *m_pLine;
	int m_Angle;

	friend bool PIT_PushThing (AActor *thing);
};

#endif
