/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */
/*+*******************************************************************

	Module Name:		g3optdec.cxx (was prot_dec.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Protocol Class Decoders

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [44]-1734-413655

	History:
		Who			When				Description
	-----------	--------------	----------------------------------
	Barry Scott	January 1995	Initial Creation
	Alwyn Teh	21 August 1995	Handle optional trailing
								parameters (e.g. BTUP RELR)
								in UIField_Ref::Decode().
	Alwyn Teh	22 August 1995	Handle bit8 in Field_Ref::Decode()
								for bit fields (e.g. CTUP HUA).

*******************************************************************-*/

#include <assert.h>

#include <g3oproth.h>
#include <g3obitsh.h>
#include <g3ouvarh.h>

#include <rw/cstring.h>
#include <rw/hashdict.h>
#include <rw/ordcltn.h>

#include <iostream>
using namespace std;

extern int __debug;

void Protocol::Decode( bit_string *msg, user_variables *uv )
{
	try
	{
		// release any stale pointer information
		pointer_info.clearAndDestroy();

		// decode from the start of the message
		msg->set_position(0);

		prot.ref->Decode( this, msg, uv ) ;
		if ( msg->length(1) != msg->position() )
		  throw MessageLong();
	}
	catch(bit_string::FetchTooMuch)
	{
		throw MessageShort();
	}
}

void Protocol::Decode( bit_string *msg, user_variables *uv, const char *ui_group_name )
{
	RWCollectableString group( ui_group_name );
	GroupInfo *info = (GroupInfo *)(group_info.findValue( &group ));
	if ( info == NULL )
	  throw InternalError( "Unknown UI Group %s", ui_group_name );

	try
	{
		// release any stale pointer information
		pointer_info.clearAndDestroy();

		// decode from the start of the message
		msg->set_position(0);

		info->ui_group->Decode( this, msg, uv );
		if ( msg->length(1) != msg->position() )
		  throw MessageLong();
	}
	catch(bit_string::FetchTooMuch)
	{
		throw MessageShort();
	}
}

void Protocol::UIField_Ref::Decode( Protocol *, bit_string *msg, user_variables * uv )
{
#if _DEBUG_DECODE
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__ << " - Decoding UIField( " << variable_name << ", "
	  	   << long_description << ")" << endl;
#endif

	//If this is a trailing optional parameter, and if no bits are left to decode, stop here,
	int new_default_value = default_value;
	int properties = get_properties();
	if ( ( properties & Protocol::is_trailer ) && ( properties & Protocol::is_optional ) )
	{
//	  cout << endl;
//	  cout << "Optional UIField_Ref trailer: " << *msg << endl;
//	  cout << " Bit Length	= " << msg->get_bit_length() << endl;
//	  cout << " Bit Position = " << msg->get_bit_position() << endl;

	  if ( ( msg->get_bit_position() >= msg->get_bit_length() ) && default_value != 0 )
		new_default_value = 0;
	}

	// If the user has not provided a value, use the default value,
	if ( !uv->exists( variable_name ) )
	{
	  bit_string value;
	  value.append_lsb( bit_width, new_default_value );

	  // define the variable with the default value
	  uv->define( variable_name, value, bit_width, field_type, field_format, long_description );
	}

#if _DEBUG_DECODE
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__ << " - Done Decoding UIField( " << variable_name
	  	   << ", " << long_description << ")" << endl;
#endif
}

void Protocol::UIGroup_Ref::Decode( Protocol *p, bit_string *msg, user_variables *uv )
{
#if _DEBUG_DECODE
	if ( __debug )
	  cerr << __FILE__ << " : line " << __LINE__ << " - Decoding UIGroup ( " << variable_group
	  	   << ", " << long_description << ")" << endl;
#endif

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->Decode( p, msg, uv );

#if _DEBUG_DECODE
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__ << " - Done Decoding UIGroup( " << variable_group
	  	   << ", " << long_description << ")" << endl;
#endif
}

void Protocol::LabelGroup_Ref::Decode( Protocol *p, bit_string *rasg, user_variables *uv )
{
#if _DEBUG_DECODE
	if ( __debug )
	  cerr << __FILE__ << ": line "	<< __LINE__ << " - Decoding LabelGroup( " << long_description << ")" << endl;
#endif

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->Decode( p, msg, uv );

#if _DEBUG_DECODE
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__ << " - Done Decoding LabelGroup ( " << long_description << ")" << endl;
#endif
}

