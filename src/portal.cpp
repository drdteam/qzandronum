#include "portal.h"
#include "p_local.h"
#include "p_lnspec.h"
#include "r_bsp.h"
#include "r_segs.h"
#include "c_cvars.h"
#include "m_bbox.h"
#include "p_tags.h"
#include "farchive.h"
#include "v_text.h"
#include "a_sharedglobal.h"
#include "i_system.h"
#include "c_dispatch.h"

// simulation recurions maximum
CVAR(Int, sv_portal_recursions, 4, CVAR_ARCHIVE|CVAR_SERVERINFO)

FDisplacementTable Displacements;

TArray<FLinePortal> linePortals;


FArchive &operator<< (FArchive &arc, FLinePortal &port)
{
	arc << port.mOrigin
		<< port.mDestination
		<< port.mXDisplacement
		<< port.mYDisplacement
		<< port.mType
		<< port.mFlags
		<< port.mDefFlags
		<< port.mAlign;
	return arc;
}


static line_t *FindDestination(line_t *src, int tag)
{
	if (tag)
	{
		int lineno = -1;
		FLineIdIterator it(tag);

		while ((lineno = it.Next()) >= 0)
		{
			if (&lines[lineno] != src)
			{
				return &lines[lineno];
			}
		}
	}
	return NULL;
}

void P_SpawnLinePortal(line_t* line)
{
	// portal destination is special argument #0
	line_t* dst = NULL;

	if (line->args[2] >= PORTT_VISUAL && line->args[2] <= PORTT_LINKED)
	{
		dst = FindDestination(line, line->args[0]);

		line->portalindex = linePortals.Reserve(1);
		FLinePortal *port = &linePortals.Last();

		memset(port, 0, sizeof(FLinePortal));
		port->mOrigin = line;
		port->mDestination = dst;
		port->mType = BYTE(line->args[2]);	// range check is done above.

		if (port->mType == PORTT_LINKED)
		{
			// Linked portals have no z-offset ever.
			port->mAlign = PORG_ABSOLUTE;
		}
		else
		{
			port->mAlign = BYTE(line->args[3] >= PORG_ABSOLUTE && line->args[3] <= PORG_CEILING ? line->args[3] : PORG_ABSOLUTE);
			if (port->mType == PORTT_INTERACTIVE)
			{
				// Due to the way z is often handled, these pose a major issue for parts of the code that needs to transparently handle interactive portals.
				Printf(TEXTCOLOR_RED "Warning: z-offsetting not allowed for interactive portals. Changing line %d to teleport-portal!\n", int(line - lines));
				port->mType = PORTT_TELEPORT;
			}
		}
		if (port->mDestination != NULL)
		{
			port->mDefFlags = port->mType == PORTT_VISUAL ? PORTF_VISIBLE : port->mType == PORTT_TELEPORT ? PORTF_TYPETELEPORT : PORTF_TYPEINTERACTIVE;


		}
	}
	else if (line->args[2] == PORTT_LINKEDEE && line->args[0] == 0)
	{
		// EE-style portals require that the first line ID is identical and the first arg of the two linked linedefs are 0 and 1 respectively.

		int mytag = tagManager.GetFirstLineID(line);

		for (int i = 0; i < numlines; i++)
		{
			if (tagManager.GetFirstLineID(&lines[i]) == mytag && lines[i].args[0] == 1)
			{
				line->portalindex = linePortals.Reserve(1);
				FLinePortal *port = &linePortals.Last();

				memset(port, 0, sizeof(FLinePortal));
				port->mOrigin = line;
				port->mDestination = &lines[i];
				port->mType = PORTT_LINKED;
				port->mAlign = PORG_ABSOLUTE;
				port->mDefFlags = PORTF_TYPEINTERACTIVE;

				// we need to create the backlink here, too.
				lines[i].portalindex = linePortals.Reserve(1);
				port = &linePortals.Last();

				memset(port, 0, sizeof(FLinePortal));
				port->mOrigin = &lines[i];
				port->mDestination = line;
				port->mType = PORTT_LINKED;
				port->mAlign = PORG_ABSOLUTE;
				port->mDefFlags = PORTF_TYPEINTERACTIVE;

			}
		}
	}
	else
	{
		// undefined type
		return;
	}
}

