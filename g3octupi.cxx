/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52} -- OPEN */

/*+*******************************************************************

	Module Name:		g3octupi.cxx (was ctup_ini.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		CTUP Protocol Definition and Initialization

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [44]-1734-413655

	History:
		Who			When				Description
	-----------	--------------	---------------------------------------
	Barry Scott	February 1995	Initial Creation
	Alwyn Teh	2nd June 1995	Add missing SIO (but not sure
								if SLS is needed)
	Alwyn Teh	6-12 June 1995	Correctness and completeness
								checks. Fix wrong definitions,
								and add missing ones...etc.
								Use _desc strings to enforce
								naming consistency wherever
								reusable. Disambiguate names.
	Alwyn Teh	3rd July 1995	Put protocol-specific information
								banner in protocol object.
	Alwyn Teh	10th July 1995	Add vproc field to UIField in order
								that UI (ATP) can check SPCSIZE
								value.
	Alwyn Teh	19th July 1995	Package vproc in new CustomUIOps.
	Alwyn Teh	27th July 1995	Implement CIC filtering.
	Alwyn Teh	31st July 1995	Use video effects when decoding
								HI descriptions. (video_ops)
	Alwyn Teh	4 August 1995	Initialize Table to size of table
								in order to save space and unnecessary
								memory reallocations.
	Alwyn Teh	17 August 1995	Add per_link and invisible properties
								to header and msgtype.
	Alwyn Teh	28 Sept 1995	Fix range and status bug.

********************************************************************-*/

#include <ctype.h>

#include <g3octuph.h>

#include <g3ouih.h>
#include <g3opmdfh.h>

static char *bad_spcsize_desc = "Bad signalling point code size, should be 14 or 24 bits only";

