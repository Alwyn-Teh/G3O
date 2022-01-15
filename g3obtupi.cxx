/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+***»***************************************************************

	Module Name:		g3obtupi.cxx (was btup_ini.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		BTUP V3 Protocol Definition, and Initialization
						BTNR 167 Section 3 Issue 3 July 1987

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [441-1734-413655

	History:
		Who			When				Description
	-----------	---------------	------------------------------------
	Barry Scott	January	1995	Initial	Creation
	Alwyn Teh	16-30 June 1995	Double-check against spec...
								Tidy up, corrections ... etc.
	Alwyn Teh	3rd July 1995	Put protocol-specific information
								banner in protocol object.
	Alwyn Teh	28 July	1995	Add CIC	filtering
	Alwyn Teh	31 July	1995	Add video effects on H1 for
								decoding.
	Alwyn Teh	31 July 1995	Add auto CIC and disp_header
								commands.
	Alwyn Teh	4 August 1995	Initialize FIC table to 65536
								entries using new DArray code.
								Other tables may also do so
								to save space and save time
								by cutting down realloc()s.
	Alwyn Teh	15 August 1995	Support decode levels for header
								by using Property( is_header ).
	Alwyn Teh	17 August 1995	Add per-link and invisible
								properties to header and msgtype.
	Alwyn Teh	21 August 1995	Implement RELR as optional trailer.
	Alwyn Teh	28 Sept 1995	Fix range and status bug.

*******************************************************************-*/

#include <g3obtuph.h>

// See BTNR190 for SIC codings
#define SIC_OK 0

BTUP_Protocol::BTUP_Protocol(void) : Protocol( "BTUP v3" )
{
//
//	Construct banner for BTUP protocol
//
banner.append(G3O_CCITT_CCS7);
banner.append("British Telecom National Telephone User Part");
banner.append("BTUP Version 3 ") ;
banner.append("BTNR 167 Section 3 Issue 3, July 1987");
banner.append("");
banner.append(G3O_BNR_Copyright);

ProtocolDesc = banner.str();

//
//  =============================
//	Common Names and Descriptions
//	=============================
//
static char *msgtype_uigroup_name = "MSGTYPE";
static char *msgtype_uigroup_desc = "Message Type";
static char *msgtype_h0_varname = "MSGTYPE:H0";
static char *msgtype_h1_varname = "MSGTYPE:H1";
static char *msgtype_H0	= "H0";
static char *msgtype_H1	= "H1";

video_ops = new Protocol::CustomUIOps( this, msgtype_h1_varname, Protocol::CustomUIOps::video_fx );

//	=============
//	Message Names
//	=============

// Message Group (Heading Codes HO) Names:
static char *fwd_addr_msgs			= "Forward Address Messages (Group 0)";
static char *fwd_setup_msgs			= "Forward Set-Up Messages (Group 1)";
static char *bwd_setup_req_msgs		= "Backward Set-Up Request Messages (Group 2)";
static char *bwd_setup_info_msgs	= "Backward Set-Up Information Messages (Group 3)";
static char *call_supv_msgs			= "Call Supervision Messages (Group 4)";
static char *circ_supv_msgs			= "Circuit Supervision Messages (Group 5)";
static char *nn2n_serv_info_msgs	= "Non End-To-End Service Information Messages (Group 6)";
static char *n2n_serv_info_msgs		= "End-To-End Service Information Messages (Group 7)";

// Group 0 - Forward Address Messages
static char *iam_desc	= "Initial Address Message (IAM)";				//	H1 = 0
static char *ifam_desc	= "Initial and Final Address Message (IFAM)";	//	H1 = 1
static char *sam_desc	= "Subsequent Address Message (SAM)";			//	H1 = 2
static char *fam_desc	= "Final Address Message (FAM)";				//	H1 = 3
static char *spare_desc	= "Spare";										//	H1 = 4-255

// Group 1 - Forward Set-Up Messages
static char *intra_exch_use_only = "Intra-exchange use only";				// H1 = 0-3
static char *asui_desc	= "Additional Set-Up Information Message (ASUI)";	// H1 = 4
//			spare_desc														   H1 = 5-255

// Group 2 - Backward Set-Up Request Messages
//			 intra_exch_use_only													   H1 = 0-1
static char *snd_desc	= "Send 'N' Digits Message (SND)";							// H1 = 2
static char *sad_desc	= "'Send All Digits' Message (SAD)";						// H1 = 3
static char *sasui_desc	= "\"Send Additional Set-Up Information\" Message (SASUI)";	// H1 = 4
//			spare_desc																   H1 = 5-255

// Group 3 - Backward Set-Up Information Messages
static char *acm_desc				= "Address Complete (Number Received) Message (ACM)";	// H1 = 0
//			intra_exch_use_only													   			   H1 = 1
static char *cong_desc				= "Congestion Message (CONG)";							// H1 = 2
static char *tcong_desc				= "Terminal Congestion Message (TCONG)";				// H1 = 3
static char *cna_desc				= "Connexion Not Admitted Message (CNA)";				// H1 = 4
static char *rpta_desc				= "Repeat Attempt Message (RPTA)";						// H1 = 5
static char *seng_desc				= "Subscriber Engaged Message (SENG)";					// H1 = 6
static char *sooo_desc				= "Subscriber Out of Order Message (SOOO)";				// H1 = 7
static char *stfr_desc				= "Subscriber Transferred Message (STFR)";				// H1 = 8
static char *reserved_non_bt_use	= "Reserved for non-BT use";							// H1 = 9
static char *cdb_desc				= "Call Drop Back Message (CDB)";						// H1 = 10
//			spare_desc																		   H1 = 11-255

// Group 4 - Call Supervision Messages
static char *anm_desc	= "Answer Message (ANM)";					// H1 = 0
static char *clr_desc	= "Clear Message (CLR)";					// H1 = 1
static char *ran_desc	= "Re-answer Message (RAN)";				// H1 = 2
static char *rel_desc	= "Release Message (REL)";					// H1 = 3
static char *cfc_desc	= "Coin and Free Checking Message (CFC)";	// H1 = 4
static char *oor_desc	= "Operator Override Message (OOR)";		// H1 = 5
static char *hwlr_desc	= "Howler Message (HWLR)";					// H1 = 6
static char *extc_desc	= "Extend Call Message (EXTC)";				// H1 = 7
//			intra_exch_use_only										   H1 = 8
//			reserved_non_bt_use										   H1 = 9-13
//			spare_desc												   H1 = 14-255

// Group 5 - Circuit Supervision Messages
//		Circuit Related Supervision Messages:
static char *ctf_desc	= "Circuit Free Message (CTF)";					// H1 = 0
static char	*blo_desc	= "Blocking Message (BLO)";						// H1 = 1
static char	*ubl_desc	= "Unblocking Message (UBL)";					// H1 = 2
static char	*bla_desc	= "Blocking Acknowledgement Message (BLA)";		// H1 = 3
static char	*uba_desc	= "Unblocking Acknowledgement Message (UBA)";	// H1 = 4
static char	*ovld_desc	= "Overload Message (UVLD)";					// H1 = 5
//			intra_exch_use_only											   H1 = 6-10

// Circuit Group Blocking and Unblocking Messages:
static char	*cgb_desc	= "Circuit Group Blocking Message (CGB)";						// H1 = 11
static char	*cgu_desc	= "Circuit Group Unblocking Message (CGU)";						// H1 = 12
static char	*cgba_desc	= "Circuit Group Blocking Acknowledgement Message (CGBA)";		// H1 = 13
static char	*cgua_desc	= "Circuit Group Unblocking Acknowledgement Message (CGUA)";	// H1 = 14

// Circuit Group Reset Messages:
static char	*cgr_desc	= "Circuit Group Reset Message (CGR)";					// H1 = 15
static char	*cgra_desc	= "Circuit Group Reset Acknowledgement Message (CGRA)";	// H1 = 16
//			spare_desc															   H1 = 17-255

// Group 6 - Non End-To-End Service Information Messages
//			intra_exch_use_only																	   H1 = 0-7
static char	*nhtr_desc = "Reserved - Network Hold and Trace Facility Request Message (NHTR)";	// H1 = 8
static char	*htrr_desc = "Reserved - Hold/Trace Request Response Message (HTRR)";				// H1 = 9
static char	*hcan_desc = "Reserved - Hold Cancel Message (HCAN)";								// H1 = 10
//			spare_desc																			   H1 = 11-255

// Group 7 - End-To-End Service Information Messages
static char *conf_desc		= "Confusion Message (CONF)";							// H1 = 0
static char *isdn_csim_desc	= "ISDN Composite Service Information Messages (SIMs)";	// H1 = 1
static char *sser_desc		= "\"Send Service\" Message (SSER)";					// H1 = 2
static char *serv_desc		= "\"Service\" Message (SERV)";							// H1 = 3
static char *aci_desc		= "Additional Call Information Messages (ACI)";			// H1 = 4
static char *opcm_desc		= "Operator Condition Message (OPCM)";					// H1 = 5
static char *uudm_desc		= "User-to-Dser Data Message (UUDM)";					// H1 = 6
static char *swap_desc		= "'Swap' Message (SWAP)";								// H1 = 7
static char *dam_desc		= "Diversion Activated Message (DAM)";					// H1 = 8
static char *eeacm_desc		= "End-to-End Address Complete Message (EEACM)";		// H1 = 9
//			reserved_non_bt_use														   H1 = 10
static char *need_desc		= "Nodal End-to-End Data Message (NEED)";				// H1 = 11
//			spare_desc																   H1 = 12-255

//  ================================
//	Parameter Names and Field Values
//	================================
static char *reserved_desc = "Reserved";

static char *sio_desc = "Service Information Octet";
static char *cic_desc = "Circuit Identification Code";
static char *dpc_desc = "Destination Point Code";
static char *opc_desc = "Originating Point Code";

// Signalling Point Code subfields
static char *member_field	= "member";		// signalling point
static char *cluster_field	= "cluster";	// signalling area
static char *network_field	= "network";	// signalling main area
static char *member_desc	= "Member / Signalling Point";
static char *cluster_desc	= "Cluster / Signalling Area";
static char *network_desc	= "Network / Signalling Main Point";

// Calling/Called Party Category
static char *cgpc_desc			= "Calling Party Category";
static char *cdpc_desc			= "Called Party Category";
static char *unknown_desc		= "Unknown";
static char *cpc_ord_res_desc	= "Ordinary (Residential)";
static char *cpc_ord_bus_desc	= "Ordinary (Business)";
static char *ccb_pub_desc		= "CCB (Public, Call Office)";
static char *ccb_rent_res_desc	= "CCB (Renter's Box) (Residential)";
static char *ccb_rent_bus_desc	= "CCB (Renter's Box) (Business)";
static char *isdn_res_desc		= "ISDN (Residential)";
static char *isdn_bus_desc		= "ISDN (Business)";
static char *pccb_pub_desc		= "Pre-payment CCB (Public)";
static char *pccb_rent_res_desc	= "Pre-payment CCB (Renter's, Residential)";
static char *pccb_rent_bus_desc = "Pre-payment CCB (Renter's, Business)";
static char *cpc_serv_line_desc = "Service Line";
static char *cpc_oss_op_desc	= "OSS Operator";
static char	*cpc_amc_op1_desc	= "AMC Operator (NND/IND)";
static char	*cpc_amc_op2_desc	= "AMC Operator (NOT))";

static char *onupvi_desc		= "Originating Node CCITT No.7 (BT) User Part Version Indicator";

// IAM & IFAM Message Indicators bits A-H
static char *olii_desc			= "Calling Line Identity (OLI) Message Indicator (bit A) for I(F)AM";
static char	*cbi_desc			= "Cross Border Indicator (CBI)";
static char	*inti_desc			= "International Indicator (INT)";
static char	*no_info_call_orig	= "No further information on call origin";
static char	*iwmi_desc			= "Interworking (I/W) Message Indicator";
static char	*pai_desc			= "Priority Access Indicator (PA)";
static char	*not_pac_desc		= "Not Priority Access Call";
static char	*pac_desc			= &not_pac_desc[4];
static char	*cfcmi_desc			= "Coin and Fee Check Message Indicator (C&FC)";
static char	*cfcmi_use_desc		= "C&FC set to 0 at originating node: "
								  "transit as received at intermediate nodes. "
								  "Reserved for intra-exchange use only.";
static char	*mdgti_desc			= "Meter Delay Guard Timeout Indicator (MDG)";
static char	*no_mdg_req_desc	= "No MDG timeout required";
static char	*mdg_req_desc		= &no_mdg_req_desc[3];
static char	*protect_desc		= "Protection Indicator";

static char	*shp_desc = "Service Handling Protocol";

static char *upvi_desc			= "CCITT No.7 (BT) User Part Version Indicator (UPVI)";
static char	*satind_desc		= "Satellite (SAT IND) / Long Propagation Delay (LPD IND) Indicator";
static char	*cti_desc			= "Call type Indicator (CTI)";
static char	*echomi_desc		= "Echo Suppressor Message Indicator (ES IND)";
static char	*fci_desc			= "Freefone Call Indicator";

static char	*netind_desc		= "Network Identifier";
static char	*rci_desc			= "Routing Control Indicator";
static char	*cpi_desc			= "Call Path Indicator";

static char	*addr_sig20_desc	= "Address Signals (for IAM and IFAM)";
static char	*addr_sig18_desc	= "Address Signals (for SAM and FAM)";
static char	*tran_addr_sig_desc	= "Translated Address Signals (for CDB)";
static char	*noas_desc			= "Number of Address Signals";
static char	*asd_desc			= "Address Signal Digits";

static char	*oli_desc			= "Calling Line Identity (OLI)";
static char	*tli_desc			= "Called Line Identity (TLI)";
static char	*pcgli_desc			= "Partial Calling Line Identity";
static char	*pcdli_desc			= "Partial Called Line Identity";

static char	*noai_desc			= "Nature of Address Indicator";
static char	*cli_noa0_desc		= "Spare (Reserved to indicate local number by CCITT)";
static char	*cli_noa2_desc		= "National (Significant) number";
static char	*cli_noa3_desc		= "Reserved for International number";

static char	*iai_desc			= "Incomplete Address Indicator";
static char	*cli_iai0_desc		= "No indication";
static char	*cli_iai1_desc		= "Incomplete Address in CLI Address digit field";

static char	*idq_desc			= "Identity Qualifier";
static char	*cli_idq0_desc		= "CLI may be released to called terminal for display";
static char	*cli_idq1_desc		= "CLI must not be released to called terminal";

static char	*asui_infoind_desc	= "ASUI Information Indicator";
static char	*nodal_id_desc		= "Nodal Identity";
static char	*tou_desc			= "Type of Unit";
static char	*tad_desc			= "Territory & District"; // also used	in	BGI
static char	*uni_desc			= "Unit Identity";	// also used	in	BGI
static char	*bgi_grp_desc		= "Business Group";
static char	*bgi_sgrp_desc		= "Business Sub-group";
static char	*bgi_ai_desc		= "Attendant Indicator";
static char	*netnum_desc		= "Network Number";
static char	*nrn_desc			= "Network Route Number";
static char	*tpn_desc			= "Telephony Process Number";
static char	*cct_desc			= "Network Circuit";
static char	*band_desc			= "Network Band";
static char	*cctuse_desc		= "Circuit Usage";
static char	*ic_desc			= "Incoming";
static char	*og_desc			= "Outgoing";
static char	*rs_desc			= "Route Selectability";
static char	*ico_desc			= "Incoming only";
static char	*ogo_desc			= "Outgoing only";
static char	*bothway_desc		= "Bothway";

static char *snd_ndr_desc		= "Number of digits requested";
static char *sasui_ind_desc		= "SASUI Information Indicator";

static char *cnar_desc			= "\"Connexion Not Admitted\" (CNA) Reason";
static char *relr_desc			= "Release Reason";
static char *cdb_clogi_desc		= "CDB Call Log Indicator";
static char *indb_desc			= "Intelligent Network Database (INDB) Information";
static char *cdb_cli_infoi_desc = "CDB Calling Line Information Indicator";

static char *anm_type_desc			= "Type of Answer";
static char *reserved_for_dpnss		= "Reserved (for DPNSS)";
static char	*na_dass_desc			= "N/A - DASS";
static char	*na_dpnss_desc			= "N/A - DPNSS";
static char	*na_dass_dpnss_desc		= "N/A - DASS & DPNSS";
static char	*num_unobtain_desc		= "Number Unobtainable";
static char	*addr_incomp_desc		= "Address Incomplete";
static char	*network_term_desc		= "Network Termination";
static char	*serv_unavail_desc		= "Service Unavailable";
static char	*sub_incompat_desc		= "Subscriber Incompatible";
static char	*sub_transf_desc		= "Subscriber Transferred";
static char	*rem_proc_err_desc		= "Remote Procedure Error";
static char	*serv_incompat_desc		= "Service Incompatible";
static char	*nae_error_desc			= "NAE Error";
static char	*prot_vio_desc			= "Protocol Violation";
static char	*cug_addr_bar_desc		= "CUG Address Barred";
static char	*dte_ctrl_nrdy_desc		= "DTE Controlled Not Ready";
static char	*dte_unctrl_nrdy_desc	= "DTE Uncontrolled Not Ready";
static char	*sub_call_term_desc		= "Subscriber Call Termination";

static char	*num_rcvd_ind_desc	= "Number Received Indicators";
static char	*chg_ind_desc		= "Charge Indicator (Reserved Field)";
static char	*lpri_desc			= "Last Party Release Indicator";
static char	*acm_iwi_desc		= "ACM Interworking Indications";
static char	*acm_echo_si_desc	= "ACM Echo Suppressor Indicator";

static char	*spare_conf_rtn			= "Spare - Confusion Message shall be returned";
static char	*spare_conf_rtn_if_rcvd	= "Spare - Confusion Message shall be returned if received";

static char	*simic_desc			= "SIM 'Information Contained' Code";
static char	*simir_desc			= "SIM 'Information Requested' Code";
static char	*simicr_0_desc		= "No information";
static char	*simicr_1_desc		= "TLI";
static char	*simicr_2_desc		= "FIC + SIC + CUGIC + OLI";
static char	*simicr_3_desc		= "FIC + SIC + OLI";
static char	*simicr_4_desc		= "FIC + SIC + CUGIC + OLI + NAE";
static char	*simicr_5_desc		= "FIC + SIC + OLI + NAE";
static char	*simicr_6_desc		= "FIC";
static char	*simicr_7_desc		= "FIC + TLI";
static char	*simicr_8_desc		= "FIC + SIC + BGI + CUGIC + OLI";
static char *simicr_9_desc		= "FIC + SIC + BGI + OLI";
static char *simicr_10_desc		= "FIC + SIC + BGI + CUGIC + OLI + NAE";
static char *simicr_11_desc		= "FIC + SIC + BGI + OLI + NAE";
static char *simicr_12_desc		= "FIC + BGI + TLI";
static char	*simicr_13_desc		= spare_conf_rtn;

static char	*fic_desc			= "Facility Indicator Code";
static char	*sic_desc			= "Service Indicator Codes";
static char	*sic_octet1_desc	= "Service Indicator Code Octet 1 (Routing Information)";
static char	*sic_octet2_desc	= "Service Indicator Code Octet 2 (Synch/Asynchronous Information)";
static char *speech_desc		= "speech";
static char *data_desc			= "data";
static char	*cugic_desc			= "Closed User Group (CUG) Interlock Code";
static char	*nae_desc			= "Network Address Extension (NAE)";
static char	*bgi_desc			= "Business Group Identity (BGI)";

static char	*aciic_desc		= "ACI 'Information Contained' Code";
static char	*aciir_desc		= "ACI 'Information Requested' Code";
static char	*aciicr_0_desc	= "No information";
static char	*aciicr_1_desc	= "Full Calling Line identity";
static char	*aciicr_2_desc	= "Full Called Line Identity";
static char	*aciicr_3_desc	= "Partial Calling Line Identity";
static char	*aciicr_4_desc	= "Partial Called Line Identity";
static char	*aciicr_5_desc	= "Full Calling Line Identity with Calling Subscriber Basic Service Markings";
static char *aciicr_6_desc	= "Full Called Line Identity with Called Subscriber Basic Service Markings";
static char *aciicr_7_desc	= "Called Subscriber Basic Service Markings";
static char *aciicr_8_desc	= "Calling Subscriber Originating Facility Service Markings";
static char *aciicr_9_desc	= "Called Subscriber Terminating Facility Service Markings";
static char *aciicr_10_desc	= "Called Line Identity, Called Party Category and Called Network Identifier";
static char	*aciicr_11_desc	= spare_conf_rtn_if_rcvd;
static char	*aci_iwi_desc	= "ACI Interworking Indicator";
static char	*opi_desc		= "ACI Operator Indicator";

static char *cgbsm_desc	= "Calling Subscriber's Basic Service Markings";
static char *cdbsm_desc	= "Called Subscriber's Basic Service Markings";

static char	*cbsm_bitA_desc = "Incoming Admin/Maintenance Call Barring indicator";
static char	*cbsm_bitB_desc = "Subscriber Controlled 'Incoming Calls Barred* (ICB) indicator";
static char	*cbsm_bitC_desc = "Pre-arranged ICB indicator";
static char	*cbsm_bitD_desc = "Permanent ICB indicator";
static char *cbsm_bitE_desc = "'Temporary Out of Service' (TOS) indicator";
static char *cbsm_bitF_desc = "'ICB-except-to-Operators' indicator";
static char *cbsm_bitG_desc = "Called Subscriber Facility Information available indicator";
static char *cbsm_bitH_desc = "Calling Subscriber Facility Information available indicator";
static char	*cbsm_bitI_desc = "Permanent 'Outgoing Calls Barred' (OCB) indicator";
static char	*cbsm_bitJ_desc = "Outgoing Local Call Barring indicator";
static char	*cbsm_bitK_desc = "Outgoing National Call Barring indicator";
static char	*cbsm_bitL_desc = "Outgoing International Call Barring indicator";
static char	*cbsm_bitM_desc = "'Calls-to-Operator Barring' indicator";
static char	*cbsm_bitN_desc = "Supplementary Facility Call Barring indicator";
static char	*cbsm_bitO_desc = "Digit Masking indicator";

static char	*cgstariff_desc = "Calling Subscriber's Tariff Group";
static char	*cdstariff_desc = "Called Subscriber's Tariff Group";

static char *cgofsm_desc	= "Calling Subscriber's Originating Facility Service Markings";
static char *cdtfsm_desc	= "Called Subscriber's Terminating Facility Service Markings";

static char *msg_ind_desc		= "Message Indicators";
static char *block_no_immed_rel = "Block with no immediate release";
static char *block_immed_rel	= "Block with immediate release";
static char	*type_field			= "type";
static char	*type_ind_desc		= "Type Indicator";
static char	*range_field		= "range";
static char	*range_desc			= "Range";
static char	*status_field		= "status";
static char	*status_desc		= "Status";

static char	*serv_code_desc		= "Service Code";
static char	*needi_desc			= "Nodal End-to-End Data Indicator";
static char *uud_desc			= "User-to-User Data";

// -------------------------------------------------------------------------------------------—--------------------
//
//	=========================
//	Service Information Octet
//	=========================
//
// preceeds the SIF - i.e. before the Label and Message Type (HO and H1)
//
//
LabelGroup SIO( sio_desc, "SIQ" );
SIO = SIO +
(UIGroup( "SIO", sio_desc ) +
  Field( "sid", simple, ff_uint, 4, 0, 15, "Service Indicator", Table(16) +
	0 + "Signalling Network Management Message" +
	1 + "Signalling Network Testing & Maintenance Message" +
	2 + spare_desc +
	3 + "Signalling Connection Control Part (SCCP)" +
	4 + "Telephone User Part (TUP)" +
	5 + "ISDN User Part (ISUP)" +
	6 + "Data User Part (Call & Circuit related Message) (DUP)" +
	7 + "Data User Part (Facility Registration & Cancellation Message) (DUP)" +
	8-15 + spare_desc
  ) * Property( is_header ) +
  (LabelGroup( "Subservice Field", "SIO" ) +
    Field( "priority", simple, ff_uint, 2, "Priority of telephone message (spare)",
	Table(4) +
	  0	+ spare_desc +
	  1	+ spare_desc +
	  2	+ spare_desc +
	  3	+ spare_desc
	) * Property( is_header ) +
	Field( "netind", simple, ff_uint, 2, "Signalling Network Indicator", Table(4) +
	  0	+ "International signalling network indicator" +
	  1	+ "Spare (for international use only)" +
	  2	+ "National signalling network" +
	  3	+ "Ancillary / intra-nodal signalling"
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
CIC = CIC * Property ( is_header|per_link );
CIC = CIC + Field( "code", simple, ff_uint, 12, cic_desc, cic_ops ) * Property( is_header );

//
//	===============
//	Label (40 bits)
//	===============
//
// Fixed size 40-bit Label with 14-bit signalling point codes
//
LabelGroup LABEL( "Label", "LABEL" );
LABEL = LABEL +
(UIGroup( "DPC", dpc_desc ) +
  (LabelGroup( dpc_desc ) +
	Field( member_field, simple, ff_uint, 3, member_desc ) * Property( is_header ) +
	Field( cluster_field, simple, ff_uint, 8, cluster_desc ) * Property( is_header ) +
	Field( network_field, simple, ff_uint, 3, network_desc ) * Property( is_header )
  )
) * Property( is_header|per_link ) +
(UIGroup( "OPC", opc_desc ) +
  (LabelGroup( opc_desc ) +
	Field( member_field, simple, ff_uint, 3, member_desc ) * Property( is_header ) +
	Field( cluster_field, simple, ff_uint, 8, cluster_desc ) * Property( is_header ) +
	Field( network_field, simple, ff_uint, 3, network_desc ) * Property( is_header )
  )
) * Property ( is_header |per_link ) +
CIC;

// -----------------------------------------------------------------------------------------------------------

//	=====================
//	Parameter Definitions
//	=====================
//

//
//	=======================================
//	Calling Party Category for IAM and IFAM
//	=======================================
//
UIGroup CGPC( "CGPC", cgpc_desc );
CGPC = CGPC +
Field( "cgpc", simple, ff_uint, 6, cgpc_desc, Table(64) +
	0	+ unknown_desc +
	1	+ cpc_ord_res_desc +
	2	+ cpc_ord_bus_desc +
	3	+ ccb_pub_desc +
	4	+ ccb_rent_res_desc +
	5	+ ccb_rent_bus_desc +
	6	+ isdn_res_desc +
	7	+ isdn_bus_desc +
	8	+ pccb_pub_desc +
	9	+ pccb_rent_res_desc +
	10	+ pccb_rent_bus_desc +
	11	+ cpc_serv_line_desc +
	12	+ spare_desc +
	13	+ cpc_oss_op_desc +
	14	+ cpc_amc_op1_desc +
	15	+ cpc_amc_op2_desc +
	16-20 + spare_desc +
	21-23 + reserved_desc +
	24-63 + spare_desc
);

//
//	============================================
//	Called Party Category for ACM and ACI Type 8
//	============================================
//
UIGroup CDPC( "CDPC", cdpc_desc );
CDPC = CDPC +
Field( "cdpc", simple, ff_uint, 6, cdpc_desc, Table(64) +
	0	+ unknown_desc +
	1	+ cpc_ord_res_desc +
	2	+ cpc_ord_bus_desc +
	3	+ ccb_pub_desc +
	4	+ ccb_rent_res_desc +
	5	+ ccb_rent_bus_desc +
	6	+ isdn_res_desc +
	7	+ isdn_bus_desc +
	8	+ pccb_pub_desc +
	9	+ pccb_rent_res_desc +
	10	+ pccb_rent_bus_desc +
	11	+ cpc_serv_line_desc +
	12	+ spare_desc +
	13	+ cpc_oss_op_desc +
	14	+ cpc_amc_op1_desc +
	15	+ cpc_amc_op2_desc +
	16-20 + spare_desc +
	21-23 + reserved_desc +
	24-63 + spare_desc
);

//
//	=================================================================
//	Originating Node CCITT No.7 (BT) User Part Version for IAM & IFAM
//	=================================================================
//
UIGroup ONUPVI("ONUPVI", onupvi_desc );
ONUPVI = ONUPVI +
Field( "onupvi", simple, ff_uint, 2, onupvi_desc, Table(4) +
	0	+ "No information (Assume Version 1/Version 2 User Part provided at Originating Node)" +
	1	+ "Version 3 User Part provided at Originating Node" +
	2-3 + "Spare - (treat as value \"1\" - i.e. Version 3 User Part provided at Originating Node)"
);

//
//	============================================
//	Message Indicators (bits A-H) for IAM & IFAM
//	============================================
//
LabelGroup MSG_IND_1( "I(F)AM Message Indicators (bits A-H)", "MSG_IND_1" );
MSG_IND_1 = MSG_IND_1 +
(UIGroup( "OLII", olii_desc ) +
	Field( "olii", simple, ff_uint, 1, olii_desc, Table(2) +
	  0	+ "Calling Line Identity (OLI) not included" +
	  1	+ "Calling Line Identity (OLI) included"
	)
) +
(UIGroup( "CBI", cbi_desc ) +
	Field( "cbi", simple, ff_uint, 1, cbi_desc, Table(2) +
	  0	+ no_info_call_orig +
	  1	+ "Call from another administration via cross border route, (i.e. not via ISC)"
	)
) +
(UIGroup( "INTI", inti_desc ) +
	Field( "int", simple, ff_uint, 1, inti_desc, Table (2) +
	  0	+ no_info_call_orig +
	  1	+ "Call incoming via international network"
	)
) +
(UIGroup( "IWMI", iwmi_desc ) +
	Field( "iwmi", simple, ff_uint, 1, iwmi_desc, Table(2) +
	  0	+ "No interworking involved" +
	  1	+ "Interworking involved"
	)
) +
(UIGroup( "PAI", pai_desc ) +
	Field( "pai", simple, ff_uint, 1, pai_desc, Table(2) +
	  0	+ not_pac_desc +
	  1	+ pac_desc
	)
) +
(UIGroup( "CFCMI", cfcmi_desc ) +
	Field( "cfcmi", simple, ff_uint, 1, cfcmi_desc, Table(2) +
	  0	+ cfcmi_use_desc +
	  1	+ cfcmi_use_desc
	)
) +
(UIGroup( "MDGTI", mdgti_desc ) +
	Field( "mdgti", simple, ff_uint, 1, mdgti_desc, Table(2) +
	  0	+ no_mdg_req_desc +
	  1	+ mdg_req_desc
	)
) +
(UIGroup( "PI", protect_desc ) +
	Field( "pi", simple, ff_uint, 1, protect_desc, Table(2) +
	  0	+ "Non-priority, non-protected call" +
	  1	+ "Priority and protected call"
	)
);

//
//	========================================
//	Service Handling Protocol for IAM & IFAM
//	========================================
//
UIGroup SHP( "SHP", shp_desc );
SHP = SHP +
  Field( "shp", simple, ff_uint, 4, shp_desc, Table(16) +
	0 + "Invoke Basic (telephony) Call Protocol - no ISDN SIM interchange required" +
	1 + "Invoke ISDN Call Protocol - i.e. the ISDN SIM A, B and C interchange is required if a "
		"CCITT No.7 (BT) signalling path is selected from originating to terminating node and "
		"the call terminates on an ISDN Terminal/ISPBX or Business Exchange Services (BES) Address." +
	2 + "Invoke 'Request Service' Protocol" +
	3 + "Invoke Nodal End-to-End Data Protocol" +
	4 + "Invoke Nodal End-to-End Data Protocol and basic ISDN SIM interchange - "
		"to effect interchange of Business Group / Sub-Group information on Business Exchange Services "
		" (BES) calls." +
	5 + "Invoke Nodal End-to-End Data Protocol and ISDN SIM interchange - "
		"to effect interchange of Business Group / Sub-Group information and supplementary service "
		"information on Business Exchange Services (BES) ISDN calls." +
	6-15 + spare_desc
);

//
//	============================================
//	Message Indicators (bits I-P) for IAM & IFAM
//	============================================
//
LabelGroup MSG_IND_2( "I(F)AM Message Indicators (bits I-P)", "MSG_IND_2" );
MSG_IND_2 = MSG_IND_2 +
(UIGroup( "UPVI", upvi_desc ) +
	Field( "upvi", simple, ff_uint, 2, upvi_desc, Table (4) +
	  0	+ "No information on preceding nodes on call" +
	  1	+ "Call has encountered a preceding node provided with Version 3 User Part" +
	  2-3 + "Spare (treat as value \"1\" - i.e. Call has encountered a preceding node provided with "
	  	  	"Version 3 User Part)"
	)
) +
(UIGroup( "SATIND", satind_desc ) +
	Field( "satind", simple, ff_uint, 1, satind_desc, Table(2) +
	  0	+ "Satellite / Long Propagation Delay path not included" +
	  1	+ "Satellite / Long Propagation Delay path included"
	)
) +
(UIGroup( "CTI", cti_desc ) +
	Field( "cti", simple, ff_uint, 3, cti_desc, Table(8) +
	  0	+ "No information on call type (treat as basic call)" +
	  1	+ "\"Diverted\" call" +
	  2-3 + reserved_desc +
	  4	+ "BES/VPN Virtual Call" +
	  5-7 + "Reserved/Spare (may treat as value \"0\" and allow call to continue)"
	)
) +
(UIGroup( "ECHOMI", echomi_desc ) +
	Field( "esind", simple, ff_uint, 1, echomi_desc, Table(2) +
	  0	+ "Outgoing Echo Suppressor not included" +
	  1	+ "Outgoing Echo Suppressor included"
	)
) +
(UIGroup( "FCI", fci_desc ) +
	Field( "fci", simple, ff_uint, 1, fci_desc, Table(2) +
	  0	+ "No information on call type" +
	  1	+ reserved_desc
	)
);

//
//	Network Identifier for IAM & IFAM
//	Called Network Identifier for ACI Type 8
//	==========================================
//
UIGroup NI( "NI", "Network Identifier for IAM & IFAM / Called Network Identifier for ACI Type 8" );
NI = NI +
Field( "ni", simple, ff_uint, 4, 0, 15, netind_desc, Table(16) +
	0	+ "No information on originating/preceding network" +
	1	+ "British Telecom (BT)" +
	2	+ "Telecom Securicor Cellular Radio (TSCR)" +
	3	+ "Mercury Communications Ltd (MCL)" +
	4	+ "Racal/Vodafone" +
	5	+ "Hull" +
	6	+ "Isle of Man" +
	7	+ "Jersey" +
	8	+ "Guernsey" +
	9	+ "Eire" +
	10	+ "VPN" +
	11	+ "BES" +
	12	+ "VPN and BES" +
	13	+ "Spare (treat	as value \"0\")	/ Pan-European GSM (Racal/Vodafone)"	+
	14	+ "Spare (treat	as value \"0\")	/ Pan-European GSM (Telecom Securicor Cellular Radio - TSCR)" +
	15	+ "Spare (treat	as value \"0\")	/ Reserved for extension identifier"
);

//
//	========================================
//	Routing Control Indicator for IAM & IFAM
//	========================================
//
UIGroup RCI( "RCI", rci_desc );
RCI = RCI +
  Field( "rci", simple, ff_uint, 4, rci_desc, Table(16) +
	0	+ "Alternative routing allowed, continuous retry allowed" +
	1	+ "Alternative routing barred, continuous retry allowed" +
	2	+ "Alternative routing allowed, continuous retry barred" +
	3	+ "Alternative routing barred, continuous retry barred" +
	4	+ "Alternative Routing only allowed once, continuous retry allowed" +
	5	+ "Alternative Routing only allowed once, continuous retry barred" +
	6-15 + "Spare (treat as value '0')"
);

//
//	==================================
//	Call Path Indicator for IAM & IFAM
//	==================================
//
LabelGroup CPI( cpi_desc, "CPI" );
CPI = CPI +
(UIGroup( "CPI", cpi_desc ) +
  Field( "cpi", simple, ff_uint, 3, cpi_desc, Table(8) +
	0	+ "Select any available route. 64 kbits/s CCITT No.7 (BT) transmission path preferred. "
		  "OOR and monitor access shall be allowed." +
	1	+ "64 kbits/s CCITT No.7 (BT) signalling essential. "
		  "OOR and monitor access shall be inhibited." +
	2	+ "3.1 kHz audio bearer service required. Specifically the speech path for this call should "
		  "avoid any speech processing equipment which may adversely affect voiceband data, "
		  "e.g. ADPCM equipment." +
	3-7 + spare_desc
  ) +
  Field( 0, simple, ff_uint, 1, "Spare - available for increase of 'Call Path Indicator' subfield" )
);

//
//	========================================
//	Address Signals for IAM, IFAM, SAM & FAM
//	========================================
//
LabelGroup ADDR_SIG20( addr_sig20_desc, "AS20" ); // for IAM and IFAM
ADDR_SIG20 = ADDR_SIG20 +
(UIGroup( "AS20", addr_sig20_desc ) +
	Field( "num", length0, ff_uint, 5, 0, 20, noas_desc ) +
	Field( 0, simple, ff_uint, 3, spare_desc ) +
	Field( "digits", bcd_pad8, ff_bcd, 4, 0, 20, "AS20:num", asd_desc )
);

LabelGroup ADDR_SIG18( addr_sig18_desc, "AS18" ); // for SAM and FAM
ADDR_SIG18 = ADDR_SIG18 +
(UIGroup( "AS18", addr_sig18_desc ) +
	Field( "num", length0, ff_uint, 5, 0, 18, noas_desc ) +
	Field( 0, simple, ff_uint, 3, spare_desc ) +
	Field( "digits", bcd_pad8, ff_bcd, 4, 0, 18, "AS18:num", asd_desc )
);

//
//	==================================
//	Translated Address Signals for CDB
//	==================================
//
LabelGroup TRANSLATED_ADDR_SIG20( tran_addr_sig_desc, "CDBTAS20" ); // for CDB
TRANSLATED_ADDR_SIG20 = TRANSLATED_ADDR_SIG20 +
(UIGroup( "CDBTAS20", tran_addr_sig_desc ) +
	Field( "num", length0, ff_uint, 5, 0, 20, noas_desc ) +
	Field( 0, simple, ff_uint, 3, spare_desc ) +
	Field( "digits", bcd_pad8, ff_bcd, 4, 0, 20, "CDBTAS20mum", asd_desc )
);

//
//	=============================================
//	Calling Line Identity (OLI) for IAM and IFAM,
//	ASUI 1, CDP, SIM (1-5,7-12) and ACI (1,3,5).
//	=============================================
//
LabelGroup OLI( oli_desc, "OLI" );
OLI = OLI +
(UIGroup( "OLI", oli_desc ) +
  (LabelGroup( "Calling Line Identity (OLI) Message Indicators" ) +
	Field( "noa", simple, ff_uint, 2, noai_desc, Table(4) +
		0	+ cli_noa0_desc +
		1	+ spare_desc +
		2	+ cli_noa2_desc +
		3	+ cli_noa3_desc
	) +
	Field( "iai", simple, ff_uint, 1, iai_desc, Table(2) +
		0	+ cli_iai0_desc +
		1	+ cli_iai1_desc
	) +
	Field( "idq", simple, ff_uint, 1, idq_desc, Table(2) +
		0	+ cli_idq0_desc +
		1	+ cli_idq1_desc
	)
  ) +
  Field( "num", length0, ff_uint, 4, 0, 15, "Number of Calling Line (OLI) Address Digits" ) +
  Field( "cli", bcd_pad8, ff_bcd, 4, 0, 15, "OLI:num", "Calling Line (OLI) Address Digits" )
);

//
//	===========================================
//	Called Line Identity (TLI) for ACI (2,6,10)
//	===========================================
//
LabelGroup TLI( tli_desc, "TLI" );
TLI = TLI +
(UIGroup( "TLI", tli_desc ) +
  (LabelGroup( "Called Line Identity (TLI) Message Indicators" ) +
	Field( "noa", simple, ff_uint, 2, noai_desc, Table(4) +
		0	+ cli_noa0_desc +
		1	+ spare_desc +
		2	+ cli_noa2_desc +
		3	+ cli_noa3_desc
	) +
	Field( "iai", simple, ff_uint, 1, iai_desc, Table(2) +
		0	+ cli_iai0_desc +
		1	+ cli_iai1_desc
	) +
	Field( "idq", simple, ff_uint, 1, idq_desc, Table(2) +
		0	+ cli_idq0_desc +
		1	+ cli_idq1_desc
	)
  ) +
  Field( "num", length0, ff_uint, 4, 0, 15, "Number of Called Line (TLI) Address Digits" ) +
  Field( "cli", bcd_pad8, ff_bcd, 4, 0, 15, "TLI:num", "Called Line (TLI) Address Digits" )
);

//
//	==============================
//	Information Indicator for ASUI
//	==============================
//
UIGroup ASUI_INFO_IND( "ASUI_IND", asui_infoind_desc );
ASUI_INFO_IND = ASUI_INFO_IND +
	Field( "asui_ic", simple, ff_uint, 8, asui_infoind_desc, Table(256) +
		0	+ spare_desc +
		1	+ "Full Calling Line Identity (OLI) included (ASUI Type 1)" +
		2	+ "Partial Calling Line Identity (PCGLI) included (ASUI Type 2)" +
		3-6	+ reserved_non_bt_use +
		7-255 + spare_desc
);

//
//	==================================================
//	Partial Calling Line Identity for ASUI 2 and ACI 2
//	==================================================
//
LabelGroup PCGLI( pcgli_desc, "PCGLI" );
PCGLI = PCGLI +
(UIGroup( "PCGLI", pcgli_desc ) +
  (LabelGroup( nodal_id_desc ) +
	Field( "tou",	bcd, ff_bcd, 8, 2, 2, tou_desc	) +
	Field( "tad",	bcd, ff_bcd, 8, 2, 2, tad_desc	) +
	Field( "uni",	bcd, ff_bcd, 16, 4, 4, uni_desc	)
  ) +
  (LabelGroup( netnum_desc ) +
	Field( "nrn",	simple, ff_uint, 8, nrn_desc ) +
	Field( "tpn",	simple, ff_uint, 6, tpn_desc ) +
	Field( 0,		simple, ff_uint, 2, spare_desc ) +
	Field( "cct",	simple, ff_uint, 4, cct_desc ) +
	Field( 0,		simple, ff_uint, 4, spare_desc) +
	Field( "band",	simple, ff_uint, 8, band_desc )
  ) +
  Field( "cctuse", simple, ff_uint, 1, cctuse_desc, Table(2) +
	0	+ ic_desc +
	1	+ og_desc
  ) +
  Field( "rs", simple, ff_uint, 2, rs_desc, Table(4) +
	0	+ spare_desc +
	1	+ ico_desc +
	2	+ ogo_desc +
	3	+ bothway_desc
  ) +
  Field( 0, simple, ff_uint, 5, spare_desc )
);

//
//	===========================================
//	Partial Called Line Identity for ACI Type 2
//	===========================================
//
LabelGroup PCDLI( pcdli_desc, "PCDLI" );
PCDLI = PCDLI +
(UIGroup( "PCDLI", pcdli_desc ) +
  (LabelGroup( nodal_id_desc ) +
	Field( "tou",	bcd, ff_bcd, 8, 2, 2, tou_desc	) +
	Field( "tad",	bcd, ff_bcd, 8, 2, 2, tad_desc	) +
	Field( "uni",	bcd, ff_bcd, 16, 4, 4, uni_desc	)
  ) +
  (LabelGroup( netnum_desc ) +
	Field( "nrn",	simple, ff_uint, 8, nrn_desc ) +
	Field( "tpn",	simple, ff_uint, 6, tpn_desc ) +
	Field( 0,		simple, ff_uint, 2, spare_desc ) +
	Field( "cct",	simple, ff_uint, 4, cct_desc ) +
	Field( 0,		simple, ff_uint, 4, spare_desc	) +
	Field( "band",	simple, ff_uint, 8, band_desc )
  ) +
  Field( "cctuse", simple, ff_uint, 1, cctuse_desc, Table(2) +
	0	+ ic_desc +
	1	+ og_desc
  ) +
  Field( "rs", simple, ff_uint, 2, rs_desc, Table(4) +
	0	+ spare_desc +
	1	+ ico_desc +
	2	+ ogo_desc +
	3	+ bothway_desc
  ) +
  Field( 0, simple, ff_uint, 5, spare_desc )
);

//
//	============================================
//	Number Received Indicators for ACM and EEACM
//	============================================
//
LabelGroup ACM_NUM_RCVD_IND( num_rcvd_ind_desc );
ACM_NUM_RCVD_IND = ACM_NUM_RCVD_IND +
(UIGroup( "CHGI", chg_ind_desc ) +
  Field( "chgi", simple, ff_uint, 1, chg_ind_desc, Table(2) +
	0	+ "No charge" +
	1	+ "Charge"
  )
) +
Field( 0, simple, ff_uint, 1, spare_desc ) +
(UIGroup( "LPRI", lpri_desc ) +
  Field( "lpri", simple, ff_uint, 1, lpri_desc, Table(2) +
	0	+ "Normal (calling party) release applies" +
	1	+ "Last Party release applies"
  )
) +
(UIGroup( "ACMIWI", acm_iwi_desc ) +
  Field( "acmiwi", simple, ff_uint, 4, acm_iwi_desc, Table(16) +
	0	+ "Analogue interworking encountered or call terminates on a non-Version 2 User Part exchange" +
	1	+	"Fully digital path available with CCITT No.7 (BT) signalling to an ISDN customer on a Version 2 User Part exchange" +
	2	+	"Fully digital path available with CCITT No.7 (BT) signalling to an analogue customer on a Version 2 User Part exchange" +
	3	+	"Fully digital path available with CCITT No.7 (BT) signalling to an ISDN customer on a Version 3 User Part exchange" +
	4	+	"Fully digital path available with CCITT No.7 (BT) signalling to an analogue customer on a Version 3 User Part exchange" +
	5	+	"Fully digital path available with CCITT No.7 (BT) signalling to an ISDN customer, analogue private network encountered" +
	6-15 +	"invalid, treat as value 3 if ISDN Composite SIM interchange has occurred; "
			"treat as value 4 if ISDN Composite SIM interchange has not occurred"
  )
) +
Field( 0, simple, ff_uint, 1, spare_desc ) +
(UIGroup( "ACMECHOSI", acm_echo_si_desc ) +
  Field( "acm_echo_si", simple, ff_uint, 1, acm_echo_si_desc, Table(2) +
	0	+ "Incoming Echo Suppressor not included" +
	1	+ "Incoming Echo Suppressor included"
  )
) +
Field( 0, simple, ff_uint, 1, spare_desc );

//
//	==========================================
//	Connexion Not Admitted Reason Field (CNAR)
//	==========================================
//
UIGroup CNAR( "CNAR", cnar_desc );
CNAR = CNAR +
Field( "cnar", simple, ff_uint, 8, cnar_desc, Table(256) +
	0	+	num_unobtain_desc +
	1	+	addr_incomp_desc +
	2	+	network_term_desc +
	3	+	serv_unavail_desc +
	4	+	sub_incompat_desc +
	5	+	sub_transf_desc +
	6	+	"N/A - Allocated for ET->NT/PBX Clearing Cause - 'Invalid Request for Supplementary Service'" +
	7	+	"Congestion (Re-routing	Not	Permitted)" +
	8	+	"Subscriber Engaged" +
	9	+	"Subscriber Out of Service"	+
	10	+	"Incoming Calls Barred"	+
	11	+	"N/A - Allocated for ET-NT/PBX Clearing Cause - 'Outgoing Calls Barred'" +
	12-17	+	reserved_desc +
	18	+	rem_proc_err_desc +
	19	+	serv_incompat_desc +
	20	+	reserved_desc +
	21	+	"N/A - Allocated to ET <-> NT/PBX Clearing Cause - 'Signal Not Understood'" +
	22	+	reserved_desc +
	23-25	+ reserved_for_dpnss +
	26	+	"N/A - Allocated to ET <-> NT/PBX Clearing Cause - 'Message Not Understood'" +
	27-29	+	reserved_for_dpnss +
	30	+	nae_error_desc +
	31	+	"N/A - Allocated for Release Message - Reason 'No Reply'" +
	32	+	"N/A - Allocated for Release Message - Reason 'Service Termination'"	+
	33	+	reserved_desc	+
	34	+	prot_vio_desc	+
	35-40	+	reserved_desc	+
	41	+	cug_addr_bar_desc +
	42-44	+	reserved_desc	+
	45	+	dte_ctrl_nrdy_desc +
	46	+	dte_unctrl_nrdy_desc +
	47	+	"N/A - Allocated for Release Message - Reason 'Null'" +
	48	+	sub_call_term_desc +
	49	+	reserved_desc +
	50	+	"N/A - Allocated for ET->NT/PBX Clearing Cause - 'ET Isolated'" +
	51	+	"N/A - Allocated for ET->NT/PBX Clearing Cause - 'Local Procedure Error'" +
	52-255	+ spare_desc
);

//
//	===================================
//	Release Message Reason Field (RELR)
//	===================================
//
UIGroup RELR( "RELR", relr_desc );
RELR = RELR +
// relr_ind is necessary for encoding and sending REL message, but not when receiving and decoding.
(UIField( "relr_ind", simple, ff_uint, 1, 0, 1, 1, "REL Reason Field inclusion indicator", Table(2) +
	0	+ "REL without Reason Field" +
	1	+ "REL with Reason Field"
  ) * Property( is_optional|is_trailer )
) +
(Group( "RELR:relr_ind" ) +
  Field( "relr", simple, ff_uint, 8, relr_desc, Table(256) +
	0	+	num_unobtain_desc +
	1	+	addr_incomp_desc +
	2	+	network_term_desc +
	3	+	serv_unavail_desc +
	4	+	sub_incompat_desc +
	5	+	sub_transf_desc +
	6	+	na_dass_desc +
	7	+	"ISDN Congestion (Re-routing Not Permitted)" +
	8	+	"ISDN Subscriber Engaged" +
	9	+	"ISDN Subscriber Out of Service" +
	10	+	"Incoming Calls Barred" +
	11	+	na_dass_desc +
	12	+	"N/W Protective Controls"	+
	13	+	"Rejected Diverted Calls"	+
	14	+	"Selective Call Barring" +
	15	+	"Telephony Congestion (Re-routing Not Permitted)" +
	16	+	"Telephony Subscriber Engaged" +
	17	+	"Telephony Subscriber Out of Service" +
	18	+	rem_proc_err_desc +
	19	+	serv_incompat_desc +
	20	+	na_dass_dpnss_desc +
	21	+	na_dass_desc +
	22	+	na_dpnss_desc +
	23	+	na_dass_dpnss_desc +
	24	+	"Facility Not Registered"	+
	25	+	na_dpnss_desc +
	26	+	na_dass_desc +
	27-29	+ na_dpnss_desc +
	30	+	nae_error_desc +
	31	+	"No Reply" +
	32	+	"Service Termination" +
	33	+	spare_desc +
	34	+	prot_vio_desc +
	35	+	na_dpnss_desc +
	36	+	"Operator Priority Access" +
	37	+	reserved_desc +
	38-40	+ spare_desc +
	41	+	cug_addr_bar_desc +
	42-44	+ spare_desc +
	45	+	dte_ctrl_nrdy_desc +
	46	+	dte_unctrl_nrdy_desc +
	47	+	"Null (unknown)" +
	48	+	sub_call_term_desc +
	49	+	spare_desc +
	50-51	+ na_dass_desc +
	52	+ reserved_desc +
	53-235	+ spare_desc +
	236-255 + "Reserved for International Use"
  ) * Property( is_optional|is_trailer )
);

RELR = RELR * Property( is_optional|is_trailer );

//
//	================================
//	SIM	'Information Contained' Code
//	SIM	'Information Requested' Code
//	================================
//
UIGroup SIM_INFO_CON( "SIMIC", simic_desc );
SIM_INFO_CON = SIM_INFO_CON +
  Field( "simic", simple, ff_uint, 8, simic_desc, Table(26) +
	0	+ simicr_0_desc +
	1	+ simicr_1_desc +
	2	+ simicr_2_desc +
	3	+ simicr_3_desc +
	4	+ simicr_4_desc +
	5	+ simicr_5_desc +
	6	+ simicr_6_desc +
	7	+ simicr_7_desc +
	8	+ simicr_8_desc +
	9	+ simicr_9_desc +
	10	+ simicr_10_desc +
	11	+ simicr_11_desc +
	12	+ simicr_12_desc +
	13-25 + simicr_13_desc
);

UIGroup SIM_INFO_REQ( "SIMIR", simir_desc );
SIM_INFO_REQ = SIM_INFO_REQ +
  Field( "simir", simple, ff_uint, 8, simir_desc, Table(26) +
	0	+ simicr_0_desc +
	1	+ simicr_1_desc +
	2	+ simicr_2_desc +
	3	+ simicr_3_desc +
	4	+ simicr_4_desc +
	5	+ simicr_5_desc +
	6	+ simicr_6_desc +
	7	+ simicr_7_desc +
	8	+ simicr_8_desc +
	9	+ simicr_9_desc +
	10	+ simicr_10_desc +
	11	+ simicr_11_desc +
	12	+ simicr_12_desc +
	13-25 + simicr_13_desc
);

//
//	================================
//	Facility Indicator Code for SIMs
//	================================
//
UIGroup FIC( "FIC", fic_desc );
FIC = FIC +
  Field( "fic", simple, ff_uint, 16, fic_desc, Table(65536) +
	0	+ "No information" +
	1	+ "Reserved for Version 1 User Part" +
	2	+ "CUG with OutgoingAccess" +
	3	+ "User-to-User Data Service available" +
	4	+ "User-to-User Data Service available and data following" +
	5	+ "CUG with Outgoing Access + User-to-User Data Service available"	+
	6	+ "CUG with Outgoing Access + User-to-User Data Service available and data following" +
	7-9	+ reserved_desc +
	10-65535 + spare_conf_rtn_if_rcvd
);

//
//	=================================================================
//
//					Service Indicator Codes for SIMs
//
//	BTNR 190 Section 4, Issue 1, Annex 1 - pp 3, 6, 7a, October 1989
//	BTNR 190 Section 4, Issue 2 Draft 1, Annex 1 - pp 1-7, April 1986
//
//	=================================================================
//
#if SIC_OK
Table SIC_Table1(16);	// Details of Speech Characteristics
SIC_Table1 = SIC_Table1 +
	0		+ "A-Law 64kbits/s"	+
	1-15	+ reserved_desc;

Table SIC_Table2(16);	// Details of Speech Characteristics
SIC_Table2 = SIC_Table2 +
	0		+ "A-Law 64kbits/s (CATEGORY 2) - digital or analogue routing" +
	1		+ reserved_desc +
	2		+ "A-Law 64kbits/s PCM (CATEGORY 1) - fully digital path required" +
	3-7		+ reserved_desc +
	8		+ "3.1kHz Audio" +
	9-14	+ "RESERVED (3.1kHz Audio)" +
	15		+ "3.1kHz Audio with Fax Group 2/3";

Table SIC_Table2A(16);	// Details of Speech Characteristics SINGLE OCTET
SIC_Table2A = SIC_Table2A +
	0		+ "A-Law 64kbits/s (CATEGORY 2) - digital or analogue routing" +
	1		+ reserved_desc +
	2		+ "A-Law 64kbits/s PCM (CATEGORY 1) - fully digital path required" +
	3-15	+ reserved_desc;

Table SIC_Table2B(15);	// Details of Speech Characteristics 2 OCTETS
SIC_Table2B = SIC_Table2B +
	0	+ reserved_desc +
	1	+ "0.6 kbits/s Recommendation X.1 and 1.461" +
	2	+ "1.2 kbits/s Recommendation X.1 and 1.461" +
	3	+ "2.4 kbits/s Recommendation X.1 and 1.461" +
	4	+ "3.6 kbits/s Recommendation V.6 and 1.463" +
	5	+ "4.8 kbits/s Recommendation X.1 and 1.461" +
	6	+ "7.2 kbits/s Recommendation V.6 and 1.463" +
	7	+ "8 kbits/s Recommendation 1.463" +
	8	+ "9.6 kbits/s Recommendation X.1 and 1.461" +
	9	+ "14.4 kbits/s Recommendation V.6 and 1.463" +
	10	+ "16 kbits/s Recommendation 1.460" +
	11	+ "19.2 kbits/s Recommendation 1.463" +
	12	+ "32 kbits/s Recommendation 1.460" +
	13	+ "48 kbits/s Recommendation X.1 and 1.461" +
	14	+ "56 kbits/s Recommendation 1.463";

Table SIC_Table3A(16);	// Data Rates for a SINGLE OCTET
SIC_Table3A = SIC_Table3A +
	0	+ reserved_desc +
	1	+ "48000 bits/s (6+2 rate adaption)" +
	2	+ "9600 bits/s (6+2 rate adaption)"	+
	3	+ reserved_desc +
	4	+ "4800 bits/s (6+2 rate adaption)"	+
	5	+ "2400 bits/s (6+2 rate adaption)"	+
	6-7	+ reserved_desc +
	8	+ "64000 bits/s"	+
	9-10 + reserved_desc +
	11	+ "8000 bits/s in bit 1" +
	12	+ "4800	bits/s (5-octet	frame in bit 1)"	+
	13	+ "2400	bits/s (half of 5-octet frame in bit 1)" +
	14	+ "8000 bits/s multisampled" +
	15	+ "64000 bits/s multisampled";

Table SIC_Table3B(16);	// Data Rates for a 2 OCTET Structure (CATEGORY 1 CALLS)
SIC_Table3B = SIC_Table3B +
	0	+ "64000 bits/s" +
	1	+ "56000 bits/s" +
	2	+ "48000 bits/s" +
	3	+ "32000 bits/s" +
	4	+ "19200 bits/s" +
	5	+ "16000 bits/s" +
	6	+ "14400 bits/s" +
	7	+ "12000 bits/s" +
	8	+ "9600 bits/s"	+
	9	+ "8000 bits/s"	+
	10	+ "7200 bits/s"	+
	11	+ "4800 bits/s"	+
	12	+ "3600 bits/s"	+
	13	+ "2400 bits/s"	+
	14	+ "1200 bits/s"	+
	15	+ "600 bits/s";

Table SIC_Table4(16);	// Data Rates (CATEGORY 1 CALLS)
SIC_Table4 = SIC_Table4 +
	0	+ "300 bits/s"	+
	1	+ "200 bits/s"	+
	2	+ "150 bits/s"	+
	3	+ "134.5 bits/s" +
	4	+ "110 bits/s"	+
	5	+ "100 bits/s"	+
	6	+ "75 bits/s" +
	7	+ "50 bits/s" +
	8	+ "75/1200 bits/s"	+
	9	+ "1200/75 bits/s"	+
	10	+ "384000 bits/s"	+
	11-15 +	reserved_desc;

Table SIC_Table5();		// Data Identification for Synchronous

Table SIC_Table6 ();	// Data Identification for Asynchronous

Table SIC_Table8(64);	// Modem Type (values extracted from ETSI T/S 46-30)
SIC_Table8 = SIC_Table8 +
	0	+ reserved_desc +
	1	+ "V.21" +
	2	+ "V.22" +
	3	+ "V.22 bis" +
	4	+ "V.23" +
	5	+ "V.26" +
	6	+ "V.26 bis" +
	7	+ "V.26 ter" +
	8	+ "V.27" +
	9	+ "V.27 bis" +
	10	+ "V.27 ter" +
	11	+ "V.2 9" +
	12	+ "V.32" +
	13	+ "V.35" +
	14-31 + reserved_desc +
	32-63 + "Reserved for national use";
#endif

// SIC Octet 1 bit 8
UIGroup SIC1_EXT( "SIC1_EXT", "SIC 1 - Extension bit" );
SIC1_EXT = SIC1_EXT +
  Field( "ext", simple, ff_uint, 1, "Extension bit", Table(2) +
	0	+ "No further octet (s)" +
	1	+ "Further octet(s)"
);

// SIC Octet 1 bits 7-6-5
UIGroup SIC1_TOI ( "SIC1_TOI", "SIC 1 - Type of Information" ) ;
SIC1_TOI = SIC1_TOI +
  Field( "toi", simple, ff_uint, 3, "Type of Information", Table(8) +
	0	+ speech_desc +
	1	+ speech_desc +
	2	+ data_desc +
	3	+ data_desc +
	4	+ "Teletex" +
	5	+ "Vidotex" +
	6	+ "Facsimile" +
	7	+ "SSTV"
);

// SIC Octet 1 bits 4-3-2-1
UIGroup SIC1_SCDR( "SIC1_SCDR", "SIC 1 - Speech Characteristics/Data Rate" );
SIC1_SCDR = SIC1_SCDR +
  Field( "scdr", simple, ff_uint, 4,
		  "Speech Characteristics/Data Rate - encoding depends on 'Type of Information' field (see BTNR 190)" );

// SIC Octet 1
LabelGroup SIC_OCTET_1( sic_octet1_desc, "SIC_OCTET_1" );
SIC_OCTET_1 = SIC_OCTET_1 +
	SIC1_SCDR +	//	bits    4321
	SIC1_TOI +	//	bits 765
	SIC1_EXT;	//	bit 8

// SIC Octet 2 Synch/Asynchronous Information
UIGroup SIC2_SASI( "SIC2_SASI", "SIC 2 - Synch/Asynchronous Information (coding when octet 1 does not indicate 3.1kHz Audio)" );
SIC2_SASI = SIC2_SASI +
  Field( "sasi", simple, ff_uint, 3, "Synch/Asynchronous Information", Table(8) +
	0	+ "Multisampled Async. (No terminal attributes)" +
	1-3	+ reserved_desc	+
	4	+ "Synchronous"	+
	5	+ "Async with 1	stop bit" +
	6	+ "Async with 1.5 stop bit" +
	7	+ "Async with 2	stop bits"
);

// SIC Octet 2 Duplex Mode indicator
UIGroup SIC2_DUPI( "SIC2_DU?I", "SIC 2 - Duplex Mode indicator" );
SIC2_DUPI = SIC2_DUPI +
  Field( "dupi", simple, ff_uint, 1, "Duplex Mode", Table(2) +
	0	+ "Full Duplex (FDX)" +
	1	+ "Half Duplex (HDX)"
);

// SIC Octet 2 Data Identification - choose between Asynchronous and Synchronous
#if SIC_OK
LabelGroup SIC2_ASYNC_DATA( "Data Identification for Asynchronous" );
SIC2_ASYNC_DATA = SIC2_ASYNC_DATA +
  Field( "btime", simple, ff_uint, 1, "Byte Timing", Table(2) +
	0	+ "Not provided" +
	1	+ "Provided"
  ) +
  Field( "dfmt", simple, ff_uint, 1, "Data Format", Table(2) +
	0	+ "Anonymous or unformatted" +
	1	+ "X.25 Packet Mode"
  ) +
  Field( "nic", simple, ff_uint, 1, "Network Independent Clock", Table(2) +
	0	+ "Clock lock to transmission" +
	1	+ "Bits E4/E5/E6 indicate phase"
  );

LabelGroup SIC2_SYNC_DATA( "Data Identification for Synchronous" );
SIC2_SYNC_DATA = SIC2_SYNC_DATA +
  Field( "dfmt", simple, ff_uint, 2, "Data Format", Table(4) +
	0	+ "Unspecified number of data bits" +
	1	+ "5 data bits" +
	2	+ "7 data bits" +
	3	+ "8 data bits"
  ) +
  Field( "fctrl", simple, ff_uint, 1, "Flow Control", Table(2) +
	0	+ "TA does not have capability" +
	1	+ "TA has ESRA capability"
  );

#endif

// SIC Octet 2
LabelGroup SIC_OCTET_2( sic_octet2_desc, "SIC_OCTET_2" );
SIC_OCTET_2 = SIC_OCTET_2 +
				SIC2_SASI +
				SIC2_DUPI +
#if SIC_OK // doesn't work at the moment, nice try
				(SelectOne( "SIC2_SASI:sasi" ) +
					0	+ SIC2_ASYNC_DATA +
					1-3 + Field( 0, simple, ff_uint, 3, reserved_desc ) +
					4	+ SIC2_SYNC_DATA +
					5-7 + SIC2_ASYNC_DATA
				) +
#else
				Field( "dataid", simple, ff_uint, 3, "Data Identification" ) +
#endif
				Field( 0, simple, ff_uint, 1, reserved_desc );

// SIC
LabelGroup SIC( sic_desc, "SIC" );
SIC = SIC +
		SIC_OCTET_1 +
		(Group( "SIC1_EXT:ext" ) +
				SIC_OCTET_2
);

//
//	===============================================
//	Closed User Group (CUG) Interlock Code for SIMs
//	===============================================
//
LabelGroup CUG_IC( cugic_desc, "CUGIC" );
CUG_IC = CUG_IC +
(UIGroup( "CUGIC", cugic_desc ) +
	Field( "D1D2D3D4", bcd, ff_bcd, 16, 4, 4, "Data Country Code (DCC) or Data Network Identification Code (DNIC)" ) +
	Field( "num", simple, ff_uint, 16, "International CUG number 'B'" )
);

//
//	========================================
//	Network Address Extension (NAE) for SIMs
//	========================================
//
LabelGroup NAE( nae_desc, "NAE" );
NAE = NAE +
(UIGroup( "NAE", nae_desc ) +
	Field( "num", length0, ff_uint, 8, 0, 6, "NAE Character count" ) +
	Field( "nae", octet, ff_ia5, 8, 0, 6, "NAE:num", "NAE Characters" )
);

//
//	=======================================
//	Business Group Indentity (BGI) for SIMs
//	=======================================
//
LabelGroup BGI ( bgi_desc, "BGI" );
BGI = BGI +
(UIGroup ( "BGI", bgi_desc ) +
	Field(	"tad",	bcd,	ff_bcd,		8,	2,	2,	tad_desc ) +
	Field(	"uni",	bcd,	ff_bcd,		16,	4,	4,	uni_desc ) +
	Field(	"grp",	simple,	ff_uint,	8,			bgi_grp_desc ) +
	Field(	"sgrp",	simple,	ff_uint,	6,			bgi_sgrp_desc ) +
	Field(	"ai",	simple,	ff_uint,	2,	0,	3,	bgi_ai_desc, Table(4) +
		0	+ "Not Operator/Attendant Originated Call" +
		1	+ "Operator/Attendant Originated Call" +
		2-3	+ "Spare - treat as value 'O'"
	)
);

//
//	================================
//	ACI 'Information Contained' Code
//	ACI 'Information Requested' Code
//	================================
//
UIGroup ACI_INFO_CON( "ACIIC", aciic_desc );
ACI_INFO_CON = ACI_INFO_CON +
  Field( "aciic", simple, ff_uint, 8, aciic_desc, Table(256) +
	0	+	aciicr_0_desc	+
	1	+	aciicr_1_desc	+
	2	+	aciicr_2_desc	+
	3	+	aciicr_3_desc	+
	4	+	aciicr_4_desc	+
	5	+	aciicr_5_desc	+
	6	+	aciicr_6_desc	+
	7	+	aciicr_7_desc	+
	8	+	aciicr_8_desc	+
	9	+	aciicr_9_desc	+
	10	+	aciicr_10_desc	+
	11-255	+	aciicr_11_desc
  );

UIGroup ACI_INFO_REQ( "ACIIR", aciir_desc );
ACI_INFO_REQ = ACI_INFO_REQ +
  Field( "aciir", simple, ff_uint, 8, aciir_desc, Table(256) +
	0	+	aciicr_0_desc	+
	1	+	aciicr_1_desc	+
	2	+	aciicr_2_desc	+
	3	+	aciicr_3_desc	+
	4	+	aciicr_4_desc	+
	5	+	aciicr_5_desc	+
	6	+	aciicr_6_desc	+
	7	+	aciicr_7_desc	+
	8	+	aciicr_8_desc	+
	9	+	aciicr_9_desc	+
	10	+ aciicr_10_desc +
	11-255	+ aciicr_11_desc
  );

//
//	=====================================
//	Operator Indicator for ACI Type 1,...
//	=====================================
//
UIGroup OPER_IND( "OPI", opi_desc );
OPER_IND = OPER_IND +
  Field( "opi", simple, ff_uint, 1, opi_desc, Table(2) +
	0	+ "No operator involved" +
	1	+ "Operator involved"
  );

//
//	=========================================================
//	Calling/Called Subscriber's Basic Service Markings Tables
//	=========================================================
//
Table CBSM_Table_A(2);
CBSM_Table_A = CBSM_Table_A +
				0	+ "Incoming Admin/Maintenance calls not barred" +
				1	+ "Incoming Admin/Maintenance calls barred (i.e. OOR or monitor barred)";

Table CBSM_Table_B(2);
CBSM_Table_B = CBSM_Table_B +
				0	+ "Subscriber Controlled ICB not active" +
				1	+ "Subscriber Controlled ICB active";

Table CBSM_Table_C(2);
CBSM_Table_C = CBSM_Table_C +
				0	+ "Pre-arranged ICB not active" +
				1	+ "Pre-arranged ICB active";

Table CBSM_Table_D(2);
CBSM_Table_D = CBSM_Table_D +
				0	+ "Permanent ICB not active" +
				1	+ "Permanent ICB active";

Table CBSM_Table_E(2);
CBSM_Table_E = CBSM_Table_E +
				0	+ "Temporary Out of Service (TOS) not active" +
				1	+ "Temporary Out of Service (TOS) active";

Table CBSM_Table_F(2);
CBSM_Table_F = CBSM_Table_F +
				0	+ "IC3 except to operators not active" +
				1	+ "ICB except to operators active";

Table CBSM_Table_G(2);
CBSM_Table_G = CBSM_Table_G +
				0	+ "Called Subscriber facility information not available" +
				1	+ "Called Subscriber facility information available";

Table CBSM_Table_H(2);
CBSM_Table_H = CBSM_Table_H +
				0	+ "Calling Subscriber facility information not available" +
				1	+ "Calling Subscriber facility information available";

Table CBSM_Table_I(2);
CBSM_Table_I = CBSM_Table_I +
				0	+ "Permanent OCB not active" +
				1	+ "Permanent OCB active";

Table CBSM_Table_J(2);
CBSM_Table_J = CBSM_Table_J +
				0	+ "Outgoing local calls not barred" +
				1	+ "Outgoing local calls barred";

Table CBSM_Table_K(2);
CBSM_Table_K = CBSM_Table_K +
				0	+ "Outgoing national calls not barred" +
				1	+ "Outgoing national calls barred";

Table CBSM_Table_L(2);
CBSM_Table_L = CBSM_Table_L +
				0	+ "Outgoing international calls not barred" +
				1	+ "Outgoing international calls barred";

Table CBSM_Table_M(2);
CBSM_Table_M = CBSM_Table_M +
				0	+ "Calls to operator not barred" +
				1	+ "Calls to operator barred";

Table CBSM_Table_N(2);
CBSM_Table_N = CBSM_Table_N +
				0	+ "Supplementary facility calls not barred" +
				1	+ "Supplementary facility calls barred (except removal of barring)";

Table CBSM_Table_O(2);
CBSM_Table_O = CBSM_Table_O +
				0	+ "Digit masking not required" +
				1	+ "Digit masking required";

//
//	==============================================================
//	Calling Subscriber's Basic Service Markings for ACI Type 3,...
//	==============================================================
//
LabelGroup CGBSM( cgbsm_desc, "CGBSM" );
CGBSM = CGBSM +
(UIGroup( "CGBSM", cgbsm_desc ) +
	Field( "A",	simple,	ff_uint,	1,	cbsm_bitA_desc,	CBSM_Table_A	)	+
	Field( "B",	simple,	ff_uint,	1,	cbsm_bitB_desc,	CBSM_Table_B	)	+
	Field( "C",	simple,	ff_uint,	1,	cbsm_bitC_desc,	CBSM_Table_C	)	+
	Field( "D",	simple,	ff_uint,	1,	cbsm_bitD_desc,	CBSM_Table_D	)	+
	Field( "B",	simple,	ff_uint,	1,	cbsm_bitE_desc,	CBSM_Table_E	)	+
	Field( "F",	simple,	ff_uint,	1,	cbsm_bitF_desc,	CBSM_Table_F	)	+
	Field( "G",	simple,	ff_uint,	1,	cbsm_bitG_desc,	CBSM_Table_G	)	+
	Field( "H",	simple,	ff_uint,	1,	cbsm_bitH_desc,	CBSM_Table_H	)	+
	Field( "I",	simple,	ff_uint,	1,	cbsm_bitI_desc,	CBSM_Table_I	)	+
	Field( "J",	simple,	ff_uint,	1,	cbsm_bitJ_desc,	CBSM_Table_J	)	+
	Field( "K",	simple,	ff_uint,	1,	cbsm_bitK_desc,	CBSM_Table_K	)	+
	Field( "L",	simple,	ff_uint,	1,	cbsm_bitL_desc,	CBSM_Table_L	)	+
	Field( "M",	simple,	ff_uint,	1,	cbsm_bitM_desc,	CBSM_Table_M	)	+
	Field( "N",	simple,	ff_uint,	1,	cbsm_bitN_desc,	CBSM_Table_N	)	+
	Field( "O",	simple,	ff_uint,	1,	cbsm_bitO_desc,	CBSM_Table_O	)	+
	Field( "P",	simple,	ff_uint,	1,	spare_desc )
) ;

//
//	=============================================================
//	Called Subscriber's Basic service markings for ACI Type 3,...
//	=============================================================
//
LabelGroup CDBSM( cdbsm_desc, "CDBSM" );
CDBSM = CDBSM +
(UIGroup( "CDBSM", cdbsm_desc ) +
	Field(	"A",	simple,	ff_uint,	1,	cbsm_bitA_desc,	CBSM_Table_A	)	+
	Field(	"B",	simple,	ff_uint,	1,	cbsm_bitB_desc,	CBSM_Table_B	)	+
	Field(	"C",	simple,	ff_uint,	1,	cbsm_bitC_desc,	CBSM_Table_C	)	+
	Field(	"D",	simple,	ff_uint,	1,	cbsm_bitD_desc,	CBSM_Table_D	)	+
	Field(	"E",	simple,	ff_uint,	1,	cbsm_bitE_desc,	CBSM_Table_E	)	+
	Field(	"F",	simple,	ff_uint,	1,	cbsm_bitF_desc,	CBSM_Table_F	)	+
	Field(	"G",	simple,	ff_uint,	1,	cbsm_bitG_desc,	CBSM_Table_G	)	+
	Field(	"H",	simple,	ff_uint,	1,	cbsm_bitH_desc,	CBSM_Table_H	)	+
	Field(	"I",	simple,	ff_uint,	1,	cbsm_bitI_desc,	CBSM_Table_I	)	+
	Field(	"J",	simple,	ff_uint,	1,	cbsm_bitJ_desc,	CBSM_Table_J	)	+
	Field(	"K",	simple,	ff_uint,	1,	cbsm_bitK_desc,	CBSM_Table_K	)	+
	Field(	"L",	simple,	ff_uint,	1,	cbsm_bitL_desc,	CBSM_Table_L	)	+
	Field(	"M",	simple,	ff_uint,	1,	cbsm_bitM_desc,	CBSM_Table_M	)	+
	Field(	"N",	simple,	ff_uint,	1,	cbsm_bitN_desc,	CBSM_Table_N	)	+
	Field(	"O",	simple,	ff_uint,	1,	cbsm_bitO_desc,	CBSM_Table_O	)	+
	Field(	"P",	simple,	ff_uint,	1,	spare_desc )
) ;

//
//	====================================================
//	Calling Subscriber's Tariff Group for ACI Type 3,...
//	====================================================
//
UIGroup CGSTARIFF( "CGSTARIFF", cgstariff_desc );
CGSTARIFF = CGSTARIFF + Field( "tariff_group", simple, ff_uint, 6, cgstariff_desc );

//
//	===================================================
//	Called Subscriber's Tariff Group for ACI Type 3,...
//	===================================================
//
UIGroup CDSTARIFF( "CDSTARIFF", cdstariff_desc );
CDSTARIFF = CDSTARIFF + Field( "tariff_group", simple, ff_uint, 6, cdstariff_desc );

//
//	=========================================================================
//	Calling Subscriber's Originating Facility Service Markings for ACI Type 5
//	=========================================================================
//
LabelGroup CGOFSM( cgofsm_desc, "CGOFSM" );
CGOFSM = CGOFSM +
(UIGroup( "CGOFSM", cgofsm_desc ) +
	Field( "A", simple, ff_uint, 1, "'Disabled Subscriber' indicator", Table(2) +
		0	+ "Not a disabled subscriber" +
		1	+ "Disabled Subscriber"
	) +
	Field( "B", simple, ff_uint, 1, "'Attended Call Office' indicator", Table(2) +
		0	+ "Not reserved for Attended Call Office" +
		1	+ "Reserved for Attended Call Office (booth working)"
	) +
	Field( "C", simple, ff_uint, 1, "'Duration/Charge Advice' indicator", Table(2) +
		0	+ "Duration and Charge Advice not required" +
		1	+ "Advise Duration and Charge required on All Calls"
	) +
	Field( "D", simple, ff_uint, 1, "'PBX Subscriber' indicator", Table(2) +
		0	+ "Not a PBX subscriber" +
		1	+ "PBX Subscriber"
	) +
	Field(	0, simple, ff_uint, 12, spare_desc )
);

//
//	========================================================================
//	Called Subscriber's Terminating Facility Service Markings for ACI Type 6
//	========================================================================
//
LabelGroup CDTFSM ( cdtfsm_desc, "CDTFSM" );
CDTFSM = CDTFSM +
(UIGroup( "CDTFSM", cdtfsm_desc ) +
	Field( "A", simple, ff_uint, 1, "'Subscriber on \"Service Interception\" (SVI)' indicator", Table(2) +
		0	+ "Subscriber not on SVI" +
		1	+ "Subscriber on SVI"
	) +
	Field( "B", simple, ff_uint, 1, "'Subscriber on \"Changed Number Interception\" (CNI)1 indicator", Table(2) +
		0	+ "Subscriber not on CNI" +
		1	+ "Subscriber on CNI"
	) +
	Field( "C", simple, ff_uint, 1, "Subscriber on PBX Night Interception", Table(2) +
		0	+ "Subscriber not on PBX Night Interception" +
		1	+ "Subscriber on PBX Night Interception"
	) +
	Field( "D", simple, ff_uint, 1, "'Call Waiting' indicator", Table(2) +
		0	+ "Call Waiting not active" +
		1	+ "Call Waiting Active"
	) +
	Field( "E", simple, ff_uint, 1, "'Fixed Destination Service' indicator", Table(2) +
		0	+ "Not a fixed destination service" +
		1	+ "Fixed Destination service"
	) +
	Field(	0, simple, ff_uint, 11, spare_desc )
);

//
//	=====================================
//	Interworking Indicator for ACI Type 8
//	=====================================
//
UIGroup ACIIWI( "ACIIWI", aci_iwi_desc );
ACIIWI = ACIIWI + Field( "aciiwi", simple, ff_uint, 1, aci_iwi_desc, Table(2) +
						0	+ "no interworking involved" +
						1	+ "analogue network encountered"
					);

//
//	==============================================
//	Nodal End-to-End Data Indicator for ACI Type 8
//	==============================================
//
UIGroup NEEDI( "NEEDI", needi_desc );
NEEDI = NEEDI + Field( "needi", simple, ff_uint, 1, needi_desc, Table(2) +
						0	+ "Nodal End-to-End Data transfer not available" +
						1	+ "Nodal End-to-End Data transfer may be invoked"
					);

//-----------------------------------------------------------------------------------------------------------------------------------------

//
//	==================================
//	FORWARD ADDRESS MESSAGES (GROUP 0)
//	==================================
//
Group H0_0; H0_0 = H0_0 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
	Field( msgtype_H1, simple, ff_uint, 8, msgtype_H1, video_ops, Table(4) +
		0	+ iam_desc +
		1	+ ifam_desc +
		2	+ sam_desc +
		3	+ fam_desc
	)
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
	0 +
		(Message( "IAM", iam_desc ) +
			CGPC +
			ONUPVI +
			MSG_IND_1 +
			SHP +
			MSG_IND_2 +
			NI +
			RCI +
			CPI +
			ADDR_SIG20 +
			(Group( "OLII:olii" ) +
				OLI
			)
		) +
	1 +
		(Message( "IFAM", ifam_desc ) +
			CGPC +
			ONUPVI +
			MSG_IND_1 +
			SHP +
			MSG_IND_2 +
			NI +
			RCI +
			CPI +
			ADDR_SIG20 +
			(Group( "OLII:olii" ) +
			OLI
			)
		) +
	2 +
		(Message( "SAM", sam_desc ) +
			Field( 0, simple, ff_uint, 8, spare_desc ) +
			ADDR_SIG18
		) +
	3 +
	(Message( "FAM", fam_desc ) +
		Field( 0, simple, ff_uint, 8, spare_desc ) +
		ADDR_SIG18
	)
);

//
//	=================================
//	FORWARD SET-UP MESSAGES (GROUP 1)
//	=================================
//
Group H0_1; H0_1 = H0_1 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
	Field( msgtype_H1, simple, ff_uint, 8, msgtype_H1, video_ops, Table(1) +
		4 + asui_desc
	)
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
	4 +
		(Message( "ASUI", asui_desc ) +
			ASUI_INFO_IND +
			(SelectOne( "ASUI_IND:asui_ic" ) +
				1	+ OLI +
				2	+ PCGLI
			)
		)
);

//
//	==========================================
//	BACKWARD SET-UP REQUEST MESSAGES (GROUP 2)
//	==========================================
//
Group H0_2; H0_2 = H0_2 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
	Field( msgtype_H1, simple, ff_uint, 8, msgtype_H1, video_ops, Table(3) +
		2	+ snd_desc +
		3	+ sad_desc +
		4	+ sasui_desc
	)
) * Property( invisible ) +
(SelectOne ( msgtype_h1_varname ) +
	2 +
		(Message( "SND", snd_desc ) +
			(UIGroup( "SND_NDR", snd_ndr_desc ) +
				Field( "ndr", simple, ff_uint, 4, snd_ndr_desc, Table(16) +
					0	+ "Invalid value - shall cause Call Failure (NU), an incident report shall be given" +
					1-15 + "Valid NDR value"
				)
			) +
			Field( 0, simple, ff_uint, 4, spare_desc )
		) +
	3 +
		(Message( "SAD", sad_desc ) +
			Field( 0, simple, ff_uint, 8, spare_desc )
		) +
	4 +
		(Message( "SASUI", sasui_desc ) +
			(UIGroup( "SASUI_IND", sasui_ind_desc ) +
				Field( "sasui_ind", simple, ff_uint, 8, 0, 1, sasui_ind_desc, Table(256) +
					0	+ spare_desc +
					1	+ "Send Calling Line Identity" +
					2	+ spare_desc +
					3-6 + reserved_non_bt_use +
					7-255 + spare_desc
				)
			)
		)
);

//
//	=============================================
//	BACKWARD SET-UP INFORMATION MESSAGE (GROUP 3)
//	=============================================
//
Group H0_3; H0_3 = H0_3 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
	Field( msgtype_H1, simple, ff_uint, 8, msgtype_H1, video_ops, Table(256) +
		0	+ acm_desc +
		1	+ intra_exch_use_only +
		2	+ cong_desc +
		3	+ tcong_desc +
		4	+ cna_desc +
		5	+ rpta_desc +
		6	+ seng_desc +
		7	+ sooo_desc +
		8	+ stfr_desc +
		9	+ reserved_non_bt_use +
		10	+ cdb_desc +
		11-255 + spare_desc
	)
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
	0 +
		(Message( "ACM", acm_desc ) +
			(LabelGroup( cdpc_desc ) +
				CDPC +
				Field( 0, simple, ff_uint, 8, spare_desc )
			) +
			ACM_NUM_RCVD_IND
		) +
	2 +
		(Message( "CONG", cong_desc )
		) +
	3 +
		(Message( "TCONG", tcong_desc )
		) +
	4 +
		(Message ( "CNA", cna_desc ) +
			CNAR +
			Field( 0, simple, ff_uint, 8, spare_desc )
		) +
	5 +
		(Message( "RPTA", rpta_desc )
		) +
	6 +
		(Message( "SENG", seng_desc )
		) +
	7 +
		(Message( "SOOO", sooo_desc )
		) +
	8 +
		(Message( "STFR", stfr_desc )
		) +
	10 +
		(Message( "CDB", cdb_desc ) +
			(UIGroup( "CLOGI", cdb_clogi_desc ) +
				Field( "clogi", simple, ff_uint, 4, 0, 1, cdb_clogi_desc, Table(16) +
					0		+ "Non-Call Log" +
					1		+ "Call Log" +
					2-15	+ "Spare - treat as value '0'"
				)
			) +
			Field( 0, simple, ff_uint, 4, spare_desc ) + // for future extension of Call Log Indicator
			(LabelGroup( indb_desc, "INDB" ) +
				(UIGroup( "INDB", indb_desc ) +
					Field( "length", length0, ff_uint, 8, 0, 9, "Length of INDB Information" ) +
					Field( "ind", simple, ff_uint, 8, "INDB Information Indicator" ) +
					Field( "data", octet, ff_hex, 8, 0, 9, "INDB:length", "INDB Information" )
				)
			) +
			(UIGroup( "CDB_CLI_IND", cdb_cli_infoi_desc ) +
				Field( "cli_info_ind", simple, ff_uint, 8, cdb_cli_infoi_desc, Table(256) +
					0		+ "NULL" +
					1		+ "Full Calling Line Identity (OLI) included" +
					2		+ "Partial Calling Line Identity (PCGLI) included" +
					3-255	+ spare_desc
				)
			) +
			(SelectOne( "CDB_CLI_IND:cli_info_ind" ) +
				1	+ OLI +
				2	+ PCGLI
			) +
			TRANSLATED_ADDR_SIG20
		)
);

//
//	===================================
//	CALL SUPERVISION MESSAGES (GROUP 4)
//	===================================
//
Group H0_4; H0_4 = H0_4 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
	Field( msgtype_H1, simple, ff_uint, 8, msgtype_H1, video_ops, Table(256) +
		0		+ anm_desc	+
		1		+ clr_desc	+
		2		+ ran_desc	+
		3		+ rel_desc	+
		4		+ cfc_desc	+
		5		+ oor_desc	+
		6		+ hwlr_desc +
		7		+ extc_desc +
		8		+ intra_exch_use_only	+
		9-13	+ reserved_non_bt_use	+
		14-255	+ spare_desc
	)
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
	0 +
		(Message( "ANM", anm_desc ) +
			(UIGroup( "ANM_TYPE", anm_type_desc ) +
				Field( "type", simple, ff_uint, 4, anm_type_desc, Table(16) +
					0		+ "Non-Chargeable" +
					1		+ "Chargeable" +
					2-15	+ "Spare - treat as value '1'; an incident report shall be made"
				) +
				Field( 0, simple, ff_uint, 4, spare_desc )
			)
		) +
	1 +
		(Message( "CLR", clr_desc )
		) +
	2 +
		(Message( "RAN", ran_desc )
		) +
	3 +
		(Message( "REL", rel_desc ) +
			RELR
		) +
	4 +
		(Message( "CFC", cfc_desc )
		) +
	5 +
		(Message( "OOR", oor_desc )
		) +
	6 +
		(Message( "HWLR", hwlr_desc )
		) +
	7 +
		(Message( "EXTC", extc_desc )
		)
);

//
//	======================================
//	CIRCUIT SUPERVISION MESSAGES (GROUP 5)
//	======================================
//
Group H0_5; H0_5 = H0_5 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
	Field( msgtype_H1, simple, ff_uint, 8, msgtype_H1, video_ops, Table(256) +
		0		+ ctf_desc +
		1		+ blo_desc +
		2		+ ubl_desc +
		3		+ bla_desc +
		4		+ uba_desc +
		5		+ ovld_desc +
		6-10	+ intra_exch_use_only +
		11		+ cgb_desc +
		12		+ cgu_desc +
		13		+ cgba_desc +
		14		+ cgua_desc +
		15		+ cgr_desc +
		16		+ cgra_desc +
		17-255	+ spare_desc
	)
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
	0 +
		(Message( "CTF", ctf_desc )
		) +
	1 +
		(Message( "BLO", blo_desc )
		) +
	2 +
		(Message( "UBL", ubl_desc )
		) +
	3 +
		(Message( "BLA", bla_desc )
		) +
	4 +
		(Message( "DBA", uba_desc )
		) +
	5 +
		(Message( "OVLD", ovld_desc )
		) +
	11 +
		(Message( "CGB", cgb_desc ) +
			(UIGroup( "CGB", cgb_desc ) +
				(LabelGroup( msg_ind_desc ) +
					Field( type_field, simple, ff_uint, 2, type_ind_desc, Table(4) +
						0	+	block_no_immed_rel	+
						1	+	block_immed_rel	+
						2-3	+	spare_desc
					) +
					Field( 0, simple, ff_uint, 6, spare_desc )
				) +
				Field( range_field, length1, ff_uint, 8, range_desc ) +
				Field( status_field, bit8, ff_bit, 1, 0, 256/8, "CGB:range", status_desc, Table(2) +
					0	+	"no blocking" +
					1	+	"blocking"
				) * Property( add_one_to_control_value )
			)
		) +
	12 +
		(Message( "CGU", cgu_desc ) +
			(UIGroup( "CGU", cgu_desc ) +
				(LabelGroup( msg_ind_desc ) +
					Field( type_field, simple, ff_uint, 2, type_ind_desc, Table(4) +
						0	+ block_no_immed_rel +
						1	+ block_immed_rel +
						2-3 + spare_desc
					) +
					Field( 0, simple, ff_uint, 6, spare_desc )
				) +
				Field( range_field, length1, ff_uint, 8, range_desc ) +
				Field( status_field, bit8, ff_bit, 1, 0, 256/8, "CGU:range", status_desc, Table(2) +
					0	+	"no blocking acknowledgement" +
					1	+	"blocking acknowledgement"
				) * Property( add_one_to_control_value )
			)
		) +
	13	+
		(Message( "CGBA", cgba_desc ) +
			(UIGroup( "CGBA", cgba_desc ) +
				(LabelGroup( msg_ind_desc ) +
					Field( type_field, simple, ff_uint, 2, type_ind_desc, Table(4) +
						0	+	block_no_immed_rel	+
						1	+	block_immed_rel	+
						2-3	+	spare_desc
					) +
					Field( 0, simple, ff_uint, 6, spare_desc )
				) +
				Field( range_field, length1, ff_uint, 8, range_desc ) +
				Field( status_field, bit8, ff_bit, 1, 0, 256/8, "CGBA:range", status_desc, Table(2) +
					0	+	"no unblocking"	+
					1	+	"unblocking"
				) * Property( add_one_to_control_value )
			)
		) +
	14	+
		(Message( "CGUA", cgua_desc ) +
			(UIGroup( "CGUA", cgua_desc ) +
				(LabelGroup( msg_ind_desc ) +
					Field( type_field, simple, ff_uint, 2, type_ind_desc, Table(4) +
						0	+	block_no_immed_rel +
						1	+	block_immed_rel +
						2-3 + spare_desc
					) +
					Field( 0, simple, ff_uint, 6, spare_desc )
				) +
				Field( range_field, length1, ff_uint, 8, range_desc ) +
				Field( status_field, bit8, ff_bit, 1, 0, 256/8, "CGUA:range", status_desc, Table(2) +
					0	+	"no unblocking acknowledgement" +
					1	+	"unblocking acknowledgement"
				) * Property( add_one_to_control_value )
			)
		) +
	15	+
		(Message( "CGR", cgr_desc ) +
			(UIGroup( "CGR", cgr_desc ) +
				Field( range_field, length1, ff_uint, 8, range_desc ) +
				Field( status_field, bit8, ff_bit, 1, 0, 256/8, "CGR:range", status_desc, Table(2) +
					0	+ "Circuit should not be reset" +
					1	+ "Circuit should be reset"
				) * Property( add_one_to_control_value )
			)
		) +
	16 +
		(Message( "CGRA", cgra_desc ) +
			(UIGroup( "CGRA", cgra_desc ) +
				Field( range_field, length1, ff_uint, 8, range_desc ) +
				Field( status_field, bit8, ff_bit, 1, 0, 256/8, "CGRA:range", status_desc, Table(2) +
					0	+ "Circuit Idle" +
					1	+ "Circuit blocked"
				) * Property( add_one_to_control_value )
			)
		)
);