void P_UpdatePortal(FLinePortal *port)
{
	if (port->mDestination == NULL)
	{
		// Portal has no destination: switch it off
		port->mFlags = 0;
	}
	else if (port->mDestination->getPortalDestination() != port->mOrigin)
	{
		//portal doesn't link back. This will be a simple teleporter portal.
		port->mFlags = port->mDefFlags & ~PORTF_INTERACTIVE;
		if (port->mType == PORTT_LINKED)
		{
			// this is illegal. Demote the type to TELEPORT
			port->mType = PORTT_TELEPORT;
			port->mDefFlags &= ~PORTF_INTERACTIVE;
		}
	}
	else
	{
		port->mFlags = port->mDefFlags;
		if (port->mType == PORTT_LINKED)
		{
			if (linePortals[port->mDestination->portalindex].mType != PORTT_LINKED)
			{
				port->mType = PORTT_INTERACTIVE;	// linked portals must be two-way.
			}
			else
			{
				port->mXDisplacement = port->mDestination->v2->x - port->mOrigin->v1->x;
				port->mYDisplacement = port->mDestination->v2->y - port->mOrigin->v1->y;
			}
		}
 	}
}

void P_FinalizePortals()
{
	for (unsigned i = 0; i < linePortals.Size(); i++)
	{
		FLinePortal * port = &linePortals[i];
		P_UpdatePortal(port);
	}
}

static bool ChangePortalLine(line_t *line, int destid)
{
	if (line->portalindex >= linePortals.Size()) return false;
	FLinePortal *port = &linePortals[line->portalindex];
	if (port->mType == PORTT_LINKED) return false;	// linked portals cannot be changed.
	if (destid == 0) port->mDestination = NULL;
	port->mDestination = FindDestination(line, destid);
	if (port->mDestination == NULL)
	{
		port->mFlags = 0;
	}
	else if (port->mType == PORTT_INTERACTIVE)
	{
		FLinePortal *portd = &linePortals[port->mDestination->portalindex];
		if (portd != NULL && portd->mType == PORTT_INTERACTIVE && portd->mDestination == line)
		{
			// this is a 2-way interactive portal
			port->mFlags = port->mDefFlags | PORTF_INTERACTIVE;
			portd->mFlags = portd->mDefFlags | PORTF_INTERACTIVE;
		}
		else
		{
			port->mFlags = port->mDefFlags;
			portd->mFlags = portd->mDefFlags;
		}
	}
	return true;
}


bool P_ChangePortal(line_t *ln, int thisid, int destid)
{
	int lineno;

	if (thisid == 0) return ChangePortalLine(ln, destid);
	FLineIdIterator it(thisid);
	bool res = false;
	while ((lineno = it.Next()) >= 0)
	{
		res |= ChangePortalLine(&lines[lineno], destid);
	}
	return res;
}

// [ZZ] lots of floats here to avoid overflowing a lot
bool P_IntersectLines(fixed_t o1x, fixed_t o1y, fixed_t p1x, fixed_t p1y,
				      fixed_t o2x, fixed_t o2y, fixed_t p2x, fixed_t p2y,
				      fixed_t& rx, fixed_t& ry)
{
	double xx = FIXED2DBL(o2x) - FIXED2DBL(o1x);
	double xy = FIXED2DBL(o2y) - FIXED2DBL(o1y);

	double d1x = FIXED2DBL(p1x) - FIXED2DBL(o1x);
	double d1y = FIXED2DBL(p1y) - FIXED2DBL(o1y);

	if (d1x > d1y)
	{
		d1y = d1y / d1x * 32767.0f;
		d1x = 32767.0;
	}
	else
	{
		d1x = d1x / d1y * 32767.0f;
		d1y = 32767.0;
	}

	double d2x = FIXED2DBL(p2x) - FIXED2DBL(o2x);
	double d2y = FIXED2DBL(p2y) - FIXED2DBL(o2y);

	double cross = d1x*d2y - d1y*d2x;
	if (fabs(cross) < 1e-8)
		return false;

	double t1 = (xx * d2y - xy * d2x)/cross;
	rx = o1x + FLOAT2FIXED(d1x * t1);
	ry = o1y + FLOAT2FIXED(d1y * t1);
	return true;
}