CTUP_Protocol::CTUP_Protocol(void) : Protocol( "CTUP" )
{
//
//	Construct banner for CTUP protocol
//
banner.append(G3O_CCITT_CCS7);
banner.append("for the National Telephone Network of China");
banner.append("China Telephone User Part (CTUP)");
banner.append("Technical Specification GFQ01-9001 - August 1990");
banner.append("Ministry of Posts and Telecommunications (MPT) of");
banner.append("the People's Republic of China");
banner.append("based on CCITT Blue Book Rec. Q.721-Q.725");
banner.append ("");
banner.append(G3O_BNR_Copyright);

ProtocolDesc = banner.str();

// Miscellaneous initializations
set_gcm_capability_cmd_ptr = 0;
spcsize_ops = 0;
cic_ops = 0;
video_ops = 0;

//
//	=============================
//	Common Names and Descriptions
//	=============================
//
static char *msgtype_uigroup_name	= "MSGTYPE";
static char *msgtype_uigroup_desc	= "Message Type";
static char *msgtype_h0_varname		= "MSGTYPE:H0";
static char *msgtype_h1_varname		= "MSGTYPE:H1";

// Highlight when decoding
video_ops = new Protocol::CustomUIOps( this, "H1", Protocol::CustomUIOps::video_fx );

//
//	=============
//	Message Names
//	=============
//
static char *acb_desc = "Access Barred signal (ACB)";
static char *acc_desc = "Automatic Congestion Control Information Message (ACC)";
static char	*acm_desc =	"Address Complete Message (ACM)";
static char	*adi_desc =	"Address Incomplete signal (ADI)";
static char	*anc_desc =	"Answer signal, charge (ANC)";
static char	*ann_desc =	"Answer signal, no charge (ANN)";
static char	*anu_desc =	"Answer signal, unqualified (ANU)";
static char *bla_desc = "Blocking-acknowledgement signal (BLA)";
static char *blo_desc = "Blocking signal (BLO)";
static char *cbk_desc = "Clear-back signal (CBK)";
static char *ccf_desc = "Continuity-Failure signal (CCF)";
static char *ccl_desc = "Calling party clear signal (CCL)";
static char *ccr_desc = "Continuity-check-request signal (CCR)";
static char *cfl_desc = "Call-Failure signal (CFL)";
static char *cgc_desc = "Circuit-Group-Congestion signal (CGC)";
static char *chg_desc = "Charging Message (CHG)";
static char *clf_desc = "Clear-forward signal (CLF)";
static char *cot_desc = "Continuity Signal (COT)";
static char *dpn_desc = "Digital path not provided signal (DPN)";
static char *eum_desc = "Extended Unsuccessful Backward Set-up Information Message (EUM)";
static char *fot_desc = "Forward-transfer signal (FOT)";
static char *gra_desc = "Circuit Group Reset-Acknowledgement Message (GRA)";
static char *grq_desc = "General Request Message (GRQ)";
static char *grs_desc = "Circuit Group Reset Message (GRS)";
static char *gsm_desc = "General Forward Set-up Information Message (GSM)";
static char	*hba_desc =	"Hardware Failure Oriented Group Blocking-Acknowledgement Message (HBA)";
static char	*hgb_desc =	"Hardware Failure Oriented Group Blocking Message (HGB)";
static char	*hgu_desc =	"Hardware Failure Oriented Group Unblocking Message (HGU)";
static char	*hua_desc =	"Hardware Failure Oriented Group Unblocking-Acknowledgement	Message	(HUA)";
static char *iai_desc = "Initial Address Message with Additional Information (IAI)";
static char *iam_desc = "Initial Address Message (IAM)";
static char *los_desc = "Line-Out-of-Service signal (LOS)";
static char *mal_desc = "Malicious Call Identification signal (MAL)";
static char	*mba_desc =	"Maintenance Oriented Group	Blocking-Acknowledgement Message (MBA)";
static char	*mgb_desc =	"Maintenance Oriented Group	Blocking Message (MGB)";
static char	*mgu_desc =	"Maintenance Oriented Group	Unblocking Message (MGU)";
static char *mpm_desc = "Metering Pulse Message (MPM)";
static char *mua_desc = "Maintenance Oriented Group Unblocking-Acknowledgement Message (MUA)";
static char	*nnc_desc =	"National-Network-Congestion signal (NNC)";
static char	*opr_desc =	"Operator signal (OPR)";
static char	*ran_desc =	"Re-answer signal (RAN)";
static char	*rlg_desc =	"Release-guard signal (RLG)";
static char	*rsc_desc =	"Reset-circuit signal (RSC)";
static char	*sam_desc =	"Subsequent Address Message	with One or More Address Signals (SAM)";
static char	*sao_desc =	"Subsequent Address Message	with One Address Signal (SAO)";
static char *sba_desc = "Software Generated Group Blocking-Acknowledgement Message (SBA)";
static char	*sec_desc =	"Switching-Equipment-Congestion signal (SEC)";
static char	*sgb_desc =	"Software Generated Group Blocking Message (SGB)";
static char	*sgu_desc =	"Software Generated Group Unblocking Message (SGU)";
static char	*slb_desc =	"Subscriber Local-Busy signal (SLB)";
static char	*ssb_desc =	"Subscriber-Busy signal (SSB)";
static char	*sst_desc =	"Send-Special-Information-Tone signal (SST)";
static char	*stb_desc =	"Subscriber Toll-Busy signal (STB)";
static char	*sua_desc =	"Software Generated Group Unblocking-Acknowledgement Message (SUA)";
static char	*uba_desc =	"Unblocking-acknowledgement signal (UBA)";
static char	*ubl_desc =	"Unblocking signal (UBL)";
static char	*unn_desc =	"Unallocated-Number signal (UNN)";

//
//	================================
//	Parameter Names and Field Values
//	================================
//
static char *accinfo_desc		= "ACC information";
static char *acpi_desc			= "Additional Calling Party Information indicator";
static char *addr_desc			= "Address Signals";
static char *addr_digs_desc		= "Address signal digits";
static char *ari_desc			= "Additional Routing Information indicator";
static char *blockack_desc		= "Blocking acknowledgement";
static char *blocking_desc		= "Blocking";
static char *cci_desc			= "Continuity-Check indicator";
static char *cgpc_desc			= "Calling Party Category";
static char *cgpcreqi_desc		= "Calling Party Category Request indicator";
static char *cgpcrspi_desc		= "Calling Party Category (response type) indicator";
static char *chgi_desc			= "Charging Information indicator";
static char *chginfo_desc		= "Charging information";
static char *cic_desc			= "Circuit Identification Code";
static char *clias_desc			= "Calling Line Identity Address Signals";
static char *clii_desc			= "Calling Line Identity indicator";
static char *cliic_desc			= "Calling line identity included";
static char *clini_desc			= "Calling line identity not included";
static char *clireqi_desc		= "Calling Line Identity Request indicator";
static char *clirspi_desc		= "Calling Line Identity (response type) indicator";
static char *cugii_desc			= "Closed User Group Information indicator";
static char *digpi_desc			= "All-Digital-Path-Required indicator";
static char *echoincl_desc		= "Outgoing half echo suppressor included";
static char *echomi_desc		= "Outgoing Echo-Suppressor (message) indicator";
static char *echonincl_desc		= "Outgoing half echo suppressor not included";
static char *echor_desc			= "Echo Suppressor Request indicator";
static char *echorspi_desc		= "Outgoing Echo-Suppressor/Canceller (response type) indicator";
static char *eumspc14_desc		= "Originating EUM 14-bit Signalling Point Code";
static char *eumspc24_desc		= "Originating EUM 24-bit Signalling Point Code";
static char *holdi_desc			= "Hold indicator";
static char *holdreqi_desc		= "Hold Request indicator";
static char *icintlcalli_desc	= "Incoming	International Call indicator";
static char *icttxid_desc		= "Incoming	Trunk & Transit Exchange Identity";
static char *icttxidrspi_desc	= "Incoming	Trunk & Transit Exchange Identity (response	type) indicator";
static char *icttxidti_desc		= "Incoming	Trunk & Transit Exchange Identity Type indicator";
static char *intnum_desc		= "International number";
static char *localsubnum_desc	= "Local subscriber number";
static char *malcallreqi_desc	= "Malicious Call Identification (request type) indicator";
static char *malcallrspi_desc	= "Malicious Call Identification (response type) indicator";
static char *natsignum_desc 	= "National (significant) number";
static char *netcapi_desc		= "Network Capability or User Facility Information indicator";
static char *noai_desc			= "Nature-of-Address indicator";
static char *noas_desc			= "Number of address signals";
static char *noblock_desc		= "No blocking";
static char *noblockack_desc 	= "No blocking acknowledgement";
static char *noci_desc			= "Nature-of-Circuit indicator";
static char *nounblkack_desc 	= "No unblocking acknowledgement";
static char *nounblock_desc		= "No Unblocking";
static char *oca_desc			= "Original Called Address";
static char *ocai_desc			= "Original Called Address indicator";
static char *ocaincl_desc		= "Original called address included";
static char *ocanincl_desc		= "Original called address not included";
static char *ocareqi_desc		= "Original Called Address Request indicator";
static char *ocarspi_desc		= "Original Called Address (response type) indicator";
static char *ocas_desc			= "Original Called Address signals";
static char *range_desc			= "Range";
static char *range_field		= "range";
static char *rci_desc			= "Redirected-Call indicator";
static char *sigarea_desc		= "Signalling Area";
static char *sigarea_field		= "sig_area";
static char *sigmainarea_desc	= "Signalling Main Area";
static char *sigmainarea_field	= "sig_main_area";
static char *sigpathi_desc		= "Signalling Path indicator";
static char *sigpoint_desc		= "Signalling Point";
static char *sigpoint_field		= "sig_pt";
static char *sio_desc			= "Service Information Octet";
static char *sparenatuse_desc	= "Spare, reserved for national use";
static char *spcsize_desc		= "Signalling Point Code size";
static char *spcsize_uv_desc	= "SPCSIZE:size";
static char *status_desc		= "Status";
static char *status_field		= "status";
static char *transxidpcli_desc	= "Transit Exchange Identity (part of calling line identity)";
static char *transxidspc14_desc	= "Transit Exchange Identity (14-bit Signalling Point Code";
static char *transxidspc24_desc	= "Transit Exchange Identity (24-bit Signalling Point Code";
static char *unblkack_desc		= "Unblocking acknowledgement";
static char *unblocking_desc	= "Unblocking";

static char *spare_desc			= "Spare";

//
//	=========================
//	Service Information Octet
//	=========================
//
// preceeds the SIF - i.e. before the Label and Message Type (HO and HI)
//
//

LabelGroup SIO( sio_desc, "SIO" );
SIO = SIO +
(UIGroup( "SIO", sio_desc ) +
  Field( "sid", simple, ff_uint, 4, 0, 15, "Service Indicator", Table(16) +
	0	+ "Signalling Network Management Message" +
	1	+ "Signalling Network Testing & Maintenance Message" +
	2	+ spare_desc +
	3	+ "Signalling Connection Control Part (SCCP)" +
	4	+ "Telephone User Part (TUP)" +
	5	+ "ISDN User Part (ISUP)" +
	6	+ "Data User Part (Call & Circuit related Message) (DUP)" +
	7	+ "Data User Part (Facility Registration & Cancellation Message) (DUP)" +
	8-15 + spare_desc
  ) * Property( is_header ) +
  (LabelGroup( "Subservice Field", "SIO" ) +
	Field( "priority", simple, ff_uint, 2, "Priority of telephone message", Table(4) +
		0	+ spare_desc +
		1	+ spare_desc +
		2	+ spare_desc +
		3	+ spare_desc
	) * Property( is_header ) +
	Field( "netind", simple, ff_uint, 2, "Network Indicator", Table(4) +
		0	+ "International network" +
		1	+ "Spare (for international use only)" +
		2	+ "National network" +
		3	+ "Reserved for national use"
	) * Property( is_header )
  )
) * Property( is_header|per_link );

//
//	===========================
//	Circuit Identification Code
//	===========================
//
cic_ops = new Protocol::CustomUIOps( this, "CIC:code", Protocol::CustomUIOps::cic_filtering );
cic_ops->auto_cic_required( 1 );

UIGroup CIC( "CIC", cic_desc );
CIC = CIC * Property( is_header|per_link );
CIC = CIC + Field( "code", simple, ff_uint, 12, cic_desc, cic_ops ) * Property( is_header );

//
//	=====================
//	Parameter Definitions
//	=====================
//

//
//	===========================================
//	Calling Party Category for IAM, IAI and GSM
//	===========================================
//
LabelGroup CGPC( cgpc_desc, "CGPC" );
CGPC = CGPC +
(UIGroup( "CGPC", cgpc_desc ) +
  Field( "cgpc", simple, ff_uint, 6, cgpc_desc, Table(64) +
	0	+ "Unknown source" +
	1	+ "Operator, language French" +
	2	+ "Operator, language English" +
	3	+ "Operator, language German" +
	4	+ "Operator, language Russian" +
	5	+ "Operator, language Spanish" +
	6	+ "Language provided by mutual agreement (Chinese)" +
	7	+ "Language provided by mutual agreement" +
	8	+ "Language provided by mutual agreement (Japanese)" +
	9	+ "National operator (with offering facility)" +
	10	+ "Ordinary calling subscriber" + // used between toll (international)
		  // and to exchanges or toll (international) and local exchanges
	11	+ "Calling subscriber with priority" + // used between toll (international)
		  // and to exchanges or toll (international) and local exchanges or between
		  // local exchanges
	12	+ "Data call" +
	13	+ "Test call" +
	14-15 + spare_desc +
	16	+ "Ordinary, no charge" +
	17	+ "Ordinary, periodic" +
	18	+ "Ordinary, subscriber meter, immediate" +
	19	+ "Ordinary, printer, immediate" +
	20	+ "Priority, no charge" +
	21	+ "Priority, periodic" +
	22-23 + spare_desc +
	24 + "Ordinary, used between local exchanges" +
	25-63 + spare_desc
  ) +
  Field( 0, simple, ff_uint, 2, spare_desc ) // pads CGPC to 8 bits (not clear if really part of CGPC)
);

//
//	==================================
//	Message Indicators for IAM and IAI
//	==================================
//
LabelGroup MSG_IND( "Message Indicators" );
MSG_IND = MSG_IND +
(UIGroup( "NOAI", noai_desc ) +
  Field( "noai", simple, ff_uint, 2, noai_desc, Table(4) +
	0	+ localsubnum_desc +
	1	+ spare_desc +
	2	+ natsignum_desc +
	3	+ intnum_desc
  )
) +
(UIGroup( "NOCI", noci_desc ) +
  Field( "noci", simple, ff_uint, 2, noci_desc, Table(4) +
	0	+ "No satellite circuit in connection" +
	1	+ "Satellite circuit in connection" +
	2-3 + spare_desc
  )
) +
(UIGroup( "CCI", cci_desc ) +
  Field( "cci", simple, ff_uint, 2, cci_desc, Table(4) +
	0	+ "Continuity-check not required" +
	1	+ "Continuity-check required on this circuit" +
	2	+ "Continuity-check performed on a previous circuit" +
	3	+ spare_desc
  )
) +
(UIGroup( "ECHOI", echomi_desc ) +
  Field( "echoi", simple, ff_uint, 1, echomi_desc, Table(2) +
	0	+ echonincl_desc +
	1	+ echoincl_desc
  )
) +
(UIGroup( "ICINTLCI", icintlcalli_desc ) +
  Field( "icintlci", simple, ff_uint, 1, icintlcalli_desc, Table(2) +
	0	+ "Call other than international incoming" +
	1	+ "Incoming international call"
  )
) +
(UIGroup( "RCI", rci_desc ) +
  Field( "rci", simple, ff_uint, 1, rci_desc, Table(2) +
	0	+ "Not a redirected call" +
	1	+ "Redirected call"
  )
) +
(UIGroup( "ALLDIGPATHI", digpi_desc ) +
  Field( "alldigpathi", simple, ff_uint, 1, digpi_desc, Table(2) +
	0	+ "Ordinary call" +
	1	+ "Digital path required"
  )
) +
(UIGroup( "SIGPATHI", sigpathi_desc ) +
  Field( "sigpathi", simple, ff_uint, 1, sigpathi_desc, Table(2) +
	0	+ "Any path" +
	1	+ "All signalling system No.7 path"
  )
) +
Field( 0, simple, ff_uint, 1, spare_desc );

//
//	====================================
//	Address Signals for IAM, IAI and SAM
//	====================================!
//
LabelGroup ADDR_SIG( addr_desc );
ADDR_SIG = ADDR_SIG +
(UIGroup( "ADDRESS", addr_desc ) +
	Field( "num", length0, ff_uint, 4, noas_desc ) +
	Field( "digits", bcd_pad8, ff_bcd, 4, 0, 15, "ADDRESS:num", addr_digs_desc )
);

//
//	=====================================================
//	Calling Line Identity Address Signals for IAI and GSM
//	=====================================================
//
LabelGroup CLI_ADDR( clias_desc );
CLI_ADDR = CLI_ADDR +
(UIGroup( "CLI", clias_desc ) +
	Field( "num", length0, ff_uint, 4, "Number of calling address signals" ) +
	Field( "digits", bcd_pad8, ff_bcd, 4, 0, 15, "CLI:num", addr_digs_desc )
);

//
//	=====================================
//	Calling Line Identity for IAI and GSM
//	=====================================
//
LabelGroup CLI( "Calling Line Identity" );
CLI = CLI +
(LabelGroup( "Address Indicators" ) +
  (UIGroup( "CLINOAI", "Calling Line Identity Nature of Address indicator" ) +
	Field( "clinoai", simple, ff_uint, 2, noai_desc, Table(4) +
		0	+ localsubnum_desc +
		1	+ sparenatuse_desc +
		2	+ natsignum_desc +
		3	+ intnum_desc
	)
  ) +
  (UIGroup( "CLIPRESI", "Calling Line Identity Presentation indicator" ) +
	Field( "clipresi", simple, ff_uint, 1, "Calling line identity presentation indicator", Table(2) +
		0	+ "Calling line identity presentation not restricted" +
		1	+ "Calling line identity presentation restricted"
	)
  ) +
  (UIGroup( "CLIINCOMPI", "Incomplete Calling Line Identity indicator" ) +
	Field( "incompi", simple, ff_uint, 1, "Incomplete calling line identity indicator", Table(2) +
		0	+ "No indication" +
		1	+ "Incomplete calling line identity"
	)
  )
) +
CLI_ADDR;

//
//	=======================================
//	Original Called Address for IAI and GSM
//	=======================================
//
LabelGroup ORIG_CALLED_ADDR( oca_desc, "OCA" );
ORIG_CALLED_ADDR = ORIG_CALLED_ADDR +
(UIGroup( "OCA", oca_desc ) +
  (LabelGroup( "Address Indicators", "OCA" ) +
	Field( "noa", simple, ff_uint, 2, noai_desc, Table(4) +
		0	+ localsubnum_desc +
		1	+ sparenatuse_desc +
		2	+ natsignum_desc +
		3	+ intnum_desc
	) +
	Field( 0, simple, ff_uint, 2, spare_desc )
  ) +
  Field( "num", length0, ff_uint, 4, noas_desc ) +
  Field( "digits", bcd_pad8, ff_bcd, 4, 0, 15, "OCA:num", ocas_desc )
);

//
//	=============================
//	First Indicator Octet for IAI
//	=============================
//
LabelGroup FIO( "First Indicator Octet" );
FIO = FIO +
(UIGroup( "NETCAPI", netcapi_desc ) +
  Field( "netcapi", simple, ff_uint, 1, netcapi_desc, Table(2) +
	0	+ "Network capability or user facility information not included" +
	1	+ "Network capability or user facility information included"
  )
) +
(UIGroup( "CUGII", cugii_desc ) +
  Field( "cugii", simple, ff_uint, 1, cugii_desc, Table(2) +
	0	+ "Closed user group information not included" +
	1	+ "Closed user group information included"
  )
) +
(UIGroup( "ACPII", acpi_desc ) +
  Field( "acpii", simple, ff_uint, 1, acpi_desc, Table(2) +
	0	+ "Additional calling party information not included" +
	1	+ "Additional calling party information included"
  )
) +
(UIGroup( "ARII", ari_desc ) +
  Field( "arii", simple, ff_uint, 1, ari_desc, Table(2) +
	0	+ "Additional routing information not included" +
	1	+ "Additional routing information included"
  )
) +
(UIGroup( "CLII", clii_desc ) +
  Field( "clii", simple, ff_uint, 1, clii_desc, Table(2) +
	0	+ clini_desc +
	1	+ cliic_desc
  )
) +
(UIGroup( "OCAI", ocai_desc ) +
  Field( "ocai", simple, ff_uint, 1, ocai_desc, Table(2) +
	0	+ ocanincl_desc +
	1	+ ocaincl_desc
  )
) +
(UIGroup( "CHGI", chgi_desc ) +
  Field( "chgi", simple, ff_uint, 1, chgi_desc, Table(2) +
	0	+ "Charging information not included" +
	1	+ "Charging information included"
  )
) +
Field( 0, simple, ff_uint, 1, spare_desc );

//
//	================================
//	Response Type Indicators for GSM
//	================================
//
LabelGroup RSPTI( "Response Type Indicators" );
RSPTI = RSPTI +
(UIGroup( "CGPCRSPI", cgpcrspi_desc ) +
  Field( "cgpcrspi", simple, ff_uint, 1, cgpcrspi_desc, Table(2) +
	0	+ "Calling party category not included" +
	1	+ "Calling party category included"
  )
) +
(UIGroup( "CLIRSPI", clirspi_desc ) +
  Field( "clirspi", simple, ff_uint, 1, clirspi_desc, Table(2) +
	0	+ clini_desc +
	1	+ cliic_desc
  )
) +
(UIGroup( "ICTNTXIDRSPI", icttxidrspi_desc ) +
  Field( "ictntxidrspi", simple, ff_uint, 1, icttxidrspi_desc, Table(2) +
	0	+ "Incoming trunk & transit exchange identity not included" +
	1	+ "Incoming trank & transit exchange identity included"
  )
) +
(UIGroup( "OCARSPI", ocarspi_desc ) +
  Field( "ocarspi", simple, ff_uint, 1, ocarspi_desc, Table(2) +
	0	+ ocanincl_desc +
	1	+ ocaincl_desc
  )
) +
(UIGroup( "ECHORSPI", echorspi_desc ) +
  Field( "echorspi", simple, ff_uint, 1, echorspi_desc, Table(2) +
	0	+ echonincl_desc +
	1	+ echoincl_desc
  )
) +
(UIGroup( "MALCALLRSPI", malcallrspi_desc ) +
  Field( "malcallrspi", simple, ff_uint, 1, malcallrspi_desc, Table(2) +
	0	+ "Malicious call identification not provided" +
	1	+ "Malicious call identification provided"
  )
) +
(UIGroup( "HOLDRSPI", holdi_desc ) +
  Field( "holdrspi", simple, ff_uint, 1, holdi_desc, Table(2) +
	0	+ "Hold not provided" +
	1	+ "Hold provided"
  )
) +
Field( 0, simple, ff_uint, 1, spare_desc );

//
//	==================================================
//	Incoming Trunk & Transit Exchange Identity for GSM
//	==================================================
//
LabelGroup ICTNTXID( icttxid_desc );
ICTNTXID = ICTNTXID +
(UIGroup ("ICTNTXIDI", icttxidti_desc ) +
  Field ( "id_type", simple, ff_uint, 2, "Identity type indicator", Table(4) +
	0	+ spare_desc +
	1	+ "Signalling point code" +
	2	+ "Available part of calling line identity" +
	3	+ spare_desc
  ) +
  Field( 0, simple, ff_uint, 2, spare_desc )
) +
(SelectOne( "ICTNTXIDI:id_type" ) +
  1 +
  	  Field( 0, simple, ff_uint, 4, "Exchange identity length indicator" ) +
	  	  (SelectOne( spcsize_uv_desc, ff_uint, 0, spcsize_desc ) +
	  		14 +
				(UIGroup( "TXID_SPC14", transxidspc14_desc ) +
					(LabelGroup( transxidspc14_desc ) +
						Field( sigpoint_field, simple, ff_uint, 3, sigpoint_desc ) +
						Field( sigarea_field, simple, ff_uint, 8, sigarea_desc ) +
						Field( sigmainarea_field, simple, ff_uint, 3, sigmainarea_desc )
					)
				) +
			24 +
				(UIGroup( "TXID_SPC24", transxidspc24_desc ) +
					(LabelGroup( transxidspc24_desc ) +
						Field( sigpoint_field, simple, ff_uint, 8, sigpoint_desc ) +
						Field( sigarea_field, simple, ff_uint, 8, sigarea_desc ) +
						Field( sigmainarea_field, simple, ff_uint, 8, sigmainarea_desc )
					)
				)
	  	  ) +
  2 +
	(UIGroup( "TXID_PCLI", transxidpcli_desc ) +
		(LabelGroup( transxidpcli_desc ) +
			Field( "exch_id_length", length0, ff_uint, 4, "Exchange identity length indicator" ) +
			Field( "exch_id", bcd_pad8, ff_bcd, 4, 0, 15, "TXlD_PCLl:exch_id_length", "Transit exchange identity" )
		)
	)
) +
Field( 0, simple, ff_uint, 4, spare_desc ) +
(UIGroup( "ICTRUNKID", "Incoming Trunk Identity" ) +
	Field( "len", length0, ff_uint, 4, "Field length indicator" ) +
	Field( "id", octet, ff_hex, 8, 0, 15, "ICTRUNKID:len", "Incoming trunk identity" )
);

//
//	========================
//	Message Type Definitions
//	========================
//

//
//	==============================
//	Forward Address Messages (FAM)
//	==============================
//
Group H0_1; H0_1 = H0_1 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
  Field( "H1", simple, ff_uint, 4, "H1", video_ops, Table(4) +
	1	+ iam_desc +
	2	+ iai_desc +
	3	+ sam_desc +
	4	+ sao_desc
  )
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
  1 +
  	  (Message( "IAM", iam_desc ) +
		CGPC +
		MSG_IND +
		ADDR_SIG
  	  ) +
  2 +
  	  (Message( "IAI", iai_desc ) +
		CGPC +
		MSG_IND +
		ADDR_SIG +
		FIO +
		//				Fields not used (for further study)
		//				--------------------------------—--
		// Network Capability or User Facility Information (national only)
		// Closed User Group Information
		// Additional Calling Party Information
		// Additional Routing Information
		//
		(Group( "CLII:clii" ) +
			CLI
		) +
		(Group( "OCAI:ocai" ) +
			ORIG_CALLED_ADDR
		)
		// Charging Information - for further study
  	  ) +
  3 +
  	  (Message( "SAM", sam_desc ) +
  		Field( 0, simple, ff_uint, 4, "Filler" ) +
		ADDR_SIG
  	  ) +
  4 +
  	  (Message( "SAO", sao_desc ) +
  		(UIGroup( "SAO", sao_desc ) +
  			Field( "digit", simple, ff_uint, 4, "SAO Address Signal" ) +
			Field( 0, simple, ff_uint, 4, "Filler" )
  		)
  	  )
);

