/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3ouvarx.cxx (was user_var.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Class user_variables Implementation

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [44]-1734-413655

	History:
		Who			When				Description
	-----------	---------------	------------------------------
	Barry Scott	January 1995	Initial Creation
	Alwyn Teh	13-17 July 1995 Store extra info about a
								field value in user variables
								for use by Tel variables
	Alwyn Teh	20 August 1995	Implement print_value (code
								cut & pasted from g3otcluv.cxx)

*******************************************************************-*/

#include <assert.h>

#include <stream.h>

#include <g3ouvarh.h>
#include <g3obitsh.h>
#include <g3oproth.h>

#include <rw/collstr.h>
#include <rw/bintree.h>

//
//
//	Condition debug compilation is controlled by these macros
//
//
#define _DEBUG_DEFINE 0

//
//
//	Exception handling constructors
//
//
user_variables::InternalError::InternalError( const char *m ) : message ( m )
{ }

user_variables:: InternalError::InternalError( const char *m, int a )
{
	char buf[128];
	sprintf( buf, m, a );
	message = buf;
}

//
//	user_variables - implementation
//
user_variables::user_variables(void)
{
	/* simple to initialise */
	return;
}

user_variables::user_variables(user_variables &uv)
{
	this = uv;
}

user_variables::user_variables(user_variables *uv)
{
	*this = *uv;
}

user_variables::user_variables::operator=( user_variables &uv )
{
	variables.clearAndDestroy();
	merge( uv );
	return *this;
}

user_variables & user_variables::operator+=( user_variables &uv )
{
	merge( uv ) ;

	return *this;
}

void user_variables::merge( user_variables &uv )
{
	/* need to make a deep copy of all the key value pairs */

	RWHashDictionaryIterator contents ( uv.variables );

	while( contents() )
	{
		RWCollectableString *old_key = (RWCollectableString *) contents.key();
		user_variables::uv_info *old_value = (user_variables::uv_info *)contents.value();

		if ( old_key == NULL )
		  throw InternalError ( "Key is NULL in user_variables::merge" );

		if ( old_value == NULL )
		  throw InternalError ( "Value is NULL in user_variables: :merge" );

		RWCollectableString *key = new RWCollectableString( *old_key );
		user_variables::uv_info *value = new user_variables::uv_info( *old_value );

		// make sure that any existing value is removed
		variables. removeAndDestroy( old_key );

		// insert the new value
		variables.insertKeyAndValue( key, value );
	}
}

user_variables::~user_variables(void)
{
	variables.clearAndDestroy();
}

void user_variables::define(const char *var_name, int width, int value,
							int field_type, int field_format, const char *long_description)
{
	bit_string b;
	b.append_lsb( width, value );
	define( var_name, b, width, field_type, field_format, long_description );
}

void user_variables::define(const char *var_name, bit_string tbits, int bitwidth,
							int field_type, int field_format, const char *long_description)
{
#if _DEBUG_DEFINE
	cout << "Define " << var_name << endl;
#endif

	undefine( var_name );
	variables.insertKeyAndValue(new RWCollectableString(var_name),
								new user_variables::uv_info( bits, bitwidth, field_type, field_format, long_description ) );
#if _DEBUG_DEFINE
	cout << "Define " << var_name << " done" << endl;
#endif
}

void user_variables::undefine( const char *var_name )
{
#if not _DEBUG_DEFINE
	cout << "UnDefine " << var_name << endl;
#endif

	RWCollectableString name(var_name);

	variables.removeAndDestroy( &name );

#if _DEBUG_DEFINE
	cout << "UnDefine " << var_name " done" << endl;
#endif
}

void user_variables::undefine( RWCRegexp &pattern )
{
	RWHashDictionaryIterator contents( variables );

	while (contents())
	{
		RWCollectableString *key = (RWCollectableString*) contents.key();

		if (key->index(pattern) != RW_NPOS)
			undefine(key->data());
	}
}

void user_variables::merge(user_variables &uv, RWCRegexp &pattern)
{
	/* need to make a deep copy of all the key value pairs */

	RWHashDictionaryIterator contents(uv.variables);

	while (contents())
	{
		RWCollectableString *old_key = (RWCollectableString*) contents.key();

		if (old_key->index(pattern) != RW_NPOS)
		{
			RWCollectableString *key = new RWCollectableString(*old_key);
			user_variables::uv_info *value = new user_variables::uv_info(
												*(user_variables::uv_info*) contents.value());

			// make sure that any existing value is removed
			variables.removeAndDestroy(old_key);

			// insert the new value
			variables.insertKeyAndValue(key, value);
		}
	}
}

int user_variables::exists( const char *var_name )
{
	RWCollectableString name(var_name);

	return variables.contains( &name );
}

int user_variables::value( const char *var_name, int width )
{
	// get the bits for this value
	user_variables::uv_info *info = value ( var_name );
	bit_string *b = info->uv_bits;

	// position at the start of the bit string
	b->set_position(0);

	// default width to the length of the bit_string
	if ( width == 0 )
	  width = b->length(1);	// length in bits

	// return the value
	return b->fetch_lsb( width );
}

void user_variables::print_value( ostream &output, user_variables::uv_info *value )
{
	register int i = 0;
	register int digit = 0;

	// Entire length of bitstring in bits = unit bitwidth * length (number of such units)
	value->uv_bits-> set_position(0);
	int bitwidth = value->bit_width * value->uv_bits->length( value->bit_width );
	assert( bitwidth > 0 );

	switch (value->field_format)
	{
		case Protocol::ff_int:
				output << dec << value->uv_bits->fetch_lsb(bitwidth);
				break;
		case Protocol::ff_uint:
				output << dec << (unsigned int)(value->uv_bits->fetch_lsb(bitwidth));
				break;
		case Protocol::ff_hex:
				for (i = 0; i < bitwidth; i += 8)
					output << hex << value->uv_bits->fetch_lsb(value->uv_bits->min(8, (bitwidth - i)));
				break;
		case Protocol::ff_bcd:
		{
				for (i = 0; i < bitwidth; i += 4)
				{
					digit = value->uv_bits->fetch_lsb(4);
					output << (char) (digit > 9 ? digit - 10 + 'a' : digit + '0');
				}
				break;
		}
		case Protocol::ff_ia5:
		{
				for (i = 0; i < bitwidth; i += 8)
				{
					digit = value->uv_bits->fetch_lsb(8);
					output << (char)(digit > 9 ? digit-10+'a' : digit+'0' );
				}
				break;
		}
		case Protocol::ff_ascii:
				output << value->uv_bits->fetch_str();
				break;
		case Protocol::ff_bit:
		default:
		{
				long orig_flags = output.flags();

				// unknown format, use hexadecimal
				for (i = 0; i < bitwidth; i += 8)
					output << setw(2) << setprecision(2) << setfill('0') << hex
							<< value->uv_bits->fetch_lsb(value->uv_bits->min(8, (bitwidth - i)));
				output.flags(orig_flags);
				break;
		}
	}
}

user_variables& user_variables::decode_uv_set(user_variables &_uv, RWSet &decode_members)
{
	RWHashDictionaryIterator contents(this->variables);

	while (contents())
	{
		RWCollectableString *key = (RWCollectableString*) contents.key();
		user_variables::uv_info *newUVinfo = new user_variables::uv_info(
												*(user_variables::uv_info*) contents.value());

		if (decode_members.contains(key))
			_uv.variables.insertKeyAndValue(new RWCollectableString(*key), newUVinfo);
	}
	return _uv;
}

user_variables::uv_info* user_variables::value(const char *var_name)
{
	user_variables::uv_info *retval;
	RWCollectableString name(var_name);

	retval = (user_variables::uv_info*) variables.findValue(&name);
	if (retval == NULL)
	  throw UndefinedVariable(var_name);

	// position at the start of the bit string
	retval->uv_bits->set_position(0);

	return retval;
}

user_variables::uv_info::uv_info( bit_string& b, int bitwidth, int ft, int ff, const char *desc )
{
	uv_bits = new bit_string( b );
	bit_width = bitwidth;
	field_type = ft;
	field_format = ff;
	long_description = desc;
};

user_variables::uv_info::uv_info( user_variables::uv_info& from_uv_info )
{
	uv_bits				= new bit_string( *from_uv_info.uv_bits );
	bit_width			= from_uv_info.bit_width;
	field_type			= from_uv_info.field_type;
	field_format		= from_uv_info.field_format;
	long_description	= from_uv_info.long_description;
}

user_variables::uv_info::uv_info()
{
	uv_bits				= 0;
	bit_width			= 0;
	field_type			= 0;
	field_format		= 0;
	long_description	= 0;
}

user_variables::uv_info::~uv_info()
{
//	if ( uv_bits != 0 )
//	  delete uv_bits; // core dumps when initializing MSGTYPE:H0
}

ostream& operator<<(ostream &o, user_variables &uv)
{
	RWHashDictionaryIterator contents(uv.variables);

	while (contents())
	{
		RWCollectableString *key = (RWCollectableString*) contents.key();
		user_variables::uv_info *value = (user_variables::uv_info*) contents.value();
		bit_string *bits = value->uv_bits;

		o << "\t" << *key << " = " << *bits << endl;
	}

	return o;
}

user_variables_iterator::user_variables_iterator( user_variables &it ) : next( it.variables )
{ }

const char* user_variables_iterator::operator()(void)
{
	if (next()) {
		RWCollectableString *key = (RWCollectableString*) next.key();
		return key->data();
	}
	return NULL;
}