inline int P_PointOnLineSideExplicit (fixed_t x, fixed_t y, fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2)
{
	return DMulScale32 (y-y1, x2-x1, x1-x, y2-y1) > 0;
}

bool P_ClipLineToPortal(line_t* line, line_t* portal, fixed_t viewx, fixed_t viewy, bool partial, bool samebehind)
{
	// check if this line is between portal and the viewer. clip away if it is.
	bool behind1 = !!P_PointOnLineSidePrecise(line->v1->x, line->v1->y, portal);
	bool behind2 = !!P_PointOnLineSidePrecise(line->v2->x, line->v2->y, portal);

	// [ZZ] update 16.12.2014: if a vertex equals to one of portal's vertices, it's treated as being behind the portal.
	//                         this is required in order to clip away diagonal lines around the portal (example: 1-sided triangle shape with a mirror on it's side)
	if ((line->v1->x == portal->v1->x && line->v1->y == portal->v1->y) ||
		(line->v1->x == portal->v2->x && line->v1->y == portal->v2->y))
			behind1 = samebehind;
	if ((line->v2->x == portal->v1->x && line->v2->y == portal->v1->y) ||
		(line->v2->x == portal->v2->x && line->v2->y == portal->v2->y))
			behind2 = samebehind;

	if (behind1 && behind2)
	{
		// line is behind the portal plane. now check if it's in front of two view plane borders (i.e. if it will get in the way of rendering)
		fixed_t dummyx, dummyy;
		bool infront1 = P_IntersectLines(line->v1->x, line->v1->y, line->v2->x, line->v2->y, viewx, viewy, portal->v1->x, portal->v1->y, dummyx, dummyy);
		bool infront2 = P_IntersectLines(line->v1->x, line->v1->y, line->v2->x, line->v2->y, viewx, viewy, portal->v2->x, portal->v2->y, dummyx, dummyy);
		if (infront1 && infront2)
			return true;
	}

	return false;
}

void P_TranslatePortalXY(line_t* src, line_t* dst, fixed_t& x, fixed_t& y)
{
	if (!src || !dst)
		return;

	fixed_t nposx, nposy;	// offsets from line

	// Get the angle between the two linedefs, for rotating
	// orientation and velocity. Rotate 180 degrees, and flip
	// the position across the exit linedef, if reversed.
	angle_t angle =
			R_PointToAngle2(0, 0, dst->dx, dst->dy) -
			R_PointToAngle2(0, 0, src->dx, src->dy);

	angle += ANGLE_180;

	// Sine, cosine of angle adjustment
	fixed_t s = finesine[angle>>ANGLETOFINESHIFT];
	fixed_t c = finecosine[angle>>ANGLETOFINESHIFT];

	fixed_t tx, ty;

	nposx = x - src->v1->x;
	nposy = y - src->v1->y;

	// Rotate position along normal to match exit linedef
	tx = FixedMul(nposx, c) - FixedMul(nposy, s);
	ty = FixedMul(nposy, c) + FixedMul(nposx, s);

	tx += dst->v2->x;
	ty += dst->v2->y;

	x = tx;
	y = ty;
}

void P_TranslatePortalVXVY(line_t* src, line_t* dst, fixed_t& vx, fixed_t& vy)
{
	angle_t angle =
		R_PointToAngle2(0, 0, dst->dx, dst->dy) -
		R_PointToAngle2(0, 0, src->dx, src->dy);

	angle += ANGLE_180;

	// Sine, cosine of angle adjustment
	fixed_t s = finesine[angle>>ANGLETOFINESHIFT];
	fixed_t c = finecosine[angle>>ANGLETOFINESHIFT];

	fixed_t orig_velx = vx;
	fixed_t orig_vely = vy;
	vx = FixedMul(orig_velx, c) - FixedMul(orig_vely, s);
	vy = FixedMul(orig_vely, c) + FixedMul(orig_velx, s);
}