//
//	=====================================================
//	NON END-TO-END SERVICE INFORMATION MESSAGES (GROUP 6)
//	=====================================================
//
Group H0_6; H0_6 = H0_6 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
	Field( msgtype_H1, simple, ff_uint, 8, msgtype_H1, video_ops, Table(256) +
		0-7		+ intra_exch_use_only +
		8		+ nhtr_desc +
		9		+ htrr_desc +
		10		+ hcan_desc +
		11-255	+ spare_desc
	)
) * Property( invisible );

//
//	=========================================================
//	Nodal End-To-End Data Message (Group 7) - see usage below
//	=========================================================
//
Message Msg_NEED( "NEED", need_desc );
Msg_NEED = Msg_NEED +
(UIGroup( "NEED", need_desc ) +
	Field( "qual", simple, ff_uint, 8, "Content Qualifier", Table(256) +
		0	+ "Null (no information on content)" +
		1	+ "VPN/BES Real Call message" +
		2	+ "VPN/BES Virtual Call message" +
		3-255	+ "Spare - treat as value '0'"
	) +
	Field( 0, simple, ff_uint, 8, spare_desc ) +
	Field( "count", simple, ff_uint, 6, 0, 45, "Nodal End-to-End Data Octet Count" ) +
	Field( 0, simple, ff_uint, 2, spare_desc ) +
	Field( "data", octet, ff_hex, 8, 0, 45, "NEED:count", "Nodal End-to-End Data" )
);