//
//	=============================
//	Forward Set-up Messages (FSM)
//	=============================
//
Group H0_2; H0_2 = H0_2 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
  Field( "H1", simple, ff_uint, 4, "H1", video_ops, Table(3) +
	1 + gsm_desc +
	3 + cot_desc +
	4 + ccf_desc
  )
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
	1 +
		(Message( "GSM", gsm_desc ) +
			RSPTI +
			(Group( "CGPCRSPI:cgpcrspi" ) +
				CGPC
			) +
			(Group( "CLIRSPI:clirspi" ) +
				CLI
			) +
			(Group( "ICTNTXIDRSPI:ictntxidrspi" ) +
				ICTNTXID
			) +
			(Group( "OCARSPI:ocarspi" ) +
				ORIG_CALLED_ADDR
			)
		) +
	3 +
		(Message( "COT", cot_desc )
		) +
	4 +
		(Message( "CCF", ccf_desc )
		)
);

//
//	==============================
//	Backward Set-up Messages (BSM)
//	==============================
//
Group H0_3; H0_3 = H0_3 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
  Field( "H1", simple, ff_uint, 4, "H1", video_ops, Table(1) +
	1 + grq_desc
  )
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
  1	+
  	  (Message( "GRQ", grq_desc ) +
  		(UIGroup( "CGPCREQI", cgpcreqi_desc ) +
  			Field( "cgpcreqi", simple, ff_uint, 1, cgpcreqi_desc, Table(2) +
				0	+ "No calling party category request" +
				1	+ "Calling party category request"
  			)
		) +
		(UIGroup( "CLIREQI", clireqi_desc ) +
			Field( "clireqi", simple, ff_uint, 1, clireqi_desc, Table(2) +
				0	+ "No calling line identity request" +
				1	+ "Calling line identity request"
			)
		) +
		(UIGroup( "OCAREQI", ocareqi_desc ) +
			Field( "ocareqi", simple, ff_uint, 1, ocareqi_desc, Table(2) +
				0	+ "No original called address request" +
				1	+ "Original called address request"
			)
		) +
		(UIGroup( "MALCALLREQI", malcallreqi_desc ) +
			Field( "malcallreqi", simple, ff_uint, 1, malcallreqi_desc, Table(2) +
				0	+ "No Malicious call identification encountered" +
				1	+ "Malicious call identification encountered"
			)
		) +
		(UIGroup( "HOLDREQI", holdreqi_desc ) +
			Field( "holdreqi", simple, ff_uint, 1, holdreqi_desc, Table(2) +
				0	+ "Hold not requested" +
				1	+ "Hold requested"
			)
		) +
		(UIGroup( "ECHOREQI", echor_desc ) +
			Field( "echoreqi", simple, ff_uint, 1, echor_desc, Table(2) +
				0	+ "No outgoing half echo suppressor requested" +
				1	+ "Outgoing half echo suppressor requested"
			)
		) +
		Field( 0, simple, ff_uint, 2, spare_desc )
  	  )
);