void P_TranslatePortalAngle(line_t* src, line_t* dst, angle_t& angle)
{
	if (!src || !dst)
		return;

	// Get the angle between the two linedefs, for rotating
	// orientation and velocity. Rotate 180 degrees, and flip
	// the position across the exit linedef, if reversed.
	angle_t xangle =
			R_PointToAngle2(0, 0, dst->dx, dst->dy) -
			R_PointToAngle2(0, 0, src->dx, src->dy);

	xangle += ANGLE_180;
	angle += xangle;
}

void P_TranslatePortalZ(line_t* src, line_t* dst, fixed_t& z)
{
	// args[2] = 0 - no adjustment
	// args[2] = 1 - adjust by floor difference
	// args[2] = 2 - adjust by ceiling difference

	switch (src->getPortalAlignment())
	{
	case PORG_FLOOR:
		z = z - src->frontsector->floorplane.ZatPoint(src->v1->x, src->v1->y) + dst->frontsector->floorplane.ZatPoint(dst->v2->x, dst->v2->y);
		return;

	case PORG_CEILING:
		z = z - src->frontsector->ceilingplane.ZatPoint(src->v1->x, src->v1->y) + dst->frontsector->ceilingplane.ZatPoint(dst->v2->x, dst->v2->y);
		return;

	default:
		return;
	}
}

// calculate shortest distance from a point (x,y) to a linedef
fixed_t P_PointLineDistance(line_t* line, fixed_t x, fixed_t y)
{
	angle_t angle = R_PointToAngle2(0, 0, line->dx, line->dy);
	angle += ANGLE_180;

	fixed_t dx = line->v1->x - x;
	fixed_t dy = line->v1->y - y;

	fixed_t s = finesine[angle>>ANGLETOFINESHIFT];
	fixed_t c = finecosine[angle>>ANGLETOFINESHIFT];

	fixed_t d2x = FixedMul(dx, c) - FixedMul(dy, s);

	return abs(d2x);
}

void P_NormalizeVXVY(fixed_t& vx, fixed_t& vy)
{
	double _vx = FIXED2DBL(vx);
	double _vy = FIXED2DBL(vy);
	double len = sqrt(_vx*_vx+_vy*_vy);
	vx = FLOAT2FIXED(_vx/len);
	vy = FLOAT2FIXED(_vy/len);
}

// portal tracer code
PortalTracer::PortalTracer(fixed_t startx, fixed_t starty, fixed_t endx, fixed_t endy)
{
	this->startx = startx;
	this->starty = starty;
	this->endx = endx;
	this->endy = endy;
	intx = endx;
	inty = endy;
	intxIn = intx;
	intyIn = inty;
	z = 0;
	angle = 0;
	depth = 0;
	frac = 0;
	in = NULL;
	out = NULL;
	vx = 0;
	vy = 0;
}

