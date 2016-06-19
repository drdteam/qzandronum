#!/usr/bin/env python3
# coding: utf-8
#
#	Copyright 2016 Teemu Piippo
#	All rights reserved.
#
#	Redistribution and use in source and binary forms, with or without
#	modification, are permitted provided that the following conditions
#	are met:
#
#	1. Redistributions of source code must retain the above copyright
#	   notice, this list of conditions and the following disclaimer.
#	2. Redistributions in binary form must reproduce the above copyright
#	   notice, this list of conditions and the following disclaimer in the
#	   documentation and/or other materials provided with the distribution.
#	3. Neither the name of the copyright holder nor the names of its
#	   contributors may be used to endorse or promote products derived from
#	   this software without specific prior written permission.
#
#	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
#	TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
#	PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
#	OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#	EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO,
#	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#	PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#	LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING
#	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

from string import capwords

passbyvalue = {'int', 'unsigned int', 'short', 'unsigned short', 'long', 'unsigned long', 'bool', 'float', 'double',
			   'BYTE', 'SBYTE', 'WORD', 'SWORD', 'DWORD', 'SDWORD', 'QWORD', 'SQWORD',
			   'FName', 'fixed_t', 'angle_t', 'size_t'}

# ----------------------------------------------------------------------------------------------------------------------

class SpecParameter:
	'''
	Represents a parameter in a server command.
	'''
	def __init__(self, typename, name, specialization=None, attributes=None, isarray=False, condition=None):
		self.typename = typename
		self.name = name
		self.specialization = specialization
		self.attributes = attributes or set()
		self.isarray = isarray
		self.condition = condition and condition.strip()

	def writeread(self, writer, command, parametername):
		raise Exception('BUG: %s does not define writeread!' % type(self).__name__)

	def writesend(self, writer, command, parametername):
		raise Exception('BUG: %s does not define writesend!' % type(self).__name__)

	def writereadchecks(self, writer, command, parametername):
		pass

	@property
	def constreference(self):
		if self.isarray:
			# Pass arrays as const references
			return 'const TArray<%s> &' % self.cxxtypename
		elif self.cxxtypename.endswith('*') or self.cxxtypename in passbyvalue:
			# Pass pointers and known types by value
			return self.cxxtypename
		else:
			# Otherwise use a const reference of own type name
			return 'const %s &' % self.cxxtypename

# ----------------------------------------------------------------------------------------------------------------------

class ByteParameter(SpecParameter):
	def __init__(self, **args):
		super().__init__(**args)

		if self.specialization:
			raise Exception('Integer types may not be specialized')

	# Writes code to read in this parameter.
	def writeread(self, writer, command, parametername):
		readfunction = 'NETWORK_Read' + self.methodname()
		writer.writeline('command.{parametername} = {readfunction}( bytestream );'.format(**locals()))

	# Writes code to write this parameter to a NetCommand.
	def writesend(self, writer, command, parametername):
		sendmethod = 'command.add' + capwords(self.methodname())
		writer.writeline('{sendmethod}( this->{parametername} );'.format(**locals()))

	# Returns the C++ type name for this parameter
	@property
	def cxxtypename(self):
		return 'int'

	def methodname(self):
		return 'Byte'

# ----------------------------------------------------------------------------------------------------------------------

class SbyteParameter(ByteParameter):
	@property
	def cxxtypename(self):
		return 'SBYTE'

# ----------------------------------------------------------------------------------------------------------------------

class ShortParameter(ByteParameter):
	def methodname(self):
		return 'Short'

# ----------------------------------------------------------------------------------------------------------------------

class UshortParameter(ShortParameter):
	@property
	def cxxtypename(self):
		return 'unsigned int'

# ----------------------------------------------------------------------------------------------------------------------

class LongParameter(ByteParameter):
	def methodname(self):
		return 'Long'

# ----------------------------------------------------------------------------------------------------------------------

class UlongParameter(LongParameter):
	@property
	def cxxtypename(self):
		return 'unsigned int'

# ----------------------------------------------------------------------------------------------------------------------

class StringParameter(SpecParameter):
	def __init__(self, **args):
		super().__init__(**args)
		self.cxxtypename = 'FString'

	def writeread(self, writer, command, parametername):
		writer.writeline('command.{parametername} = NETWORK_ReadString( bytestream );'.format(**locals()))

	def writesend(self, writer, command, parametername):
		writer.writeline('command.addString( this->{parametername} );'.format(**locals()))

# ----------------------------------------------------------------------------------------------------------------------

class FloatParameter(SpecParameter):
	def __init__(self, **args):
		super().__init__(**args)
		self.cxxtypename = 'float'

	def writeread(self, writer, command, parametername):
		writer.writeline('command.{parametername} = NETWORK_ReadFloat( bytestream );'.format(**locals()))

	def writesend(self, writer, command, parametername):
		writer.writeline('command.addFloat( this->{parametername} );'.format(**locals()))

# ----------------------------------------------------------------------------------------------------------------------

