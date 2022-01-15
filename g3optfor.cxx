/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+******************************************************************

	Module Name:		g3optfor.cxx (was prot_for.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Protocol Class Output Formatting Methods

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [44]-1734-413655

	History:
		Who			When				Description
	-----------	--------------	-------------------------------------
	Barry Scott	January	1995	Initial Creation
	Alwyn Teh	22 August 1995	Field_Ref::Format(): handle ff_bit
								bit8 parameter (e.g. CTUP HUA:status)
	Alwyn Teh	28 Sept 1995	Fix range and status bug in length1
								and bit8.

*******************************************************************-*/

#include <assert.h>

#include <g3oproth.h>
#include <g3opforh.h>

#include <g3obitsh.h>
#include <g3ouvarh.h>

#include <rw/ordcltn.h>

extern int __debug;

inline int min( int a, int b )
{
	if ( a < b )
	  return a;
	else
	  return b;
}

void Protocol::Format( bit_string *msg, user_variables *uv, ProtocolFormat *pf )
{
	//
	// we need the result of a decode to process the format correctly
	//
	try
	{
		if ( pf->is_decode_required() )
		  Decode( msg, uv );
	}
	catch( ... )
	{
		// we assume that the exceptions that this catches will
		// throw again on the format and be handled elegantly
	}

	try
	{
		// release any stale pointer information
		pointer_info.clearAndDestroy();

		// format from the start of the message
		msg->set_position(0);

		prot.ref->Format( this, msg, uv, pf );
		if ( msg->length(1) > msg->position(1) )
		  throw MessageLong();
	}
	catch( MessageLong )
	{
		// report the error to the user
		pf->format_error( "Error: message is too long" );

		format_excess_data( msg, pf );
	}
	catch( bit_string::FetchTooMuch )
	{
		// tell the user the message is short
		pf->format_error( "Error: message is short" );

		format_excess_data( msg, pf );
	}
}

void Protocol::Format( bit_string *msg, user_variables *uv, ProtocolFormat *pf, const char *ui_group_name )
{
	RWCollectableString group( ui_group_name );
	GroupInfo *info = (GroupInfo *)(group_info.findValue( &group ));
	if ( info == NULL )
	  throw InternalError( "Unknown UI Group %s", ui_group_name );

	try
	{
		// release any stale pointer information
		pointer_info.clearAndDestroy();

		// format from the start of the message
		msg->set_position(0);

		info->ui_group->Format( this, msg, uv, pf );
		if ( msg->length(1) != msg->position() )
		  throw MessageLong();
	}
	catch( MessageLong )
	{
		// report the error to the user
		pf->format_error( "Error: message is too long" );

		format_excess_data( msg, pf );
	}
	catch( bit_string::FetchTooMuch )
	{
		// tell the user the message is short
		pf->format_error( "Error: message is short" );

		format_excess_data( msg, pf );
	}
}

void Protocol::UIField_Ref::Format( Protocol *, bit_string *, user_variables * uv, ProtocolFormat * pf )
{
	bit_string *b = uv->value( variable_name )->uv_bits;
	int value = b->fetch_lsb( bit_width );

	// lookup the name of this value
	const char *value_name = table_of_names[value];

	b->set_position(0);

	pf->format_field(field_type, field_format,
					 b, -1, b->length(1),
					 variable_group, variable_name, long_description, value_name,
					 get_custom_ops());
}

void Protocol::UIGroup_Ref::Format( Protocol *p, bit_string * msg, user_variables * uv, ProtocolFormat * pf )
{
#if _DEBUG_FORMAT
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__ << " - Formating UIGroup( " << variable_group
	  	   << ", " << long_description << ")" << endl;
#endif

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->Format( p, msg, uv, pf );

#if _DEBUG_FORMAT
if ( __debug )
	cerr << __FILE__ <<	": line " << __LINE__ << " - Done Formating UIGroup( " << variable_group
		 << ", " << long_description << ")" << endl;
#endif
}

void Protocol::SelectOne_Ref::Format( Protocol *p, bit_string * msg, user_variables * uv, ProtocolFormat * pf )
{
#if _DEBUG_FORMAT
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__
		   << " - Formating SelectOne( "
		   << variable_name << ", "
		   << long_description << ")" << endl;
#endif

	Field_Ref::Format( p, msg, uv, pf );

	// get the value that Field_Ref::Format will have pulled out
	int value;
	if ( bit_width != 0 )
	  // protocol based select group
	  value = uv->value( variable_name, bit_width );
	else
	  // ui based select group
	  value = uv->value( variable_name, 0 );

	for ( int m = 0; m < num_msgs; m++ )
	   if ( value == msg_types [m] )
	   {
		 msgs[m]->Format( p, msg, uv, pf );
		 break;
	   }

	// it's possible to get here without finding a message to format,
	// which is handled by our caller
#if _DEBUG_FORMAT
if ( __debug )
	cerr << __FILE__ << ": line " << __LINE__
		 << " - Done Formating SelectOne( "
		 << variable_name << ", "
		 << long_description << ")" << endl;
#endif
}

void Protocol::LabelGroup_Ref::Format( Protocol *p, bit_string * msg, user_variables * uv, ProtocolFormat * pf )
{
#if _DEBUG_FORMAT
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__ << " - Formating LabelGroup( " << long_description << ")" << endl;
#endif

	pf->format_labelling_prologue( msg->position(), variable_group, long_description );

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->Format( p, msg, uv, pf );

	pf->format_labelling_epilogue( msg->position(), variable_group, long_description );

#if _DEBUG_FORMAT
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__ << " - Done Formating LabelGroup ( " << long_description << ")" << endl;
#endif
}

void Protocol::OptionGroup_Ref::Format( Protocol *p, bit_string *msg, user_variables *uv, ProtocolFormat * pf )
{
#if _DEBUG_DECODE
	if ( __debug )
	  cerr << "Formating OptionGroup( " << long_description << ")" << endl;
#endif

	// decode figure out if this option is present
	if ( uv->exists( OptionName( variable_group ) ) )
	  if ( uv->value( OptionName( variable_group ), 0 ) )
	  {
		for ( int m = 0; m < num_msgs; m++ )
		   msgs[m]->Format( p, msg, uv, pf );

#if _DEBUG_DECODE
	if ( __debug )
	  cerr << "Done Formating OptionGroup( " << long_description << ")" << endl;
#endif
	  }
}

void Protocol::Group_Ref::Format( Protocol *p, bit_string * msg, user_variables * uv, ProtocolFormat * pf )
{
#if _DEBUG_FORMAT
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__
	  	   << " - Formating Group( "
		   << variable_name << ", "
		   << long_description << ")" << endl;
#endif

	if ( !controlled_by.isNull() )
	{
	  if ( !uv->exists( controlled_by ) )
		throw InternalError( "Group_Ref::Format failed to find controlled_by variable %s", controlled_by );

	  int indicator = uv->value( controlled_by, 0 );
	  if ( !indicator )
		return;
	}

	Field_Ref::Format( p, msg, uv, pf );

	for ( int m = 0; m < num_msgs; m++ )
	  msgs[m]->Format( p, msg, uv, pf );

#if _DEBUG_FORMAT
	if ( __debug )
	  cerr <<  __FILE__  << ": line " << __LINE__
	  	   << " - Done Formating Group{ "
		   << variable_name << ", "
		   << long_description << ")" << endl;
#endif
}

void Protocol::LengthGroup_Ref::Format( Protocol *p, bit_string * msg, user_variables * uv, ProtocolFormat * pf )
{
#if _DEBUG_FORMAT
	if ( __debug )
	  cerr << __FILE__  << ": line " << __LINE__
		   << " - Formating LengthGroup( "
		   << variable_name << ", "
		   << long_description << ")" << endl;
#endif

	Field_Ref::Format( p, msg, uv, pf );

	// get back the size of the group in bytes
	int size = uv->value( variable_name, 0 );

	// copy that number of bytes from msg
	bit_string groups_bits ;
	msg->fetch( size*8, &groups_bits );

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->Format( p, &groups_bits, uv, pf );

#if _DEBUG_FORMAT
	if ( __debug )
	cerr << __FILE__ << ": line " << __LINE__
		 << " - Done Formating LengthGroup ( "
		 << variable_name << ", "
		 << long_description << ")" << endl;
#endif
}

void Protocol::Message_Ref::Format( Protocol *p, bit_string * msg, user_variables * uv, ProtocolFormat * pf )
{
#if _DEBUG_FORMAT
	if ( debug )
	  cerr << __FILE__ << ": line " << __LINE__ << " - Formating Message(" << variable_name
	  	   << ", " << long_description << ")" << endl;
#endif

	for ( int m=0; m<num_msgs; m++ )
	   msgs[m]->Format( p, msg, uv, pf );

#if _DEBUG_FORMAT
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__ << " - Done Format Message (" << variable_name
		   << ", " << long_description << ")" << endl;
#endif
}

void Protocol::Field_Ref::Format( Protocol * /*p*/, bit_string * msg, user_variables * uv, ProtocolFormat * pf )
{
	if ( bit_width == 0 )
	  return;

#if _DEBUG_FORMAT
	if ( __debug )
	  cerr << __FILE__ << ": line " << __LINE__
	  	   << " - Formating Field( "
		   << variable_name << ", "
		   << long_description << ")" << endl;
#endif

	// If this is a trailing optional parameter, and if no bits are left to format, stop here,
	int properties = get_properties();
	if ( ( properties & Protocol::is_trailer ) && ( properties & Protocol::is_optional ) )
	{
//	  cout << endl;
//	  cout << "Optional Field_Ref trailer: " << *msg << endl;
//	  cout << " Bit Length = " << msg->get_bit_length()) << endl;
//	  cout << " Bit Position = " << msg->get_bit_position() << endl;

	  if ( msg->get_bit_position() >= msg->get_bit_length() )
		return;
	}

	int control_size = 0;
	bit_string b, pad;

	int start_pos = msg->position();
	int pad_start_pos = 0;

	switch( field_type )
	{
		case bcd_odd_indicator:
			// ignore controlled_by fields
			break;

		default:
			//
			// if this field is controlled get its size
			//
			if ( !controlled_by.isNull() )
			{
			  if ( !uv->exists( controlled_by ) )
				throw InternalError( "Field_Ref::Format failed to find controlled_by variable %s", controlled_by );
			  control_size = uv->value( controlled_by, 0 );
			}
	}

	const char *value_name = NULL;

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

			// disused by using add_one_to_control_value on status, shouldn't muck about with range value
			// if ( length > 0 )
			//	 length++;	// move 1 to n onto 2 to n+1
			b.append_lsb( bit_width, length );
		}
		break;

		case bcd:
		{
			// deal with fixed size bcd fields
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
			pad_start_pos = msg->position();
			if ( (msg->position() % 8) != 0 )
			  pad.append_lsb( 4, msg->fetch_lsb( 4 ) ); // save the padding
		}
		break;

		case bcd_to_end:
		{
			int digits = msg->length(4) - msg->position(4) - control_size;

			// copy each BCD digit
			for ( ; digits > 0; digits-- )
			   b.append_lsb( 4, msg->fetch_lsb( 4 ) );

			// see if we need to align
			pad_start_pos = msg->position() ;
			if ( msg->position() % 8 != 0 )
			  pad.append_lsb( 4, msg->fetch_lsb( 4 ) ); // save the padding
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

		case bit8:
		{
			if ( controlled_by.isNull() ) // not controlled by another field, encode all bits
			{
			  b.append ( msg );
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
				   b.append_lsb( bit_width, msg->fetch_lsb( bit_width ) );

				if ( pad_bits )
				  b.append_lsb( pad_bits, msg->fetch_lsb( pad_bits ) );
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

			// lookup the name of this value
			value_name = table_of_names[value];
		}
		break;
	}

	// only defined if there is a name
	if ( !variable_name.isNull() )
	  uv->define( variable_name, b, bit_width, field_type, field_format, long_description );

	pf->format_field(field_type, field_format, &b, start_pos, b.length(1),
					 variable_group, variable_name, long_description, value_name,
					 get_custom_ops());

	if ( pad.length(1) > 0 )
	{
	  pf->format_field(simple, ff_uint, &pad, pad_start_pos, pad.length(1),
			  	  	   variable_group, NULL, "Padding", NULL);
	}
}

void Protocol::format_excess_data( bit_string *msg, ProtocolFormat *pf )
{
	bit_string excess_bits;

	// see if there is any data to format
	if ( msg->position(1) >= msg->length(1) )
	  // nope - do nothing
	  return;

	// print the excess data as a grouping
	pf->format_labelling_prologue( msg->position(1), "EXCESS", "Excess message data" );

	// align to a byte boundary
	if ( (msg->position(1) % 8) != 0 )
	{
	  int remaining = msg->length(1) - msg->position(1);
	  int len = min( msg->position(1) % 8, remaining );

	  excess_bits.empty();
	  excess_bits.append_lsb( len, msg->fetch_lsb( len ) );

	  pf->format_field( simple, ff_hex, &excess_bits, msg->position(1), len, "EXCESS", NULL, "Excess", NULL );
	}

	// print the rest of the excess data
	while ( msg->position(1) < msg->length(1) )
	{
		 int remaining = msg->length(1) - msg->position(1);
		 int len = min( 8, remaining );

		 excess_bits.empty(); excess_bits.append_lsb( len, msg->fetch_lsb( len ) );

		 pf->format_field( simple, ff_hex, &excess_bits, msg->position(1), len, "EXCESS", NULL, "Excess", NULL );
	}

	// close off the grouping
	pf->format_labelling_epilogue( msg->position(1), "EXCESS", "Excess message data" );
}