bool PortalTracer::TraceStep()
{
	if (depth > sv_portal_recursions)
		return false;

	this->in = NULL;
	this->out = NULL;
	this->vx = 0;
	this->vy = 0;

	int oDepth = depth;

	fixed_t dirx = endx-startx;
	fixed_t diry = endy-starty;
	P_NormalizeVXVY(dirx, diry);

	dirx = 0;
	diry = 0;

	FPathTraverse it(startx-dirx, starty-diry, endx+dirx, endy+diry, PT_ADDLINES | PT_COMPATIBLE);

	intercept_t *in;
	while ((in = it.Next()))
	{
		line_t* li;

		if (in->isaline)
		{
			li = in->d.line;

			if (li->isLinePortal())
			{
				if (P_PointOnLineSide(startx-dirx, starty-diry, li))
					continue; // we're at the back side of this line

				line_t* out = li->getPortalDestination();

				this->in = li;
				this->out = out;

				// we only know that we crossed it, but we also need to know WHERE we crossed it
				fixed_t vx = it.Trace().dx;
				fixed_t vy = it.Trace().dy;

				fixed_t x = it.Trace().x + FixedMul(vx, in->frac);
				fixed_t y = it.Trace().y + FixedMul(vy, in->frac);

				P_NormalizeVXVY(vx, vy);

				this->vx = vx;
				this->vy = vy;

				// teleport our trace

				if (!out->backsector)
				{
					intx = x + vx;
					inty = y + vy;
				}
				else
				{
					intx = x - vx;
					inty = y - vy;
				}

				//P_TranslateCoordinatesAndAngle(li, out, startx, starty, noangle);
				//P_TranslateCoordinatesAndAngle(li, out, endx, endy, angle);
				//if (hdeltaZ)
				//	P_TranslateZ(li, out, deltaZ);
				//P_TranslateCoordinatesAndAngle(li, out, vx, vy, noangle);

				P_TranslatePortalXY(li, out, startx, starty);
				P_TranslatePortalVXVY(li, out, this->vx, this->vy);
				intxIn = intx;
				intyIn = inty;
				P_TranslatePortalXY(li, out, intx, inty);
				P_TranslatePortalXY(li, out, endx, endy);
				P_TranslatePortalAngle(li, out, angle);
				P_TranslatePortalZ(li, out, z);
				frac += in->frac;
				depth++;
				break; // breaks to outer loop
			}

			if (!(li->flags & ML_TWOSIDED) || (li->flags & ML_BLOCKEVERYTHING))
				return false; // stop tracing, 2D blocking line
		}
	}

	//Printf("returning %d; vx = %.2f; vy = %.2f\n", (oDepth != depth), FIXED2DBL(this->vx), FIXED2DBL(this->vy));

	return (oDepth != depth); // if a portal has been found, return false
}



//============================================================================
//
// CollectSectors
//
// Collects all sectors that are connected to any sector belonging to a portal
// because they all will need the same displacement values
//
//============================================================================

static bool CollectSectors(int groupid, sector_t *origin)
{
	if (origin->PortalGroup != 0) return false;	// already processed
	origin->PortalGroup = groupid;

	TArray<sector_t *> list(16);
	list.Push(origin);

	for (unsigned i = 0; i < list.Size(); i++)
	{
		sector_t *sec = list[i];

		for (int j = 0; j < sec->linecount; j++)
		{
			line_t *line = sec->lines[j];
			sector_t *other = line->frontsector == sec ? line->backsector : line->frontsector;
			if (other != NULL && other != sec && other->PortalGroup != groupid)
			{
				other->PortalGroup = groupid;
				list.Push(other);
			}
		}
	}
	return true;
}


//============================================================================
//
// AddDisplacementForPortal
//
// Adds the displacement for one portal to the displacement array
// (one version for sector to sector plane, one for line to line portals)
//
//============================================================================

static void AddDisplacementForPortal(AStackPoint *portal)
{
	int thisgroup = portal->Mate->Sector->PortalGroup;
	int othergroup = portal->Sector->PortalGroup;
	if (thisgroup == othergroup)
	{
		Printf("Portal between sectors %d and %d has both sides in same group and will be disabled\n", portal->Sector->sectornum, portal->Mate->Sector->sectornum);
		portal->special1 = portal->Mate->special1 = SKYBOX_PORTAL;
		return;
	}
	if (thisgroup <= 0 || thisgroup >= Displacements.size || othergroup <= 0 || othergroup >= Displacements.size)
	{
		Printf("Portal between sectors %d and %d has invalid group and will be disabled\n", portal->Sector->sectornum, portal->Mate->Sector->sectornum);
		portal->special1 = portal->Mate->special1 = SKYBOX_PORTAL;
		return;
	}

	FDisplacement & disp = Displacements(thisgroup, othergroup);
	if (!disp.isSet)
	{
		disp.x = portal->scaleX;
		disp.y = portal->scaleY;
		disp.isSet = true;
	}
	else
	{
		if (disp.x != portal->scaleX || disp.y != portal->scaleY)
		{
			Printf("Portal between sectors %d and %d has displacement mismatch and will be disabled\n", portal->Sector->sectornum, portal->Mate->Sector->sectornum);
			portal->special1 = portal->Mate->special1 = SKYBOX_PORTAL;
			return;
		}
	}
}


