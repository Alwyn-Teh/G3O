# Header setups

# SIO setup

def_sio $outlink \c
		4 \c Service Indicator (TUP = 4)
		  \c Sub-service field:
		0 \c	(bits BA) Priority of telephone message (SPARE)
		2; #	(bits DC) Network Indicator (National = 2)

# Label setups

def_dpc $outlink 1 2 3
def_opc $outlink 4 5 6
def_cic $outlink 3210

# -----------------------------------------------------------------------------------------------

# Header setups

# SIO setup

def_sio $outlink \c
		4 \c Service Indicator (TUP = 4)
		  \c Sub-service field:
		0 \c	(bits BA) Priority of telephone message (SPARE)
		2; #	(bits DC) Network Indicator (National * 2)

# Label setups

def_dpc $outlink 4 5 6
def_opc $outlink 1 2 3
def_cic $outlink 2001

# -----------------------------------------------------------------------------------------------

# GCM setups

connect loopback
identify btup_upstairs
set outlink 7
register $outlink

source btup.header1

# -----------------------------------------------------------------------------------------------

# GCM setups

connect loopback
identify btup_downstairs
set outlink 8
register $outlink

source btup.header2

# -----------------------------------------------------------------------------------------------

# Header setups

# SIO setup

def_sio $outlink \c
		4 \c Service Indicator (TUP = 4)
		  \c Sub-service field:
		0 \c	(bits BA) Priority of telephone message (SPARE)
		2; #	(bits DC) Network Indicator (National = 2)

# Label setups

def_spcsize 24; # CTUP uses 24 bits by default

# 24-bit SPCs		Signalling Point	Signalling Area		Signalling Main Area
#					   [ 0 - 255 ]		  [ 0 - 255 ]		     [ 0 - 255 ]
# -----------		----------------	---------------		--------------------
def_opc24 $outlink			1				  2						 3
def_dpc24 Soutlink			4				  5						 6

# 14-bit SPCs		Signalling Point	Signalling Area		Signalling Main Area
#					    [ 0 - 7 ]		  [ 0 - 255 ]			 [ 0 - 7 ]
# -----------		----------------	---------------		--------------------
def_opc14 Soutlink			7				  4						 7
def_dpc14 Soutlink			5				  9						 2

# 12-bit CIC [ 0 - 4095 ]

def_cic $outlink 3896

# -----------------------------------------------------------------------------------------------

# Header setups

# SIO setup

def_sio $outlink \c
		4 \c Service Indicator (TUP = 4)
		  \c Sub-service field:
		0 \c	(bits BA) Priority of telephone message (SPARE)
		2; #	(bits DC) Network Indicator (National = 2)

# Label setups

def_spcsize 24; # CTUP uses 24 bits by default

# 24-bit SPCs		Signalling Point	Signalling Area		Signalling Main Area
#						[ 0 - 255 ]		  [ 0 - 255 ]			 [ 0 - 255 ]
# -----------		----------------	---------------		--------------------
def_opc24 $outlink			4				  5						 6
def_dpc24 $outlink			1				  2						 3

# 14-bit SPCs		Signalling Point	Signalling Area		Signalling Main Area
#						[ 0 - 7 ]		  [ 0 - 255 ]			 [ 0 - 7 ]
# -----------		----------------	---------------		--------------------
def_opc14 $outlink			5				  9						 2
def_dpc14 $outlink			7				  4						 7

# 12-bit CIC [ 0 - 4095 ]

def_cic $outlink 3896

# -----------------------------------------------------------------------------------------------

# GCM setups

connect loopback
identify ctup_upstairs
set outlink 7
register $outlink

source ctup.header1

# -----------------------------------------------------------------------------------------------

# GCM setups

connect loopback
identify ctup_downstairs
set outlink 8
register $outlink

source ctup.header2

# -----------------------------------------------------------------------------------------------

# Quick call procedure for called party

connect loopback
identify Called

if [info exist outlink] then {
  register Soutlink
} else {
  set outlink 8
  register $outlink
}

if ![info exist inlink] then {
  set inlink $outlink
}

def_sio $outlink \c
		4 \c Service Indicator (TUP = 4)
		  \c Sub-service field:
		0 \c	(bits BA) Priority of telephone message (SPARE)
		2; #	(bits DC) Network Indicator (National = 2)

def_spcsize 24; # CTUP uses 24 bits by default

# 24-bit SPCs		Signalling Point	Signalling Area		Signalling Main Area
#						[ 0 - 255 ]		  [ 0 - 255 ]			 [ 0 - 255 ]
# -----------		----------------	---------------		--------------------
def_opc24 $outlink			4				  5						 6
def_dpc24 $outlink			1				  2						 3

