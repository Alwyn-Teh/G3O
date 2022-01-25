/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */
/*+*******************************************************************

	Module Name:		g3ouifor.cxx (was ui_form.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		ProtocolFormat Implementation

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [443-1734-413655

	History:
		Who			When				Description
	-------------	-------------	-----------------------------
	Barry Scott		January 1995	Initial Creation
	Alwyn Teh		16 June 1995	Make format field width
									variable.
	Alwyn Teh		19 June 1995	Word wrap value field.
	Alwyn Teh		31 July 1995	Highlight description when
									decoding if required.

********************************************************************-*/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>

#include <iostream>
#include <iomanip.h>
#include <strstream.h>

#include <rw/cstring.h>
#include <rw/regexp.h>
#include <rw/ctoken.h>

#include <g3oproth.h>
#include <g3opforh.h>
#include <g3obitsh.h>

#include <tbf.h>
#include <atph.h>

extern "C" const char * Tbf_Video( uint8 video_code, const char *string );

//
//
//	IO Manipulators
//
//
typedef const char *stringRef;
IOMANIPdeclare(stringRef);

ostream& fixed( ostream& o, stringRef str )
{
	if ( (int)strlen( str ) > o.width() )
	  o. write( str, o.width(0) );
	else
	  o << str;

	return o;
}

OMANIP(stringRef) fixed( stringRef str )
{
	return OMANIP(stringRef)(fixed, str);
}

ostream& ditto( ostream& o, stringRef str )
{
	int width = o.width();
	int size = 0;

	// collect the ditto data in a string stream
	ostrstream ditto;
	char *text;

	while ( *str != 0 )
	{
		if ( *str == ' ' )
		{
		  ditto << setw(size/2) << "" << "\"" << setw(size - 1 - size/2) << "" << " ";
		  size = 0;
		}
		else
		  size++;

		str++;
	}

	if ( size > 0 )
	  ditto << setw(size/2) << "" << "\"" << setw(size - 1 - size/2) << "";

	ditto << ends;

	// get the address of the string - which we must delete
	text = ditto.str();

	o << text;

	delete text;

	return o;
}

OMANIP(stringRef) ditto( stringRef str )
{
	return OMANIP(stringRef)(ditto, str);
}

inline int min( int a, int b )
{
	if ( a < b )
	  return a;
	else
	  return b;
}

static const char *binary_string( int val, int start_pos, int val_size )
{
	static char bits[8+1];
	int first_char = (start_pos % 8);
	/* the width of this fields bits in this byte */

	strcpy( bits, "        " );

	for ( int bit = 0; bit < val_size; bit++, val >>= 1 )
	   bits[7-first_char - bit] = (val & 1) != 0 ? '1' : 'O';

	return bits;
}

//
//
//	ProtocolFormat
//
//

#define FORMAT_WIDTH 40	// default minimum width

ProtocolFormat::ProtocolFormat( ostream *stream )
{
	output = stream;
	group_depth = 0;
	vsprintf_bufptr = 0;
	_decode_required = 1;
	FormatWidth();
}

int ProtocolFormat::FormatWidth()
{
	char *column_env_str = getenv("COLUMNS");
	int colenv;

	// 132 columns xterm should be used, but check anyway.
	if (column_env_str != NULL && *column_env_str != '\0')
	  screen_width = ((colenv = atoi(column_env_str)) > 0) ? colenv : 132;
	else
	  screen_width = 132;

	// Centralize (-ish) the column of displayed bits.
	format_width = (screen_width / 2) - 16;

	// Ensure a minimum width of FORMAT_WIDTH.
	format_width = (format_width > FORMAT_WIDTH) ? format_width : FORMAT_WIDTH;

	return format_width;
}