static void AddDisplacementForPortal(FLinePortal *portal)
{
	int thisgroup = portal->mOrigin->frontsector->PortalGroup;
	int othergroup = portal->mDestination->frontsector->PortalGroup;
	if (thisgroup == othergroup)
	{
		Printf("Portal between lines %d and %d has both sides in same group\n", int(portal->mOrigin-lines), int(portal->mDestination-lines));
		portal->mType = linePortals[portal->mDestination->portalindex].mType = PORTT_TELEPORT;
		return;
	}
	if (thisgroup <= 0 || thisgroup >= Displacements.size || othergroup <= 0 || othergroup >= Displacements.size)
	{
		Printf("Portal between lines %d and %d has invalid group\n", int(portal->mOrigin - lines), int(portal->mDestination - lines));
		portal->mType = linePortals[portal->mDestination->portalindex].mType = PORTT_TELEPORT;
		return;
	}

	FDisplacement & disp = Displacements(thisgroup, othergroup);
	if (!disp.isSet)
	{
		disp.x = portal->mXDisplacement;
		disp.y = portal->mYDisplacement;
		disp.isSet = true;
	}
	else
	{
		if (disp.x != portal->mXDisplacement || disp.y != portal->mYDisplacement)
		{
			Printf("Portal between lines %d and %d has displacement mismatch\n", int(portal->mOrigin - lines), int(portal->mDestination - lines));
			portal->mType = linePortals[portal->mDestination->portalindex].mType = PORTT_TELEPORT;
			return;
		}
	}
}

//============================================================================
//
// ConnectGroups
//
// Do the indirect connections. This loop will run until it cannot find any new connections
//
//============================================================================

static bool ConnectGroups()
{
	// Now 
	BYTE indirect = 1;
	bool bogus = false;
	bool changed;
	do
	{
		changed = false;
		for (int x = 1; x < Displacements.size; x++)
		{
			for (int y = 1; y < Displacements.size; y++)
			{
				FDisplacement &dispxy = Displacements(x, y);
				if (dispxy.isSet)
				{
					for (int z = 1; z < Displacements.size; z++)
					{
						FDisplacement &dispyz = Displacements(y, z);
						if (dispyz.isSet)
						{
							FDisplacement &dispxz = Displacements(x, z);
							if (dispxz.isSet)
							{
								if (dispxy.x + dispyz.x != dispxz.x || dispxy.y + dispyz.y != dispxz.y)
								{
									bogus = true;
								}
							}
							else
							{
								dispxz.x = dispxy.x + dispyz.x;
								dispxz.y = dispxy.y + dispyz.y;
								dispxz.isSet = true;
								dispxz.indirect = indirect;
								changed = true;
							}
						}
					}
				}
			}
		}
		indirect++;
	} while (changed);
	return bogus;
}


//============================================================================
//
// P_CreateLinkedPortals
//
// Creates the data structures needed for linked portals
// Removes portals from sloped sectors (as they cannot work on them)
// Group all sectors connected to one side of the portal
// Caclculate displacements between all created groups.
//
// Portals with the same offset but different anchors will not be merged.
//
//============================================================================