//
//	=================================================
//	END-TO-END SERVICE INFORMATION MESSAGES (GROUP 7)
//	=================================================
//
Group H0_7; H0_7 = H0_7 +
(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
	Field( msgtype_H1, simple, ff_uint, 8, msgtype_H1, video_ops, Table(256) +
		0		+ conf_desc +
		1		+ isdn_csim_desc +
		2		+ sser_desc +
		3		+ serv_desc +
		4		+ aci_desc +
		5		+ opcm_desc +
		6		+ uudm_desc +
		7		+ swap_desc +
		8		+ dam_desc +
		9		+ eeacm_desc +
		10		+ reserved_desc +
		11		+ need_desc +
		12-255	+ spare_desc
	)
) * Property( invisible ) +
(SelectOne( msgtype_h1_varname ) +
	0 +
		(Message( "CONF", conf_desc ) +
			(UIGroup( "CONF", conf_desc ) +
				Field( "qual", simple, ff_uint, 8, "Qualifier", Table(256) +	// H1H! !!!!!!!!! with or without qualifier ?????????
					0		+ "Null" +
					1		+ "Version 2 User Part Node" +
					2		+ "Version 3 User Part Node" +
					3-255	+ spare_desc
				)
			)
		) +
	1 +
		(Message( "SIM", isdn_csim_desc ) +
			SIM_INFO_CON +
			(SelectOne( "SIMIC:simic" ) +
				0 +	// SIM 'A' Type 1 (without Facility Indicator)
					(Group() +
						SIM_INFO_REQ	// simir should be 3
					) +
				1 +	// SIM VC Type 4 (without Facility Indicator)
					(Group() +
						SIM_INFO_REQ +	// simir should be 0
						Field( 0, simple, ff_uint, 8, spare_desc ) +
						TLI
					) +
				2 +	// SIM 'B' Type 3 (with CUG)
					(Group() +
						SIM_INFO_REQ +	// simir should be 1 or 7
						FIC +
						SIC +
						CUG_IC +
						Field( 0, simple, ff_uint, 8, spare_desc ) +
						OLI
					) +
				3 +	// SIM 'B' Type 2 (without CUG or NAE)
					(Group() +
						SIM_INFO_REQ +	// simir should be 1 or 7
						FIC +
						SIC +
						Field( 0, simple, ff_uint, 8, spare_desc ) +
						OLI
					) +
				4 +	// SIM 'B' Type 6 (with 6-character NAE and CUG)
					(Group() +
						SIM_INFO_REQ +	// simir should be 7
						FIC +
						SIC +
						CUG_IC +
						Field( 0, simple, ff_uint, 8, spare_desc ) +
						OLI +
						NAE
					) +
				5 +	// SIM 'B' Type 5 (with 6-character NAE)
					(Group() +
						SIM_INFO_REQ +	// simir should be 7
						FIC +
						SIC +
						Field( 0, simple, ff_uint, 8, spare_desc ) +
						OLI +
						NAE
					) +
				6 +	// SIM 'A' Type 7 (with Facility Indicator)
					(Group() +
						SIM_INFO_REQ +	// simir should be 3, 5, 9 or 11
						FIC
					) +
				7 +	// SIM 'C' Type 8 (with Facility Indicator)
					(Group() +
						SIM_INFO_REQ +	// simir should be 0
						FIC +
						Field( 0, simple, ff_uint, 8, spare_desc ) +
						TLI
					) +
				8 +	// SIM 'B' Type 10 (with BGI and CUG)
					(Group() +
						SIM_INFO_REQ +	// simir should be 12
						FIC +
						SIC +
						BGI +
						CUG_IC +
						Field( 0, simple, ff_uint, 8, spare_desc ) +
						OLI
					) +
				9 +	// SIM 'B' Type 9 (with BGI)
					(Group() +
						SIM_INFO_REQ +	// simir should be 12
						FIC +
						SIC +
						BGI +
						Field( 0, simple, ff_uint, 8, spare_desc ) +
						OLI
					) +
				10 + // SIM 'B' Type 12 (with BGI, CUG and NAE)
					(Group() +
						SIM_INFO_REQ +	// simir should be 12
						FIC +
						SIC +
						BGI +
						CUG_IC +
						Field( 0, simple, ff_uint, 8, spare_desc ) +
						OLI +
						NAE
					) +
				11 +	// SIM 'B' Type 11 (with BGI and 6-character NAE)
					(Group() +
						SIM_INFO_REQ +	// simir should be 12
						FIC +
						SIC +
						BGI +
						Field( 0, simple, ff_uint, 8, spare_desc ) +
						OLI +
						NAE
					) +
				12 +	// SIM 'C' Type 13 (with Facility Indicator and BGI)
					(Group() +
						SIM_INFO_REQ +	// simir should be 0
						FIC +
						BGI +
						Field( 0, simple, ff_uint, 8, spare_desc ) +
						TLI
					)
			)
		) +
	2	+
		(Message( "SSER", sser_desc )
		) +
	3	+
		(Message( "SERV", serv_desc ) +
			(UIGroup( "SERVC", serv_code_desc ) +
				Field( "serve", simple, ff_uint, 8, serv_code_desc, Table(256) +
					0		+ reserved_desc	+
					1		+ "OOR"	+
					2		+ "SVI"	+
					3		+ "CNI"	+
					4		+ "PBX Night Interception" +
					5		+ "Trunk Subscriber" +
					6		+ "Emergency Trunk Subscriber" +
					7		+ "Fire Telephone"	+
					8		+ "Distant Operator Assistance" +
					9		+ "Distant EQ" +
					10		+ "Distant DQ" +
					11-255	+ spare_conf_rtn_if_rcvd
				)
			)
		) +
	4 +
		(Message( "ACI", aci_desc ) +
			ACI_INFO_CON +
				(SelectOne( "ACIIC:aciic" ) +
					0 +
						(Group() +	// ACI Type 7 containing Request for Additional Call Information
							ACI_INFO_REQ	// aciir should contain a value other than 0
						) +
					1 +	// ACI Type 1 with Full Calling Line Identity
						(Group() +
							ACI_INFO_REQ +	// aciir should be 0
							Field( 0, simple, ff_uint, 23, spare_desc ) +
							OPER_IND +
							OLI
						) +
					2 +	// ACI Type 1 with Full Called Line Identity
						(Group() +
							ACI_INFO_REQ +	// aciir should be 0
							Field( 0, simple, ff_uint, 23, spare_desc ) +
							OPER_IND +
							TLI
						) +
					3 +	// ACI Type 2 with Partial Calling Line Identity
						(Group() +
							ACI_INFO_REQ +	// aciir should be 0
							PCGLI
						) +
					4 +	// ACI Type 2 with Partial Called Line Identity
						(Group() +
							ACI_INFO_REQ +	//aciir should be 0
							PCDLI
						) +
					5 +	// ACI Type 3 with Full OLI and CGBSM
						(Group() +
							ACI_INFO_REQ +	// aciir should be 0
							CGBSM +
							CGSTARIFF +
							Field( 0, simple, ff_uint, 1, spare_desc ) +
							OPER_IND +
							OLI
						) +
					6 +	// ACI Type 3 with Full TLI and CDBSM
						(Group() +
							ACI_INFO_REQ +	// aciir should be 0
							CDBSM +
							CDSTARIFF +
							Field( 0, simple, ff_uint, 1, spare_desc ) +
							OPER_IND +
							TLI
						) +
					7 +	// ACI Type 4 with CDBSM
						(Group() +
							ACI_INFO_REQ +	// aciir should be 0
							CDBSM +
							CDSTARIFF +
							Field( 0, simple, ff_uint, 2, spare_desc )
						) +
					8 +	// ACI Type 5 with CGOFSM
						(Group() +
							ACI_INFO_REQ +	// aciir should be 0
							CGOFSM
						) +
					9 +	// ACI Type 6 with CDTFSM
						(Group() +
							ACI_INFO_REQ +	// aciir should be 0
							CDTFSM
						) +
					10 +	// ACI Type 8 containing Called Network Identifier, CDPC and TLI
						(Group() +
							ACI_INFO_REQ +	// aciir should be 0
							Field( 0, simple, ff_uint, 12, spare_desc ) +
							NI +	// NI shared with IAM & IFAM
							CDPC +
							ACIIWI +
							NEEDI +
							TLI
						)
				)
		) +
	5 +
		(Message( "OPCM", opcm_desc ) +
			(UIGroup( "OC", opcm_desc ) +
				Field( "oci", simple, ff_uint, 8, "Operator Condition Indicator", Table(256) +
					0		+ "Operator Answer" +
					1		+ "Operator in Circuit"	+
					2		+ "Operator Out of Circuit" +
					3		+ "Operator Recall" +
					4		+ "Operator Relinquish"	+
					5		+ "Subscriber Clear" +
					6-255	+ spare_conf_rtn_if_rcvd
				) +
				Field( "cnq", simple, ff_uint, 8, "Condition Qualifier", Table(256) +
					0		+ "NULL" +
					1		+ "Path Split" +
					2		+ "Path Not Split" +
					3-255	+ spare_conf_rtn_if_rcvd
				)
			)
		) +
	6 +
		(Message( "UUDM", uudm_desc ) +
			Field( 0, simple, ff_uint, 8, spare_desc ) +
			(UIGroup( "UUD", uud_desc ) +
				Field( "count", simple, ff_uint, 6, 0, 45, "User-to-User Data Octet Count" ) +
				Field( 0, simple, ff_uint, 2, spare_desc ) +
				Field( "data", octet, ff_hex, 8, 0, 45, "UUD:count", uud_desc )
			)
		) +
	7 +
		(Message( "SWAP", swap_desc ) +
			SIC +
			Field( 0, simple, ff_uint, 8, spare_desc )
		) +
	8 +
		(Message( "DAM", dam_desc ) +
			Field( 0, simple, ff_uint, 8, spare_desc )
		) +
	9 +
		(Message( "EEACM", eeacm_desc ) +
			(LabelGroup( cdpc_desc ) +
				CDPC +
				Field( 0, simple, ff_uint, 8, spare_desc )
			) +
			ACM_NUM_RCVD_IND
		) +
	10 +
		(Message( "CCSAPAM", "Call Completion Service (CCS 'A') Party Answer Message - Reserved" )
		) +
	11 + Msg_NEED
);