//
//	=====================================================
//	Successful Backward Set-up Information Messages (SBM)
//	=====================================================
//
Group H0_4; H0_4 = H0_4 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
  Field( "H1", simple, ff_uint, 4, "H1", video_ops, Table(2) +
	1	+ acm_desc +
	2	+ chg_desc
  )
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
  1 +
  	  (Message( "ACM", acm_desc ) +
  		(UIGroup( "ACM", "Address Complete" ) +
  			(LabelGroup( "Message Indicators", "ACM" ) +
  				Field( "acs", simple, ff_uint, 2, "Type of Address-Complete signal indicators", Table(4) +
					0	+ "Address Complete signal (ACM)" +
					1	+ "Address Complete signal, charge (ADC)" +
					2	+ "Address Complete signal, no charge (ADN)" +
					3	+ "Address Complete signal, coinbox (ADX)"
  				) +
				Field( "free", simple, ff_uint, 1, "Subscriber-free indicator", Table(2) +
					0	+ "No subscriber-free indication" +
					1	+ "Subscriber free (ADC->AFC, ADN->AFN or ADX->AFX)"
				) +
				Field( "echo", simple, ff_uint, 1, "Incoming echo suppressor/canceller indicator", Table(2) +
					0	+ "No incoming half echo suppressor included" +
					1	+ "Incoming half echo suppressor included"
				) +
				Field( "forward", simple, ff_uint, 1, "Call forwarding", Table(2) +
					0	+ "Call not forwarded" +
					1	+ "Call forwarded"
				) +
				Field( "path", simple, ff_uint, 1, "Signalling path", Table(2) +
					0	+ "Any path" +
					1	+ "All Signalling system No.7 path"
				) +
				Field( 0, simple, ff_uint, 2, spare_desc )
  			)
  		)
  	  ) +
  2	+
  	  (Message( "CHG", chg_desc ) +
  		(UIGroup( "CHG", chginfo_desc ) +
  			Field( "chg", octet, ff_hex, 8, 1, 0, chginfo_desc )
  		)
  	  )
);