void P_CreateLinkedPortals()
{
	TThinkerIterator<AStackPoint> it;
	AStackPoint *mo;
	TArray<AStackPoint *> orgs;
	int id = 0;
	bool bogus = false;

	while ((mo = it.Next()))
	{
		if (mo->special1 == SKYBOX_LINKEDPORTAL)
		{
			if (mo->Mate != NULL)
			{
				orgs.Push(mo);
				mo->reactiontime = ++id;
			}
			else
			{
				// this should never happen, but if it does, the portal needs to be removed
				mo->Destroy();
			}
		}
	}
	if (orgs.Size() == 0)
	{
		return;
	}
	for (int i = 0; i < numsectors; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			ASkyViewpoint *box = sectors[i].SkyBoxes[j];
			if (box != NULL && box->special1 == SKYBOX_LINKEDPORTAL)
			{
				secplane_t &plane = j == 0 ? sectors[i].floorplane : sectors[i].ceilingplane;
				if (plane.a || plane.b)
				{
					// The engine cannot deal with portals on a sloped plane.
					sectors[i].SkyBoxes[j] = NULL;
					Printf("Portal on %s of sector %d is sloped and will be disabled\n", j==0? "floor":"ceiling", i);
				}
			}
		}
	}

	// Group all sectors, starting at each portal origin.
	id = 1;
	for (unsigned i = 0; i < orgs.Size(); i++)
	{
		if (CollectSectors(id, orgs[i]->Sector)) id++;
		if (CollectSectors(id, orgs[i]->Mate->Sector)) id++;
	}
	for (unsigned i = 0; i < linePortals.Size(); i++)
	{
		if (linePortals[i].mType == PORTT_LINKED)
		{
			if (CollectSectors(id, linePortals[i].mOrigin->frontsector)) id++;
			if (CollectSectors(id, linePortals[i].mDestination->frontsector)) id++;
		}
	}

	Displacements.Create(id);
	// Check for leftover sectors that connect to a portal
	for (int i = 0; i<numsectors; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			ASkyViewpoint *box = sectors[i].SkyBoxes[j];
			if (box != NULL)
			{
				if (box->special1 == SKYBOX_LINKEDPORTAL && box->Sector->PortalGroup == 0)
				{
					CollectSectors(box->Sector->PortalGroup, box->Sector);
					box = box->Mate;
					if (box->special1 == SKYBOX_LINKEDPORTAL && box->Sector->PortalGroup == 0)
					{
						CollectSectors(box->Sector->PortalGroup, box->Sector);
					}
				}
			}
		}
	}
	for (unsigned i = 0; i < orgs.Size(); i++)
	{
		AddDisplacementForPortal(orgs[i]);
	}
	for (unsigned i = 0; i < linePortals.Size(); i++)
	{
		if (linePortals[i].mType == PORTT_LINKED)
		{
			AddDisplacementForPortal(&linePortals[i]);
		}
	}

	for (int x = 1; x < Displacements.size; x++)
	{
		for (int y = x + 1; y < Displacements.size; y++)
		{
			FDisplacement &dispxy = Displacements(x, y);
			FDisplacement &dispyx = Displacements(y, x);
			if (dispxy.isSet && dispyx.isSet &&
				(dispxy.x != -dispyx.x || dispxy.y != -dispyx.y))
			{
				Printf("Link offset mismatch between groups %d and %d\n", x, y);	// need to find some sectors to report.
				bogus = true;
			}
			// todo: Find sectors that have no group but belong to a portal.
		}
	}
	bogus |= ConnectGroups();
	if (bogus)
	{
		// todo: disable all portals whose offsets do not match the associated groups
	}

	// reject would just get in the way when checking sight through portals.
	if (rejectmatrix != NULL)
	{
		delete[] rejectmatrix;
		rejectmatrix = NULL;
	}
	// finally we must flag all planes which are obstructed by the sector's own ceiling or floor.
	for (int i = 0; i < numsectors; i++)
	{
		sectors[i].CheckPortalPlane(sector_t::floor);
		sectors[i].CheckPortalPlane(sector_t::ceiling);
	}
	//BuildBlockmap();
}

CCMD(dumplinktable)
{
	for (int x = 1; x < Displacements.size; x++)
	{
		for (int y = 1; y < Displacements.size; y++)
		{
			FDisplacement &disp = Displacements(x, y);
			Printf("%c%c(%6d, %6d)", TEXTCOLOR_ESCAPE, 'C' + disp.indirect, disp.x >> FRACBITS, disp.y >> FRACBITS);
		}
		Printf("\n");
	}
}



