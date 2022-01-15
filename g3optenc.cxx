/* EDITION AAO2 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */
/*+*****»*************************************************************

	Module Name:		g3optenc.cxx (was prot_enc.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Protocol Class Encoders

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [44]-1734-413655

	History:
		Who			When				Description
	-----------	--------------	-----------------------------------
	Barry Scott	January 1995	Initial Creation
	Alwyn Teh	17 July 1995	Extend user variables contents
								from bit_string to uv_info.
	Alwyn Teh	22 August 1995	Field_Ref::Encode(): handle ff_bit
								bit8 parameter (e.g. CTUP HUA:status)

*******************************************************************-*/

#include <assert.h>

#include <g3oproth.h>
#include <g3obitsh.h>
#include <g3ouvarh.h>

#include <rw/cstring.h>
#include <rw/hashdict.h>
#include <rw/ordcltn.h>

void Protocol::Encode( const char *msg_type_name, user_variables *uv, bit_string *msg )
{
	// release any stale pointer information
	pointer_info.clearAndDestroy();

	// find the values for encoding this message
	RWCollectableString s( msg_type_name );
	RWCollectable *c = msg_type_uservars.findValue( &s );
	user_variables *msg_type_vars = (user_variables *) c;

	if ( msg_type_vars == NULL )
	  // error somewhere throw an exception
	  throw UnknownMessageType();

#if _DEBUG_ENCODE
	cerr << "Encoding MSG: " << msg_type_name << " using variables " << endl;
	cerr << *uv << endl;
	cerr << " and msg specific variables " << endl;
	cerr << *msg_type_vars << endl;
#endif

	// initialise the msg_vars from the given uv
	user_variables msg_vars( uv );

	// merge in the values from the msg_type_uservars
	msg_vars += *msg_type_vars;

	// encode the message
	try {
		prot.ref->Encode( this, &msg_vars, msg );
	}
	catch ( user_variables::UndefinedVariable uv )
	{
		throw FieldNotDefined( uv.name );
	}
}

void Protocol:: Encode ( user_variables *uv, bit_string *msg, const char *ui_group_name )
{
	RWCollectableString group( ui_group_name );
	GroupInfo *info = (GroupInfo *)(group_info.findValue( &group ));

	if ( info == NULL )
	  throw InternalError( "Unknown UI Group %s", ui_group_name );

	// release any stale pointer information
	pointer_info.clearAndDestroy();

	// encode the message
	try {
		info->ui_group->Encode( this, uv, msg );
	}
	catch( user_variables::UndefinedVariable uv )
	{
		throw FieldNotDefined( uv.name );
	}
}

void Protocol::UIField_Ref::Encode( Protocol *, user_variables * uv, bit_string * )
{
#if _DEBUG_ENCODE
	cerr << "Encoding UIField( " << variable_name << ", " << long_description << ")" << endl;
#endif

	// if the user has not provided a value
	if ( !uv->exists( variable_name ) )
	{
	  bit_string value; value.append_lsb( bit_width, default_value );

	  // define the variable with the default value
	  uv->define( variable_name, value, bit_width, field_type, field_format, long_description );
	}

#if _DEBDG_ENCODE
	cerr << "Done Encoding UIField( " << variable_name << ", " << long_description << ")" << endl;
#endif
}

void Protocol::UIGroup_Ref::Encode( Protocol *p, user_variables * uv, bit_string * msg )
{
#if _DEBUG_ENCODE
	cerr << "Encoding UIGroup( " << variable_group << ", " << long_description << ")" << endl;
#endif

	//
	//	See if there is any override value provided
	//
	RWCString name = OverrideName(variable_group);

	if ( uv->exists( name ) )
	{
	  // append the override value
	  msg->append( uv->value( name )->uv_bits );
	}
	else
	  // generate the UIGroup from the user variables
	  for ( int m = 0; m < num_msgs; m++ )
		 msgs[m]->Encode( p, uv, msg );

#if _DEBUG_ENCODE
	cerr << "Done Encoding UIGroup( " << variable_group << ", " << long_description << ")" << endl;
#endif
}

void Protocol::OptionGroup_Ref::Encode( Protocol *p, user_variables * uv, bit_string * msg )
{
#if _DEBUG_ENCODE
	cerr << "Encoding OptionGroup( " << variable_group << ", " << long_description << ")" << endl;
#endif

	//
	//	See if this optional group is required
	//
	RWCString name = OptionName(variable_group);

	if ( uv->exists( name ) )
	  if ( uv->value( name, 0 ) )
		// generate the OptionGroup from the user variables
		for ( int m = 0; m < num_msgs; m++ )
		   msgs[m]->Encode( p, uv, msg );

#if _DEBUG_ENCODE
	cerr << "Done Encoding OptionGroup( " << variable_group << ", " << long_description << ")" << endl;
#endif
}