//
//	=======================================================
//	Unsuccessful Backward Set-up Information Messages (UBM)
//	=======================================================
//
Group H0_5; H0_5 = H0_5 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
  Field( "H1", simple, ff_uint, 4, "H1", video_ops, Table(12) +
	1	+ sec_desc +
	2	+ cgc_desc +
	3	+ nnc_desc +
	4	+ adi_desc +
	5	+ cfl_desc +
	6	+ ssb_desc +
	7	+ unn_desc +
	8	+ los_desc +
	9	+ sst_desc +
	10	+ acb_desc +
	11	+ dpn_desc +
	15	+ eum_desc
  )
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
  1 +
  	  (Message( "SEC", sec_desc )
  	  ) +
  2 +
  	  (Message( "CGC", cgc_desc )
  	  ) +
  3 +
  	  (Message( "NNC", nnc_desc )
  	  ) +
  4 +
  	  (Message( "ADI", adi_desc )
  	  ) +
  5 +
  	  (Message( "CFL", cfl_desc )
  	  ) +
  6 +
  	  (Message( "SSB", ssb_desc )
  	  ) +
  7 +
  	  (Message( "UNN", unn_desc )
  	  ) +
  8 +
  	  (Message( "LOS", los_desc )
  	  ) +
  9 +
  	  (Message( "SST", sst_desc )
  	  ) +
  10 +
  	  (Message( "ACB", acb_desc )
  	  ) +
  11 +
  	  (Message( "DPN", dpn_desc )
  	  ) +
  15 +
  	  (Message( "EUM", eum_desc ) +
  		(UIGroup( "EUMOI", "EUM Octet indicator" ) +
  			Field( "unsuci", simple, ff_uint, 4, "Unsuccessful indicator", Table(16) +
				0	+ spare_desc +
				1	+ "Subscriber busy" +
				2-15 + spare_desc
  			) +
			Field( 0, simple, ff_uint, 4, spare_desc )
  		) +
		(SelectOne( spcsize_uv_desc, ff_uint, 0, spcsize_desc ) +
			14 +
				(UIGroup( "EUM_SPC14", eumspc14_desc ) +
					(LabelGroup( eumspc14_desc ) +
						Field( sigpoint_field, simple, ff_uint, 3, sigpoint_desc ) +
						Field( sigarea_field, simple, ff_uint, 8, sigarea_desc ) +
						Field( sigmainarea_field, simple, ff_uint, 3, sigmainarea_desc )
					)
				) +
			24 +
				(UIGroup( "EUM_SPC24", eumspc24_desc ) +
					(LabelGroup( eumspc24_desc ) +
						Field( sigpoint_field, simple, ff_uint, 8, sigpoint_desc ) +
						Field( sigarea_field, simple, ff_uint, 8, sigarea_desc ) +
						Field( sigmainarea_field, simple, ff_uint, 8, sigmainarea_desc )
					)
				)
		)
  	  )
);