void Protocol::OptionGroup_Ref::Decode( Protocol *p, bit_string *msg, user_variables *uv )
{
#if _DEBUG_DECODE
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__ << " - Decoding OptionGroup( " << long_description << ")" << endl;
#endif

	// remember how far the decode has gone
	int start_position = msg->position(1);
	try {
	   for ( int m = 0; m < num_msgs; m++ )
		  msgs[m]->Decode( p, msg, uv );

	   // say that the option is present
	   uv->define( OptionName( variable_group ), 8, 1, field_type, field_format, long_description );
	}
	catch( TypeCodeMismatch )
	{
#if _DEBUG_DECODE
	   if ( __debug )
		 cerr << __FILE__ << ": line " << __LINE__ << " - Aborting Decoding OptionGroup( "
			  << long_description << ")" << endl;
#endif

	   // backup to the start point
	   msg->set_position( start_position );
	}

#if _DEBUG_DECODE
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__ << " - Done Decoding OptionGroup ( " << long_description << ")" << endl;
#endif
}

void Protocol::Message_Ref::Decode( Protocol *p, bit_string *msg, user_variables *uv )
{
#if _DEBUG_DECODE
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__ << " - Decoding Message(" << variable_field << ", "
		   << long_description << ")" << endl;
#endif

	// make a string
	bit_string b( variable_field );
	bit_width = variable_field.length() * 8;
	field_format = ff_ascii;
	uv->define( MessageTypeName(), b, bit_width, field_type, field_format, long_description );

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->Decode( p, msg, uv );

#if _DEBUG_DECODE
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__ << " - Done Decoding Message(" << variable_field
		   << ", " << long_description << ")" << endl;
#endif
}

void Protocol::SelectOne_Ref::Decode( Protocol *p, bit_string *msg, user_variables *uv )
{
#if _DEBUG_DECODE
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__
		   << " - Decoding SelectOne( "
		   << variable_name << ", "
		   << long_description << ">" << endl;
#endif

	int value;

	if ( bit_width != 0 )
	  // protocol based select group
	{
	  value = msg->fetch_lsb( bit_width );
	  bit_string b;
	  b.append_lsb( bit_width, value );
	  if ( !variable_name.isNull() )
		uv->define( variable_name, b, bit_width, field_type, field_format, long_description );
	}
	else
	  // ui based select group
	  value = uv->value( variable_name, 0 );

	for ( int m = 0; m < num_msgs; m++ )
	   if ( value == msg_types[m] )
	   {
		 msgs[m]->Decode( p, msg, uv );
		 break;
	   }

	// it's possible to get here without finding a message to decode
	// which is handled by our caller
#if _DEBUG_DECODE
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__
		   << " - Done Decoding SelectOne( "
		   << variable_name << ", "
		   << long_description << ")" << endl;
#endif
}

void Protocol::Group_Ref::Decode( Protocol *p, bit_string *msg, user_variables *uv )
{
#if _DEBUG_DECODE
	if ( __debug )
	cerr << __FILE__ << ": line " << __LINE__
		 << " - Decoding Group( "
		 << variable_name << ", "
		 << long_description << ")" << endl;
#endif

	if ( !controlled_by.isNull() )
	{
	  if ( !uv->exists( controlled_by ) )
		throw InternalError( "Group::Decode failed to find controlled_by variable %s", controlled_by );

	  int indicator = uv->value( controlled_by, 0 );
	  if ( !indicator )
		return;
	}

	Field_Ref::Decode( p, msg, uv );

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->Decode( p, msg, uv );

#if _DEBUG_DECODE
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__
		   << " - Done Decoding Group( "
		   << variable_name << ", "
		   << long_description << ")" << endl;
#endif
}

void Protocol::LengthGroup_Ref::Decode( Protocol *p, bit_string *msg, user_variables *uv )
{
#if _DEBUG_DECODE
	if ( __debug )
	  cerr <<  __FILE__  << ": line " << __LINE__
		   << " - Decoding LengthGroup( "
		   << variable_name << ", "
		   << long_description << ")" << endl;
#endif

	// decode the length field
	Field_Ref::Decode( p, msg, uv );

	// get back the size of the group in bytes
	int size = uv->value( variable_name, 0 );

	// copy that number of bytes from msg
	bit_string groups_bits;
	msg->fetch( size*8, &groups_bits );

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->Decode( p, &groups_bits, uv );

#if _DEBUG_DECODE
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__
		  << " - Done Decoding LengthGroup( "
		  << variable_name << ", "
		  << long_description << ")" << endl;
#endif
}

