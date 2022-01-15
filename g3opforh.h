/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3opforh.h (was protform.h)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Class ProtocolFormat Declaration

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [44]-1734-413655

	History:
		Who			When				Description
	-----------	-------------	----------------------------
	Barry Scott	January 1995	Initial Creation
	Alwyn Teh	19 June 1995	Support word wrapping for
								value fields.

*******************************************************************-*/

#ifndef __TUP_PROTFORM_H
#define __TUP_PROTFORM_H

#include <g3opdbgh.h>

#include <iostream.h>
#include <iomanip.h>

#include <rw/cstring.h>
#include <g3oproth.h>

class ProtocolFormat
{
public:
		ProtocolFormat( ostream *output );
		virtual ~ProtocolFormat();

		virtual void format_error(const char *error_condition);

		virtual void format_labelling_prologue(int start_pos,
								const char *ui_group,	// the name of the grouping
								const char *long_description);

		virtual void format_labelling_epilogue(int start_pos,
								const char *ui_group,	// the name of the grouping
								const char *long_description);

		virtual void format_field(
						enum Protocol::field_type field_type,		// the storage type of this field
						enum Protocol::field_format field_format,	// how to format this field
						bit_string *value,							// the value of this field
						int start_pos,								// the starting bit position of this field
						int value_size,								// the bit size of this field
						const char	*ui_group,						// the name of the grouping
						const char	*variable_name,					// variable that holds this value
						const char	*long_description,				// full description of this field
						const char	*value_description,				// name of this value
						Protocol::CustomUIOps *custom_ops = 0);

		void decode_not_required( void )	{ _decode_required = 0; }
		void decode_required( void )		{ _decode_required = 1; }
		int is_decode_required( void )		{ return _decode_required; }

protected:
		int FormatWidth();
		int _decode_required;

		static int protfmt_vsprintf( char *fmtstr, ... );

public:
		int group_depth;

		ostream *output;

		int screen_width, format_width;

		static RWCString *vsprintf_bufptr;
};

//
//	IO manipulators
//
inline ostream& left( ostream& o )
{
		o.setf( ios::left, ios::adjustfield );
		return o;
}

inline ostream& right( ostream& o )
{
		o.setf( ios::right, ios::adjustfield );
		return o;
}

#endif /* __TUP_PROTFORM_H */
