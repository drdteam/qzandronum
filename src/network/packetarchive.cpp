//-----------------------------------------------------------------------------
//
// Zandronum Source
// Copyright (C) 2003 Brad Carney
// Copyright (C) 2016 Teemu Piippo
// Copyright (C) 2016 Zandronum Development Team
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
// Filename: packetarchive.cpp
//
// Description: Encapsulates the packet loss recovery routines
//
//-----------------------------------------------------------------------------

#include "../sv_main.h"
#include "packetarchive.h"

//*****************************************************************************
//
PacketArchive::PacketArchive() :
	_initialized ( false )
{
	Clear();
}

//*****************************************************************************
//
PacketArchive::~PacketArchive()
{
	Free();
}

//*****************************************************************************
//
void PacketArchive::Initialize( size_t maxPacketSize )
{
	if ( _initialized == false )
	{
		_packetData.Init( maxPacketSize * PACKET_BUFFER_SIZE, BUFFERTYPE_WRITE );
		Clear();
		_initialized = true;
	}
}

//*****************************************************************************
//
void PacketArchive::Free()
{
	if ( _initialized )
	{
		_packetData.Free();
		_initialized = false;
	}
}

//*****************************************************************************
//
unsigned int PacketArchive::StorePacket( const NETBUFFER_s& packet )
{
	if ( _initialized == false )
		return 0;

	_packetData.ulCurrentSize = _packetData.CalcSize();

	// If we've reached the end of our reliable packets buffer, start writing at the beginning.
	if (( _packetData.ulCurrentSize + packet.CalcSize() ) >= _packetData.ulMaxSize )
	{
		_packetData.ByteStream.pbStream = _packetData.pbData;
		_packetData.ulCurrentSize = 0;
	}

	// Save where the beginning is of each packet within the reliable packets
	// buffer.
	unsigned int i = _sequenceNumber % PACKET_BUFFER_SIZE;
	_records[i].position = _packetData.ulCurrentSize;
	_records[i].sequenceNumber = _sequenceNumber;

	// Write what we want to send out to our reliable packets buffer, so that it can be
	// retransmitted later if necessary. Also save the size.
	_records[i].size = packet.WriteTo( _packetData.ByteStream );

	return _sequenceNumber++;
}

//*****************************************************************************
//
void PacketArchive::Clear()
{
	if ( _initialized )
		_packetData.Clear();

	_sequenceNumber = 0;

	for ( size_t i = 0; i < countof( _records ); ++i )
		_records[i].position = _records[i].size = _records[i].sequenceNumber = 0;
}

//*****************************************************************************
//
bool PacketArchive::FindPacket( unsigned int packetNumber, const BYTE*& data, size_t& size ) const
{
	if ( _initialized == false )
		return false;

	// [BB] We know the internal index the packet should have.
	size_t index = packetNumber % PACKET_BUFFER_SIZE;
	if ( _records[index].sequenceNumber == packetNumber )
	{
		data = _packetData.pbData + _records[index].position;
		size = _records[index].size;
		return true;
	}
	// [BB] If we still have it, we should have found the packet above.
	// Nevertheless, to guarantee consistency with the old behaviort, look
	// through all packets.

	// Search through all PACKET_BUFFER_SIZE of the stored packets. We're looking for the packet that
	// that we want to send to the client by matching the sequences.
	for ( size_t i = 0; i < countof( _records ); ++i )
	{
		if ( _records[i].sequenceNumber == packetNumber )
		{
			data = _packetData.pbData + _records[i].position;
			size = _records[i].size;
			return true;
		}
	}

	return false;
}
