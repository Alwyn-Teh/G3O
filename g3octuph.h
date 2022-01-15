/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */
/*+*******************************************************************

	Module Name:		g3octuph.h (was ctup.h)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		CTUP Protocol Header

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [44]-1734-413655

	History:
		Who				When				Description
	------------	---------------	------------------------------
	Barry Scott		January	1995	Initial Creation
	Alwyn Teh		10 July 1995	Add	spcsize_vproc() and
									set_mtp_capability().
	Alwyn Teh		11 July 1995	Add	UICustomInit() in order
									to create "set_gcm_capability"
									command which is specific to
									CTUP.
	Alwyn Teh		24-28 July 95	Put SPC vproc and CIC filter
									stuff in custom ops objects.
	Alwyn Teh		31 July 1995	Add	auto CIC feature

********************************************************************-*/

#ifndef __CTUP_PROTOCOL_H
#define  CTUP_PROTOCOL_H

#include <atph.h>

#include <g3ouih.h>
#include <g3ouicmh.h>

#include <g3oproth.h>

#include <gal.h>

class set_gcm_capability_cmd;

class CTUP_Protocol : public Protocol
{
public:
		CTUP_Protocol(void);
		~CTUP_Protocol(void);

		// UI (ATP) vproc to ensure SPC 14 or 24 bits only
		// must be compatible with Atp_VprocType
		static char * spcsize_vproc( void *spcsize_ptr );

		virtual void UICustomInit( Genie3_cli *cli );

		static const char * set_mtp_capability( Genie3_cli *, const char *capability = 0 );

private:
		char * mtp_capability = 0; // CCITT_#7 or ANSI_#7

		set_gcm_capability_cmd *set_gcm_capability_cmd_ptr;

		Protocol::CustomUIOps *spcsize_ops; // to contain vproc
		Protocol::CustomUIOps *cic_ops; // for activating cic filtering and auto cic
		Protocol::CustomUIOps *video_ops; // for highlighting HI description
};

class set_gcm_capability_cmd : public Command
{
public:
		set_gcm_capability_cmd( Genie3_cli *, CTUP_Protocol * );
		Atp_Result cmd( int, char ** );

private:
		CTUP_Protocol	*protocol;
};

#endif /* ___CTUP_PROTOCOL_H */