//
//	===============================
//	Call Supervision Messages (CSM)
//	===============================
//
Group H0_6; H0_6 = H0_6 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
  Field( "H1", simple, ff_uint, 4, "H1", video_ops, Table(8) +
	0	+ anu_desc +
	1	+ anc_desc +
	2	+ ann_desc +
	3	+ cbk_desc +
	4	+ clf_desc +
	5	+ ran_desc +
	6	+ fot_desc +
	7	+ ccl_desc
  )
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
	0 +
		(Message( "ANU", anu_desc )
		) +
	1 +
		(Message( "ANC", anc_desc )
		) +
	2 +
		(Message( "ANN", ann_desc )
		) +
	3 +
		(Message( "CBK", cbk_desc )
		) +
	4 +
		(Message( "CLF", clf_desc )
		) +
	5 +
		(Message( "RAN", ran_desc )
		) +
	6 +
		(Message( "FOT", fot_desc )
		) +
	7 +
		(Message( "CCL", ccl_desc )
		)
);

//
//	==================================
//	Circuit Supervision Messages (CCM)
//	==================================
//
Group H0_7; H0_7 = H0_7 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
  Field( "H1", simple, ff_uint, 4, "H1", video_ops, Table(7) +
	1	+ rlg_desc +
	2	+ blo_desc +
	3	+ bla_desc +
	4	+ ubl_desc +
	5	+ uba_desc +
	6	+ ccr_desc +
	7	+ rsc_desc
  )
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
	1 +
		(Message( "RLG", rlg_desc )
		) +
	2 +
		(Message( "BLO", blo_desc )
		) +
	3 +
		(Message( "BLA", bla_desc )
		) +
	4 +
		(Message( "UBL", ubl_desc )
		) +
	5 +
		(Message( "UBA", uba_desc )
		) +
	6 +
		(Message( "CCR", ccr_desc )
		) +
	7 +
		(Message( "RSC", rsc_desc )
		)
);

