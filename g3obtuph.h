/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3obtuph.h (was btup.h)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		BTUP Protocol Header

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [44]-1734-413655

	History:
		Who				When					Description
	------------	----------------	---------------------------
	Barry Scott		January	1995		Initial Creation
	Alwyn Teh		28 July 1995		Add	CIC filtering
	Alwyn Teh		31 July 1995		Add	video effects on H1
	Alwyn Teh		31 July 1995		Add	auto CIC feature

********************************************************************-*/

#ifndef __BTUP_PROTOCOL_H
#define __BTUP_PROTOCOL_H

#include <g3oproth.h>

class BTUP_Protocol : public Protocol
{
public:
		BTUP_Protocol(void);
		~BTUP_Protocol(void);

private:
		Protocol::CustomUIOps *cic_ops; // for activating cic filtering and auto cic
		Protocol::CustomUIOps *video_ops; // for highlighting HI description
};

#endif /* __BTUP_PROTOCOL_H */
