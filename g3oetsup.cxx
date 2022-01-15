/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3oetsup.cxx (was etsi_ini.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		ETSI ISUP Protocol Definition and Initialization

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [443-1734-413655

	History:
		Who				When				Description
	-----------		------------	----------------------------
	Barry Scott		January 1995	Initial Creation

*******************************************************************-*/

#include <g3oetsph.h>

ETSI_ISUP_Protocol::ETSI_ISUP_Protocol(void) : Protocol ()
{
	prot =	prot +
			(UIGroup( "LABEL", "Label size" ) +
				Field( "size", simple, ff_uint, 0, 14, 24, "Label size" )) +
			(SelectOne( "LABEL:size", ff_uint, 0, "Label size" ) +
				14 +
					(LabelGroup( "Label" ) +
						(UIGroup( "DPC14", "Destination Point Code 14 bit") +
							(LabelGroup( "Destination Point Code" ) +
								Field( "network", simple, ff_uint, 3, "Network" ) +
								Field( "cluster", simple, ff_uint, 8, "Cluster" ) +
								Field( "member", simple, ff_uint, 3, "Member" )
							)
						) +
						(UIGroup( "OPC14", "Origination Point Code 14 bit") +
							(LabelGroup( "Origination Point Code" ) +
								Field( "network", simple, ff_uint, 3, "Network" ) +
								Field( "cluster", simple, ff_uint, 8, "Cluster" ) +
								Field( "member", simple, ff_uint, 3, "Member" )
							)
						) +
						(UIGroup( "CIC", "circuit identifier" ) +
							Field( "cic", simple, ff_uint, 12, "circuit identifier" ))
					) +
				24 +
					(LabelGroup( "Label" ) +
						(UIGroup( "DPC24", "Destination Point Code 24 bit") +
							(LabelGroup( "Destination Point Code" ) +
								Field( "network", simple, ff_uint, 8, "Network"	)	+
								Field( "cluster", simple, ff_uint, 8, "Cluster"	)	+
								Field( "member", simple, ff_uint, 8, "Member" )
							)
						) +
						(UIGroup( "0PC24", "Origination Point Code 24 bit") +
							(LabelGroup( "Origination Point Code" ) +
								Field( "network", simple, ff_uint, 8, "Network"	)	+
								Field( "cluster", simple, ff_uint, 8, "Cluster"	)	+
								Field( "member", simple, ff_uint, 8, "Member" )
							)
						) +
						(UIGroup( "CIC", "circuit identifier" ) +
							Field( "cic", simple, ff_uint, 12, "circuit identifier" )) +
							Field( 0, simple, ff_uint, 4, "Spare" )
					)
			) +
			(Message( "TEST", "A test message" ) +
				(UIGroup( "fred", "Fred stuff" ) +
					Field( "pointer_a", pointer, ff_uint, 8, "Pointer" ) +
					(OptionGroup( "fred", "parameter fred" ) +
						TypeCode( "param type code", 34 ) +
						(LengthGroup( "len", 8, "Parameter length" ) +
							Field( "a", bcd_odd_indicator, ff_uint, 1, "fred:bcd", "Field a" ) +
							Field( "b", simple, ff_uint, 7, "Field b" ) +
							Field( "c", simple, ff_uint, 8, "Field c" ) +
							Field( "bcd", bcd_to_end, ff_bcd, 4, 0, 16, "fred:a", "Field BCD" )
						)
					) +
					Field( 0, marker, ff_uint, 0, "fred:pointer_a", "Marker for pointer a" ) +
					Field( "d", simple, ff_uint, 8, "Field d" )
				)
			);

	build_ui_database();
}
