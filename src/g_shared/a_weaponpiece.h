// [Dusk] Added header guard here to prevent multi-definition errors.
// sv_commands.h refers to this file, and a lot of files refer to it.
#ifndef __A_WEAPONPIECE_H__
#define __A_WEAPONPIECE_H__

// 
class PClassWeaponPiece : public PClassInventory
{
	DECLARE_CLASS(PClassWeaponPiece, PClassInventory)
protected:
public:
	virtual void ReplaceClassRef(PClass *oldclass, PClass *newclass);
};

class AWeaponPiece : public AInventory
{
	DECLARE_CLASS_WITH_META(AWeaponPiece, AInventory, PClassWeaponPiece)
	HAS_OBJECT_POINTERS
protected:
	bool PrivateShouldStay ();
public:
	
	void Serialize(FSerializer &arc);
	bool TryPickup (AActor *&toucher);
	bool TryPickupRestricted (AActor *&toucher);
	bool ShouldStay ();
	virtual const char *PickupMessage ();
	virtual void PlayPickupSound (AActor *toucher);

	int PieceValue;
	PClassActor *WeaponClass;
	TObjPtr<AWeapon> FullWeapon;
};

// an internal class to hold the information for player class independent weapon piece handling
// [BL] Needs to be available for SBarInfo to check weaponpieces
class AWeaponHolder : public AInventory
{
	DECLARE_CLASS(AWeaponHolder, AInventory)

public:
	int PieceMask;
	PClassActor * PieceWeapon;

	
	void Serialize(FSerializer &arc);
};

#endif // __A_WEAPONPIECE_H__