# 14-bit SPCs		Signalling Point	Signalling Area		Signalling Main Area
#						[ 0 - 7 ]		  [ 0 - 255 ]			 [ 0 - 7 ]
# -----------		----------------	---------------		--------------------
def_opc14 Soutlink			5				  9						 2
def_dpc14 Soutlink			7				  4						 7

# 12-bit CIC [ 0 - 4095 ]

def_cic $outlink 3896
def_acm 1 1 1 0 1

# Wait for incoming IAM message

set_receive manual
reset_msg_queue
receive_msg 10 link $inlink

if !$msg_received { disconnect; return "Timed out waiting for an IAM" }

if { [string compare $rcvmsg(msgtype.name), IAM ] } then {
  puts stdout "\n                                  (IAM received...”
  puts stdout "                                     link $rcvmsg_link"
  puts stdout "                                     time $rcvmsg_timestamp)\n"
  puts stdout "<----------------------------------- ACM (AFC) "
  send_msg $outlink ACM
  puts stdout "<----------------------------------- ANC"
  send_msg $outlink ANC
  receive_msg 30 link $iniink
  if !$msg_received { disconnect; return "Timed out waiting for a CLF" }
  if { [string compare $rcvmsg(msgtype.name), CLF ] } then {
    puts stdout "\n                                  (CLF received..."
    puts stdout "                                    link $rcvmsg_link"
    puts stdout "                                    time $rcvmsg_timestamp)\n"
    puts stdout "<---------------------------------- RLG"
    send_msg $outlink RLG
    disconnect
    return "Caller hang-up"
  }
}

# -----------------------------------------------------------------------------------------------

# Quick call procedure for called party

connect loopback
identify Called

if [info exist outlink] then {
  register $outlink
} else {
  set outlink 8
  register $outlink
}

if ![info exist inlink] then {
  set inlink [expr $outlink - 1]
}

def_sio 4 \c Service Indicator (TUP = 4)
          \c Sub-service field:
        0 \c	(bits BA) Priority of telephone message (SPARE)
        2; #	(bits DC) Network Indicator (National = 2)

def_spcsize 24; # CTUP uses 24 bits by default

# 24-bit SPCs		Signalling Point	Signalling Area		Signalling Main Area
#						[ 0 - 255 ]		  [ 0 - 255 ]			 [ 0 - 255 ]
# -----------		----------------	---------------		--------------------
def_opc24					4				  5						 6
def_dpc24					1				  2						 3

# 14-bit SPCs		Signalling Point	Signalling Area		Signalling Main Area
#						[ 0 - 7 ]		  [ 0 - 255 ]			 [ 0 - 7 ]
# -----------		----------------	---------------		--------------------
def_opc14					5				  9						 2
def_dpc14					7				  4						 7

# 12-bit CIC [ 0 - 4095 ]

def_cic 3896

def_acm 1 1 1 0 1

# -----------------------------------------------------------------------------------------------

# A quick call procedure for a calling party

connect loopback
identify Caller

if [info exist outlink] then {
  register $outlihk
} else {
  set outlink 7
  register $outlink
}

if ![info exist inlink] then {
  set inlink $outlink
}

def_sio $outlink \c
		4 \c Service Indicator (TUP = 4)
		  \c Sub-service field:
		0 \c	(bits BA) Priority of telephone message (SPARE)
		2; #	(bits DC) Network Indicator (National = 2)

def_spcsize 24; # CTUF uses 24 bits by default

# 24-bit SPCs		Signalling Point	Signalling Area		Signalling Main Area
#						[ 0 - 255 ]		  [ 0 - 255 ]			 [ 0 - 255 ]
# -----------		----------------	---------------		--------------------
def_opc24 $outlink			1				  2						 3
def_dpc24 $outlink			4				  5						 6

# 14-bit SPCs		Signalling Point	Signalling Area		Signalling Main Area
#						[ 0 - 7 ]		  [ 0 - 255 ]			 [ 0 - 7 ]
# -----------		----------------	---------------		--------------------
def_opc14 $outlink			7				  4						 7
def_dpc14 $outlink			5				  9						 2

# 12-bit CIC [ 0 - 4095 ]

def_cic $outlink 3896

# Setup parameters for IAM
def_cgpc bin000110 ; # Calling Party Category