// -------------------------------------------------------------------------------------------------------------------

//
//	===============================
//	Protocol Definition (Top Level)
//	===============================
//
prot =	prot +
		SIO +
		LABEL +
		(UIGroup( msgtype_uigroup_name, msgtype_uigroup_desc ) +
			Field( msgtype_H0, simple, ff_uint, 8, msgtype_H0, Table(8) +
				0	+ fwd_addr_msgs +
				1	+ fwd_setup_msgs +
				2	+ bwd_setup_req_msgs +
				3	+ bwd_setup_info_msgs +
				4	+ call_supv_msgs +
				5	+ circ_supv_msgs +
				6	+ nn2n_serv_info_msgs +
				7	+ n2n_serv_info_msgs
			)
		) * Property( invisible ) +
		(SelectOne( msgtype_h0_varname ) +
			0	+ H0_0 +
			1	+ H0_1 +
			2	+ H0_2 +
			3	+ H0_3 +
			4	+ H0_4 +
			5	+ H0_5 +
			6	+ H0_6 +
			7	+ H0_7
		);

		build_ui_database();
}

BTUP_Protocol::~BTUP_Protocol(void)
{
	if ( ProtocolDesc != 0 )
	{
	  delete (void *) ProtocolDesc;
	  ProtocolDesc = 0;
	}

	if ( video_ops != 0 )
	{
	  delete video_ops;
	  video_ops = 0;
	}

	if ( cic_ops != 0 )
	{
	  delete cic_ops;
	  cic_ops = 0;
	}
}