//
//
//	Decode A Field
//
//
void Protocol::Field_Ref::Decode( Protocol * /*p*/, bit_string *msg, user_variables *uv )
{
	// see if there is nothing to do
	if ( bit_width == 0 )
	  return;

#if _DEBUG_DECODE
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__
		   << " - Decoding Field( "
		   << variable_name << ", "
		   << dec << bit_width << ", "
		   << long_description << ")" << endl;
#endif

	// If this is a trailing optional parameter, and if no bits are left to decode, stop here.
	int properties = get_properties();
	if ( ( properties & Protocol::is_trailer ) && ( properties & Protocol::is_optional ) )
	{
//	  cout << endl;
//	  cout << "Optional Field_Ref trailer: " << *msg << endl;
//	  cout << " Bit Length = " << msg->get_bit_length() << endl;
//	  cout << " Bit Position = " << msg->get_bit_position() << endl;

	  if ( msg->get_bit_position() >= msg->get_bit_length() )
		return;
	}

	int control_size = 0;
	bit_string b;

	switch ( field_type )
	{
		case bcd_odd_indicator:
				// ignore controlled_by fields
				break;
		default:
				//
				// if this field is controlled, get its size
				//
				if ( !controlled_by.isNull() )
				{
				  if ( !uv->exists( controlled_by ) )
					throw InternalError( "Field::Decode failed to find controlled_by variable %s", controlled_by );
				  control_size = uv->value( controlled_by, 0 );
				}
	}

	switch( field_type )
	{
		// simple length count from 0 to n
		case length0:
		{
			int length = msg->fetch_lsb( bit_width );
			b.append_lsb( bit_width, length );
		}
		break;
		// complex length count 0 to n maps to 0,2 to n+l
		case length1:
		{
			int length = msg->fetch_lsb( bit_width );
			// if ( length > 0 )
			//	 length++;	// move 1 to n onto 2 to n+1
			// disused by using add_one_to_control_value on status, shouldn't muck about with range value

			b.append_lsb( bit_width, length );
		}
		break;
		case bcd:
		{
			// deal with fixed size bed fields
			if ( controlled_by.isNull() )
			  control_size = bit_width/4;

			// copy each BCD digit
			for ( ; control_size > 0; control_size-- )
			   b.append_lsb( 4, msg->fetch_lsb( 4 ) );
		}
		break;
		case bcd_pad8:
		{
			// deal with fixed size bed fields
			if ( controlled_by.isNull() )
			  control_size = bit_width/4;

			// copy each BCD digit
			for ( ; control_size > 0; control_size-- )
			   b.append_lsb( 4, msg->fetch_lsb( 4 ) );

			// see if we need to align
			if ( (msg->position() % 8) != 0 )
			  msg->fetch_lsb( 4 ); // lose the padding
		}
		break;
		case bcd_to_end:
		{
			int digits = msg->length(4) - msg->position(4) - control_size;

			// copy each BCD digit
			for ( ; digits > 0; digits-- )
			   b.append_lsb( 4, msg->fetch_lsb( 4 ) );

			// see if we need to align
			if ( (msg->position() % 8) != 0 )
			  msg->fetch_lsb( 4 ); // lose the padding
		}
		break;
		case octet:
		{
			// deal with fixed size octet fields
			if ( controlled_by.isNull() )
			  control_size = bit_width/8;

			// copy each octet
			for ( ; control_size > 0; control_size-- )
			   b.append_lsb( 8, msg->fetch_lsb( 8 ) );
		}
		break;
		case type_code:
		{
			int value = msg->fetch_lsb( bit_width );

			if ( value != min_valid )
			  throw TypeCodeMismatch();
		}
		break;
		case bit8:
		{
			if ( controlled_by.isNull() ) // not controlled by another field, encode all bits
			{
			  msg->append( &b );
			}
			else
			{
			  int properties = this->get_properties();
			  if ( properties & add_one_to_control_value )
				control_size += 1;
			  else
			  if ( properties & add_one_to_control_value_except_zero )
				control_size += ( control_size ) ? 1 : 0;

			  if ( control_size ) // in bits
			  {
				int pad_bits = 0;
				int bits_used = control_size % 8; // bits used within the last byte

				if ( bits_used )
				  pad_bits = 8 - bits_used;

				for ( ; control_size > 0; control_size-- )
				   msg->append_lsb( bit_width, b.fetch_lsb( bit_width ) );

				if ( pad_bits )
				  msg->append_lsb( pad_bits, 0 );
			  }
			  else
				return; // control_size is zero, nothing to decode
			}
		}
		break;

		default:
		{
			int value = msg->fetch_lsb( bit_width );
			b.append_lsb( bit_width, value );
		}
		break;
	}

	// only defined if there is a name
	if ( !variable_name.isNull() )
	  uv->define( variable_name, b, bit_width, field_type, field_format, long_description );
}