void Protocol::Message_Ref::Encode( Protocol *p, user_variables * uv, bit_string * msg )
{
#if _DEBUG_ENCODE
	cerr << "Encoding Message(" << variable_field << ", " << long_description << ")" << endl;
#endif

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->Encode( p, uv, msg );

#if _DEBUG_ENCODE
	cerr << "Done Encoding Message(" << variable_field << ", " << long_description << ")" << endl;
#endif
}

void Protocol::LabelGroup_Ref::Encode( Protocol *p, user_variables * uv, bit_string * msg )
{
#if _DEBUG_ENCODE
	cerr << "Encoding LabelGroup(" << long_description << ")" << endl;
#endif

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->Encode( p, uv, msg );

#if _DEBUG_ENCODE
	cerr << "Done Encoding LabelGroup(" << long_description << ")" << endl;
#endif
}

void Protocol::Group_Ref::Encode( Protocol *p, user_variables * uv, bit_string * msg )
{
#if _DEBUG_ENCODE
	cerr << "Encoding Group( "
		 << variable_name << ", "
		 << long_description << ")" << endl;
#endif

	if ( !controlled_by.isNull () )
	{
	  if ( !uv->exists( controlled_by ) )
		throw InternalError( "Group::Encode failed to find controlled_by variable %s", controlled_by );

	  int indicator = uv->value( controlled_by, 0 );
	  if ( !indicator )
		return;
	}

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->Encode( p, uv, msg );

#if _DEBUG_ENCODE
	cerr << "Done Encoding Group( "
		 << variable_name << ", "
		 << long_description << ")" << endl;
#endif
}

void Protocol::LengthGroup_Ref::Encode( Protocol *p, user_variables * uv, bit_string * msg )
{
#if _DEBUG_ENCODE
	cerr << "Encoding LengthGroup( "
		 << variable_name << ", "
		 << long_description << ")" << endl;
#endif

	int start = msg->length(1);

	// insert a length of 0
	msg->append_lsb( bit_width, 0 );

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->Encode( p, uv, msg );

	// now that we know the length of group plug in the correct length
	msg->replace_lsb( start, bit_width, (msg->length(1) - start - 8)/8 );

#if _DEBDG_ENCODE
	cerr << "Done Encoding LengthGroup( "
		 << variable_name << ", "
		 << long_description << ")" << endl;
#endif
}

void Protocol::SelectOne_Ref::Encode( Protocol *p, user_variables * uv, bit_string * msg )
{
#if _DEBUG_ENCODE
	cerr << "Encoding SelectOne( "
		 << variable_name << ", "
		 << long_description << ")" << endl;
#endif

	int value = uv->value( variable_name, bit_width );

	// output the select value
	msg->append_lsb( bit_width, value );

	for ( int m = 0; m < num_msgs; m++ )
	   if ( value == msg_types[m] )
	   {
		 msgs[m]->Encode( p, uv, msg );
		 break;
	   }

// it's possible to get here without finding a message to encode
// which is handled by our caller
#if _DEBUG_ENCODE
	cerr << "Done Encoding SelectOne( "
		 << variable_name << ", "
		 << long_description << ")" << endl;
#endif
}