class BoolParameter(SpecParameter):
	def __init__(self, **args):
		super().__init__(**args)
		self.cxxtypename = 'bool'

	def writeread(self, writer, command, parametername):
		# Bool needs a little bit different treatment
		writer.writeline('command.{parametername} = !!NETWORK_ReadByte( bytestream );'.format(**locals()))

	def writesend(self, writer, command, parametername):
		writer.writecontext('command.addByte( this->{parametername} );'.format(**locals()))

# ----------------------------------------------------------------------------------------------------------------------

class BitParameter(SpecParameter):
	def __init__(self, **args):
		super().__init__(**args)
		self.cxxtypename = 'bool'

	def writeread(self, writer, command, parametername):
		writer.writeline('command.{parametername} = NETWORK_ReadBit( bytestream );'.format(**locals()))

	def writesend(self, writer, command, parametername):
		writer.writecontext('command.addBit( this->{parametername} );'.format(**locals()))

# ----------------------------------------------------------------------------------------------------------------------

class VariableParameter(SpecParameter):
	def __init__(self, **args):
		super().__init__(**args)
		self.cxxtypename = 'int'

	def writeread(self, writer, command, parametername):
		writer.writeline('command.{parametername} = NETWORK_ReadVariable( bytestream );'.format(**locals()))

	def writesend(self, writer, command, parametername):
		writer.writecontext('command.addVariable( this->{parametername} );'.format(**locals()))

# ----------------------------------------------------------------------------------------------------------------------

class ShortbyteParameter(SpecParameter):
	def __init__(self, **args):
		super().__init__(**args)
		self.cxxtypename = 'int'

	def writeread(self, writer, command, parametername):
		writer.writeline('command.{parametername} = NETWORK_ReadShortByte( bytestream, {specialization} );'.format(specialization=self.specialization, **locals()))

	def writesend(self, writer, command, parametername):
		specialization = self.specialization
		writer.writecontext('command.addShortByte( this->{parametername}, {specialization} );'.format(**locals()))

# ----------------------------------------------------------------------------------------------------------------------

class ActorParameter(SpecParameter):
	def __init__(self, **args):
		super().__init__(**args)

	@property
	def cxxtypename(self):
		if self.specialization:
			return self.specialization + ' *'
		else:
			return 'AActor *'

	def writeread(self, writer, command, parametername):
		# To read in an actor we'll first read in a short integer to represent the network id.
		# Then, we resolve this to the actor pointer.
		# If the actor pointer is specialized, we try to downcast it. If downcasting is not possible, the
		#     specialized pointer becomes NULL instead.
		# If the pointer is NULL and we haven't explicitly allowed nulls, the command is invalid and Execute()
		#     will not be called.
		netid = next(writer.tempvar)
		self.readnetid = netid # Remember the netid variable name for checks

		# Write the code to read in the netid
		writer.declare('int', netid)
		writer.writeline('{netid} = NETWORK_ReadShort( bytestream );'.format(**locals()))

	def writereadchecks(self, writer, command, parametername):
		netid = self.readnetid
		writer.writecontext('''
			if ( CLIENT_ReadActorFromNetID( {netid}, RUNTIME_CLASS( {specialization} ), {allownull},
											reinterpret_cast<AActor *&>( command.{parametername} ),
											"{commandname}", "{parametername}" ) == false )
			{{
				return true;
			}}

			'''.format(commandname=command.name, specialization=self.specialization or 'AActor',
					   allownull=('nullallowed' in self.attributes) and 'true' or 'false', **locals()))

	def writesend(self, writer, command, parametername):
		writer.writeline('command.addShort( this->{parametername} ? this->{parametername}->lNetID : -1 );'.format(**locals()))

# ----------------------------------------------------------------------------------------------------------------------

class ClassParameter(SpecParameter):
	def __init__(self, **args):
		super().__init__(**args)
		self.cxxtypename = 'const PClass *'

	def writeread(self, writer, command, parametername):
		# To read in a class we'll first write code to read in the identification number, and then
		# use NETWORK_GetClassFromIdentification to turn it into the class parameter.
		netid = next(writer.tempvar)
		self.readnetid = netid
		writer.declare('int', netid)
		writer.writeline('{netid} = NETWORK_ReadShort( bytestream );'.format(**locals()))
		writer.writeline('command.{parametername} = NETWORK_GetClassFromIdentification( {netid} );'.format(**locals()))

		# If the class parameter is specialized, ensure that it is a descendant of the specified class.
		# If not, NULL the parameter
		if self.specialization:
			writer.writeline('')
			writer.writecontext('''
				if ( command.{parametername}->IsDescendantOf( RUNTIME_CLASS( {specialization} )) == false )
					command.{parametername} = NULL;

				'''.format(parametername=parametername, specialization=self.specialization))

	def writereadchecks(self, writer, command, parametername):
		netid = self.readnetid

		# If we don't allow the parameter to be NULL, write code to check whether it is valid.
		if 'nullallowed' not in self.attributes:
			writer.writeline('')
			writer.writecontext('''
				if ( command.{parametername} == NULL )
				{{
					CLIENT_PrintWarning( "{commandname}: unknown class ID for {parametername}: %d\\n", {netid} );
					return true;
				}}

				'''.format(commandname = command.name, **locals()))

	def writesend(self, writer, command, parametername):
		writer.writeline('command.addShort( this->{parametername} ? this->{parametername}->getActorNetworkIndex() : -1 );'.format(**locals()))

