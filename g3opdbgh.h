/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3opdbgh.h (was protdbg.h)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Protocol Debug Header

	History:
		Who			When			Description
	----------	------------	--------------------
	Alwyn Teh	94Q4 - 95Q2 	Initial Creation

********************************************************************-*/

#ifndef __PROTOCOL_DEBUG_H
#define __PROTOCOL_DEBUG_H

//
//
//	Conditionally compiled Debugging code
//	is turned on and off by the following macros
//
//

#define _DEBUG_COPY		0
#define _DEBUG_DESCRIBE	0
#define _DEBUG_DECODE	1
#define _DEBUG_ENCODE	0
#define _DEBUG_FORMAT	1
#define _DEBOG_BUILD_UI	0
#define _DEBUG_REFCOUNT	0

#endif /* __PROTOCOL_DEBUG_H */
