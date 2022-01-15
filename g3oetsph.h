/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3oetsph.h (was etsiisup.h)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		ETSI ISUP Protocol Header

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [44]-1734-413655

	History:
		Who			When				Description
	-----------	-------------	----------------------------
	Barry Scott	January 1995	Initial Creation

*******************************************************************-*/

#ifndef __ETSI_ISUP_H
#define __ETSI_ISUP_H

#include <g3oproth.h>

class ETSI_ISUP_Protocol : public Protocol
{
public:
		ETSI_ISUP_Protocol(void);
};

#endif /* __ETSI_ISUP_H */