# ----------------------------------------------------------------------------------------------------------------------

class PlayerParameter(SpecParameter):
	def __init__(self, **args):
		super().__init__(**args)
		self.cxxtypename = 'player_t *'

	def writeread(self, writer, command, parametername):
		# Player self. We store both the index and a pointer to the player structure.
		writer.writeline('command.{parametername} = &players[NETWORK_ReadByte( bytestream )];'.format(**locals()))

	def writereadchecks(self, writer, command, parametername):
		playernumber = next(writer.tempvar)
		writer.declare('int', playernumber)
		writer.writeline('{playernumber} = command.{parametername} - players;'.format(**locals()))
		writer.writeline('')
		if 'indextestonly' in self.attributes:
			writer.writeline('if (( {playernumber} < 0 ) || ( {playernumber} >= MAXPLAYERS ))'.format(**locals()))
		else:
			writer.writeline('if ( PLAYER_IsValidPlayer( {playernumber} ) == false )'.format(**locals()))
		writer.writeline('{')
		writer.writeline('\tCLIENT_PrintWarning( "{commandname}: Invalid player number: %d\\n", {playernumber} );' \
			.format(commandname=command.name, **locals()))
		writer.writeline('\treturn true;')
		writer.writeline('}')
		writer.writeline('')

		# Allow for a check for the player's body
		if 'motest' in self.attributes:
			writer.writeline('')
			writer.writeline('if ( command.{parametername}->mo == NULL )'.format(**locals()))
			writer.writeline('\treturn true;')
			writer.writeline('')

	def writesend(self, writer, command, parametername):
		writer.writeline('command.addByte( this->{parametername} - players );'.format(**locals()))

# ----------------------------------------------------------------------------------------------------------------------

class Vector3Parameter(SpecParameter):
	def __init__(self, **args):
		super().__init__(**args)
		self.cxxtypename = 'FVector3'

	def writeread(self, writer, command, parametername):
		writer.writecontext('''
			command.{parametername}.X = NETWORK_ReadFloat( bytestream );
			command.{parametername}.Y = NETWORK_ReadFloat( bytestream );
			command.{parametername}.Z = NETWORK_ReadFloat( bytestream );'''.format(**locals()))

	def writesend(self, writer, command, parametername):
		writer.writecontext('''
			command.addFloat( this->{parametername}.X );
			command.addFloat( this->{parametername}.Y );
			command.addFloat( this->{parametername}.Z );'''.format(**locals()))

# ----------------------------------------------------------------------------------------------------------------------

class FixedParameter(SpecParameter):
	def __init__(self, **args):
		super().__init__(**args)
		self.cxxtypename = 'fixed_t'

	def writeread(self, writer, command, parametername):
		writer.writeline('command.{parametername} = NETWORK_ReadLong( bytestream );'.format(**locals()))

	def writesend(self, writer, command, parametername):
		writer.writeline('command.addLong( this->{parametername} );'.format(**locals()))

# ----------------------------------------------------------------------------------------------------------------------

class AproxfixedParameter(SpecParameter):
	def __init__(self, **args):
		super().__init__(**args)
		self.cxxtypename = 'fixed_t'

	def writeread(self, writer, command, parametername):
		writer.writeline('command.{parametername} = NETWORK_ReadShort( bytestream ) << FRACBITS;'.format(**locals()))

	def writesend(self, writer, command, parametername):
		writer.writeline('command.addShort( this->{parametername} >> FRACBITS );'.format(**locals()))

# ----------------------------------------------------------------------------------------------------------------------

class AngleParameter(FixedParameter):
	def __init__(self, **args):
		super().__init__(**args)
		self.cxxtypename = 'angle_t'

# ----------------------------------------------------------------------------------------------------------------------

class AproxangleParameter(AproxfixedParameter):
	def __init__(self, **args):
		super().__init__(**args)
		self.cxxtypename = 'angle_t'

# ----------------------------------------------------------------------------------------------------------------------

class JoinslotParameter(SpecParameter):
	def __init__(self, **args):
		super().__init__(**args)
		self.cxxtypename = 'JoinSlot'

	def writeread(self, writer, command, parametername):
		writer.writeline('command.{parametername}.player = NETWORK_ReadByte( bytestream );'.format(**locals()))
		writer.writeline('command.{parametername}.team = NETWORK_ReadByte( bytestream );'.format(**locals()))

	def writesend(self, writer, command, parametername):
		writer.writeline('command.addByte( this->{parametername}.player );'.format(**locals()))
		writer.writeline('command.addByte( this->{parametername}.team );'.format(**locals()))