void Protocol::Field_Ref::Encode( Protocol *p, user_variables * uv, bit_string * msg )
{
#if _DEBUG_ENCODE
	cerr << "Encoding Field( "
		 << variable_name << ", "
		 << dec << bit_width << ", "
		 << long_description << ")" << endl;
#endif

	//
	//	deal with pointers and markers first
	//
	switch( field_type )
	{
		case pointer:
		{
			p->pointer_info.insertKeyAndValue(	new RWCollectableString( variable_name ),
												new pointer_details( msg->length(1), bit_width ));
			msg->append_lsb( bit_width, 0 );
			return;
		}
		case marker:
		{
			RWCollectableString control_name( controlled_by );
			pointer_details *ptr = (pointer_details *)p->pointer_info.findValue( &control_name );

			if ( ptr == NULL )
			  throw InternalError( "Pointer not found for marker %s", controlled_by );

			msg->replace_lsb( ptr->Start(), ptr->Width(), (msg->length(1) - ptr->Start())/8 );
			return;
		}
		default:
			break;
	}

	// see if there is nothing to do
	if ( bit_width == 0 )
	  return;

	bit_string b;

	//
	// if this field is controlled, get its size
	//
	int control_size = 0;
	switch( field_type )
	{
		case bcd_odd_indicator:
			// if there is a variable with a value use that
			// as an override to the calculated length
			if ( variable_name && uv->exists( variable_name ) )
			  control_size = uv->value( variable_name, 0 );
			else
			if ( !controlled_by.isNull() )
			{
			  if ( !uv->exists( controlled_by ) )
				throw InternalError( "Field::Encode failed to find controlled_by variable %s", controlled_by );

			  // find the length in digits
			  control_size = uv->value( controlled_by )->uv_bits->length(4);
			}

			// set true if an odd number of digits
			control_size &= 1;
			break;

		case bcd_to_end:
			//
			// Find the bits of the value
			//
			// only fetch value if there is a name
			if ( !variable_name.isNull() )
			{
			  bit_string *v = uv->value( variable_name )->uv_bits;
			  b = *v;
			  b.set_position(0);
			}
			else
			  // default to a zero value
			  b.append_lsb( bit_width, 0 );

			// work out the length in bcd digits
			control_size = b.length( 4 );
			break;

		default:
			if ( !controlled_by.isNull () )
			{
			  if ( !uv->exists( controlled_by ) )
				throw InternalError( "Field::Encode failed to find controlled_by variable %s", controlled_by );

			  control_size = uv->value( controlled_by, 0 );
			}
			//
			// Find the bits of the value
			//
			// only fetch value if there is a name
			if ( !variable_name.isNull() )
			{
			  bit_string *v = uv->value( variable_name )->uv_bits;
			  b = *v;
			  b.set_position(0);
			}
			else
			  // default to a zero value
			  b.append_lsb( bit_width, 0 );
	}

	try
	{
		switch( field_type )
		{
			// simple length count from 0 to n
			case length0:
			{
				int length = b.fetch_lsb( bit_width );
				msg->append_lsb( bit_width, length );
			}
			break;

			// complex length count 0,2 to n maps to 0,1 to n-1
			case length1:
			{
				int length = b.fetch_lsb( bit_width );

				// disused by using add_one_to_control_value on status, shouldn't muck about with range value
				// if ( length == 1 )
				//	 throw InternalError("Field::Encode Cannot handle length1 field with value 1");
				// if ( length > 0 )
				//	 length--;	// move 2 to n onto 1 to n-1

				msg->append_lsb( bit_width, length );
			}
			break;

			case bcd:
			{
				// deal with fixed size bed fields
				if ( controlled_by.isNull() )
				  control_size = bit_width/4;

				// copy each BCD digit
				for ( ; control_size > 0; control_size-- )
				   msg->append_lsb( 4, b.fetch_lsb( 4 ) );
			}
			break;

			case bcd_to_end:	// as per pad8
			case bcd_pad8:
			{
				// deal with fixed size bed fields
				if ( controlled_by.isNull() )
				  control_size = bit_width/4;

				// copy each BCD digit
				for ( ; control_size > 0; control_size-- )
				   msg->append_lsb( 4, b.fetch_lsb( 4 ) );

				// see if we need to align
				if ( (msg->length(1) % 8) != 0 )
				  msg->append_lsb( 4, 0 ); // add the padding
			}
			break;

			case octet:
			{
				// deal with fixed size octet fields
				if ( controlled_by.isNull() )
				  control_size = bit_width/8;

				// copy each octet
				for ( ; control_size > 0; control_size-- )
				   msg->append_lsb( 8, b.fetch_lsb( 8 ) );
			}
			break;

			case bcd_odd_indicator:
				msg->append_lsb( bit_width, control_size );
				break;

			case type_code:
			// insert the type code held in the min_value field literally
			msg->append_lsb( bit_width, min_valid );
			break;

			case bit8:
			{
				if ( controlled_by.isNull() ) // not controlled by another field, encode all bits
				{
				  msg- >append ( &b ) ;
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
					return; // control_size is zero, nothing to encode
				}
			}
			break;

			default:
			{
				int value = b.fetch_lsb( bit_width );
				msg->append_lsb( bit_width, value );
			}
			break;
		}
	}
	catch( bit_string::FetchTooMuch )
	{
		throw FieldTooSmall( variable_name );
	}
}