//
//	========================================
//	Circuit Group Supervision Messages (GRM)
//	========================================
//
Group H0_8; H0_8 = H0_8 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
  Field( "H1", simple, ff_uint, 4, "H1", video_ops, Table(14) +
	1	+ mgb_desc +
	2	+ mba_desc +
	3	+ mgu_desc +
	4	+ mua_desc +
	5	+ hgb_desc +
	6	+ hba_desc +
	7	+ hgu_desc +
	8	+ hua_desc +
	9	+ grs_desc +
	10	+ gra_desc +
	11	+ sgb_desc +
	12	+ sba_desc +
	13	+ sgu_desc +
	14	+ sua_desc
  )
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
  1 +
  	  (Message( "MGB", mgb_desc ) +
  		(UIGroup( "MGB", mgb_desc ) +
  			Field( range_field, length1, ff_uint, 8, range_desc ) +
			Field( status_field, bit8, ff_bit, 1, 0, 256/8, "MGB:range", status_desc, Table(2) +
				0	+ noblock_desc +
				1	+ blocking_desc
			) * Property( add_one_to_control_value_except_zero )
  		)
  	  ) +
  2 +
  	  (Message( "MBA", mba_desc ) +
  		(UIGroup( "MBA", mba_desc ) +
  			Field( range_field, length1, ff_uint, 8, range_desc ) +
			Field( status_field, bit8, ff_bit, 1, 0, 256/8, "MBA:range", status_desc, Table(2) +
				0 + noblockack_desc +
				1 + blockack_desc
			) * Property( add_one_to_control_value_except_zero )
  		)
  	  ) +
  3 +
  	  (Message( "MGU", mgu_desc ) +
  		(UIGroup( "MGU", mgu_desc ) +
  			Field( range_field, length1, ff_uint, 8, range_desc ) +
			Field( status_field, bit8, ff_bit, 1, 0, 256/8, "MGU;range", status_desc, Table(2) +
				0	+ nounblock_desc +
				1	+ unblocking_desc
			) * Property( add_one_to_control_value_except_zero )
  		)
  	  ) +
  4 +
  	  (Message( "MUA", mua_desc ) +
  		(UIGroup( "MUA", mua_desc ) +
  			Field( range_field, length1, ff_uint, 8, range_desc ) +
			Field( status_field, bit8, ff_bit, 1, 0, 256/8, "MUA:range", status_desc, Table(2) +
				0	+ nounblkack_desc +
				1	+ unblkack_desc
			) * Property( add_one_to_control_value_except_zero )
  		)
  	  ) +
  5 +
  	  (Message( "HGB", hgb_desc ) +
  		(UIGroup( "HGB", hgb_desc ) +
			Field( range_field, length1, ff_uint, 8, range_desc ) +
			Field( status_field, bit8, ff_bit, 1, 0, 256/8, "HGB:range", status_desc, Table(2) +
				0	+ noblock_desc +
				1	+ blocking_desc
			) * Property( add_one_to_control_value_except_zero )
  		)
  	  ) +
  6 +
  	  (Message( "HBA", hba_desc ) +
  		(UIGroup( "HBA", hba_desc ) +
			Field( range_field, length1, ff_uint, 8, range_desc ) +
			Field( status_field, bit8, ff_bit, 1, 0, 256/8, "HBA:range", status_desc, Table(2) +
				0	+ noblockack_desc +
				1	+ blockack_desc
			) * Property( add_one_to_control_value_except_zero )
  		)
  	  ) +
  7 +
  	  (Message( "HGU", hgu_desc ) +
  		(UIGroup( "HGU", hgu_desc ) +
			Field( range_field, length1, ff_uint, 8, range_desc ) +
			Field( status_field, bit8, ff_bit, 1, 0, 256/8, "HGU:range", status_desc, Table(2) +
				0	+ nounblock_desc +
				1	+ unblocking_desc
			) * Property( add_one_to_control_value_except_zero )
  		)
  	  ) +
  8 +
  	  (Message( "HUA", hua_desc ) +
  		(UIGroup( "HUA", hua_desc ) +
  			Field( range_field, length1, ff_uint, 8, range_desc ) +
			Field( status_field, bit8, ff_bit, 1, 0, 256/8, "HUA:range", status_desc, Table(2) +
				0	+ nounblkack_desc +
				1	+ unblkack_desc
			) * Property( add_one_to_control_value_except_zero )
  		)
  	  ) +
  9 +
  	  (Message( "GRS", grs_desc ) +
  		(UIGroup( "GRS", grs_desc ) +
  			Field( range_field, length1, ff_uint, 8, range_desc )
  		)
  	  ) +
  10 +
  	  (Message( "GRA", gra_desc ) +
  		(UIGroup( "GRA", gra_desc ) +
  			Field( range_field, length1, ff_uint, 8, range_desc ) +
			Field( status_field, bit8, ff_bit, 1, 0, 256/8, "GRA:range", status_desc, Table(2) +
				0	+ "No Blocking for maintenance reasons" +
				1	+ "Blocking for maintenance reasons"
			) * Property( add_one_to_control_value_except_zero )
  		)
  	  ) +
  11 +
  	  (Message( "SGB", sgb_desc ) +
  		(UIGroup( "SGB", sgb_desc ) +
			Field( range_field, length1, ff_uint, 8, range_desc ) +
			Field( status_field, bit8, ff_bit, 1, 0, 256/8, "SGB:range", status_desc, Table(2) +
				0	+ noblock_desc +
				1	+ blocking_desc
			) * Property( add_one_to_control_value_except_zero )
  		)
  	  ) +
  12 +
  	  (Message( "SBA", sba_desc ) +
  		(UIGroup( "SBA", sba_desc ) +
			Field( range_field, length1, ff_uint, 8, range_desc ) +
			Field( status_field, bit8, ff_bit, 1, 0, 256/8, "SBA:range", status_desc, Table(2) +
				0	+ noblockack_desc +
				1	+ blockack_desc
			) * Property( add_one_to_control_value_except_zero )
  		)
  	  ) +
  13 +
  	  (Message( "SGU", sgu_desc ) +
  		(UIGroup( "SGU", sgu_desc ) +
			Field( range_field, length1, ff_uint, 8, range_desc ) +
			Field( status_field, bit8, ff_bit, 1, 0, 256/8, "SGU:range", status_desc, Table(2) +
				0	+ nounblock_desc +
				1	+ unblocking_desc
			) * Property( add_one_to_control_value_except_zero )
  		)
  	  ) +
  14 +
  	  (Message( "SUA", sua_desc ) +
  		(UIGroup( "SUA", sua_desc ) +
			Field( range_field, length1, ff_uint, 8, range_desc ) +
			Field( status_field, bit8, ff_bit, 1, 0, 256/8, "SUA:range", status_desc, Table(2) +
				0	+ nounblkack_desc +
				1	+ unblkack_desc
			) * Property( add_one_to_control_value_except_zero )
  		)
  	  )
);

//
//	==============================================
//	Circuit Network Management Message Group (CNM)
//	==============================================
//
Group H0_10; H0_10 = H0_10 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
  Field( "HI", simple, ff_uint, 4, "HI", video_ops, Table(1) +
	1 + acc_desc
  )
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
  1 +
  	  (Message( "ACC", acc_desc ) +
  		(LabelGroup( "Message Indicators" ) +
  			(UIGroup( "ACCMI", accinfo_desc ) +
  				Field( "acc_info", simple, ff_uint, 2, "ACC information (congestion levels)", Table(4) +
					0	+ spare_desc +
					1	+ "Congestion level 1" +
					2	+ "Congestion level 2" +
					3	+ spare_desc
  				) +
				Field( 0, simple, ff_uint, 6, spare_desc )
  			)
  		)
  	  )
);

//
//	==================================================
//	National Successful Backward Set-up Messages (NSB)
//	==================================================
//
Group H0_12; H0_12 = H0_12 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
  Field( "HI", simple, ff_uint, 4, "HI", video_ops, Table (1) +
	2 + mpm_desc
  )
) * Property( invisible ) +
(SelectOne ( msgtype_h1_varname ) +
  2 +
  	  (Message( "MPM", mpm_desc ) +
  		(UIGroup( "MPM_PDLSE", "Charging information" ) +
  			Field( "pulse", simple, ff_uint, 16, "Charging information (pulses per unit charging time)" )
  		)
  	  )
);

//
//	========================================
//	National Call Supervision Messages (NCB)
//	========================================
//
Group H0_13; H0_13 = H0_13 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
  Field( "H1", simple, ff_uint, 4, "H1", video_ops, Table(1) +
	1 + opr_desc
  )
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
  1 +
  	  (Message( "OPR", opr_desc )
  	  )
);

//
//	====================================================
//	National Unsuccessful Backward Set-up Messages (NUB)
//	====================================================
//
Group H0_14; H0_14 = H0_14 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
  Field( "H1", simple, ff_uint, 4, "H1", video_ops, Table(2) +
	1	+ slb_desc +
	2	+ stb_desc
  )
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
  1 +
	(Message( "SLB", slb_desc )
	) +
  2 +
	(Message( "STB", stb_desc )
	)
);

//
//	=================================
//	National-Area-Used Messages (NAM)
//	=================================
//
Group H0_15; H0_15 = H0_15 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
  Field( "HI", simple, ff_uint, 4, "HI", video_ops, Table(1) +
	1 + mal_desc
  )
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
  1 +
  	  (Message( "MAL", mal_desc )
  	  )
);

//
//	===============================
//	Protocol Definition (Top Level)
//	===============================
//
// Initialize SPCSIZE custom operations
spcsize_ops = new Protocol::CustomUIOps( this, spcsize_vproc ); // use this vproc when parsing
spcsize_ops->decode_with_default_uv( this, spcsize_uv_desc );	// use the default setting, if any, for decoding

