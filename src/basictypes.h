#ifndef __BASICTYPES_H
#define __BASICTYPES_H

#include <stdint.h>

typedef int8_t					SBYTE;
typedef uint8_t					BYTE;
typedef int16_t					SWORD;
typedef uint16_t				WORD;
typedef int32_t					SDWORD;
typedef uint32_t				uint32;
typedef int64_t					SQWORD;
typedef uint64_t				QWORD;

// [BC] New additions.
typedef	unsigned short			USHORT;
typedef	short					SHORT;
#ifdef __WINE__
typedef unsigned int ULONG;
typedef int LONG;
#else
typedef	unsigned long			ULONG;
typedef	long					LONG;
#endif
typedef unsigned int			UINT;
typedef	int						INT;

// windef.h, included by windows.h, has its own incompatible definition
// of DWORD as a long. In files that mix Doom and Windows code, you
// must define USE_WINDOWS_DWORD before including doomtype.h so that
// you are aware that those files have a different DWORD than the rest
// of the source.

#ifndef USE_WINDOWS_DWORD
typedef uint32					DWORD;
#endif
typedef uint32					BITFIELD;
typedef int						INTBOOL;

// a 64-bit constant
#ifdef __GNUC__
#define CONST64(v) (v##LL)
#define UCONST64(v) (v##ULL)
#else
#define CONST64(v) ((SQWORD)(v))
#define UCONST64(v) ((QWORD)(v))
#endif

#if !defined(GUID_DEFINED)
#define GUID_DEFINED
typedef struct _GUID
{
    DWORD	Data1;
    WORD	Data2;
    WORD	Data3;
    BYTE	Data4[8];
} GUID;
#endif

union QWORD_UNION
{
	QWORD AsOne;
	struct
	{
#ifdef __BIG_ENDIAN__
		unsigned int Hi, Lo;
#else
		unsigned int Lo, Hi;
#endif
	};
};

//
// fixed point, 32bit as 16.16.
//
#define FRACBITS						16
#define FRACUNIT						(1<<FRACBITS)

typedef SDWORD							fixed_t;
typedef DWORD							dsfixed_t;				// fixedpt used by span drawer

struct fixedvec2
{
	fixed_t x, y;

	fixedvec2 &operator +=(const fixedvec2 &other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}
};

struct fixedvec3
{
	fixed_t x, y, z;

	fixedvec3 &operator +=(const fixedvec3 &other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	fixedvec3 &operator +=(const fixedvec2 &other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}

	fixedvec3 &operator -=(const fixedvec2 &other)
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}

	fixedvec3 &operator -=(const fixedvec3 &other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	operator fixedvec2()
	{
		return { x, y };
	}

};

inline fixedvec2 operator +(const fixedvec2 &v1, const fixedvec2 &v2)
{
	return { v1.x + v2.x, v1.y + v2.y };
}

inline fixedvec2 operator -(const fixedvec2 &v1, const fixedvec2 &v2)
{
	return { v1.x - v2.x, v1.y - v2.y };
}

inline fixedvec3 operator +(const fixedvec3 &v1, const fixedvec3 &v2)
{
	return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

inline fixedvec3 operator +(const fixedvec3 &v1, const fixedvec2 &v2)
{
	return { v1.x + v2.x, v1.y + v2.y, v1.z };
}

inline fixedvec3 operator -(const fixedvec3 &v1, const fixedvec2 &v2)
{
	return{ v1.x - v2.x, v1.y - v2.y, v1.z };
}

inline fixedvec3 operator -(const fixedvec3 &v1, const fixedvec3 &v2)
{
	return{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}

#define FIXED_MAX						(signed)(0x7fffffff)
#define FIXED_MIN						(signed)(0x80000000)

#define DWORD_MIN						((uint32)0)
#define DWORD_MAX						((uint32)0xffffffff)


#ifdef __GNUC__
#define GCCPRINTF(stri,firstargi)		__attribute__((format(printf,stri,firstargi)))
#define GCCFORMAT(stri)					__attribute__((format(printf,stri,0)))
#define GCCNOWARN						__attribute__((unused))
#else
#define GCCPRINTF(a,b)
#define GCCFORMAT(a)
#define GCCNOWARN
#endif


#endif