# Message Indicators
#					 LKJIHGFEDCBA
#					 ------------
def_noai		{bin           10 } ; # Nature-of-Address indicator (national)
def_noci		{bin         01   } ; # Nature-of-Circuit indicator (satellite)
def_cci			{bin       01     } ; # Continuity-Check indicator (check required)
def_echoi		{bin      0       } ; # Outgoing Echo-Suppressor indicator (not included)
def_icintlci	{bin     0        } ; # Incoming International Call indicator (other)
def_rci			{bin    0         } ; # Redirected-Call indicator (not)
def_alldigpathi	{bin   0          } ; # All-Digital-Path-Required indicator (ordinary)
def_sigpathi	{bin  1           } ; # Signalling Path indicator (all CCS7)
# spare				 0

# Address Signals
def_ADDRESS		6		\c Number of address signals
				735078 ; # Address signals

# Call-setup procedure - connection of call to an idle subscriber

puts stdout "\nIAM---------------------------------->"
send_msg $outlink IAM

set_receive manual
reset_msg_queue
receive_msg 10 link $inlink

if !$msg_received { disconnect; return "Timed out waiting for an ACM" }

if { [string compare $rcvmsg(msgtype.name), ACM ] } then {
  puts stdout "\n(ACM received..."
  puts stdout " link $rcvmsg_link"
  puts stdout " time $rcvmsg_timestamp)\n"
  puts stdout "       - - - Ring-back tone - - -"
  receive_msg 10 link $inlink
  if !$msg_received { disconnect; return "Timed out waiting for an ANC" }
  if { [string compare $rcvmsg(msgtype.name), ANC ] } then {
    puts stdout "\n(ANC received..."
    puts stdout " link $rcvmsg_link"
    puts stdout " time $rcvmsg_timestamp)\n"
    puts stdout "       - - - Conversation (10 sec) - - -"
    sleep 10
    puts stdout "(Calling Party on-hook first)"
    puts Stdout " CLF----------------------------------->"
    send_msg $outlink CLF
    receive_msg 10 link $inlink
    if !$msg_received { disconnect; return "Timed out waiting for an RLG" }
    if { [string compare $rcvmsg(msgtype.name), RLG ] } then {
      puts stdout "\n(RLG received..."
      puts stdout " link $rcvmsg_link"
      puts stdout " time $rcvmsg_timestamp)\n"
      disconnect
      return "Call terminated"
    }
  }
}

# -----------------------------------------------------------------------------------------------

# A quick call procedure for a calling party

connect loopback
identify Caller

if [info exist outlink] then {
  register $outlink
} else {
  set outlink 7
  register $outlink
}

if ![info exist inlink] then {
  set inlink [expr $outlink + 1]
}

def_sio 4 \c Service Indicator (TUP = 4)
		  \c Sub-service field:
		0 \c	(bits BA) Priority of telephone message (SPARE)
		2; #	(bits DC) Network Indicator (National = 2)

def_spcsize 24; # CTUP uses 24 bits by default

# 24-bit SPCs		Signalling Point	Signalling Area		Signalling Main Area
#						[ 0 - 255 ]		  [ 0 - 255 ]			 [ 0 - 255 ]
# -----------		----------------	---------------		--------------------
def_opc24					1				  2						 3
def_dpc24					4				  5						 6

# 14-bit SPCs		Signalling Point	Signalling Area		Signalling Main Area
#						[ 0 - 7 ]		  [ 0 - 255 ]			 [ 0 - 7 ]
# -----------		----------------	---------------		--------------------
def_opc14					7				  4						 7
def_dpc14					5				  9						 2

# 12-bit CIC [ 0 - 4095 ]

def_cic 3896

# Setup parameters for IAM

def_cgpc binOOO11O ; # Calling Party Category

# Message Indicators
#					 LKJIHGFEDCBA
#					 ------------
def_noai		{bin           10 } ; # Nature-of-Address indicator (national)
def_noci		{bin         01   } ; # Nature-of-Circuit indicator (satellite)
def_cci			{bin       01     } ; # Continuity-Check indicator (check required)
def_echoi		{bin      0       } ; # Outgoing Echo-Suppressor indicator (not included)
def_icintlci	{bin     0        } ; # Incoming International Call indicator (other)
def_rci			{bin    0         } ; # Redirected-Call indicator (not)
de£_alldigpathi	{bin   0          } ; # All-Digital-Path-Required indicator (ordinary)
def_sigpathi	{bin  1           } ; # Signalling Path indicator (all CCS7)
# spare              0

# Address Signals
def_ADDRESS 6		\c Number of address signals
			735078 ; # Address signals