prot = prot + SIO +
  (UIGroup( "SPCSIZE", "Signalling Point Code size (14 or 24 bits only)" ) +
	UIField( "size", simple, ff_uint, 5, 14, 24, 24, spcsize_desc, spcsize_ops,
		Table(11) +
			14 +	"14-bit signalling point code" +
			15-23 + bad_spcsize_desc +
			24 +	"24-bit signalling point code"
	) * Property( is_header )
  ) +
  (SelectOne( spcsize_uv_desc, ff_uint, 0, spcsize_desc ) +
		14 + // based on CCITT
			(LabelGroup( "40-bit Label", "LABEL" ) +
				(UIGroup( "DPC14", "Destination Point Code 14 bits") +
					(LabelGroup( "Destination Point Code" ) +
						Field( sigpoint_field, simple, ff_uint, 3, sigpoint_desc ) * Property( is_header )	+
						Field( sigarea_field, simple, ff_uint, 8, sigarea_desc ) * Property( is_header )	+
						Field( sigmainarea_field, simple, ff_uint, 3, sigmainarea_desc ) * Property( is_header )
					)
				) * Property( is_header|per_link ) +
				(UIGroup( "OPC14", "Originating Point Code 14 bits") +
					(LabelGroup( "Originating Point Code" ) +
						Field( sigpoint_field, simple, ff_uint, 3, sigpoint_desc ) * Property( is_header ) +
						Field( sigarea_field, simple, ff_uint, 8, sigarea_desc ) * Property( is_header ) +
						Field( sigmainarea_field, simple, ff_uint, 3, sigmainarea_desc ) * Property( is_header )
					)
				) * Property( is_header|per_link ) +
				CIC
			) +
		24 + // like ANSI but is CTUP GF' 001-9001
			(LabelGroup( "64-bit Label", "LABEL" ) +
				(UIGroup( "DPC24", "Destination Point Code 24 bits") +
					(LabelGroup( "Destination Point Code" ) +
						Field( sigpoint_field, simple, ff_uint, 8, sigpoint_desc ) * Property( is_header ) +
						Field( sigarea_field, simple, ff_uint, 8, sigarea_desc ) * Property( is_header ) +
						Field( sigmainarea_field, simple, ff_uint, 8, sigmainarea_desc ) * Property( is_header )
				)
			) * Property( is_header|per_link ) +
			(UIGroup( "OPC24", "Originating Point Code 24 bits") +
				(LabelGroup( "Originating Point Code" ) +
					Field( sigpoint_field, simple, ff_uint, 8, sigpoint_desc ) * Property( is_header ) +
					Field( sigarea_field, simple, ff_uint, 8, sigarea_desc ) * Property( is_header ) +
					Field( sigmainarea_field, simple, ff_uint, 8, sigmainarea_desc ) * Property( is_header )
				)
			) * Property( is_header|per_link ) +
			CIC + Field( 0, simple, ff_uint, 4, spare_desc ) * Property( is_header )
  )
) +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
  Field( "H0", simple, ff_uint, 4, "H0", Table(13) +
	1	+ "Forward Address Message group (FAM)" +
	2	+ "Forward Set-up Message group (FSM)" +
	3	+ "Backward Set-up Message group (BSM)" +
	4	+ "Successful Backward Set-up Information Message group (SBM)" +
	5	+ "Unsuccessful Backward Set-up Information Message group (UBM)" +
	6	+ "Call Supervision Message group (CSM)" +
	7	+ "Circuit Supervision Message group (CCM)" +
	8	+ "Circuit Group Supervision Message group (GRM)" +
	10	+ "Circuit Network Management Message group (CNM)" +
	12	+ "National Successful Backward Set-up Message group (NSB)" +
	13	+ "National Call Supervision Message group (NCB)" +
	14	+ "National Unsuccessful Backward Set-up Message group (NUB)" +
	15	+ "National Area Message group (NAM)"
  )
) * Property( invisible ) +
(SelectOne( msgtype_h0_varname ) +
	1	+ H0_1 +
	2	+ H0_2 +
	3	+ H0_3 +
	4	+ H0_4 +
	5	+ H0_5 +
	6	+ H0_6 +
	7	+ H0_7 +
	8	+ H0_8 +
	10	+ H0_10 +
	12	+ H0_12 +
	13	+ H0_13 +
	14	+ H0_14 +
	15	+ H0_15
);

//
//	=================================
//	Build the User Interface Database
//	=================================
//
	build_ui_database();
}

CTUP_Protocol::~CTUP_Protocol(void)
{
	if ( ProtocolDesc != 0 )
	{
	  delete (void *) ProtocolDesc;
	  ProtocolDesc = 0;
	}

	if ( set_gcm_capability_cmd_ptr != 0 )
	  delete set_gcm_capability_cmd_ptr;

	if ( spcsize_ops != 0 )
	  delete spcsize_ops;

	if ( cic_ops != 0 )
	  delete cic_ops;

	if ( video_ops != 0 )
	  delete video_ops;
}

//
//	===========================================
//	Protocol-specific Custom bits and pieces...
//	===========================================
//
char * CTUP_Protocol::spcsize_vproc( void *spcsize_ptr )
{
	unsigned int spcsize = 0;

	if ( spcsize_ptr != 0 )
	  spcsize = *(unsigned int *)spcsize_ptr;

	if ( spcsize == 14 || spcsize == 24 )
	  return 0;
	else
	  return bad_spcsize_desc;
}

void CTUP_Protocol::UICustomInit( Genie3_cli *cli )
{
	if ( cli == 0 )
	  return;

	set_gcm_capability_cmd_ptr = new set_gcm_capability_cmd( cli, this );
}

static char *ansi_capability	= "ANSI_#7";
static char *ccitt_capability	= "CCITT_#7";

const char * CTUP_Protocol::set_mtp_capability( Genie3_cli *ui, const char *capability )
{
	if ( capability == 0 || *capability == '\0' || strcasecmp("query", capability) == 0 ||
		(strcasecmp(capability, ansi_capability) != 0 && strcasecmp(capability, ccitt_capability) != 0) )
	  return (char *) ui->get_mtp_capability();

	char *cap = (char *) capability;
	for (int i = 0; cap[i] != 0; i++)
	   cap[i] = toupper( cap[i] );

	ui->set_mtp_capability( capability );
	Gal_SetCapability( (char *) ui->get_mtp_capability() );

	return ui->get_mtp_capability();
}

set_gcm_capability_cmd::set_gcm_capability_cmd( Genie3_cli *cli, CTUP_Protocol *p ) : Command( cli ), protocol( p )
{
	static Atp_KeywordType set_gcm_cap_keywords[] =
	{
		{ "ANSI_#7",	0, "Set GCM to ANSI CCS7 MTP capability" },
		{ "CCITT_#7",	1, "Set GCM to CCITT CCS7 MTP capability" },
		{ "query",		2, "Query current GCM CCS7 MTP capability" },
		{ 0 }
	};

	pd = new parmdef;

	pd->BeginParms();
	  pd->KeywordDef("cap", "Message Transfer Part Capability", 2, set_gcm_cap_keywords, 0);
	pd->EndParms();

	Create("set_gcm_capability", "Set MTP transport capability for GCM", Gal_GetHelpAreaResMgtId() );
}

Atp_Result set_gcm_capability_cmd::cmd( int, char ** )
{
	char *cap = Atp_Str("cap");

	const char *result = protocol->set_mtp_capability( ui, cap );

	ui->Tcl.SetResult( (char *)result, TCL_VOLATILE );

	return ATP_OK;
}