int ProtocolFormat::protfmt_vsprintf(char *fmtstr, ...)
{
	// protfmt_vsprintf may be called by Atp_PrintfWordwrap() several times
	va_list ap;
	va_start(ap, fmtstr);

	int len = Atp_FormatStrlen( fmtstr, ap );
	char *new_string = new char[len+1];
	::vsprintf(new_string, fmtstr, ap);

	if ( vsprintf_bufptr == 0 )
	  vsprintf_bufptr = new RWCString;

	(*vsprintf_bufptr).append(new_string);

	delete[] new_string;

	va_end(ap);

	return len;
}

ProtocolFormat::~ProtocolFormat(void)
{ }

static char *grouping_start [4] =
{
	"   ",
	".--",
	"|.-",
	"||."
};

static char *grouping_end[4] =
{
	"   ",
	"`--",
	"|`-",
	"||`"
};

static char *line_prefix[4] =
{
	"   ",
	"|  ",
	"|| ",
	"|||"
};

void ProtocolFormat::format_field
(
	enum Protocol::field_type,		// the storage type of this field
	enum Protocol::field_format field_format,		// how to format this field
	bit_string *value,				// the value of this field
	int start_pos,					// the starting bit position of this field
	int value_size,					// the bit size of this field
	const char * /*ui_group*/,		// ui group name
	const char * /*variable_name*/,	// variable that holds this value
	const char *long_description,	// full description of this field
	const char *value_description,	// name of this value
	Protocol::CustomUIOps *custom_ops
)
{
	long orig_flags = output->flags();

	int val_width = min( value_size, 8-(start_pos % 8) );
	int val = value->fetch_lsb( val_width );

	FormatWidth();

	RWCString long_desc(long_description);
	RWCTokenizer next(long_desc);
	RWCString token, desc_line;
	int room_remaining = format_width - 3;
	int toklen = 0 ;
	int desc_index =0;

	token = next();
	while (!token.isNull())
	{
		if (room_remaining > (toklen = token.length()))
		{
		  desc_line.append (token);
		  desc_line.append(" ");
		  room_remaining -= (toklen+1);
		}
		else
		{
		  // Current token cannot fit in line, so continue onto next line.
		  desc_index = desc_line.length() - 1;
		  if ( isspace(desc_line[desc_index]) )
			desc_line.replace(desc_index, 1, "..."); // strip doesn't seem to work

		  *output << line_prefix[group_depth]
				  << setw(format_width) << fixed(desc_line.data()) << endl;

		  room_remaining = format_width - 3;
		  desc_line.remove(0);
		  desc_line.append(token); // don't loose current token!
		  desc_line.append(" ");
		  room_remaining -= (toklen+1);
		}
		token = next();
	}

	desc_index = desc_line.length() - 1;
	if ( isspace(desc_line[desc_index]) )
	  desc_line.replace(desc_index, 1, ""); // strip doesn't seem to work

	*output << line_prefix[group_depth]
			<< setw(format_width) << fixed(desc_line.data()) << " ";

	// Display appropriate number of bits
	if ( start_pos >= 0 )
	  *output << binary_string( val, start_pos, val_width ) << " ";
	else
	  // the case of a UIField
	  *output << "         ";

	value->set_position(0);

	switch( field_format )
	{
		case Protocol::ff_int:
				*output << "[" << dec << value->fetch_lsb( value_size ) << "] " << flush;
				break;
		case Protocol::ff_uint:
				*output << "[" << dec << (unsigned int)(value->fetch_lsb( value_size )) << "] " << flush;
				break;
		case Protocol::ff_hex:
		{
				*output << "[" << setiosflags(ios::showbase);
				for ( int i = 0; i < value_size; i += 8 )
				{
				   if ( i > 0 )
					 *output << " ";
				   *output << hex << value->fetch_lsb( min( 8, value_size-i ) );
				}
				*output << "]" << resetiosflags(ios::showbase) << flush;
		}
		break;
		case Protocol::ff_bcd:
		{
				*output << "Digits: [";
				for ( int i = 0; i < value_size; i += 4 )
				{
				   int digit = value->fetch_lsb( 4 );
				   *output << (char)(digit > 9 ? digit - 10 + 'A' : digit + '0' );
				}
				*output << "] " << flush;
		}
		break;
		case Protocol::ff_bit:
		{
				int i, j;
				*output << "[" << setiosflags(ios::showbase);
				for ( i = j = 0; i < value_size; i += 8, j++ )
				{
				   if ( i > 0 ) *output << " ";
				   if ( j > 0 && (j % 8) == 0 )
					 *output << endl << setw(format_width + 14) << "";
				   *output << hex << value->fetch_lsb( min( 8, value_size-i ) );
				}
				*output << "]" << resetiosflags(ios::showbase) << flush;
		}
		break;
		default:
			*output << value << flush;
	}

#if 0
extern int Atp_PrintfWordwrap
				_PROTO_((
						int (*printf_function) (char *fmtstr, ...) ,
												/* function to be used */
						int screen_width, /* will check COLUMN env if < 0 */
						int start_column, /* of 1st character, starts at 1 */
						int indent, /* starting column of subsequent lines */
						char *fcrmat_string,
						...	/* arg0, arg1, arg2, ... argn */
						)) ;
#endif

	if ( value_description )
	{
	  int start_column = format_width + 18;
	  int indent = start_column;
	  const char *value_desc = value_description;
	  int video_chars_length = 0;

	  if ( custom_ops != 0 && custom_ops->get_video_fx() && Tbf_GetSpecialVideo())
	  {
		value_desc = Tbf_Video(BOLD, value_description);
		video_chars_length = strlen(value_desc) - strlen(value_description);
	  }

	  Atp_PrintfWordwrap(ProtocolFormat::protfmt_vsprintf,
						 screen_width+video_chars_length, start_column, indent,
						 "%s", value_desc);

	  RWCString replacement("\n");			// for each newline character...
	  replacement += line_prefix[0];		// followed by 3 white spaces...
	  RWCRegexp reg(replacement.data());	// use regexp to locate first occurrence...
	  int len = strlen(line_prefix[0]);		// and replace with appropriate line_prefix
	  replacement.replace( 1, len, line_prefix[group_depth] ); // prepare correct replacement

	  size_t idx = 0;
	  len = replacement.length();
	  while ( ( idx = vsprintf_bufptr->index( reg, idx ) ) != RW_NPOS )
	  {
		  vsprintf_bufptr->replace( idx, len, replacement );
		  idx += 1;
	  }

	  *output << " " << *vsprintf_bufptr;

	  delete vsprintf_bufptr;
	  vsprintf_bufptr = 0;
	}
	*output << endl;

	// there is never any extra data on a UIField
	if ( start_pos < 0 )
	  return;

	value_size -= val_width;
	start_pos += val_width;
	value->set_position( val_width );

	FormatWidth();
	while ( value_size > 0 )
	{
		val_width = min( value_size, 8 - (start_pos % 8) );
		val = value->fetch_lsb( val_width );

		*output << line_prefix[group_depth] << setw(format_width) << ditto(long_description) << " "
				<< binary_string( val, start_pos, val_width ) << endl;

		value_size -= val_width;
		start_pos += val_width;
	}

	output->flags( orig_flags ); // reset
}

void ProtocolFormat::format_labelling_prologue( int , const char * /*ui_group*/, const char *long_description )
{
	group_depth++;
	FormatWidth();
	*output << grouping_start[group_depth]
			<< setw(format_width) << setfill('-') << long_description << " --------"
			<< setfill(' ') << endl;
}

void ProtocolFormat::format_labelling_epilogue( int , const char * /*group*/, const char * /*long_description*/ )
{
	FormatWidth();
	RWCString border('-', format_width + 8 + 1); // 8 characters for bit positions + 1 char space

	*output << grouping_end[group_depth] << border << endl;
	group_depth--;
}

void ProtocolFormat::format_error( const char *error_description )
{
	// clean up the group labelling
	while ( group_depth > 0 )
		 format_labelling_epilogue( 0, "", "" );
	FormatWidth();
	*output << "** " << setw(format_width) << fixed(error_description) << " ********" << endl;
}
