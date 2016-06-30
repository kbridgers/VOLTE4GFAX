/****************************************************************************
                              Copyright (c) 2010
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 *****************************************************************************
*
*   \file switch_utility.c
*   \brief
*   This file demonstrates the useability of the IFX Ethernet Switch APIs to
*   configure an IFX Ethernet Switch device
*   \author TW-Team
*   \date 2009-04-09
*
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>   /* open */
#include <unistd.h> /* close */
#include <errno.h>
#include <ctype.h>
#include <ifx_types.h>
#include <ifx_ethsw.h>

#include "ifx_ethsw_api.h"

#define MAX_COMMAND_ARGS 64
#define MAX_CMD_STR_LEN  200

char print_mac_table_str[]={"MAC_TablePrint"};
char print_device_list_str[]={"print_device_list"};

#define NR_FE_FEATURES 9
char* fe_feature_str[NR_FE_FEATURES] = { "FE_PORT_AUTONEGOTIATION", \
                                         "FE_PORT_LINK_OK", \
                                         "FE_PORT_DUPLEX_MODE", \
                                         "FE_PORT_SPEED", \
                                         "FE_PORT_PAUSE_FLOW_CTRL", \
                                         "FE_PORT_PRIORITY", \
                                         "FE_PORT_MONITOR_INGRESS", \
                                         "FE_PORT_MONITOR_EGRESS", \
                                         "FE_PORT_AUTHENTICATION" };


#define MAX_FE_MODES 4
char* fe_mode_str[NR_FE_FEATURES][MAX_FE_MODES] =
{ {"disabled",     "enabled",     "INVALID", "INVALID"},
  {"link down",    "link up",     "INVALID", "INVALID"},
  {"half duplex",  "full duplex", "INVALID", "INVALID"},
  {"10 MBit/s",    "100 MBit/s",  "INVALID", "INVALID"},
  {"disabled",     "enabled",     "INVALID", "INVALID"},
  {"0",            "1",           "2",       "3"},
  {"disabled",     "enabled",     "INVALID", "INVALID"},
  {"disabled",     "enabled",     "INVALID", "INVALID"},
  {"not authorized to receive or transmit",
   "authorized to transmit but not to receive",
   "authorized to receive but not to transmit",
   "authorized to transmit and receive"} };



#define NR_GE_FEATURES 10
char* ge_feature_str[NR_GE_FEATURES] = { "GE_PORT_AUTONEGOTIATION", \
                                         "GE_PORT_LINK_OK", \
                                         "GE_PORT_DUPLEX_MODE", \
                                         "GE_PORT_SPEED", \
                                         "GE_RX_PORT_PAUSE_FLOW_CTRL", \
                                         "GE_TX_PORT_PAUSE_FLOW_CTRL", \
                                         "GE_PORT_PRIORITY", \
                                         "GE_PORT_MONITOR_INGRESS", \
                                         "GE_PORT_MONITOR_EGRESS", \
                                         "GE_PORT_AUTHENTICATION" };

#define MAX_GE_MODES 4
char* ge_mode_str[NR_GE_FEATURES][MAX_GE_MODES] =
{ {"disabled\n",     "enabled\n",     "INVALID",  "INVALID"},
  {"link down\n",    "link up\n",     "INVALID",  "INVALID"},
  {"half duplex\n",  "full duplex\n", "INVALID",  "INVALID"},
  {"10 MBit/s\n",    "100 MBit/s\n",  "1 GBit/s", "INVALID"},
  {"disabled\n",     "enabled\n",     "INVALID", "INVALID"},
  {"disabled\n",     "enabled\n",     "INVALID", "INVALID"},
  {"0\n",            "1\n",           "2\n",       "3\n"},
  {"disabled\n",     "enabled\n",     "INVALID", "INVALID"},
  {"disabled\n",     "enabled\n",     "INVALID", "INVALID"},
  {"not authorized to receive or transmit\n", \
   "authorized to transmit but not to receive\n", \
   "authorized to receive but not to transmit\n", \
   "authorized to transmit and receive\n"} };

#ifdef VR9
IFX_ETHSW_QoS_DSCP_ClassCfg_t DSCP_TABLE;
IFX_ETHSW_QoS_PCP_ClassCfg_t PCP_TABLE;
#endif
/**
 * \brief Switch command structure that contains
 * - name:  command string that is expected to be entered from command
 *          line as argv[1]
 * - param: parameter description that describes the amount and type of
 *          parameters attached to the command
 * - ioctl_command: IOCTL command for device driver access
 *
 */
typedef struct switch_command
{
   const char         *name;
   const char         *param;          /* i=int, l=long, s=string */
   const unsigned int ioctl_command;
   const char         *param_string;
   const char         *detailed_info;
} switch_command_t;

/**
 * \brief Switch commands available for configuration
 */
struct switch_command commands[] =
{
#if 0
    { "print_device_list", "",  0xFFFFFFFF, "",
      "Prints the list of registered devices\n" },
#endif
  { "CfgGet", "", IFX_ETHSW_CFG_GET,"",
    "\tRETURN:      Configure DATA\n" },
    // take off eMulticastTableAgeTimer
  { "CfgSet", "lllls", IFX_ETHSW_CFG_SET,
    //"<Enable> <Mac Table Aging Timer> <VLAN Aware> <Packet Length> <Pause Mac Mode> <Pause Mac Address>",
    "<Mac Table Aging Timer> <VLAN Aware> <Packet Length> <Pause Mac Mode> <Pause Mac Address>",
    //"\tEnable:                          0: Disable 1:Enable\n"
    //"\t                                 Only support on WAIT_INIT=1\n"
    "\tMac Table Aging timer:           1: 1 second\n"
    "\t                                 2: 10 seconds\n"
    "\t                                 3: 300 seconds\n"
    "\t                                 4: 1 hour\n"
    "\t                                 5: 1 day\n"
    "\tVLAN Aware:                      0: Disable\n"
    "\t                                 1: Enable\n"
    "\tPacket Length:                   Input Number\n"
    "\tPause Mac Mode:                  0: Disable\n"
    "\t                                 1: Enable\n"
    "\tPause Mac Address:               xx:xx:xx:xx:xx:xx \n"},
  { "PortCfgGet", "l", IFX_ETHSW_PORT_CFG_GET,
    "<Port Id>",
    "\tPortId:          0..6\n" },
  { "PortCfgSet", "lllllllllll", IFX_ETHSW_PORT_CFG_SET,
    "<PortId> <Enable> <Unicast unknown drop> <Multicast unknown drop>\
     <Reserved packet drop> <Broadcast packet drop> <Aging> <Learning Mac Port Lock>  \
     <Learning Limit> <Port Moniter> <Flow Control>",
    "\tportid:                    0..6\n"
    "\tenable:                    0 - Disable this port\n"
    "\t                           1 - Enable this port\n"
    "\tUnicast Unknown drop:      Enable :1  Disable :0\n"
    "\tMulticast Unknown drop:    Enable :1  Disable :0\n"
    "\tReserved Packet drop:      Enable :1  Disable :0\n"
    "\tBroadcast Packet drop:     Enable :1  Disable :0\n"
    "\tAging:                     Enable :1  Disable :0\n"
    "\tLearning Mac PortLock:     Enable :1  Disable :0\n"
#if  defined(AR9) || defined(DANUBE) || defined(AMAZON_SE)
    "\tLearning Limit:            0..31, 0:Doesn't limit the number of addresses to be learned\n"
#else
    "\tLearning Limit:            0: Disable\n"
    "\t                           255:Default\n"
    "\t                           or specify the value\n"
#endif
    "\tPort Monitor :             0: Normal port usage\n"
    "\t                           1: Port Ingress packets are mirrored to the monitor port.\n"
    "\t                           2: Port Egress packets are mirrored to the monitor port.\n"
    "\t                           3: Port Ingress and Egress packets are mirrored to the monitor port.\n"
    "\tFlow Control :             0: Auto 1: Flow RX 2: Flow TX 3:Flow RXTX 4: FLOW OFF\n"
    },
  { "PortLinkCfgGet", "l", IFX_ETHSW_PORT_LINK_CFG_GET,
    "<Port Id>",
    "\tPort Id:                   0..6\n"
    "\tReturn the status.\n"},
  { "PortLinkCfgSet", "llllllllll", IFX_ETHSW_PORT_LINK_CFG_SET,
    "<Port Id> <Force Duplex> <Duplex> <Force Speed> <Speed> <Force Link> <Link> <MII Mode> <MII Type> <Clock Mode>",
    "\tPort Id:                   0..6 \n"
    "\tForce Duplex:              0: NA 1: Force\n"
    "\tDuplex :                   0: Force Full duplex  1:Force Half duplex\n"
    "\tForce Speed:               0: NA 1: Force\n"
    "\tSpeed:                     10: 10Mb 100:100Mb 200:200Mb 1000:1GMb \n"
    "\tForce Link:                0: NA 1: Force\n"
    "\tLink:                      0: UP 1: DOWN\n"
    "\tMII Mode:                  0: MII 1: RMII 2:GMII 3:RGMII\n"
    "\teMII Type:                 0: MAC 1: PHY\n"
    "\tClock Mode:                0: NA 1:Master 2:Slave\n"},
  { "PortRedirectGet", "l", IFX_ETHSW_PORT_REDIRECT_GET,
    "<Port Id>",
    "\tPort Id:                   \n"},
  { "PortRedirectSet", "lll", IFX_ETHSW_PORT_REDIRECT_SET,
    "<Port Id> <Redirect Egress> <Redirect Ingress>",
    "\tPort Id:                   0..2 (internal switch)\n"
    "\tRedirect Egress:             Enable :1  Disable :0\n"
    "\tRedirect Ingress:            Enable :1  Disable :0\n"},
  { "MAC_TableEntryRead", "", IFX_ETHSW_MAC_TABLE_ENTRY_READ,
    "",
    "\t\n"},
  { "MAC_TableEntryAdd", "lllls", IFX_ETHSW_MAC_TABLE_ENTRY_ADD,
    "<DataBase ID> <Port Id> <Age Timer> <Static Entry> <Mac Address>",
    "\tDataBase ID:                FID number\n"
    "\tPort Id:                    0..6\n"
    "\tAge Timer:                  Value (From 1s to 1,000,000s)\n"
    "\tStatic Entry:               Enable :1  Disable :0\n"
/*    "\ttraffic class:              (0..3)\n" */
    "\tMac Address:                xx:xx:xx:xx:xx:xx\n"},
  { "MAC_TableEntryRemove", "ls", IFX_ETHSW_MAC_TABLE_ENTRY_REMOVE,
    "<DataBase ID> <Mac Address>",
    "\tDataBase ID:                FID number\n"
    "\tMac Address:                xx:xx:xx:xx:xx:xx\n"},
  { "MAC_TableClear", "", IFX_ETHSW_MAC_TABLE_CLEAR,
    "",
    "\tClear all MAC Table list\n" },
  { "VLAN_IdCreate", "ll", IFX_ETHSW_VLAN_ID_CREATE,
    "<VLAN ID> <FID>",
    "\tVLAN ID:             0..4095\n"
    "\tFID:                 FID number\n"},
  { "VLAN_IdDelete", "l", IFX_ETHSW_VLAN_ID_DELETE,
    "<VLAN ID>",
    "\tVLAN ID:             0..4095\n"},
  { "VLAN_IdGet", "l", IFX_ETHSW_VLAN_ID_GET,
    "<VLAN ID>",
    "\tVLAN ID:             0..4095\n"
    "\tReturn FID:                 FID number\n"},
  { "VLAN_PortMemberAdd", "lll", IFX_ETHSW_VLAN_PORT_MEMBER_ADD,
    "<VLAN ID> <Port ID> <Tag base Number Egress>",
      "\tVLAN ID:                    0..4095\n"
    "\tPort ID:                    0..6\n"
    "\tbVlanTagEgress:             1 - Enable 0 - Disable\n"},
    { "VLAN_PortMapTableRead", "", IFX_ETHSW_VLAN_PORT_MEMBER_READ,
    "",
    "\t\n"},
  { "VLAN_PortMemberRemove", "ll", IFX_ETHSW_VLAN_PORT_MEMBER_REMOVE,
    "<VLAN ID> <Port ID>",
    "\tVLAN ID:                    ID number\n"
    "\tPort ID:                    0..6\n"},
  { "VLAN_PortCfgGet", "l", IFX_ETHSW_VLAN_PORT_CFG_GET,
    "<Port ID>",
    "\tPort ID:                    0..6\n"},
  { "VLAN_PortCfgSet", "lllllll", IFX_ETHSW_VLAN_PORT_CFG_SET,
    "<Port ID> <Port VID> <VLAN Unknow Drop> <VLAN ReAssign> <Violation Mode> <Admit Mode> <TVM>",
    "\tPort ID:                    0..6\n"
    "\tPort VID:                   VID number\n"
    "\tVLAN Unknow Drop            Enable:1  Disable:0\n"
    "\tVLAN ReAssign:              Enable:1  Disable:0\n"
    "\tViolation mode:             NA:0 INGRESS: 1 EGRESS:2 BOTH(Ingress&Egress):3\n"
    "\tAdmit Mode:                 Admit All:0\n"
    "\t                            Admit UnTagged:1\n"
    "\t                            Admit Tagged:2\n"
    "\tTVM:                        Enable:1  Disable:0\n"},
  { "VLAN_ReservedAdd", "l", IFX_ETHSW_VLAN_RESERVED_ADD,
    "<nVId>",
#if defined(AR9) || defined(DANUBE) || defined(AMAZON_SE)
    "\tnVId:                      0: VID=0\n"
    "\t                           1: VID=1\n"
    "\t                           0xFFF: VID=FFF\n"},
#else
    "\tVLAN ID:             0..4095\n"},
#endif
  { "VLAN_ReservedRemove", "l", IFX_ETHSW_VLAN_RESERVED_REMOVE,
#if defined(AR9) || defined(DANUBE) || defined(AMAZON_SE)
    "<nVId>",
    "\tnVId:                      0: VID=0\n"
    "\t                           1: VID=1\n"
    "\t                           0xFFF: VID=FFF\n"},
#else
    "\tVLAN ID:             0..4095\n"},
#endif
  { "MulticastRouterPortAdd", "l", IFX_ETHSW_MULTICAST_ROUTER_PORT_ADD,
    "<Port ID>",
    "\tPort ID:                    Port number\n"},
  { "MulticastRouterPortRemove", "l", IFX_ETHSW_MULTICAST_ROUTER_PORT_REMOVE,
    "<Port ID>",
    "\tPort ID:                    Port number\n"},
  { "MulticastRouterPortRead", "", IFX_ETHSW_MULTICAST_ROUTER_PORT_READ,
    "",
    "\tReturn the Router Port Menber (HEX value).\n"},
  { "MulticastTableEntryAdd", "llssl", IFX_ETHSW_MULTICAST_TABLE_ENTRY_ADD,
    "<Port ID> <IP Version> <GDA > <GSA> <Mode Member>",
    "\tPort ID:                    0..6\n"
    "\tIP Version:                 0: IPv4 1:IPv6\n"
    "\tGDA:                        Group Destination IP address (IP4: xxx.xxx.xxx.xxx) 	\
    								(IP6 -> fof6:0013:abcd:xxxx:xxxx:xxxx:xxxx:xxxx)\n"
    "\tGSA:                        Group Source IP address (IPv4: xxx.xxx.xxx.xxx)	\
    								(IP6 -> fof6:0013:abcd:xxxx:xxxx:xxxx:xxxx:xxxx)\n"
    "\tMode Member:                0:Include 1:Exclude 2:DonT Care\n"},
  { "MulticastTableEntryRemove", "llssl", IFX_ETHSW_MULTICAST_TABLE_ENTRY_REMOVE,
    "<Port ID> <IP Version> <GDA > <GSA> <Mode Member>",
    "\tPort ID:                    0..6\n"
    "\tIP Version:                 0: IPv4 1:IPv6\n"
     "\tGDA:                        Group Destination IP address (IP4: xxx.xxx.xxx.xxx) 	\
    								(IP6 -> fof6:0013:abcd:xxxx:xxxx:xxxx:xxxx:xxxx)\n"
    "\tGSA:                        Group Source IP address (IPv4: xxx.xxx.xxx.xxx)	\
    								(IP6 -> fof6:0013:abcd:xxxx:xxxx:xxxx:xxxx:xxxx)\n"
    "\tMode Member:                0:Include 1:Exclude 2:DonT Care\n"},
  { "MulticastTableEntryRead", "", IFX_ETHSW_MULTICAST_TABLE_ENTRY_READ,
    "",
    "\t\n"},
  { "MulticastSnoopCfgGet", "", IFX_ETHSW_MULTICAST_SNOOP_CFG_GET,
    "",
    "\tReturn the info data        .\n"},
  { "MulticastSnoopCfgSet", "lllllllllll", IFX_ETHSW_MULTICAST_SNOOP_CFG_SET,
    "<IGMP Snooping> <IGMPv3> <cross-VLAN packets> <Port Forward> <Forward Port ID> <ClassOfService> <Robust>\
     <Query interval> <Report Suppression> <FastLeave> <LearningRouter>",
    "\tIGMP Snooping:              0:Disable 1:Auto Learning 2:Snoop Forward\n"
    "\tIGMPv3 Support:             0:Disable 1:Enable\n"
    "\tcross-VLAN packets:         0:Disable 1:Enable\n"
    "\tPort Forward:               0:Default 1:Discard 2:CPU port 3:Forward Port\n"
    "\tForwardPortId               Port ID \n"
    "\tClassOfService              0 - 254 s :Default(10s) \n"
    "\tRobust:                     0..3\n"
    "\tQuery interval:             Hex value from 100ms to 25.5s\n"
    "\tJoin&Report:                0:Both 1:Report Suppression only 2:Transparent Mode\n"
    "\tFastLeave:                  0:Disable 1:Enable\n"
    "\tLearningRouter              0:Disable 1:Enable\n"},
  { "RMON_Clear", "l",  IFX_ETHSW_RMON_CLEAR,
    "<Port Id>",
    "\tPort Id:                   0 ..6\n"},
  { "RMON_Get", "l",  IFX_ETHSW_RMON_GET,
    "<Port Id>",
    "\tPort Id:                   0 ..6\n"},
  { "MDIO_DataRead", "ll", IFX_ETHSW_MDIO_DATA_READ,
    "<PHY addr> <Register inside PHY>",
    "\tphy addr:           0..14\n"
    "\tregister:           0..24\n"},
  { "MDIO_DataWrite", "lll", IFX_ETHSW_MDIO_DATA_WRITE,
    "<PHY addr> <Register inside PHY> <DATA>",
    "\tPHY Addr:           0..4\n"
    "\tRegister:           0..24\n"
    "\tDATA:               value\n"},
#if  defined(AR9) || defined(DANUBE) || defined(AMAZON_SE)
  { "MDIO_CfgGet", "", IFX_ETHSW_MDIO_CFG_GET,
    "",
    "<RETURN MDIO SPEED>\n"},
  { "MDIO_CfgSet", "ll",  IFX_ETHSW_MDIO_CFG_SET,
    "<bMDIO_Enable> <nMDIO_Speed>",
    "\tbMDIO_Enable:          0: N/A \n"
    "\nMDIO_Speed:           0:6.25 MHZ\n"
    "\t                       1:2.5 MHZ\n"
    "\t                       2:1.0 MHZ\n"},
#endif
#ifdef VR9
  { "MDIO_CfgGet", "", IFX_ETHSW_MDIO_CFG_GET,
    "<RETURN MDIO ENABLE PORT>\n"},
  { "MDIO_CfgSet", "ll",  IFX_ETHSW_MDIO_CFG_SET,
    "<bMDIO_Enable> <bEnable_port_rate>",
    "\tbMDIO_Enable:          0: DISABLe 1:ENABLE \n"
    "\t                       0:25 MHZ\n"
    "\t                       1:12.5 MHZ\n"
    "\t                       2:6.25 MHZ\n"
    "\t                       3:4.167 MHZ\n"
    "\t                       4:3.125 MHZ\n"
    "\t                       5:2.5 MHZ\n"
    "\t                       6:2.083 MHZ\n"
    "\t                       7:1.786 MHZ\n"
    "\t                       8:1.563 MHZ\n"
    "\t                       9:1.389 MHZ\n"
    "\t                       10:1.25 MHZ\n"
    "\t                       11:1.136 MHZ\n"
    "\t                       12:1.042 MHZ\n"
    "\t                       13:0.962 MHZ\n"
    "\t                       14:0.893 MHZ\n"
    "\t                       15:0.833 MHZ\n"
    "\t                       255:97.6 MHZ\n"},
  { "PortRGMII_ClkCfgGet", "l",  IFX_ETHSW_PORT_RGMII_CLK_CFG_GET,
    "<Port Id>",
    "\tPort Id:                   0 ..6\n"},
  { "PortRGMII_ClkCfgSet", "lll",  IFX_ETHSW_PORT_RGMII_CLK_CFG_SET,
    "<Port Id> <Delay RX> <Delay TX>",
    "\tPort Id:                   0 ..6\n"
    "\tDelay RX:                  multiple of 500ps\n"
    "\tDelay TX:                  multiple of 500ps\n"},
  { "RegisterGet", "l",  IFX_FLOW_REGISTER_GET,
    "<nRegAddr>",
    "\tnRegAddr:          \n" },
  { "RegisterSet", "ll",  IFX_FLOW_REGISTER_SET,
    "<nRegAddr> <nData>",
    "\tnRegAddr:          \n"
    "\tnData:             \n" },
  { "CPU_PortExtendCfgGet", "",  IFX_ETHSW_CPU_PORT_EXTEND_CFG_GET,
    "Return the port extend config info\n"},
  { "CPU_PortExtendCfgSet", "llsslllllll",  IFX_ETHSW_CPU_PORT_EXTEND_CFG_SET,
    "<eHeaderAdd> <bHeaderRemove> <sHeader nMAC_Src> <sHeader nMAC_Dst> <sHeader nEthertype> <sHeader nVLAN_Prio> <sHeader nVLAN_CFI> <sHeader nVLAN_ID> <ePauseCtrl> <bFcsRemove> <nWAN_Ports>",
    "\teHeaderAdd:          0: No Header 1: Ethernet Header 2:Ethernet and VLAN Header\n"
    "\tbHeaderRemove:       0: NA 1: Remove\n"
    "\tsHeader:             Header Data (nMAC_Src, nMAC_Dst, nEthertype, nVLAN_Prio, nVLAN_CFI, nVLAN_ID)\n"
    "\tePauseCtrl:          0: Forward 1: Dispach\n"
    "\tbFcsRemove:          0: NA 1: Remove FCS\n"
    "\tnWAN_Ports:          WAN Port Number\n"},
#endif
  { "CPU_PortCfgGet", "l",  IFX_ETHSW_CPU_PORT_CFG_GET,
    "<Port Id>",
    "\tPort Id:                   0 .. 6\n"},
  { "CPU_PortCfgSet", "llllll", IFX_ETHSW_CPU_PORT_CFG_SET,
    "<Port Id> <bFcsCheck> <bFcsGenerate> <bSpecialTagEgress> <bSpecialTagIngress> <bCPU_PortValid>",
    "\tnPortId:             0 .. 6\n"
    "\tbFcsCheck:           0: No Check 1: Check FCS\n"
    "\tbFcsGenerate:        0: Without FCS 1: With FCS\n"
    "\tbSpecialTagEgress:   0: Disable 1: Enable\n"
    "\tbSpecialTagIngress:  0: Disable 1: Enable\n"
    "\tbCPU_PortValid:      0: Not Define CPU Port 1: Define CPU Port\n"},
  { "MonitorPortGet", "l", IFX_ETHSW_MONITOR_PORT_CFG_GET,
    "<Port Id>",
    "\tPort Id:                   0 ..6\n"},
  { "MonitorPortSet", "ll", IFX_ETHSW_MONITOR_PORT_CFG_SET,
    "<Port Id> <bMonitorPort>",
    "\tPort Id:                   0 ..6\n"
    "\tbMonitorPort:              0: False, 1: True\n" },
  { "PHY_AddrGet", "l", IFX_ETHSW_PORT_PHY_ADDR_GET,
    "<Port Id>",
#if  defined(AR9)
    "\tPort Id:                   0..6\n"},
#elif defined(DANUBE)
    "\tPort Id:                   0..4\n"},
#elif defined(AMAZON_SE)
    "\tPort Id:                   0..3\n"},
#else
    "\tPort Id:                   0..6\n"},
#endif
  { "QOS_PortCfgGet", "l",  IFX_ETHSW_QOS_PORT_CFG_GET,
    "<nPortId>",
    "\tnPortId:          0..6\n"
    "Return the port QOS config info\n"},
  { "QOS_PortCfgSet", "lll",  IFX_ETHSW_QOS_PORT_CFG_SET,
    "<nPortId> <eClassMode> <nTrafficClass>",
    "\tnPortId:          0..6\n"
    "\teClassMode:       \n"
    "\t                  0: No traffic class assignment based on DSCP or PCP\n"
    "\t                  1: Traffic class assignment based on DSCP\n"
    "\t                  2: Traffic class assignment based on PCP\n"
#if  defined(AR9) || defined(DANUBE) || defined(AMAZON_SE)
    "\t                  3: Traffic class assignment based on DSCP and PCP,with DSCP has higher precedence then PCP\n"
    "\t                  4: Traffic class assignment based on PCP and DSCP,with PCP has higher precedence then DSCP\n"
#else
    "\t                  3: Traffic class assignment based on DSCP and PCP\n"
    "\t                  4: PCP and DSCP Untag\n"
#endif
#if  defined(AR9) || defined(DANUBE) || defined(AMAZON_SE)
    "\tnTrafficClass:    Port Priority: 0 ~ 3\n"},
#else
    "\tnTrafficClass:    Port Priority: 0 ~ F\n"},
#endif
  { "QOS_DscpClassGet", "",  IFX_ETHSW_QOS_DSCP_CLASS_GET,
    "<RETURN QOS DSCP Traffic Class, Total:64 entry>\n"},
  { "QOS_DscpClassSet", "ll",  IFX_ETHSW_QOS_DSCP_CLASS_SET,
    "<Index> <nTrafficClass data> ",
    "\tIndex:                 0..63\n"
#ifdef VR9
    "\tnTrafficClass:         Hex Data: 0..15\n"},
#else
    "\tValue:                     0..3\n" },
#endif
  { "QOS_PcpClassGet", "",  IFX_ETHSW_QOS_PCP_CLASS_GET,
   "<RETURN QOS PCP Traffic Class, Total:8 entry>\n"},
  { "QOS_PcpClassSet", "ll",  IFX_ETHSW_QOS_PCP_CLASS_SET,
    "<Index> <nTrafficClass data> ",
    "\tIndex:                 0..7\n"
#ifdef VR9
    "\tnTrafficClass:         Hex Data : 0 .. 15\n"},
#else
    "\tValue:                     0..3\n" },
#endif
#if defined(AR9) || defined(DANUBE) || defined(AMAZON_SE)
#if 0
  { "QOS_PortShaperGet", "lll",  IFX_PSB6970_QOS_PORT_SHAPER_GET,
    "<nPort> <nTrafficClass> <eType>",
    "\tnPort:                     0..6\n"
    "\tnTrafficClass:             0..3\n"
    "\teType:                     0: Strict Priority\n"
    "\t                           1: Weighted Fair Queuing\n"},
  { "QOS_PortShaperSet", "llll",  IFX_PSB6970_QOS_PORT_SHAPER_SET,
    "<nPort> <nTrafficClass> <eType> <nRate>",
    "\tnPort:                     0..6\n"
    "\tnTrafficClass:             0..3\n"
    "\teType:                     0: Strict Priority\n"
    "\t                           1: Weighted Fair Queuing\n"
    "\tValue:                     N  MBit/s\n" },
#else
  { "QOS_PortShaperCfgGet", "l",  IFX_PSB6970_QOS_PORT_SHAPER_CFG_GET,
    "<nPort> ",
    "\tnPort:                     0..6\n"},
  { "QOS_PortShaperCfgSet", "ll",  IFX_PSB6970_QOS_PORT_SHAPER_CFG_SET,
    "<nPort> <eWFQ_Type>",
    "\tnPort:                     0..6\n"
    "\teWFQ_Type:                 0: Weight\n"
    "\t                           1: Rate\n"},
  { "QOS_PortShaperStrictGet", "ll",  IFX_PSB6970_QOS_PORT_SHAPER_STRICT_GET,
    "<nPort> <nTrafficClass>",
    "\tnPort:                     0..6\n"
    "\tnTrafficClass:             0..3\n"},
  { "QOS_PortShaperStrictSet", "lll",  IFX_PSB6970_QOS_PORT_SHAPER_STRICT_SET,
    "<nPort> <nTrafficClass> <nRate>",
    "\tnPort:                     0..6\n"
    "\tnTrafficClass:             0..3\n"
    "\tnRate:                     N  MBit/s\n" },
  { "QOS_PortShaperWFQGet", "ll",  IFX_PSB6970_QOS_PORT_SHAPER_WFQ_GET,
    "<nPort> <nTrafficClass>",
    "\tnPort:                     0..6\n"
    "\tnTrafficClass:             0..3\n"},
  { "QOS_PortShaperWFQSet", "lll",  IFX_PSB6970_QOS_PORT_SHAPER_WFQ_SET,
    "<nPort> <nTrafficClass> <nRate>",
    "\tnPort:                     0..6\n"
    "\tnTrafficClass:             0..3\n"
    "\tnRate:                     N in ratio or in MBit/s\n" },
#endif
  { "QOS_PortPolicerGet", "l",  IFX_PSB6970_QOS_PORT_POLICER_GET,
    "<nPort>",
    "\tnPort:                     0..6\n"},
  { "QOS_PortPolicerSet", "ll",  IFX_PSB6970_QOS_PORT_POLICER_SET,
    "<nPort> <nRate>",
    "\tnPort:                     0..6\n"
    "\tValue:                     N  MBit/s\n" },
  { "QOS_MFC_PortCfgGet", "l", IFX_PSB6970_QOS_MFC_PORT_CFG_GET,
    "<nPort>",
    "\tnPortId:                 0..6\n"},
  { "QOS_MFC_PortCfgSet", "lll", IFX_PSB6970_QOS_MFC_PORT_CFG_SET,
    "<nPort> <bPriorityPort> <bPriorityEtherType>",
    "\tnPortId:                 0..6\n"
    "\tbPriorityPort:           0: Disable\n"
    "\t                         1: Enable\n"
    "\tbPriorityEtherType:      0: Disable\n"
    "\t                         1: Enable\n"},
  { "QOS_MFC_Add", "lllllllll", IFX_PSB6970_QOS_MFC_ADD,
    "<eFieldSelection> <nPortSrc> <nPortDst> <nPortSrcRange> \n"
    "                                   <nPortDstRange> <nProtocol> <nEtherType> <nTrafficClass>\n"
    "                                   <ePortForward>",
    "\teFieldSelection:         1:UDP/TCP Source Port Filter\n"
    "\t                         2:UDP/TCP Destination Port Filter\n"
    "\t                         4:IP Protocol Filter\n"
    "\t                         8:Ethertype Filter\n"
    "\tnPortSrc                 Source port base\n"
    "\tnPortDst                 Destination port base\n"
    "\tnPortSrcRange            Check from nPortSrc till smaller nPortSrc + nPortSrcRange\n"
    "\tnPortDstRange            Check from nPortDst till smaller nPortDst + nPortDstRange.\n"
    "\tnProtocol                Protocol type\n"
    "\tnEtherType               Ether type\n"
    "\tnTrafficClass            Egress priority queue\n"
    "\tePortForward:            0:Default\n"
    "\t                         1:Discard\n"
    "\t                         2:Forward to the CPU port\n"},
    //"\t                         3:Forward to a port\n"},
  { "QOS_MFC_Del", "lllllll", IFX_PSB6970_QOS_MFC_DEL,
    "<eFieldSelection> <nPortSrc> <nPortDst> <nPortSrcRange>\n"
    "                                   <nPortDstRange> <nProtocol> <nEtherType>",
    "\teFieldSelection:         1:UDP/TCP Source Port Filter\n"
    "\t                         2:UDP/TCP Destination Port Filter\n"
    "\t                         4:IP Protocol Filter\n"
    "\t                         8:Ethertype Filter\n"
    "\tnPortSrc                 Source port base\n"
    "\tnPortDst                 Destination port base\n"
    "\tnPortSrcRange            Check from nPortSrc till smaller nPortSrc + nPortSrcRange\n"
    "\tnPortDstRange            Check from nPortDst till smaller nPortDst + nPortDstRange.\n"
    "\tnProtocol                Protocol type\n"
    "\tnEtherType               Ether type\n"},
  { "QOS_MFC_EntryRead", "", IFX_PSB6970_QOS_MFC_ENTRY_READ,
    "",
    "\t\n"},
  { "QOS_StormGet", "", IFX_PSB6970_QOS_STORM_GET ,
    "",
    "\t\n"},
  { "QOS_StormSet", "lllll", IFX_PSB6970_QOS_STORM_SET,
    "<bBroadcast> <bMulticast> <bUnicast> <nThreshold10M> <nThreshold100M>",
    "\tbStormBroadcast:         0: Disable\n"
    "\t                         1: Enable\n"
    "\tbStormMulticast:         0: Disable\n"
    "\t                         1: Enable\n"
    "\tbStormUnicast:           0: Disable\n"
    "\t                         1: Enable\n"
    "\tnThreshold10M:           10M Threshold (the number of the packets received during 50 ms)\n"
    "\tnThreshold100M:          100M Threshold (the number of the packets received during 50 ms)\n"},
  { "STP_PortCfgGet", "l", IFX_ETHSW_STP_PORT_CFG_GET ,
    "<nPortId>",
    "\tnPortId:                   0..6\n"},
  { "STP_PortCfgSet", "ll", IFX_ETHSW_STP_PORT_CFG_SET,
    "<nPortId> <ePortState>",
    "\tnPortId:                   0..6\n"
    "\tePortState:                0: Forwarding state\n"
    "\t                           1: Disabled/Discarding state\n"
    "\t                           2: Learning state\n"
    "\t                           3: Blocking/Listening state\n" },
  { "STP_BPDU_RULE_Get", "", IFX_ETHSW_STP_BPDU_RULE_GET ,
    "",
    "\t\n" },
  { "STP_BPDU_RULE_Set", "l", IFX_ETHSW_STP_BPDU_RULE_SET,
    "<eForwardPort>",
    "\teForwardPort:              0: Default portmap\n"
    "\t                           1: Discard\n"
    "\t                           2: Forward to cpu port\n"},
  { "8021X_PortCfgGet", "l", IFX_ETHSW_8021X_PORT_CFG_GET ,
    "<nPortId>",
    "\tnPortId:                 0..6\n"},
  { "8021X_PortCfgSet", "ll", IFX_ETHSW_8021X_PORT_CFG_SET,
    "<nPortId> <eState>",
    "\tnPortId:                 0..6\n"
    "\teState:                  0: Receive and transmit direction are authorized.\n"
    "\t                         1: Receive and transmit direction are unauthorized.\n"},
  { "RegisterGet", "l",  IFX_PSB6970_REGISTER_GET,
    "<nRegAddr>",
    "\tnRegAddr:          \n" },
  { "RegisterSet", "ll",  IFX_PSB6970_REGISTER_SET,
    "<nRegAddr> <nData>",
    "\tnRegAddr:          \n"
    "\tnData:             \n" },
  { "VersionGet", "l",  IFX_ETHSW_VERSION_GET,
    "<nId>",
    "\tnId:                      0:Switch API Version\n"
    "\t                            \n" },
  { "CapGet", "", IFX_ETHSW_CAP_GET,
    "<nCapType>",
    "\tnCapType                  0: physical Ethernet ports.\n"
    "\t                          1: virtual Ethernet ports\n"
    "\t                          2: internal packet memory [in Bytes]\n"
    "\t                          3: priority queues per device\n"
    "\t                          4: meter instances\n"
    "\t                          5: rate shaper instances\n"
    "\t                          6: VLAN groups\n"
    "\t                          7: Forwarding database IDs\n"
    "\t                          8: MAC table entries\n"
    "\t                          9: multicast level 3 hardware table entries\n"
    "\t                          10: PPPoE sessions\n" },
  { "PHY_Query", "l", IFX_ETHSW_PORT_PHY_QUERY,
    "<Port Id>",
    "\tPort Id:                   0 ..6\n"},
  { "RGMII_CfgGet", "l", IFX_ETHSW_PORT_RGMII_CLK_CFG_GET,
    "<Port Id>",
    "\tPort Id:                   0 ..6\n"},
  { "RGMII_CfgSet", "lll", IFX_ETHSW_PORT_RGMII_CLK_CFG_SET,
    "<Port Id> <nDelayRx> <nDelayTx>",
    "\tPort Id:                   0 ..1\n"
    "\tnDelayRx:                  0: 0ns, 6: 1.5ns, 7: 1.75ns 8: 2ns \n"
    "\tnDelayTx:                  0: 0ns, 6: 1.5ns, 7: 1.75ns 8: 2ns \n" },
  { "Reset", "l",  IFX_PSB6970_RESET,
    "<eReset>",
    "\teReset:                  0: Reset Phy\n"},
  { "HW_Init", "l",  IFX_ETHSW_HW_INIT,
    "<eInitMode>",
    "\teInitMode:               0: Basic hardware configuration\n"},
#if 1
  { "PowerManagementGet", "", IFX_PSB6970_POWER_MANAGEMENT_GET,
    "\tRETURN:              Enable/Disable\n"},
  { "PowerManagementSet", "l", IFX_PSB6970_POWER_MANAGEMENT_SET,
      "<Enable/Disable>",
    "\tEnable/Disable:             Enable :1  Disable :0\n"},
#endif
#endif
/****************************************************************************/
/* API list which is only for VR9 */
/****************************************************************************/
#ifdef VR9
   { "Reset", "",  IFX_FLOW_RESET,
      "\tRETURN:                  0: Switch Reset\n"},
   { "HW_Init", "",  IFX_ETHSW_HW_INIT,
      "\tRETURN:              Switch HW INIT\n"},
   {  "PCE_RuleRead", "l", IFX_FLOW_PCE_RULE_READ ,
      "<nIndex>",
      "\tnIndex:              Index \n" },
   {  "PCE_RuleWrite", "llllllllllllsllsllllllllllsllsllllllllllllllllllllllllllllllll", IFX_FLOW_PCE_RULE_WRITE ,
      "<nIndex> <Pattern parameters> <Action Parameters>",
      "\tnIndex :                  Index\n"
      "\t <Pattern Table Parameters>\n"
      "\tbEnable:                  Table Enable\n"
      "\tbPortIdEnable:            Port Enable\n"
      "\tnPortId:                  Port ID\n"
      "\tbDSCP_Enable:             Enable DSCP\n"
      "\tnDSCP:                    DSCP Value\n"
      "\tbPCP_Enable:              Enable PCP\n"
      "\tnPCP:                     PCP Value\n"
      "\tbPktLngEnable:            Enable Packet Length\n"
      "\tnPktLng                   Packet Length\n"
      "\tnPktLngRange:             Packet Length Range\n"
      "\tbMAC_DstEnable:           Enable Dst MAC Address\n"
      "\tnMAC_Dst[6]:              Dst MAC Address (xx:xx:xx:xx:xx:xx)\n"
      "\tnMAC_DstMask:             Dst MAC Address Mask\n"
      "\tbMAC_SrcEnable:           Enable Src MAC Address\n"
      "\tnMAC_Src[6]:              Src MAC Address (xx:xx:xx:xx:xx:xx)\n"
      "\tnMAC_SrcMask:             Src MAC Address Mask\n"
      "\tbAppDataMSB_Enable:       Enable Application Data MSB\n"
      "\tnAppDataMSB:              MSB Application Data\n"
      "\tbAppMaskRangeMSB_Select:  Enable MSB Application Data Mask\n"
      "\tnAppMaskRangeMSB:         MSB Application Data Mask\n"
      "\tbAppDataLSB_Enable:       Enable Application Data LSB\n"
      "\tnAppDataLSB:              LSB Application Data\n"
      "\tbAppMaskRangeLSB_Select:  Select LSB Application Data Mask\n"
      "\tnAppMaskRangeLSB:         LSB Application Data Mask\n"
      "\teDstIP_Select:            Select Dst IP Address\n"
      "\tnDstIP:                   Dst IP Address (xxx.xxx.xxx.xxx)\n"
      "\tnDstIP_Mask:              Dst IP Address Mask\n"
      "\teSrcIP_Select:            Select Src IP Address\n"
      "\tnSrcIP:                   Src IP Address (xxx.xxx.xxx.xxx)\n"
      "\tnSrcIP_Mask:              Src IP Address Mask\n"
      "\tbEtherTypeEnable:         Enable Ethernet Type\n"
      "\tnEtherType:               Ethernet Type Data\n"
      "\tnEtherTypeMask:           Ethernet Type Mask\n"
      "\tbProtocolEnable:          Enable Protocol\n"
      "\tnProtocol:                Protocol Data\n"
      "\tnProtocolMask:            Protocol Data Mask\n"
      "\tbSessionIdEnable:         Enable Session ID\n"
      "\tnSessionId:               Session ID Data\n"
      "\tbVid:                     Enable VLAN ID\n"
      "\tnVid:                     VLAN ID Data\n\n"
      "\t <Action Table Parameters>\n"
      "\teTrafficClassAction:      Action Traffic Class\n"
      "\tnTrafficClassAlternate:   Traffic Class Alternate\n"
      "\teSnoopingTypeAction:      Action Snooping Type\n"
      "\teLearningAction:          Action Learning\n"
      "\teIrqAction:               Action Irq\n"
      "\teCrossStateAction:        Action Cross State\n"
      "\teCritFrameAction:         Action Critical Frame\n"
      "\teTimestampAction:         Action Time Stamp\n"
      "\tePortMapAction:           Action Port Map\n"
      "\tnForwardPortMap:          Action Port Forwarding\n"
      "\tbRemarkAction:            Action Remarking\n"
      "\tbRemarkPCP:               Enable Remarking PCP\n"
      "\tbRemarkDSCP:              Enable Remarking DSCP\n"
      "\tbRemarkClass:             Enable Remarking Class\n"
      "\teMeterAction:             Action Metering\n"
      "\tnMeterId:                 Metering ID Data\n"
      "\tbRMON_Action:             Action RMON\n"
      "\tnRMON_Id:                 RMON ID Data\n"
      "\teVLAN_Action:             Action VLAN\n"
      "\tnVLAN_Id:                 VLAN ID Data\n"
      "\teVLAN_CrossAction:        Action Cross VLAN\n"},
   {  "PCE_RuleDelete", "l", IFX_FLOW_PCE_RULE_DELETE ,
      "<nIndex>",
      "\tnIndex:              Index \n" },
   {  "QOS_QueuePortGet", "ll", IFX_ETHSW_QOS_QUEUE_PORT_GET ,
      "<nPortId> <nTrafficClassId>",
      "\tnPortId:             Port ID\n"
      "\tnTrafficClassId:     Traffic Class index\n"},
   {  "QOS_QueuePortSet", "lll", IFX_ETHSW_QOS_QUEUE_PORT_SET ,
      "<nQueueId> <nPortId> <nTrafficClassId>",
      "\tnQueueId:            QoS queue index \n"
      "\tnPortId:             Port ID\n"
      "\tnTrafficClassId:     Traffic Class index\n"},
   {  "QOS_MeterCfgGet", "l", IFX_ETHSW_QOS_METER_CFG_GET ,
      "<nMeterId>",
      "\tnMeterId:            Meter index \n" },
   {  "QOS_MeterCfgSet", "lllll", IFX_ETHSW_QOS_METER_CFG_SET,
      "<bEnable> <nMeterId> <nCbs> <nEbs> <nRate>",
      "\tbEnable:             Enable/Disable the meter shaper\n"
      "\tnMeterId:            Meter index \n"
      "\tnCbs:                Committed Burst Size (CBS [bytes])\n"
      "\tnEbs:                Excess Burst Size (EBS [bytes])\n"
      "\tnRate:               Rate [Mbit/s]\n"},
   {  "QOS_MeterPortGet", "", IFX_ETHSW_QOS_METER_PORT_GET ,
      "",
      "\t\n" },
   {  "QOS_MeterPortAssign", "llll", IFX_ETHSW_QOS_METER_PORT_ASSIGN,
      "<nMeterId> <eDir> <nPortIngressId> <nPortEgressId>",
      "\tnMeterId:               Meter index\n"
      "\teDir:                   Port assignment 0:None 1:Ingress 2:Egress 3:Both\n"
      "\tnPortIngressId:         Ingress Port Id\n"
      "\tnPortEgressId:          Egress Port Id\n"},
   {  "QOS_MeterPortDeassign", "llll", IFX_ETHSW_QOS_METER_PORT_DEASSIGN,
      "<nMeterId> <eDir> <nPortIngressId> <nPortEgressId>",
      "\tnMeterId:               Meter index\n"
      "\teDir:                   Port assignment 0:None 1:Ingress 2:Egress 3:Both\n"
      "\tnPortIngressId:         Ingress Port Id\n"
      "\tnPortEgressId:          Egress Port Id\n"},
   {  "QOS_WredCfgGet", "", IFX_ETHSW_QOS_WRED_CFG_GET ,
      "",
      "\t\n" },
   {  "QOS_WredCfgSet", "lllllll", IFX_ETHSW_QOS_WRED_CFG_SET,
      "<eProfile> <nRed_Min> <nRed_Max> <nYellow_Min> <nYellow_Max> <nGreen_Min> <nGreen_Max>",
      "\teProfile:             Drop Probability Profile\n"
      "\t                        0:Pmin = 25%, Pmax = 75% (default)\n"
      "\t                        1:Pmin = 25%, Pmax = 50%\n"
      "\t                        2:Pmin = 50%, Pmax = 50%\n"
      "\t                        3:Pmin = 50%, Pmax = 75%\n"
      "\tnRed_Min:             WRED Red Threshold Min [8Byte segments / s]\n"
      "\tnRed_Max:             WRED Red Threshold Max [8Byte segments / s]\n"
      "\tnYellow_Min:          WRED Yellow Threshold Min [8Byte segments / s]\n"
      "\tnYellow_Max:          WRED Yellow Threshold Max [8Byte segments / s]\n"
      "\tnGreen_Min:           WRED Green Threshold Min [8Byte segments / s]\n"
      "\tnGreen_Max:           WRED Green Threshold Max [8Byte segments / s]\n"},
#if 1 // Should be remove later.
   {  "QOS_WredQueueCfgGet", "l", IFX_ETHSW_QOS_WRED_QUEUE_CFG_GET ,
      "<nQueueId>",
      " /* \tQoS queue index */ \n" },
   {  /*"QOS_WredQueueCfgSet", "llllllll", IFX_ETHSW_QOS_WRED_QUEUE_CFG_SET,*/
   	"QOS_WredQueueCfgSet", "lllllll", IFX_ETHSW_QOS_WRED_QUEUE_CFG_SET,
      "<nQueueId> <nRed_Min> <nRed_Max> <nYellow_Min> <nYellow_Max> <nGreen_Min> <nGreen_Max>",
      "\tnQueueId:             QoS queue index\n"
/*      "\teProfile:             Drop Probability Profile\n"
      "\t                        0:Pmin = 25%, Pmax = 75% (default)\n"
      "\t                        1:Pmin = 25%, Pmax = 50%\n"
      "\t                        2:Pmin = 50%, Pmax = 50%\n"
      "\t                        3:Pmin = 50%, Pmax = 75%\n" */
      "\tnRed_Min:             WRED Red Threshold Min [segments / Queue]\n"
      "\tnRed_Max:             WRED Red Threshold Max [segments / Queue]\n"
      "\tnYellow_Min:          WRED Yellow Threshold Min [segments / Queue]\n"
      "\tnYellow_Max:          WRED Yellow Threshold Max [segments / Queue]\n"
      "\tnGreen_Min:           WRED Green Threshold Min [segments / Queue]\n"
      "\tnGreen_Max:           WRED Green Threshold Max [segments / Queue]\n"},
#endif
   {  "QOS_SchedulerCfgGet", "l", IFX_ETHSW_QOS_SCHEDULER_CFG_GET ,
      "<nQueueId>",
      "\tQoS queue index \n" },
   {  "QOS_SchedulerCfgSet", "lll", IFX_ETHSW_QOS_SCHEDULER_CFG_SET,
      "<nQueueId> <eType> <nWeight>",
      "\tnQueueId:             QoS queue index \n"
      "\teType:                Scheduler Type  0:Strict Priority 1:Weighted Fair Queuing\n"
      "\tnWeight:              Ratio\n"},
   {  "QOS_StormAdd", "llll", IFX_ETHSW_QOS_STORM_CFG_SET ,
      "<nMeterId> <bBroadcast> <bMulticast> <bUnknownUnicast>",
      "\tnMeterId:                 Meter index\n"
      "\tbBroadcast:               1: Enable 0: Disable\n"
      "\tbMulticast:               1: Enable 0: Disable\n"
      "\tbUnknownUnicast:          1: Enable 0: Disable\n"},
   {  "QOS_StormGet", "", IFX_ETHSW_QOS_STORM_CFG_GET ,
      "",
      "\tReturn the STORM Configure\n"},
   {  "QOS_ShaperCfgGet", "l", IFX_ETHSW_QOS_SHAPER_CFG_GET ,
      "<nRateShaperId>",
      "\tRate Shapper ID\n" },
   {  "QOS_ShaperCfgSet", "llll", IFX_ETHSW_QOS_SHAPER_CFG_SET,
      "<nRateShaperId> <bEnable> <nCbs> <nRate> ",
      "\tnRateShaperId:             Rate Shapper ID\n"
      "\tbEnable:                   1: Enable  0:Discard\n"
      "\tnCbs:                      Committed Burst Size\n"
      "\tnRate:                     Rate [Mbit/s]\n"},
   {  "QOS_ShaperQueueAssign", "ll", IFX_ETHSW_QOS_SHAPER_QUEUE_ASSIGN ,
      "<nRateShaperId> <nQueueId>",
      "\tnRateShaperId:             Rate shaper index\n"
      "\tnQueueId:                  QoS queue index\n"},
   {  "QOS_ShaperQueueDeassign", "ll", IFX_ETHSW_QOS_SHAPER_QUEUE_DEASSIGN,
      "<nRateShaperId> <nQueueId>",
      "\tnRateShaperId:             Rate shaper index\n"
      "\tnQueueId:                  QoS queue index\n"},
   //  { "PCE_TablePrint", "l", 0xF9,
   //    "<TableIndex>",
   //    "\tTableIndex:                PCE Table Index\n"},
   {  "QOS_ClassPCPGet", "",  IFX_ETHSW_QOS_CLASS_PCP_GET,
      "<RETURN the Class to PCP mapping table, Total: 16 entry>\n"},
   {  "QOS_ClassPCPSet", "ll",  IFX_ETHSW_QOS_CLASS_PCP_SET,
      "<Index> <nTrafficClass data> ",
      "\tIndex:                 0..15\n"
      "\tnPCP:                  Hex Data : 0 .. 7\n"},
   {  "QOS_ClassDSCPGet", "",  IFX_ETHSW_QOS_CLASS_DSCP_GET,
      "<RETURN the Class to DSCP mapping table, Total: 16 entry>\n"},
   {  "QOS_ClassDSCPSet", "ll",  IFX_ETHSW_QOS_CLASS_DSCP_SET,
      "<Index> <nTrafficClass data> ",
      "\tIndex:                 0..15\n"
      "\tnDSCP:                  Hex Data : 0 .. 63\n"},
   {  "QOS_PortRemarkingCfgGet", "l",  IFX_ETHSW_QOS_PORT_REMARKING_CFG_GET,
      "<nPortId>",
      "\tnPortId:               0..6\n"
      "<RETURN the Remarking information>\n"},
   {  "QOS_PortRemarkingCfgSet", "lllll",  IFX_ETHSW_QOS_PORT_REMARKING_CFG_SET,
      "<nPortId> <eDSCP_IngressRemarkingEnable> <bDSCP_EgressRemarkingEnable> <bPCP_IngressRemarkingEnable> <bPCP_EgressRemarkingEnable>",
      "\tnPortId:               0..6\n"
      "\teDSCP_IngressRemarkingEnable:            0: Disable 1:TC6 2:TC3 3:DP3 4:DP3_TC3\n"
      "\tbDSCP_EgressRemarkingEnable:             0: Disable 1:Enable\n"
      "\tbPCP_IngressRemarkingEnable:             0: Disable 1:Enable\n"
      "\tbPCP_EgressRemarkingEnable:              0: Disable 1:Enable\n"},
   {  "STP_PortCfgGet", "l", IFX_ETHSW_STP_PORT_CFG_GET ,
      "<nPortId>",
      "\tnPortId:                   0..6\n"},
   {  "STP_PortCfgSet", "ll", IFX_ETHSW_STP_PORT_CFG_SET,
      "<nPortId> <ePortState>",
      "\tnPortId:                   0..6\n"
      "\tePortState:                0: Forwarding state\n"
      "\t                           1: Disabled/Discarding state\n"
      "\t                           2: Learning state\n"
      "\t                           3: Blocking/Listening state\n" },
   {  "STP_BPDU_RULE_Get", "", IFX_ETHSW_STP_BPDU_RULE_GET ,
      "",
      "\t\n" },
   {  "STP_BPDU_RULE_Set", "ll", IFX_ETHSW_STP_BPDU_RULE_SET,
      "<eForwardPort> <nForwardPortId>",
      "\teForwardPort:              0: Default portmap\n"
      "\t                           1: Discard\n"
      "\t                           2: Forward to cpu port\n"
      "\t                           3: Forward to a port\n"
      "\tnForwardPortId:            PortID\n"},
   {  "8021X_PortCfgGet", "l", IFX_ETHSW_8021X_PORT_CFG_GET ,
      "<nPortId>",
      "\tnPortId:                 0..6\n"},
   {  "8021X_PortCfgSet", "ll", IFX_ETHSW_8021X_PORT_CFG_SET,
      "<nPortId> <eState>",
      "\tnPortId:                 0..6\n"
      "\teState:                  0: port state authorized.\n"
      "\t                         1: port state unauthorized.\n"
      "\t                         2: RX authorized.\n"
      "\t                         3: TX authorized.\n"},
   {  "PHY_Query", "l", IFX_ETHSW_PORT_PHY_QUERY,
      "<Port Id>",
      "\tPort Id:                   0 ..6\n"},
   { "VersionGet", "l",  IFX_ETHSW_VERSION_GET,
    "<nId>",
    "\tnId:                      0:Switch API Version\n"
    "\t                            \n" },
/****************************************************************************/
#endif
   { "RMON_ExtendGet", "l",  IFX_FLOW_RMON_EXTEND_GET,
    "<Port Id>",
    "\tPort Id:                   0 ..6\n"},
   {  "8021X_EAPOL_RuleGet", "", IFX_ETHSW_8021X_EAPOL_RULE_GET,
      "",
      "\t\n"},
   {  "8021X_EAPOL_RuleSet", "ll", IFX_ETHSW_8021X_EAPOL_RULE_SET,
      "<eForwardPort> <nForwardPortId>",
      "\teForwardPort:            0: Default\n"
      "\t                         1: Discard\n"
      "\t                         2: Forward to the CPU port\n"
      "\t                         3: Forward to a port\n"
      "\tnForwardPortId:          port\n"},
   { "CapGet", "", IFX_ETHSW_CAP_GET,
      "<nCapType>",
      "\tnCapType                  0: physical Ethernet ports.\n"
      "\t                          1: virtual Ethernet ports\n"
      "\t                          2: internal packet memory [in Bytes]\n"
      "\t                          3: Buffer segment size\n"
      "\t                          4: priority queues per device\n"
      "\t                          5: meter instances\n"
      "\t                          6: rate shaper instances\n"
      "\t                          7: VLAN groups\n"
      "\t                          8: Forwarding database IDs\n"
      "\t                          9: MAC table entries\n"
      "\t                          10: multicast level 3 hardware table entries\n"
      "\t                          11: PPPoE sessions\n"},
   {  "Enable", "",  IFX_ETHSW_ENABLE,
      "\t Enable the SWITCH \n"},
   {  "Disable", "",  IFX_ETHSW_DISABLE,
      "\t Disable the SWITCH \n"},
};


unsigned long    cmd_arg[MAX_COMMAND_ARGS]={};     /* max. 64 command arguments (integer/long/string) */
char    cmd_arg_strings[MAX_CMD_STR_LEN]; /* max 100 characters as string parameters */

/* switch control char device and assigned file descriptor */
static int switch_fd = 0;
#ifdef AR9
static char switch_dev[]="/dev/switch_api/1";
#endif

#ifdef DANUBE
static char switch_dev[]="/dev/switch_api/1";
#endif

#ifdef AMAZON_SE
static char switch_dev[]="/dev/switch_api/1";
#endif

#ifdef VR9
static char switch_dev[]="/dev/switch_api/0";
#endif

int cmd_index = -1;

int nr_commands=sizeof(commands)/sizeof(commands[0]);


/**
 * \brief Prints the list of available commands stored in commands[]
 *
 * \param VOID
 *
 * \return
 *  NONE
 */
void print_commands( void )
{
   int c;

   printf("\nSwitch Utility Version : v%s\n",SWITCH_API_DRIVER_VERSION);
   printf("Available commands:\n");
   for (c = 0; c < nr_commands; c++)
   {
      printf("\t%s\n", commands[c].name);
   }
}

/**
 * \brief Prints the command usage
 *
 * \param index Command index in the Global Commands array
 *
 * \return
 *  NONE
 */
void print_command( int index )
{
   printf("Usage:  switch_utility %s %s\n", commands[index].name, commands[index].param_string);
   printf("\n%s\n", commands[index].detailed_info);
}


/**
 * \brief This function checks the command line strings for correctness
 * argv[1] has to be a valid command that can be converted to an switch ioctl
 * command.
 *
 * \param command String containing the command
 *
 * \return Position of the given command in the command array if command is
 * found in the command array. Else -1 to indicate failure
 *
 * \remark nr_strings include all command line parameters
 */
int check_command( char *command )
{
   int c;

   for (c = 0; c < nr_commands; c++)
   {
      if( strcmp(command, commands[c].name) == 0 )
      {
         /* yeah, we found our command */
         break;
      }
   }
   if (c >= nr_commands)
   {
      printf("Command %s not found\n", command);
      print_commands();
      return -1;
   }

   return c;
}


/**
 * \brief Converts the passed mac address string (01:02:03:ab:cd:ef)
 * to an array of values (0x01, 0x02, 0x03, 0xab, 0xcd, 0xef)
 *
 * \param mac_adr_str MAC Address in String format
 *
 * \param mac_adr_ptr MAC Address in array format
 * \return
 *   IFX_SUCCESS and mac_add_ptr is updated with the array format
 *   IFX_ERROR on failure
 */
int convert_mac_adr_str( char *mac_adr_str, unsigned char *mac_adr_ptr )
{
   char *str_ptr=mac_adr_str;
   char *endptr;
   int i, val=0;

   if (strlen(mac_adr_str) != (12+5))
   {
      printf("Invalid length of address string!\n");
      return IFX_ERROR;
   }

   for (i=0; i<6; i++)
   {
      val = strtoul(str_ptr, &endptr, 16);
      if (*endptr && (*endptr != ':') && (*endptr != '-'))
            return IFX_ERROR;
      *(mac_adr_ptr+i)=val;
      str_ptr = endptr+1;
   }

   return IFX_SUCCESS;
}

/**
 * \brief Converts the passed ip address string (192.168.100.1)
 * to an array of values (0xC0, 0xA8, 0x64, 0x01)
 *
 * \param ip_adr_str IP Address in String format
 *
 * \param ip_adr_ptr IP Address in array format
 * \return
 *   IFX_SUCCESS and ip_add_ptr is updated with the array format
 *   IFX_ERROR on failure
 */
int convert_ip_adr_charStr( char *ip_adr_str, unsigned int *ip_adr_ptr )
{
   char *str_ptr=ip_adr_str;
   char *endptr;
   int i, val=0;
   unsigned int data = 0;

   if (strlen(ip_adr_str) > (12+3))
   {
      printf("Invalid length of IP address string!\n");
      return IFX_ERROR;
   }

   for (i=3; i>=0; i--)
   {
      val = strtoul(str_ptr, &endptr, 10);
      if (*endptr && (*endptr != '.') && (*endptr != '-'))
            return IFX_ERROR;

      //printf("String Value = %03d\n",val);
      data |= (val << (i * 8));
      //printf("String Value = %08x\n",data);
      str_ptr = endptr+1;
   }
   *(ip_adr_ptr) = data;
   return IFX_SUCCESS;
}

int convert_ipv4_str( char *ip_adr_str, unsigned int *ip_adr_ptr )
{
    char *str_ptr=ip_adr_str;
    char *endptr;
    int i, val=0;
    unsigned int data = 0;

    if (strlen(ip_adr_str) > (12+3))
    {
      printf("Invalid length of IP address string!\n");
      return IFX_ERROR;
    }

    for (i=0; i<4; i++)
    {
      val = strtoul(str_ptr, &endptr, 10);
      if (*endptr && (*endptr != '.') && (*endptr != '-'))
            return IFX_ERROR;

      //printf("String Value = %03d\n",val);
      data |= (val << ((3-i) * 8));
      //printf("String Value = %08x\n",data);
      str_ptr = endptr+1;
    }
    *(ip_adr_ptr) = data;
    return IFX_SUCCESS;
}

int convert_ipv6_str( char *ip_adr_str, void *ip_adr_ptr )
{
    IFX_uint16_t *pIPv6  = (IFX_uint16_t *)ip_adr_ptr;
    char *str_ptr=ip_adr_str;
    char *endptr;
    int i ;
    unsigned int val=0;

    if (strlen(ip_adr_str) > (32+7))
    {
      printf("Invalid length of IP V6 address string!\n");
      return IFX_ERROR;
    }

    for (i=0; i<8; i++)
    {
      val = strtoul(str_ptr, &endptr, 16);
      if (*endptr && (*endptr != ':') && (*endptr != '-'))
            return IFX_ERROR;

      *pIPv6++ = val;
      str_ptr = endptr+1;
    }
    //*(ip_adr_ptr) = data;
    return IFX_SUCCESS;
}


/**
 * \brief Converts the passed ip address string (192.168.100.1)
 * to an array of values (0xC0, 0xA8, 0x64, 0x01)
 *
 * \param ip_adr_str IP Address in String format
 *
 * \param ip_adr_ptr IP Address in array format
 * \return
 *   IFX_SUCCESS and ip_add_ptr is updated with the array format
 *   IFX_ERROR on failure
 */
int convert_ip_adr_str( char *ip_adr_str, unsigned int *ip_adr_ptr )
{
   char *str_ptr=ip_adr_str;
   char *endptr;
   int i, val=0;

   if (strlen(ip_adr_str) > (12+3))
   {
      printf("Invalid length of IP address string!\n");
      return IFX_ERROR;
   }

   for (i=0; i<4; i++)
   {
      val = strtoul(str_ptr, &endptr, 10);
      if (*endptr && (*endptr != '.') && (*endptr != '-'))
            return IFX_ERROR;
      *(ip_adr_ptr+i)=val;
      str_ptr = endptr+1;
   }

   return IFX_SUCCESS;
}
/**
 * \brief Converts the input parameters in the command
 * to the specified format and update the command array
 *
 * \param params List of parameters specified in command
 *
 * \param nr_params Number of parameters
 * \return
 *   IFX_SUCCESS or 0 on success
 *   -1 on failure
 */
int convert_parameters( char **params, int nr_params)
{
   const char *parg_string;
   int     cmd_arg_str_offset = 0;
   int     argc = 0;
   const char *arg;
   int param_pos;
   int slen;
   char *endptr;

   if( nr_params==0 )
   {
      /* Nothing to convert -> return OK */
      return IFX_SUCCESS;
   }

   /*
    * transfer the parameter strings to the cmd_arg array by integer conversion
    */
   parg_string = commands[cmd_index].param;
   param_pos=0;
   while( (*parg_string) &&
      (argc < MAX_COMMAND_ARGS) &&
      (param_pos < nr_params) )
   {
      arg = params[param_pos];
      slen = strlen(arg);
      switch (*parg_string)
      {
      case 'i':
      case 'l':
         cmd_arg[argc++] = strtoul(arg, &endptr, 0);
         break;
      case 's':
         memcpy(&cmd_arg_strings[cmd_arg_str_offset], arg, slen);
         cmd_arg_strings[cmd_arg_str_offset + slen] = '\0';
         cmd_arg[argc++] = (long)&cmd_arg_strings[cmd_arg_str_offset];
         cmd_arg_str_offset += slen+1;
         break;
      default:
         printf("Invalid format string in module, please fix and re-compile\n");
         return -1;
      }
      param_pos++;
      parg_string++;
   }

   return 0;
}

/**
 * \brief Print the Meter Port Table
 *
 * \param VOID
 *
 * \return
 *   IFX_SUCCESS
 */
int MeterPort_TABLE_PRINT( void )
{
   union ifx_sw_param x;
   int retval=IFX_SUCCESS;

   memset(&x.qos_meterportget, 0x00, sizeof(x.qos_meterportget));
   x.qos_meterportget.bInitial = IFX_TRUE;
   retval = ioctl( switch_fd, IFX_ETHSW_QOS_METER_PORT_GET, &x.qos_meterportget);
   if(retval != IFX_SUCCESS)
   {
      printf("IFX_ETHSW_QOS_METER_PORT_GET Error\n");
      return -1;

   }

   if (x.qos_meterportget.bLast == IFX_FALSE)
   {
      printf(" | Meter Index = %2d | Direction = %2d | Ingress PortID = %2d | Egress PortID = %2d\n",
            x.qos_meterportget.nMeterId,x.qos_meterportget.eDir,
            x.qos_meterportget.nPortIngressId, x.qos_meterportget.nPortEgressId);

      do {
         memset(&x.qos_meterportget, 0x00, sizeof(x.qos_meterportget));
         retval=ioctl( switch_fd, IFX_ETHSW_QOS_METER_PORT_GET, &x.qos_meterportget );

         if(retval != IFX_SUCCESS)
         {
            printf("IFX_ETHSW_QOS_METER_PORT_GET Error\n");
            return -1;
         }

      printf(" | Meter Index = %2d | Direction = %2d | Ingress PortID = %2d | Egress PortID = %2d\n",
            x.qos_meterportget.nMeterId,x.qos_meterportget.eDir,
            x.qos_meterportget.nPortIngressId, x.qos_meterportget.nPortEgressId);


      } while (x.qos_meterportget.bLast == IFX_FALSE);
   }

   return IFX_SUCCESS;
}

/**
 * \brief Print Capability
 *
 * \param VOID
 *
 * \return
 *   IFX_SUCCESS
 */
int CapGet( void )
{
   IFX_ETHSW_cap_t cap;
   int i;

   for (i = 0; i < IFX_ETHSW_CAP_TYPE_LAST; i++ )
   {
      memset(&cap, 0x00, sizeof(cap));
      cap.nCapType = i;
      if(ioctl(switch_fd, IFX_ETHSW_CAP_GET, &cap) != 0)
         return (-2);

      if (cap.nCap != 0) {
         printf("\tCapability: %s = %d\n", cap.cDesc, cap.nCap);
      }
   }

   return 0;
}

int ifx_ethsw_vlan_port_member_read()
{
	IFX_ETHSW_VLAN_portMemberRead_t portMemberRead;

	printf("--------------------------------------------------------------\n");
	printf(" VLAN ID            | Port               | Tag Member\n");
	printf("--------------------------------------------------------------\n");
	memset(&portMemberRead, 0x00, sizeof(portMemberRead));
	portMemberRead.bInitial = IFX_TRUE;
	for (;;) {
		if (ioctl( switch_fd, IFX_ETHSW_VLAN_PORT_MEMBER_READ, &portMemberRead )!= IFX_SUCCESS) {
			return (-2);
		}
	
		if (portMemberRead.bLast == IFX_TRUE)
			break;
		if (portMemberRead.nVId)
			printf("%20d | 0x%04x | 0x%04x\n", portMemberRead.nVId, portMemberRead.nPortId ,portMemberRead.nTagId );
		memset(&portMemberRead, 0x00, sizeof(portMemberRead));
		portMemberRead.bInitial = IFX_FALSE;
	}
	printf("--------------------------------------------------------------\n");
	return 0;
}


/**
 * \brief Print the MAC table
 *
 * \param VOID
 *
 * \return
 *   IFX_SUCCESS
 */
int MAC_TABLE_PRINT( void )
{
   union ifx_sw_param x;
   int retval=IFX_SUCCESS;
   int port;

   printf("--------------------------------------------------------------\n");
   printf("   MAC Address       | port |   age    | FID | Static  \n");
   printf("--------------------------------------------------------------\n");


   memset(&x.MAC_tableRead, 0x00, sizeof(x.MAC_tableRead));
   x.MAC_tableRead.bInitial = IFX_TRUE;
   retval = ioctl( switch_fd, IFX_ETHSW_MAC_TABLE_ENTRY_READ, &x.MAC_tableRead);
   if(retval != IFX_SUCCESS)
   {
      printf("IFX_ETHSW_MAC_TABLE_ENTRY_READ Error\n");
      return -1;

   }

   if (x.MAC_tableRead.bLast == IFX_FALSE)
   {
     if ((x.MAC_tableRead.nMAC[0] == 0) && (x.MAC_tableRead.nMAC[1] == 0) && (x.MAC_tableRead.nMAC[2] == 0) &&
       (x.MAC_tableRead.nMAC[3] == 0) && (x.MAC_tableRead.nMAC[4] == 0) && (x.MAC_tableRead.nMAC[5] == 0))
     {
       /* Nothing to be done */
     }
     else
     {
         printf("  %02x:%02x:%02x:%02x:%02x:%02x  |  %d   |  %4d    | %d   | %d\n",
            x.MAC_tableRead.nMAC[0], x.MAC_tableRead.nMAC[1], x.MAC_tableRead.nMAC[2],
            x.MAC_tableRead.nMAC[3], x.MAC_tableRead.nMAC[4], x.MAC_tableRead.nMAC[5],
            x.MAC_tableRead.nPortId,x.MAC_tableRead.nAgeTimer, x.MAC_tableRead.nFId,
            x.MAC_tableRead.bStaticEntry );
     }

     do {
       memset(&x.MAC_tableRead, 0x00, sizeof(x.MAC_tableRead));
       retval=ioctl( switch_fd, IFX_ETHSW_MAC_TABLE_ENTRY_READ, &x.MAC_tableRead );

       if(retval != IFX_SUCCESS)
       {
          printf("IFX_ETHSW_MAC_TABLE_ENTRY_READ Error\n");
          return -1;

       }

       if (x.MAC_tableRead.bLast == IFX_FALSE)
       {
      if ((x.MAC_tableRead.nMAC[0] == 0) && (x.MAC_tableRead.nMAC[1] == 0) && (x.MAC_tableRead.nMAC[2] == 0) &&
             (x.MAC_tableRead.nMAC[3] == 0) && (x.MAC_tableRead.nMAC[4] == 0) && (x.MAC_tableRead.nMAC[5] == 0))
          {
             /* Do nothing */
          }
          else
      {
         if ( x.MAC_tableRead.bStaticEntry == 1)
         {
           for (port = 0; port < 7; port++)
           {
                 if ( ((x.MAC_tableRead.nPortId >> port) & 0x1) == 1)
                 {
                    printf("  %02x:%02x:%02x:%02x:%02x:%02x  |  %d   |  %4d    | %d   | %d \n",
                        x.MAC_tableRead.nMAC[0], x.MAC_tableRead.nMAC[1], x.MAC_tableRead.nMAC[2],
                        x.MAC_tableRead.nMAC[3], x.MAC_tableRead.nMAC[4], x.MAC_tableRead.nMAC[5],
                        port, x.MAC_tableRead.nAgeTimer, x.MAC_tableRead.nFId,
                        x.MAC_tableRead.bStaticEntry);
                 }
               }
             }
             else
             {
                printf("  %02x:%02x:%02x:%02x:%02x:%02x  |  %d   |  %4d    | %d   | %d\n",
                 x.MAC_tableRead.nMAC[0], x.MAC_tableRead.nMAC[1], x.MAC_tableRead.nMAC[2],
                 x.MAC_tableRead.nMAC[3], x.MAC_tableRead.nMAC[4], x.MAC_tableRead.nMAC[5],
                 x.MAC_tableRead.nPortId, x.MAC_tableRead.nAgeTimer, x.MAC_tableRead.nFId,
                 x.MAC_tableRead.bStaticEntry );
             }
          }
        }
       } while (x.MAC_tableRead.bLast == IFX_FALSE);
    }

   printf("--------------------------------------------------------------\n");

   return IFX_SUCCESS;
}


/**
 * \brief Print the MAC table
 *
 * \param VOID
 *
 * \return
 *   IFX_SUCCESS
 */
int print_mac_table( void )
{

   int ret;
   int count = 0, i;
   union ifx_sw_param x;
   int retval=IFX_SUCCESS;

   printf("--------------------------------------------------------------\n");
   printf("  idx |   MAC Address       | port |   age    | FID | Static  \n");
   printf("--------------------------------------------------------------\n");

   for (i=0; i<7; i++)
   {
     memset(&x.MAC_tableRead, 0x00, sizeof(x.MAC_tableRead));
     x.MAC_tableRead.bInitial = 1;
     x.MAC_tableRead.nPortId = i;
     ret = ioctl( switch_fd, IFX_ETHSW_MAC_TABLE_ENTRY_READ, &x.MAC_tableRead);
     if(retval != IFX_SUCCESS)
     {
      printf("IFX_ETHSW_MAC_TABLE_ENTRY_READ Error\n");
      return -1;

     }
     if (x.MAC_tableRead.bLast == 0)
     {
      printf("   %2d |  %02x:%02x:%02x:%02x:%02x:%02x  |  %d   |  %4d    | %d   | %d\n",
           count,
             x.MAC_tableRead.nMAC[0], x.MAC_tableRead.nMAC[1], x.MAC_tableRead.nMAC[2],
             x.MAC_tableRead.nMAC[3], x.MAC_tableRead.nMAC[4], x.MAC_tableRead.nMAC[5],
             i,
             x.MAC_tableRead.nAgeTimer, x.MAC_tableRead.nFId,
             x.MAC_tableRead.bStaticEntry );
       count++;
       do {
         memset(&x.MAC_tableRead, 0x00, sizeof(x.MAC_tableRead));
         x.MAC_tableRead.bInitial = 0;
         x.MAC_tableRead.nPortId = i;
         retval=ioctl( switch_fd, IFX_ETHSW_MAC_TABLE_ENTRY_READ, &x.MAC_tableRead );
         if(retval != IFX_SUCCESS)
         {
          printf("IFX_ETHSW_MAC_TABLE_ENTRY_READ Error\n");
          return -1;

         }
         if (x.MAC_tableRead.bLast == 0)
         {

          printf("   %2d |  %02x:%02x:%02x:%02x:%02x:%02x  |  %d   |  %4d    | %d   | %d\n",
               count,
                 x.MAC_tableRead.nMAC[0], x.MAC_tableRead.nMAC[1], x.MAC_tableRead.nMAC[2],
                 x.MAC_tableRead.nMAC[3], x.MAC_tableRead.nMAC[4], x.MAC_tableRead.nMAC[5],
                 i,
                 x.MAC_tableRead.nAgeTimer, x.MAC_tableRead.nFId,
                 x.MAC_tableRead.bStaticEntry );
            count++;
         }
       } while (x.MAC_tableRead.bLast == 0);

     }
    }

   printf("--------------------------------------------------------------\n");

   return IFX_SUCCESS;
}

/**
 * \brief Print the IGMP table entry
 *
 * \param VOID
 *
 * \return
 *   IFX_SUCCESS
 */
int print_IPv4(  IFX_uint32_t ipv4 )
{
    printf(" %03d.%03d.%03d.%03d |",
    (ipv4 >> 24)  & 0xFF,
    (ipv4 >> 16)  & 0xFF,
    (ipv4 >> 8)  & 0xFF,
    ipv4 & 0xFF);
    return IFX_SUCCESS;
}

int print_IPv6(IFX_uint16_t *ipv6 )
{
	printf(" %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x |", ipv6[0], ipv6[1],ipv6[2],ipv6[3],	\
		ipv6[4], ipv6[5],ipv6[6],ipv6[7]);
	 return IFX_SUCCESS;
}



int print_IPv6_GDA( IFX_uint16_t *ipv6 )
{
    printf("    %04x:%04x    |", ipv6[6], ipv6[7]);
    return IFX_SUCCESS;
}

int print_IPv6_GSA( IFX_uint16_t *ipv6 )
{
    printf("  %04x:%04x:%04x |", ipv6[5], ipv6[6], ipv6[7]);
    return IFX_SUCCESS;
}

int IGMP_table_entry_print( union ifx_sw_param *p )
{
	int i;
	unsigned int port = p->multicast_TableEntryRead.nPortId; 
	for ( i = 0; i < 15 ; i ++ ) {
		if ( (port >> i ) & 0x1 )
			printf(" %d,", (i));
	}

	printf(" | ");
	// GDA field
	if (p->multicast_TableEntryRead.eIPVersion == IFX_ETHSW_IP_SELECT_IPV4)
		print_IPv4(p->multicast_TableEntryRead.uIP_Gda.nIPv4);
	else if (p->multicast_TableEntryRead.eIPVersion == IFX_ETHSW_IP_SELECT_IPV6) {
		print_IPv6((IFX_uint16_t*)&p->multicast_TableEntryRead.uIP_Gda.nIPv6);
	}
    

   // GSA field
   if (p->multicast_TableEntryRead.eIPVersion == IFX_ETHSW_IP_SELECT_IPV4)
   {
      if (p->multicast_TableEntryRead.uIP_Gsa.nIPv4 == 0)
         printf("      NA         |");

      if (p->multicast_TableEntryRead.uIP_Gsa.nIPv4 > 0)
         print_IPv4(p->multicast_TableEntryRead.uIP_Gsa.nIPv4);
   } else if (p->multicast_TableEntryRead.eIPVersion == IFX_ETHSW_IP_SELECT_IPV6) {
   		int i, flag=0;
   		for (i=0;i<8;i++) {
   			if (p->multicast_TableEntryRead.uIP_Gsa.nIPv6[i] != 0 ){
   				flag = 1;
   				break;
   			}
   		}
   		if (flag )
   			printf(" %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x |", p->multicast_TableEntryRead.uIP_Gsa.nIPv6[0],	\
   			 p->multicast_TableEntryRead.uIP_Gsa.nIPv6[1],p->multicast_TableEntryRead.uIP_Gsa.nIPv6[2],	\
   			 p->multicast_TableEntryRead.uIP_Gsa.nIPv6[3],p->multicast_TableEntryRead.uIP_Gsa.nIPv6[4],	\
   			 p->multicast_TableEntryRead.uIP_Gsa.nIPv6[5],p->multicast_TableEntryRead.uIP_Gsa.nIPv6[6],	\
   			 p->multicast_TableEntryRead.uIP_Gsa.nIPv6[7]);
		else
         	printf("      NA         |");
	}

   // Member Mode field
   printf("    %d\n",p->multicast_TableEntryRead.eModeMember);
    return IFX_SUCCESS;
}

int print_igmp_membership_table_entry( union ifx_sw_param *p )
{
    // Port field
    printf(" %2d   |", p->multicast_TableEntryRead.nPortId);

    // GDA field
    if (p->multicast_TableEntryRead.eIPVersion == IFX_ETHSW_IP_SELECT_IPV4)
    {   // IPV4
        print_IPv4(p->multicast_TableEntryRead.uIP_Gda.nIPv4);
    }
    else
    {   // IPV6
        print_IPv6_GDA((IFX_uint16_t*)&p->multicast_TableEntryRead.uIP_Gda.nIPv6);
    }

    // GSA field
    if (p->multicast_TableEntryRead.eIPVersion == IFX_ETHSW_IP_SELECT_IPV4)
    {
        if (p->multicast_TableEntryRead.uIP_Gsa.nIPv4 == 0)
        {
            printf("       NA        |");
        }
        else
        {
            print_IPv4(p->multicast_TableEntryRead.uIP_Gsa.nIPv4);
        }
    }
    else
    {   // IPV6
        print_IPv6_GSA((IFX_uint16_t*)&p->multicast_TableEntryRead.uIP_Gsa.nIPv6);
    }

    // Member Mode field
    printf("    %d\n",p->multicast_TableEntryRead.eModeMember);

    return IFX_SUCCESS;
}

/**
 * \brief Print the Multicast Router Port
 *
 * \param VOID
 *
 * \return
 *   IFX_SUCCESS
 */
int print_multicast_router_port( void )
{
   int ret;
   union ifx_sw_param x;

   memset(&x.multicast_RouterPortRead, 0x00, sizeof(x.multicast_RouterPortRead));
   x.multicast_RouterPortRead.bInitial = IFX_TRUE;
   ret = ioctl( switch_fd, IFX_ETHSW_MULTICAST_ROUTER_PORT_READ, &x.multicast_RouterPortRead);
   if(ret != IFX_SUCCESS)
   {
      printf("IFX_ETHSW_MULTICAST_ROUTER_PORT_READ Error.\n");
      return IFX_ERROR;
   }

   if (x.multicast_RouterPortRead.bLast == IFX_FALSE)
   {
      printf("   Router Port = %d\n",x.multicast_RouterPortRead.nPortId);
      do {
         memset(&x.multicast_RouterPortRead, 0x00, sizeof(x.multicast_RouterPortRead));
         ret = ioctl( switch_fd, IFX_ETHSW_MULTICAST_ROUTER_PORT_READ, &x.multicast_RouterPortRead);
         if(ret != IFX_SUCCESS)
         {
            printf("IFX_ETHSW_MULTICAST_ROUTER_PORT_READ Error.\n");
            return IFX_ERROR;
         }

         if (x.multicast_RouterPortRead.bLast == IFX_FALSE)
            printf("   Router Port = %d\n",x.multicast_RouterPortRead.nPortId);

      } while (x.multicast_RouterPortRead.bLast == IFX_FALSE);
   }

   return IFX_SUCCESS;
}
/**
 * \brief Print the IGMP table
 *
 * \param VOID
 *
 * \return
 *   IFX_SUCCESS
 */
int print_igmp_membership_table( void )
{
    int ret;
    union ifx_sw_param x;

    memset(&x.multicast_TableEntryRead, 0x00, sizeof(x.multicast_TableEntryRead));
    x.multicast_TableEntryRead.bInitial = IFX_TRUE;
    ret = ioctl( switch_fd, IFX_ETHSW_MULTICAST_TABLE_ENTRY_READ, &x.multicast_TableEntryRead);
    if(ret != IFX_SUCCESS)
    {
         printf("IFX_ETHSW_MULTICAST_TABLE_ENTRY_READ Error.\n");
         return IFX_ERROR;
    }

    if (x.multicast_TableEntryRead.bLast == IFX_FALSE)
    {
            printf("-------------------------------------------------------\n");
            printf(" Port |      GDA        |       GSA       | Member Mode\n");
            printf("-------------------------------------------------------\n");
#if defined(VR9)
              IGMP_table_entry_print(&x);
#else
              print_igmp_membership_table_entry(&x);
#endif
            do {
                memset(&x.multicast_TableEntryRead, 0x00, sizeof(x.multicast_TableEntryRead));
                x.multicast_TableEntryRead.bInitial = IFX_FALSE;
                ret=ioctl( switch_fd, IFX_ETHSW_MULTICAST_TABLE_ENTRY_READ, &x.multicast_TableEntryRead );
                if(ret != IFX_SUCCESS)
                {
                    printf("IFX_ETHSW_MULTICAST_TABLE_ENTRY_READ Error\n");
                    return IFX_ERROR;
                }
                if ( x.multicast_TableEntryRead.bLast == IFX_FALSE)
                {
#if defined(VR9)
                    IGMP_table_entry_print(&x);
#else
                    print_igmp_membership_table_entry(&x);
#endif
                }
            } while (x.multicast_TableEntryRead.bLast == IFX_FALSE);
     }
     else
     {
        if (x.multicast_TableEntryRead.bInitial == IFX_FALSE)
        {
            printf("no IGMP Table..\n");
        }
     }

     return IFX_SUCCESS;
}

/**
 * \brief Print the MFC table entry
 *
 * \param VOID
 *
 * \return
 *   IFX_SUCCESS
 */
int print_mfc_table_entry( union ifx_sw_param *p )
{
    IFX_PSB6970_QoS_MfcCfg_t  *pFilter;

    pFilter = &(p->qos_MfcEntryRead.sFilter);
    if ( pFilter->sFilterMatchField.eFieldSelection != 0xFF)
    {

        if ( pFilter->sFilterMatchField.eFieldSelection == IFX_PSB6970_QOS_MF_ETHERTYPE)
        {
            printf("%5d %7s %7s %12s %12s %8s %9xH%12d %11d\n",
                    pFilter->sFilterMatchField.eFieldSelection,
                    "", "", "", "", "",
                    pFilter->sFilterMatchField.nEtherType,
                    pFilter->sFilterInfo.nTrafficClass,
                    pFilter->sFilterInfo.ePortForward
                    );
        }
        else if ( pFilter->sFilterMatchField.eFieldSelection == IFX_PSB6970_QOS_MF_PROTOCOL)
        {
            printf("%5d %7s %7s %12s %12s %8d %9s %12s %11d\n",
                    pFilter->sFilterMatchField.eFieldSelection,
                    "", "", "", "",
                    pFilter->sFilterMatchField.nProtocol,
                    "", "",
                    pFilter->sFilterInfo.ePortForward
                    );
        }
        else if ( pFilter->sFilterMatchField.eFieldSelection == IFX_PSB6970_QOS_MF_SRCPORT)
        {
            printf("%5d %7d %7s %12d %12s %8s %9s %12d %11d\n",
                    pFilter->sFilterMatchField.eFieldSelection,
                    pFilter->sFilterMatchField.nPortSrc,
                    "",
                    pFilter->sFilterMatchField.nPortSrcRange,
                    "", "", "",
                    pFilter->sFilterInfo.nTrafficClass,
                    pFilter->sFilterInfo.ePortForward
                    );
        }
        else if ( pFilter->sFilterMatchField.eFieldSelection == IFX_PSB6970_QOS_MF_DSTPORT)
        {
            printf("%5d %7s %7d %12s %12d %8s %9s %12d %11d\n",
                    pFilter->sFilterMatchField.eFieldSelection,
                    "",
                    pFilter->sFilterMatchField.nPortDst,
                    "",
                    pFilter->sFilterMatchField.nPortDstRange,
                    "", "",
                    pFilter->sFilterInfo.nTrafficClass,
                    pFilter->sFilterInfo.ePortForward
                    );
        }
    }

    return IFX_SUCCESS;
}
/**
 * \brief Print the MFC filter table
 *
 * \param VOID
 *
 * \return
 *   IFX_SUCCESS
 */
int print_mfc_filter_table( void )
{
    int ret;
    union ifx_sw_param x;

    printf("Field PortSrc PortDst PortSrcRange PortDstRange Protocol EtherType TrafficClass PortForward\n");
    printf("===== ======= ======= ============ ============ ======== ========= ============ ===========\n");

    memset(&x.qos_MfcEntryRead, 0x00, sizeof(x.qos_MfcEntryRead));
    x.qos_MfcEntryRead.bInitial = IFX_TRUE;
    ret=ioctl( switch_fd, IFX_PSB6970_QOS_MFC_ENTRY_READ, &x.qos_MfcEntryRead );
    if(ret != IFX_SUCCESS)
    {
         printf("IFX_ETHSW_MULTICAST_TABLE_ENTRY_READ Error\n");
         return IFX_ERROR;
    }
    if (x.qos_MfcEntryRead.bLast == IFX_FALSE)
    {
        print_mfc_table_entry(&x);
        do {
            memset(&x.qos_MfcEntryRead, 0x00, sizeof(x.qos_MfcEntryRead));
            x.qos_MfcEntryRead.bInitial = IFX_FALSE;
            ret=ioctl( switch_fd, IFX_PSB6970_QOS_MFC_ENTRY_READ, &x.qos_MfcEntryRead );
            if(ret != IFX_SUCCESS)
            {
                printf("IFX_ETHSW_MULTICAST_TABLE_ENTRY_READ Error\n");
                return IFX_ERROR;
            }
            if ( x.qos_MfcEntryRead.bLast == IFX_FALSE)
                print_mfc_table_entry(&x);
        } while (x.qos_MfcEntryRead.bLast == IFX_FALSE);
     }
   return IFX_SUCCESS;
}

/**
 * \brief Print the list of Ethernet switch devices registered
 *
 * \param VOID
 *
 * \return
 *   IFX_SUCCESS
 */
int print_device_list( void )
{
#if 0
   int ret, i = 0;
   IFX_DEV_INFO_t dev_info[ETHSW_MAX_HL_DEVICES];

   ret = ioctl( switch_fd, IFX_ETHSW_DEV_GET, &dev_info);
   if (ret >= 0) {
     printf("====================================\n");
     printf("Device List\n");
     printf("====================================\n");
     for(i=0; i<ret; i++) {
       printf("Device Name [%s]\n", dev_info[i].devName);
       printf("Major Number [%d]\n", dev_info[i].majorNumber);
       printf("Minor Number [%d]\n\n", dev_info[i].minorBase);
     }
     printf("====================================\n");
   }
  #endif
   return IFX_SUCCESS;
}

/**
 * \brief Execute the user specified command
 *
 * \param cmd Command specified by user
 *
 * \return
 *   IFX_SUCCESS on successful execution of cmd
 *   IFX_ERROR on failure
 */
int execute_user_command( const char* cmd )
{
   int retval = IFX_ERROR;

   if( strcmp( print_mac_table_str, cmd ) == 0)
   {
      if( print_mac_table() == IFX_SUCCESS)
         retval =  IFX_SUCCESS;
   }
   else if( strcmp( print_device_list_str, cmd ) == 0)
   {
      if( print_device_list() == IFX_SUCCESS)
         retval =  IFX_SUCCESS;
   }
   else
      printf("Command %s not found\n", cmd);

   return retval;
}

/**
 * \brief Build the ioctl command based on the input
 * command and parameters
 *
 * \param fd Switch device file descriptor
 *
 * \param ioctl_command IOCTL command
 *
 * \return
 *   IFX_SUCCESS on successful execution of cmd
 *   -1 on failure
 */
int build_ioctl_command( int fd, unsigned int ioctl_command )
{
   void* ioctl_params = NULL;
   union ifx_sw_param x;
   int retval=IFX_SUCCESS;
   IFX_uint32_t value_low, value_high ;
   IFX_uint8_t value, index, i=0;

   index = 0;
   switch(ioctl_command) {
       case IFX_ETHSW_MAC_TABLE_ENTRY_READ:
#if  defined(AR9) || defined(DANUBE) || defined(AMAZON_SE)
            print_mac_table();
#else
            MAC_TABLE_PRINT();
#endif
           break;
           case IFX_ETHSW_VLAN_PORT_MEMBER_READ:
            ifx_ethsw_vlan_port_member_read();
            break;
       case IFX_ETHSW_MAC_TABLE_ENTRY_ADD:
           memset(&x.MAC_tableAdd, 0x00, sizeof(x.MAC_tableAdd));
           x.MAC_tableAdd.nFId = cmd_arg[0];
           x.MAC_tableAdd.nPortId = cmd_arg[1];
           x.MAC_tableAdd.nAgeTimer = cmd_arg[2];
           x.MAC_tableAdd.bStaticEntry = cmd_arg[3];
/*           x.MAC_tableAdd.nTrafficClass = cmd_arg[4];*/
           if( convert_mac_adr_str((char*)cmd_arg[4], &x.MAC_tableAdd.nMAC[0]) != IFX_SUCCESS)
               return -1;
           ioctl_params = (void *)&x.MAC_tableAdd;
           break;
       case IFX_ETHSW_MAC_TABLE_ENTRY_REMOVE:
           memset(&x.MAC_tableRemove, 0x00, sizeof(x.MAC_tableRemove));
           x.MAC_tableRemove.nFId = cmd_arg[0];
           if( convert_mac_adr_str((char*)cmd_arg[1], &x.MAC_tableRemove.nMAC[0]) != IFX_SUCCESS)
               return -1;
           ioctl_params = (void *)&x.MAC_tableRemove;
           break;
       case IFX_ETHSW_MAC_TABLE_CLEAR:
               break;
       case IFX_ETHSW_RMON_CLEAR:
           memset(&x.RMON_clear, 0x00, sizeof(x.RMON_clear));
           x.RMON_clear.nPortId = cmd_arg[0];
           ioctl_params = (void *)&x.RMON_clear;
           break;
       case IFX_ETHSW_RMON_GET:
           memset(&x.RMON_cnt, 0x00, sizeof(x.RMON_cnt));
           x.RMON_cnt.nPortId = cmd_arg[0];
           ioctl_params = (void *)&x.RMON_cnt;
           break;
       case IFX_ETHSW_PORT_REDIRECT_GET:
           memset(&x.portRedirectData, 0x00, sizeof(x.portRedirectData));
           x.portRedirectData.nPortId = cmd_arg[0];
           ioctl_params = (void *)&x.portRedirectData;
           break;
       case IFX_ETHSW_PORT_REDIRECT_SET:
           memset(&x.portRedirectData, 0x00, sizeof(x.portRedirectData));
           x.portRedirectData.nPortId = cmd_arg[0];
           x.portRedirectData.bRedirectEgress = cmd_arg[1];
           x.portRedirectData.bRedirectIngress = cmd_arg[2];
           ioctl_params = (void *)&x.portRedirectData;
           break;
       case IFX_ETHSW_MDIO_DATA_READ:
           memset(&x.mdio_Data, 0x00, sizeof(x.mdio_Data));
           x.mdio_Data.nAddressDev = cmd_arg[0];
           x.mdio_Data.nAddressReg = cmd_arg[1];
           ioctl_params = (void *)&x.mdio_Data;
           break;
       case IFX_ETHSW_MDIO_DATA_WRITE:
           memset(&x.mdio_Data, 0x00, sizeof(x.mdio_Data));
           x.mdio_Data.nAddressDev = cmd_arg[0];
           x.mdio_Data.nAddressReg = cmd_arg[1];
           x.mdio_Data.nData = cmd_arg[2];
           ioctl_params = (void *)&x.mdio_Data;
           break;
       case IFX_PSB6970_POWER_MANAGEMENT_GET:
           memset(&x.power_management, 0x00, sizeof(x.power_management));
           ioctl_params = (void *)&x.power_management;
           break;
       case IFX_PSB6970_POWER_MANAGEMENT_SET:
           memset(&x.power_management, 0x00, sizeof(x.power_management));
           x.power_management.bEnable = cmd_arg[0];
           ioctl_params = (void *)&x.power_management;
           break;
       case IFX_ETHSW_VLAN_ID_CREATE:
           memset(&x.vlan_IdCreate, 0x00, sizeof(x.vlan_IdCreate));
           x.vlan_IdCreate.nVId = cmd_arg[0];
           x.vlan_IdCreate.nFId = cmd_arg[1];
           ioctl_params = (void *)&x.vlan_IdCreate;
           break;
       case IFX_ETHSW_VLAN_ID_DELETE:
           memset(&x.vlan_IdDelete, 0x00, sizeof(x.vlan_IdDelete));
           x.vlan_IdDelete.nVId = cmd_arg[0];
           ioctl_params = (void *)&x.vlan_IdDelete;
           break;
       case IFX_ETHSW_VLAN_ID_GET:
           memset(&x.vlan_IdGet, 0x00, sizeof(x.vlan_IdGet));
           x.vlan_IdGet.nVId = cmd_arg[0];
           ioctl_params = (void *)&x.vlan_IdGet;
           break;
       case IFX_ETHSW_VLAN_PORT_MEMBER_ADD:
           memset(&x.vlan_portMemberAdd, 0x00, sizeof(x.vlan_portMemberAdd));
           x.vlan_portMemberAdd.nVId = cmd_arg[0];
           x.vlan_portMemberAdd.nPortId = cmd_arg[1];
           x.vlan_portMemberAdd.bVLAN_TagEgress = cmd_arg[2];
           ioctl_params = (void *)&x.vlan_portMemberAdd;
           break;
       case IFX_ETHSW_VLAN_PORT_MEMBER_REMOVE:
           memset(&x.vlan_portMemberRemove, 0x00, sizeof(x.vlan_portMemberRemove));
           x.vlan_portMemberRemove.nVId = cmd_arg[0];
           x.vlan_portMemberRemove.nPortId = cmd_arg[1];
           ioctl_params = (void *)&x.vlan_portMemberRemove;
           break;
       case IFX_ETHSW_VLAN_PORT_CFG_GET:
           memset(&x.vlan_portcfg, 0x00, sizeof(x.vlan_portcfg));
           x.vlan_portcfg.nPortId = cmd_arg[0];
           ioctl_params = (void *)&x.vlan_portcfg;
           break;
       case IFX_ETHSW_VLAN_PORT_CFG_SET:
           memset(&x.vlan_portcfg, 0x00, sizeof(x.vlan_portcfg));
           x.vlan_portcfg.nPortId = cmd_arg[0];
           x.vlan_portcfg.nPortVId = cmd_arg[1];
           x.vlan_portcfg.bVLAN_UnknownDrop = cmd_arg[2];
           x.vlan_portcfg.bVLAN_ReAssign = cmd_arg[3];
           x.vlan_portcfg.eVLAN_MemberViolation = cmd_arg[4];
           x.vlan_portcfg.eAdmitMode = cmd_arg[5];
           x.vlan_portcfg.bTVM = cmd_arg[6];
           ioctl_params = (void *)&x.vlan_portcfg;
           break;
       case IFX_ETHSW_VLAN_RESERVED_ADD:
           memset(&x.vlan_Reserved, 0x00, sizeof(x.vlan_Reserved));
           x.vlan_Reserved.nVId = cmd_arg[0];
           ioctl_params = (void *)&x.vlan_Reserved;
           break;
       case IFX_ETHSW_VLAN_RESERVED_REMOVE:
           memset(&x.vlan_Reserved, 0x00, sizeof(x.vlan_Reserved));
           x.vlan_Reserved.nVId = cmd_arg[0];
           ioctl_params = (void *)&x.vlan_Reserved;
           break;
       case IFX_ETHSW_MULTICAST_ROUTER_PORT_ADD:
           memset(&x.multicast_RouterPortAdd, 0x00, sizeof(x.multicast_RouterPortAdd));
           x.multicast_RouterPortAdd.nPortId = cmd_arg[0];
          ioctl_params = (void *)&x.multicast_RouterPortAdd;
          break;
       case IFX_ETHSW_MULTICAST_ROUTER_PORT_REMOVE:
           memset(&x.multicast_RouterPortRemove, 0x00, sizeof(x.multicast_RouterPortRemove));
           x.multicast_RouterPortRemove.nPortId = cmd_arg[0];
          ioctl_params = (void *)&x.multicast_RouterPortRemove;
          break;
       case IFX_ETHSW_MULTICAST_ROUTER_PORT_READ:
           memset(&x.multicast_RouterPortRead, 0x00, sizeof(x.multicast_RouterPortRead));
           x.multicast_RouterPortRead.bInitial = cmd_arg[0];
           x.multicast_RouterPortRead.nPortId = cmd_arg[1];
          ioctl_params = (void *)&x.multicast_RouterPortRead;
          break;
       case IFX_ETHSW_MULTICAST_TABLE_ENTRY_ADD:
#if  defined(AR9) || defined(DANUBE) || defined(AMAZON_SE)
            memset(&x.multicast_TableEntryAdd, 0x00, sizeof(x.multicast_TableEntryAdd));
            x.multicast_TableEntryAdd.nPortId = cmd_arg[0];
            x.multicast_TableEntryAdd.eIPVersion = cmd_arg[1];
            /* Need to convert the GDA IP addr */
            if ( x.multicast_TableEntryAdd.eIPVersion == IFX_ETHSW_IP_SELECT_IPV4 )
            {
                if( convert_ipv4_str((char*)cmd_arg[2], &x.multicast_TableEntryAdd.uIP_Gda.nIPv4) != IFX_SUCCESS)
                return -1;
            }
            else if ( x.multicast_TableEntryAdd.eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 )
            {
                if( convert_ipv6_str((char*)cmd_arg[2], &x.multicast_TableEntryAdd.uIP_Gda.nIPv6) != IFX_SUCCESS)
                return -1;
            }
            else
            {
                printf ("The input IP Version is not accept.\n");
            }

            /* Need to convert the GSA IP addr */
            if ( x.multicast_TableEntryAdd.eIPVersion == IFX_ETHSW_IP_SELECT_IPV4 )
            {
                if( convert_ipv4_str((char*)cmd_arg[3], &x.multicast_TableEntryAdd.uIP_Gsa.nIPv4) != IFX_SUCCESS)
                  return -1;
            }
            else if ( x.multicast_TableEntryAdd.eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 )
            {
                if( convert_ipv6_str((char*)cmd_arg[3], &x.multicast_TableEntryAdd.uIP_Gsa.nIPv6) != IFX_SUCCESS)
                  return -1;
            }
            else
            {
                printf ("The input IP Version is not accept.\n");
            }
            x.multicast_TableEntryAdd.eModeMember =  cmd_arg[4];
            ioctl_params = (void *)&x.multicast_TableEntryAdd;
#elif defined(VR9)
           memset(&x.multicast_TableEntryAdd, 0x00, sizeof(x.multicast_TableEntryAdd));
           x.multicast_TableEntryAdd.nPortId = cmd_arg[0];
           x.multicast_TableEntryAdd.eIPVersion = cmd_arg[1];
           /* Need to convert the GDA IP addr */
           if ( x.multicast_TableEntryAdd.eIPVersion == IFX_ETHSW_IP_SELECT_IPV4 )
           {
               if( convert_ipv4_str((char*)cmd_arg[2], &x.multicast_TableEntryAdd.uIP_Gda.nIPv4) != IFX_SUCCESS)
                  return -1;
           }
           else if ( x.multicast_TableEntryAdd.eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 )
           {
//           	printf ("The input IP Version 6 is  accepted GDA.\n");
           	 if( convert_ipv6_str((char*)cmd_arg[2], &x.multicast_TableEntryAdd.uIP_Gda.nIPv6) != IFX_SUCCESS)
                return -1;
/*               printf(" %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x |", x.multicast_TableEntryAdd.uIP_Gda.nIPv6[0],	\
               x.multicast_TableEntryAdd.uIP_Gda.nIPv6[1], x.multicast_TableEntryAdd.uIP_Gda.nIPv6[2],	\
               x.multicast_TableEntryAdd.uIP_Gda.nIPv6[3], x.multicast_TableEntryAdd.uIP_Gda.nIPv6[4],
               x.multicast_TableEntryAdd.uIP_Gda.nIPv6[5], x.multicast_TableEntryAdd.uIP_Gda.nIPv6[6],	\
               x.multicast_TableEntryAdd.uIP_Gda.nIPv6[7] ); */
   
           }
           else
               printf ("The input IP Version is not accept.\n");

           /* Need to convert the GSA IP addr */
           if ( x.multicast_TableEntryAdd.eIPVersion == IFX_ETHSW_IP_SELECT_IPV4 )
           {
              if( convert_ipv4_str((char*)cmd_arg[3], &x.multicast_TableEntryAdd.uIP_Gsa.nIPv4)
                 != IFX_SUCCESS)
                  return -1;
           }
           else if ( x.multicast_TableEntryAdd.eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 )
           {
//           	printf ("The input IP Version 6 is  accepted SDA.\n");
           	if( convert_ipv6_str((char*)cmd_arg[3], &x.multicast_TableEntryAdd.uIP_Gsa.nIPv6) != IFX_SUCCESS)
                  return -1;
/*            printf(" %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x |", x.multicast_TableEntryAdd.uIP_Gsa.nIPv6[0],	\
                x.multicast_TableEntryAdd.uIP_Gsa.nIPv6[1], x.multicast_TableEntryAdd.uIP_Gsa.nIPv6[2],	\
                x.multicast_TableEntryAdd.uIP_Gsa.nIPv6[3], x.multicast_TableEntryAdd.uIP_Gsa.nIPv6[4],
                x.multicast_TableEntryAdd.uIP_Gsa.nIPv6[5], x.multicast_TableEntryAdd.uIP_Gsa.nIPv6[6],	\
                 x.multicast_TableEntryAdd.uIP_Gsa.nIPv6[7] ); */
           }
           else
             printf ("The input IP Version is not accept.\n");
           x.multicast_TableEntryAdd.eModeMember =  cmd_arg[4];
          ioctl_params = (void *)&x.multicast_TableEntryAdd;
#endif
          break;
       case IFX_ETHSW_MULTICAST_TABLE_ENTRY_REMOVE:
#if  defined(AR9) || defined(DANUBE) || defined(AMAZON_SE)
            memset(&x.multicast_TableEntryRemove, 0x00, sizeof(x.multicast_TableEntryRemove));
            x.multicast_TableEntryRemove.nPortId = cmd_arg[0];
            x.multicast_TableEntryRemove.eIPVersion = cmd_arg[1];
            /* Need to convert the GDA IP addr */
            if ( x.multicast_TableEntryRemove.eIPVersion == IFX_ETHSW_IP_SELECT_IPV4 )
            {
                if( convert_ipv4_str((char*)cmd_arg[2], &x.multicast_TableEntryRemove.uIP_Gda.nIPv4) != IFX_SUCCESS)
                return -1;
            }
            else if ( x.multicast_TableEntryRemove.eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 )
            {
                if( convert_ipv6_str((char*)cmd_arg[2], &x.multicast_TableEntryRemove.uIP_Gda.nIPv6) != IFX_SUCCESS)
                return -1;
            }
            else
            {
                printf ("The input IP Version is not accept.\n");
            }
            /* Need to convert the GSA IP addr */
            if ( x.multicast_TableEntryRemove.eIPVersion == IFX_ETHSW_IP_SELECT_IPV4 )
            {
                if( convert_ipv4_str((char*)cmd_arg[3], &x.multicast_TableEntryRemove.uIP_Gsa.nIPv4) != IFX_SUCCESS)
                return -1;
            }
            else if ( x.multicast_TableEntryRemove.eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 )
            {
                if( convert_ipv6_str((char*)cmd_arg[3], &x.multicast_TableEntryRemove.uIP_Gsa.nIPv6) != IFX_SUCCESS)
                return -1;
            }
            else
            {
                printf ("The input IP Version is not accept.\n");
            }
            x.multicast_TableEntryRemove.eModeMember = cmd_arg[4];
            ioctl_params = (void *)&x.multicast_TableEntryRemove;
#elif defined(VR9)
           memset(&x.multicast_TableEntryRemove, 0x00, sizeof(x.multicast_TableEntryRemove));
           x.multicast_TableEntryRemove.nPortId = cmd_arg[0];
           x.multicast_TableEntryRemove.eIPVersion = cmd_arg[1];
           /* Need to convert the GDA IP addr */
           if ( x.multicast_TableEntryRemove.eIPVersion == IFX_ETHSW_IP_SELECT_IPV4 ) {
              if( convert_ipv4_str((char*)cmd_arg[2], &x.multicast_TableEntryRemove.uIP_Gda.nIPv4) != IFX_SUCCESS)
                  return -1;
           } else if ( x.multicast_TableEntryRemove.eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 ){
                if( convert_ipv6_str((char*)cmd_arg[2], &x.multicast_TableEntryAdd.uIP_Gda.nIPv6) != IFX_SUCCESS)
                return -1;
/*              printf(" %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x |", x.multicast_TableEntryAdd.uIP_Gda.nIPv6[0],	\
               x.multicast_TableEntryAdd.uIP_Gda.nIPv6[1], x.multicast_TableEntryAdd.uIP_Gda.nIPv6[2],	\
               x.multicast_TableEntryAdd.uIP_Gda.nIPv6[3], x.multicast_TableEntryAdd.uIP_Gda.nIPv6[4],
               x.multicast_TableEntryAdd.uIP_Gda.nIPv6[5], x.multicast_TableEntryAdd.uIP_Gda.nIPv6[6],	\
               x.multicast_TableEntryAdd.uIP_Gda.nIPv6[7] );  	*/
           } else {
            printf ("The input IP Version is not accept.\n");
           }
           /* Need to convert the GSA IP addr */
           if ( x.multicast_TableEntryRemove.eIPVersion == IFX_ETHSW_IP_SELECT_IPV4 ) {
              if( convert_ipv4_str((char*)cmd_arg[3], &x.multicast_TableEntryRemove.uIP_Gsa.nIPv4) != IFX_SUCCESS)
                  return -1;
           } else if ( x.multicast_TableEntryRemove.eIPVersion == IFX_ETHSW_IP_SELECT_IPV6 ){

              if( convert_ipv6_str((char*)cmd_arg[3], &x.multicast_TableEntryAdd.uIP_Gsa.nIPv6) != IFX_SUCCESS)
                  return -1;
/*            printf(" %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x |", x.multicast_TableEntryAdd.uIP_Gsa.nIPv6[0],	\
                x.multicast_TableEntryAdd.uIP_Gsa.nIPv6[1], x.multicast_TableEntryAdd.uIP_Gsa.nIPv6[2],	\
                x.multicast_TableEntryAdd.uIP_Gsa.nIPv6[3], x.multicast_TableEntryAdd.uIP_Gsa.nIPv6[4],
                x.multicast_TableEntryAdd.uIP_Gsa.nIPv6[5], x.multicast_TableEntryAdd.uIP_Gsa.nIPv6[6],	\
                 x.multicast_TableEntryAdd.uIP_Gsa.nIPv6[7] );  */
           } else {
            printf ("The input IP Version is not accept.\n");
           }
#if 0
           printf ("|GDA:  %02d.%02d.%02d.%02d  |GSA:  %02d.%02d.%02d.%02d  | \n",
             //(x.multicast_TableEntryRemove.uIP_Gda.nIPv4 >> 24) & 0xFF,
             //(x.multicast_TableEntryRemove.uIP_Gda.nIPv4 >> 16) & 0xFF,
             //(x.multicast_TableEntryRemove.uIP_Gda.nIPv4 >> 8) & 0xFF,
             //x.multicast_TableEntryRemove.uIP_Gda.nIPv4 & 0xFF,
             //(x.multicast_TableEntryRemove.uIP_Gsa.nIPv4 >> 24) & 0xFF,
             //(x.multicast_TableEntryRemove.uIP_Gsa.nIPv4 >> 16) & 0xFF,
             //(x.multicast_TableEntryRemove.uIP_Gsa.nIPv4 >> 8) & 0xFF,
             //x.multicast_TableEntryRemove.uIP_Gsa.nIPv4 & 0xFF);
               x.multicast_TableEntryAdd.uIP_Gda.nIPv4 & 0xFF,
              (x.multicast_TableEntryAdd.uIP_Gda.nIPv4 >> 8) & 0xFF,
              (x.multicast_TableEntryAdd.uIP_Gda.nIPv4 >> 16) & 0xFF,
              (x.multicast_TableEntryAdd.uIP_Gda.nIPv4 >> 24) & 0xFF,
               x.multicast_TableEntryAdd.uIP_Gsa.nIPv4 & 0xFF,
              (x.multicast_TableEntryAdd.uIP_Gsa.nIPv4 >> 8) & 0xFF,
              (x.multicast_TableEntryAdd.uIP_Gsa.nIPv4 >> 16) & 0xFF,
              (x.multicast_TableEntryAdd.uIP_Gsa.nIPv4 >> 24) & 0xFF);
#endif
           x.multicast_TableEntryRemove.eModeMember = cmd_arg[4];
          ioctl_params = (void *)&x.multicast_TableEntryRemove;
#endif
          break;
       case IFX_ETHSW_MULTICAST_TABLE_ENTRY_READ:
#if  defined(AR9) || defined(DANUBE) || defined(AMAZON_SE) || defined(VR9)
            print_igmp_membership_table();
#elif defined(VR9_XX)
           memset(&x.multicast_TableEntryRead, 0x00, sizeof(x.multicast_TableEntryRead));
           x.multicast_TableEntryRead.bInitial = 0;
           x.multicast_TableEntryRead.bLast = 0;
           x.multicast_TableEntryRead.nPortId = cmd_arg[0];
          ioctl_params = (void *)&x.multicast_TableEntryRead;
#endif
          break;
       case IFX_ETHSW_MULTICAST_SNOOP_CFG_GET:
           memset(&x.multicast_SnoopCfgGet, 0x00, sizeof(x.multicast_SnoopCfgGet));
          ioctl_params = (void *)&x.multicast_SnoopCfgGet;
          break;
       case IFX_ETHSW_MULTICAST_SNOOP_CFG_SET:
           memset(&x.multicast_SnoopCfgSet, 0x00, sizeof(x.multicast_SnoopCfgSet));
           x.multicast_SnoopCfgSet.eIGMP_Mode = cmd_arg[0];
           x.multicast_SnoopCfgSet.bIGMPv3 = cmd_arg[1];
           x.multicast_SnoopCfgSet.bCrossVLAN = cmd_arg[2];
           x.multicast_SnoopCfgSet.eForwardPort = cmd_arg[3];
           x.multicast_SnoopCfgSet.nForwardPortId = cmd_arg[4];
           x.multicast_SnoopCfgSet.nClassOfService = cmd_arg[5];
           x.multicast_SnoopCfgSet.nRobust = cmd_arg[6];
           x.multicast_SnoopCfgSet.nQueryInterval = cmd_arg[7];
           x.multicast_SnoopCfgSet.eSuppressionAggregation = cmd_arg[8];
           x.multicast_SnoopCfgSet.bFastLeave = cmd_arg[9];
           x.multicast_SnoopCfgSet.bLearningRouter = cmd_arg[10];
          ioctl_params = (void *)&x.multicast_SnoopCfgSet;
          break;
       case IFX_ETHSW_PORT_RGMII_CLK_CFG_GET:
               memset(&x.portRGMII_clkcfg, 0x00, sizeof(x.portRGMII_clkcfg));
               x.portRGMII_clkcfg.nPortId = cmd_arg[0];
               ioctl_params = (void *)&x.portRGMII_clkcfg;
               break;
       case IFX_ETHSW_PORT_RGMII_CLK_CFG_SET:
               memset(&x.portRGMII_clkcfg, 0x00, sizeof(x.portRGMII_clkcfg));
               x.portRGMII_clkcfg.nPortId = cmd_arg[0];
               x.portRGMII_clkcfg.nDelayRx = cmd_arg[1];
               x.portRGMII_clkcfg.nDelayTx = cmd_arg[2];
               ioctl_params = (void *)&x.portRGMII_clkcfg;
               break;
       case IFX_ETHSW_MONITOR_PORT_CFG_GET:
               memset(&x.monitorPortCfg, 0x00, sizeof(x.monitorPortCfg));
               x.monitorPortCfg.nPortId = cmd_arg[0];
               ioctl_params = (void *)&x.monitorPortCfg;
               break;
       case IFX_ETHSW_MONITOR_PORT_CFG_SET:
               memset(&x.monitorPortCfg, 0x00, sizeof(x.monitorPortCfg));
               x.monitorPortCfg.nPortId = cmd_arg[0];
               x.monitorPortCfg.bMonitorPort = cmd_arg[1];
               ioctl_params = (void *)&x.monitorPortCfg;
               break;
       case IFX_ETHSW_CPU_PORT_CFG_GET:
               memset(&x.CPU_PortCfg, 0x00, sizeof(x.CPU_PortCfg));
               x.CPU_PortCfg.nPortId = cmd_arg[0];
               ioctl_params = (void *)&x.CPU_PortCfg;
               break;
       case IFX_ETHSW_CPU_PORT_CFG_SET:
               memset(&x.CPU_PortCfg, 0x00, sizeof(x.CPU_PortCfg));
               x.CPU_PortCfg.nPortId = cmd_arg[0];
               x.CPU_PortCfg.bFcsCheck = cmd_arg[1];
               x.CPU_PortCfg.bFcsGenerate =  cmd_arg[2];
               x.CPU_PortCfg.bSpecialTagEgress = cmd_arg[3];
               x.CPU_PortCfg.bSpecialTagIngress = cmd_arg[4];
               x.CPU_PortCfg.bCPU_PortValid = cmd_arg[5];
               ioctl_params = (void *)&x.CPU_PortCfg;
               break;
       case IFX_ETHSW_QOS_PORT_CFG_GET:
               memset(&x.qos_portcfg, 0x00, sizeof(x.qos_portcfg));
               x.qos_portcfg.nPortId = cmd_arg[0];
               ioctl_params = (void *)&x.qos_portcfg;
               break;
       case IFX_ETHSW_QOS_PORT_CFG_SET:
               memset(&x.qos_portcfg, 0x00, sizeof(x.qos_portcfg));
               x.qos_portcfg.nPortId = cmd_arg[0];
               x.qos_portcfg.eClassMode = cmd_arg[1];
               x.qos_portcfg.nTrafficClass = cmd_arg[2];
               ioctl_params = (void *)&x.qos_portcfg;
               break;
       case IFX_ETHSW_PORT_PHY_ADDR_GET:
               memset(&x.phy_addr, 0x00, sizeof(x.phy_addr));
               x.phy_addr.nPortId = cmd_arg[0];
               ioctl_params = (void *)&x.phy_addr;
               break;
#if defined(AR9) || defined(DANUBE) || defined(AMAZON_SE)
       case IFX_PSB6970_REGISTER_GET:
           memset(&x.register_access, 0x00, sizeof(x.register_access));
           x.register_access.nRegAddr = cmd_arg[0];
          ioctl_params = (void *)&x.register_access;
           break;
       case IFX_PSB6970_REGISTER_SET:
           memset(&x.register_access, 0x00, sizeof(x.register_access));
           x.register_access.nRegAddr = cmd_arg[0];
           x.register_access.nData = cmd_arg[1];
          ioctl_params = (void *)&x.register_access;
           break;
       case IFX_ETHSW_QOS_DSCP_CLASS_GET:
               memset(&x.qos_dscpclasscfgget, 0x00, sizeof(x.qos_dscpclasscfgget));
               ioctl_params = (void *)&x.qos_dscpclasscfgget;
               break;
       case IFX_ETHSW_QOS_DSCP_CLASS_SET:
               memset(&x.qos_dscpclasscfgset, 0x00, sizeof(x.qos_dscpclasscfgset));
               index = cmd_arg[0];
               value = cmd_arg[1];
               if ( index >= 64 )
               {
                    printf ("The input index is not accept.\n");
                    return IFX_ERROR;
               }
               if (ioctl(fd, IFX_ETHSW_QOS_DSCP_CLASS_GET, (void *)&x.qos_dscpclasscfgset) != IFX_SUCCESS)
                  return IFX_ERROR;
               x.qos_dscpclasscfgset.nTrafficClass[index] = value;
               ioctl_params = (void *)&x.qos_dscpclasscfgset;
               break;
       case IFX_ETHSW_QOS_PCP_CLASS_GET:
               memset(&x.qos_pcpclasscfgget, 0x00, sizeof(x.qos_pcpclasscfgget));
               ioctl_params = (void *)&x.qos_pcpclasscfgget;
               break;
       case IFX_ETHSW_QOS_PCP_CLASS_SET:
               memset(&x.qos_pcpclasscfgset, 0x00, sizeof(x.qos_pcpclasscfgset));
               index = cmd_arg[0];
               value = cmd_arg[1];
               if ( index >= 8 )
               {
                    printf ("The input index is not accept.\n");
                    return IFX_ERROR;
               }
               if (ioctl(fd, IFX_ETHSW_QOS_PCP_CLASS_GET, (void *)&x.qos_pcpclasscfgset) != IFX_SUCCESS)
                  return IFX_ERROR;
               x.qos_pcpclasscfgset.nTrafficClass[index] = value;
               ioctl_params = (void *)&x.qos_pcpclasscfgset;
               break;
       case IFX_ETHSW_VERSION_GET:
               memset(&x.Version, 0x00, sizeof(x.Version));
               x.Version.nId = cmd_arg[0];
               ioctl_params = (void *)&x.Version;
               break;
       case IFX_ETHSW_CAP_GET:
               memset(&x.cap, 0x00, sizeof(x.cap));
               x.cap.nCapType = cmd_arg[0];
               ioctl_params = (void *)&x.cap;
               break;
       case IFX_ETHSW_PORT_PHY_QUERY:
               memset(&x.phy_Query, 0x00, sizeof(x.phy_Query));
               x.phy_Query.nPortId = cmd_arg[0];
               ioctl_params = (void *)&x.phy_Query;
               break;
       case IFX_ETHSW_STP_PORT_CFG_GET:
               memset(&x.STP_portCfg, 0x00, sizeof(x.STP_portCfg));
               x.STP_portCfg.nPortId = cmd_arg[0];
               ioctl_params = (void *)&x.STP_portCfg;
               break;
       case IFX_ETHSW_STP_PORT_CFG_SET:
               memset(&x.STP_portCfg, 0x00, sizeof(x.STP_portCfg));
               x.STP_portCfg.nPortId = cmd_arg[0];
               x.STP_portCfg.ePortState = cmd_arg[1];
               ioctl_params = (void *)&x.STP_portCfg;
               break;
       case IFX_ETHSW_STP_BPDU_RULE_GET:
               memset(&x.STP_BPDU_Rule, 0x00, sizeof(x.STP_BPDU_Rule));
               ioctl_params = (void *)&x.STP_BPDU_Rule;
               break;
       case IFX_ETHSW_STP_BPDU_RULE_SET:
               memset(&x.STP_BPDU_Rule, 0x00, sizeof(x.STP_BPDU_Rule));
               x.STP_BPDU_Rule.eForwardPort = cmd_arg[0];
               x.STP_BPDU_Rule.nForwardPortId = cmd_arg[1];
               ioctl_params = (void *)&x.STP_BPDU_Rule;
               break;
#if 0
       case IFX_PSB6970_QOS_PORT_SHAPER_GET:
               memset(&x.qos_portShapterCfg, 0x00, sizeof(x.qos_portShapterCfg));
               x.qos_portShapterCfg.nPort = cmd_arg[0];
               x.qos_portShapterCfg.nTrafficClass = cmd_arg[1];
               x.qos_portShapterCfg.eType = cmd_arg[2];
               ioctl_params = (void *)&x.qos_portShapterCfg;
               break;
       case IFX_PSB6970_QOS_PORT_SHAPER_SET:
               memset(&x.qos_portShapterCfg, 0x00, sizeof(x.qos_portShapterCfg));
               x.qos_portShapterCfg.nPort = cmd_arg[0];
               x.qos_portShapterCfg.nTrafficClass = cmd_arg[1];
               x.qos_portShapterCfg.eType = cmd_arg[2];
               x.qos_portShapterCfg.nRate = cmd_arg[3];
               ioctl_params = (void *)&x.qos_portShapterCfg;
               break;
#else
       case IFX_PSB6970_QOS_PORT_SHAPER_CFG_GET:
               memset(&x.qos_portShapterCfg, 0x00, sizeof(x.qos_portShapterCfg));
               x.qos_portShapterCfg.nPort = cmd_arg[0];
               ioctl_params = (void *)&x.qos_portShapterCfg;
               break;
       case IFX_PSB6970_QOS_PORT_SHAPER_CFG_SET:
               memset(&x.qos_portShapterCfg, 0x00, sizeof(x.qos_portShapterCfg));
               x.qos_portShapterCfg.nPort = cmd_arg[0];
               x.qos_portShapterCfg.eWFQ_Type = cmd_arg[1];
               ioctl_params = (void *)&x.qos_portShapterCfg;
               break;
       case IFX_PSB6970_QOS_PORT_SHAPER_STRICT_GET:
               memset(&x.qos_portShapterStrictCfg, 0x00, sizeof(x.qos_portShapterStrictCfg));
               x.qos_portShapterStrictCfg.nPort = cmd_arg[0];
               x.qos_portShapterStrictCfg.nTrafficClass = cmd_arg[1];
               ioctl_params = (void *)&x.qos_portShapterStrictCfg;
               break;
       case IFX_PSB6970_QOS_PORT_SHAPER_STRICT_SET:
               memset(&x.qos_portShapterStrictCfg, 0x00, sizeof(x.qos_portShapterStrictCfg));
               x.qos_portShapterStrictCfg.nPort = cmd_arg[0];
               x.qos_portShapterStrictCfg.nTrafficClass = cmd_arg[1];
               x.qos_portShapterStrictCfg.nRate = cmd_arg[2];
               ioctl_params = (void *)&x.qos_portShapterStrictCfg;
               break;
       case IFX_PSB6970_QOS_PORT_SHAPER_WFQ_GET:
               memset(&x.qos_portShapterWFQ_Cfg, 0x00, sizeof(x.qos_portShapterWFQ_Cfg));
               x.qos_portShapterWFQ_Cfg.nPort = cmd_arg[0];
               x.qos_portShapterWFQ_Cfg.nTrafficClass = cmd_arg[1];
               ioctl_params = (void *)&x.qos_portShapterWFQ_Cfg;
               break;
       case IFX_PSB6970_QOS_PORT_SHAPER_WFQ_SET:
               memset(&x.qos_portShapterWFQ_Cfg, 0x00, sizeof(x.qos_portShapterWFQ_Cfg));
               x.qos_portShapterWFQ_Cfg.nPort = cmd_arg[0];
               x.qos_portShapterWFQ_Cfg.nTrafficClass = cmd_arg[1];
               x.qos_portShapterWFQ_Cfg.nRate = cmd_arg[2];
               ioctl_params = (void *)&x.qos_portShapterWFQ_Cfg;
               break;
#endif
       case IFX_PSB6970_QOS_PORT_POLICER_GET:
               memset(&x.qos_portPolicerCfg, 0x00, sizeof(x.qos_portPolicerCfg));
               x.qos_portPolicerCfg.nPort = cmd_arg[0];
               ioctl_params = (void *)&x.qos_portPolicerCfg;
               break;
       case IFX_PSB6970_QOS_PORT_POLICER_SET:
               memset(&x.qos_portPolicerCfg, 0x00, sizeof(x.qos_portPolicerCfg));
               x.qos_portPolicerCfg.nPort = cmd_arg[0];
               x.qos_portPolicerCfg.nRate = cmd_arg[1];
               ioctl_params = (void *)&x.qos_portPolicerCfg;
               break;
       case IFX_ETHSW_MDIO_CFG_GET:
               memset(&x.mdio_cfg, 0x00, sizeof(x.mdio_cfg));
               ioctl_params = (void *)&x.mdio_cfg;
               break;
       case IFX_ETHSW_MDIO_CFG_SET:
               memset(&x.mdio_cfg, 0x00, sizeof(x.mdio_cfg));
               x.mdio_cfg.bMDIO_Enable = cmd_arg[0];
               x.mdio_cfg.nMDIO_Speed = cmd_arg[1];
               ioctl_params = (void *)&x.mdio_cfg;
               break;
       case IFX_ETHSW_8021X_PORT_CFG_GET:
               memset(&x.PNAC_portCfg, 0x00, sizeof(x.PNAC_portCfg));
               x.PNAC_portCfg.nPortId = cmd_arg[0];
               ioctl_params = (void *)&x.PNAC_portCfg;
               break;
       case IFX_ETHSW_8021X_PORT_CFG_SET:
               memset(&x.PNAC_portCfg, 0x00, sizeof(x.PNAC_portCfg));
               x.PNAC_portCfg.nPortId = cmd_arg[0];
               x.PNAC_portCfg.eState = cmd_arg[1];
               ioctl_params = (void *)&x.PNAC_portCfg;
               break;
       case IFX_ETHSW_8021X_EAPOL_RULE_GET:
               memset(&x.PNAC_EAPOL_Rule, 0x00, sizeof(x.PNAC_EAPOL_Rule));
               ioctl_params = (void *)&x.PNAC_EAPOL_Rule;
               break;
       case IFX_ETHSW_8021X_EAPOL_RULE_SET:
               memset(&x.PNAC_EAPOL_Rule, 0x00, sizeof(x.PNAC_EAPOL_Rule));
               x.PNAC_EAPOL_Rule.eForwardPort = cmd_arg[0];
               ioctl_params = (void *)&x.PNAC_EAPOL_Rule;
               break;
       case IFX_PSB6970_QOS_STORM_GET:
               memset(&x.qos_stormCfg, 0x00, sizeof(x.qos_stormCfg));
               ioctl_params = (void *)&x.qos_stormCfg;
               break;
       case IFX_PSB6970_QOS_STORM_SET:
               memset(&x.qos_stormCfg, 0x00, sizeof(x.qos_stormCfg));
               x.qos_stormCfg.bBroadcast = cmd_arg[0];
               x.qos_stormCfg.bMulticast = cmd_arg[1];
               x.qos_stormCfg.bUnicast = cmd_arg[2];
               x.qos_stormCfg.nThreshold10M = cmd_arg[3];
               x.qos_stormCfg.nThreshold100M = cmd_arg[4];
               ioctl_params = (void *)&x.qos_stormCfg;
               break;
       case IFX_PSB6970_QOS_MFC_PORT_CFG_GET:
               memset(&x.qos_MfcPortCfg, 0x00, sizeof(x.qos_MfcPortCfg));
               x.qos_MfcPortCfg.nPort = cmd_arg[0];
               ioctl_params = (void *)&x.qos_MfcPortCfg;
               break;
       case IFX_PSB6970_QOS_MFC_PORT_CFG_SET:
               memset(&x.qos_MfcPortCfg, 0x00, sizeof(x.qos_MfcPortCfg));
               x.qos_MfcPortCfg.nPort = cmd_arg[0];
               x.qos_MfcPortCfg.bPriorityPort = cmd_arg[1];
               x.qos_MfcPortCfg.bPriorityEtherType = cmd_arg[2];
               ioctl_params = (void *)&x.qos_MfcPortCfg;
               break;
       case IFX_PSB6970_QOS_MFC_ADD:
               memset(&x.qos_MfcCfg, 0x00, sizeof(x.qos_MfcCfg));
               x.qos_MfcCfg.sFilterMatchField.eFieldSelection = cmd_arg[0];
               x.qos_MfcCfg.sFilterMatchField.nPortSrc = cmd_arg[1];
               x.qos_MfcCfg.sFilterMatchField.nPortDst = cmd_arg[2];
               x.qos_MfcCfg.sFilterMatchField.nPortSrcRange = cmd_arg[3];
               x.qos_MfcCfg.sFilterMatchField.nPortDstRange = cmd_arg[4];
               x.qos_MfcCfg.sFilterMatchField.nProtocol = cmd_arg[5];
               x.qos_MfcCfg.sFilterMatchField.nEtherType = cmd_arg[6];
               x.qos_MfcCfg.sFilterInfo.nTrafficClass = cmd_arg[7];
               x.qos_MfcCfg.sFilterInfo.ePortForward = cmd_arg[8];
               ioctl_params = (void *)&x.qos_MfcCfg;
               break;
       case IFX_PSB6970_QOS_MFC_DEL:
               memset(&x.qos_MfcMatchField, 0x00, sizeof(x.qos_MfcMatchField));
               x.qos_MfcMatchField.eFieldSelection = cmd_arg[0];
               x.qos_MfcMatchField.nPortSrc = cmd_arg[1];
               x.qos_MfcMatchField.nPortDst = cmd_arg[2];
               x.qos_MfcMatchField.nPortSrcRange = cmd_arg[3];
               x.qos_MfcMatchField.nPortDstRange = cmd_arg[4];
               x.qos_MfcMatchField.nProtocol = cmd_arg[5];
               x.qos_MfcMatchField.nEtherType = cmd_arg[6];
               ioctl_params = (void *)&x.qos_MfcMatchField;
               break;
       case IFX_PSB6970_QOS_MFC_ENTRY_READ:
               break;
       case IFX_PSB6970_RESET:
               memset(&x.Reset, 0x00, sizeof(x.Reset));
               x.Reset.eReset = cmd_arg[0];
               ioctl_params = (void *)&x.Reset;
               break;
       case IFX_ETHSW_HW_INIT:
               memset(&x.HW_Init, 0x00, sizeof(x.HW_Init));
               x.HW_Init.eInitMode = cmd_arg[0];
               ioctl_params = (void *)&x.HW_Init;
               break;
       case IFX_ETHSW_ENABLE:
               break;
       case IFX_ETHSW_DISABLE:
               break;
#endif
#ifdef VR9
       case IFX_FLOW_REGISTER_GET:
           memset(&x.register_access, 0x00, sizeof(x.register_access));
           x.register_access.nRegAddr = cmd_arg[0];
          ioctl_params = (void *)&x.register_access;
           break;
       case IFX_FLOW_REGISTER_SET:
           memset(&x.register_access, 0x00, sizeof(x.register_access));
           x.register_access.nRegAddr = cmd_arg[0];
           x.register_access.nData = cmd_arg[1];
          ioctl_params = (void *)&x.register_access;
           break;
       case IFX_ETHSW_MDIO_CFG_GET:
           memset(&x.mdio_cfg, 0x00, sizeof(x.mdio_cfg));
           ioctl_params = (void *)&x.mdio_cfg;
           break;
       case IFX_ETHSW_MDIO_CFG_SET:
           memset(&x.mdio_cfg, 0x00, sizeof(x.mdio_cfg));
           x.mdio_cfg.bMDIO_Enable = cmd_arg[0];
           x.mdio_cfg.nMDIO_Speed = cmd_arg[1];
           ioctl_params = (void *)&x.mdio_cfg;
           break;
       case IFX_ETHSW_CPU_PORT_EXTEND_CFG_GET:
           memset(&x.portextendcfg, 0x00, sizeof(x.portextendcfg));
           ioctl_params = (void *)&x.portextendcfg;
           break;
       case IFX_ETHSW_CPU_PORT_EXTEND_CFG_SET:
           memset(&x.portextendcfg, 0x00, sizeof(x.portextendcfg));
           x.portextendcfg.eHeaderAdd = cmd_arg[0];
           x.portextendcfg.bHeaderRemove = cmd_arg[1];
           if( convert_mac_adr_str((char*)cmd_arg[2], &x.portextendcfg.sHeader.nMAC_Src[0]) != IFX_SUCCESS)
               return -1;

           if( convert_mac_adr_str((char*)cmd_arg[3], &x.portextendcfg.sHeader.nMAC_Dst[0]) != IFX_SUCCESS)
               return -1;
           x.portextendcfg.sHeader.nEthertype = cmd_arg[4];
           x.portextendcfg.sHeader.nVLAN_Prio = cmd_arg[5];
           x.portextendcfg.sHeader.nVLAN_CFI = cmd_arg[6];
           x.portextendcfg.sHeader.nVLAN_ID = cmd_arg[7];
           x.portextendcfg.ePauseCtrl = cmd_arg[8];
           x.portextendcfg.bFcsRemove = cmd_arg[9];
           x.portextendcfg.nWAN_Ports = cmd_arg[10];
           ioctl_params = (void *)&x.portextendcfg;
           break;
       case IFX_ETHSW_QOS_DSCP_CLASS_GET:
           memset(&x.qos_dscpclasscfgget, 0x00, sizeof(x.qos_dscpclasscfgget));
          ioctl_params = (void *)&x.qos_dscpclasscfgget;
           break;
       case IFX_ETHSW_QOS_DSCP_CLASS_SET:
           /* Get the DSCP traffic class table before set */
           if (cmd_arg[1] < 16) {
             memset(&x.qos_dscpclasscfgget, 0x00, sizeof(x.qos_dscpclasscfgget));
             memset(&x.qos_dscpclasscfgset, 0x00, sizeof(x.qos_dscpclasscfgset));
             ioctl_params = (void *)&x.qos_dscpclasscfgget;
             retval=ioctl( fd, IFX_ETHSW_QOS_DSCP_CLASS_GET, ioctl_params );
             if(retval != IFX_SUCCESS)
             {
               printf("IOCTL failed for ioctl command 0x%08X, returned %d\n",
               ioctl_command, retval);
               return -1;
             }
             for (i=0; i <= 63 ; i++){
               x.qos_dscpclasscfgset.nTrafficClass[i] = x.qos_dscpclasscfgget.nTrafficClass[i];
             }
             value = cmd_arg[0];
             x.qos_dscpclasscfgset.nTrafficClass[value] = cmd_arg[1];
             ioctl_params = (void *)&x.qos_dscpclasscfgset;
          }
          else
            printf("The Traffic class value can't not be over 16\n");

           break;
       case IFX_ETHSW_QOS_PCP_CLASS_GET:
           memset(&x.qos_pcpclasscfgget, 0x00, sizeof(x.qos_pcpclasscfgget));
          ioctl_params = (void *)&x.qos_pcpclasscfgget;
           break;
       case IFX_ETHSW_QOS_PCP_CLASS_SET:
           if (cmd_arg[1] < 16) {
             /* Get the PCP traffic class table before set */
             memset(&x.qos_pcpclasscfgget, 0x00, sizeof(x.qos_pcpclasscfgget));
             memset(&x.qos_pcpclasscfgset, 0x00, sizeof(x.qos_pcpclasscfgset));
             ioctl_params = (void *)&x.qos_pcpclasscfgget;
             retval=ioctl( fd, IFX_ETHSW_QOS_PCP_CLASS_GET, ioctl_params );
             if(retval != IFX_SUCCESS)
             {
               printf("IOCTL failed for ioctl command 0x%08X, returned %d\n",
               ioctl_command, retval);
               return -1;
             }
             for (i=0; i <= 7 ; i++){
               x.qos_pcpclasscfgset.nTrafficClass[i] = x.qos_pcpclasscfgget.nTrafficClass[i];
             }
             value = cmd_arg[0];
             x.qos_pcpclasscfgset.nTrafficClass[value] = cmd_arg[1];
             ioctl_params = (void *)&x.qos_pcpclasscfgset;
           }
           else
            printf("The Traffic class value can't not be over 16\n");

           break;
       case IFX_ETHSW_QOS_CLASS_PCP_GET:
               memset(&x.qos_classpcpcfgget, 0x00, sizeof(x.qos_classpcpcfgget));
               ioctl_params = (void *)&x.qos_classpcpcfgget;
               break;
       case IFX_ETHSW_QOS_CLASS_PCP_SET:
               memset(&x.qos_classpcpcfgset, 0x00, sizeof(x.qos_classpcpcfgset));
               index = cmd_arg[0];
               value = cmd_arg[1];
               if ( index >= 16 )
               {
                    printf ("The input index is not accept.\n");
                    return IFX_ERROR;
               }
               if (ioctl(fd, IFX_ETHSW_QOS_CLASS_PCP_GET, (void *)&x.qos_classpcpcfgget) != IFX_SUCCESS)
                  return IFX_ERROR;
               x.qos_classpcpcfgset.nPCP[index] = value;
               ioctl_params = (void *)&x.qos_classpcpcfgset;
               break;
       case IFX_ETHSW_QOS_CLASS_DSCP_GET:
               memset(&x.qos_classdscpcfgget, 0x00, sizeof(x.qos_classdscpcfgget));
               ioctl_params = (void *)&x.qos_classdscpcfgget;
               break;
       case IFX_ETHSW_QOS_CLASS_DSCP_SET:
               memset(&x.qos_classdscpcfgset, 0x00, sizeof(x.qos_classdscpcfgget));
               index = cmd_arg[0];
               value = cmd_arg[1];
               if ( index >= 16 )
               {
                    printf ("The input index is not accept.\n");
                    return IFX_ERROR;
               }
               if (ioctl(fd, IFX_ETHSW_QOS_CLASS_DSCP_GET, (void *)&x.qos_classdscpcfgset) != IFX_SUCCESS)
                  return IFX_ERROR;
               x.qos_classdscpcfgset.nDSCP[index] = value;
               ioctl_params = (void *)&x.qos_classdscpcfgset;
               break;
       case IFX_ETHSW_QOS_PORT_REMARKING_CFG_GET:
               memset(&x.qos_portremarking, 0x00, sizeof(x.qos_portremarking));
               x.qos_portremarking.nPortId = cmd_arg[0];
               ioctl_params = (void *)&x.qos_portremarking;
               break;
       case IFX_ETHSW_QOS_PORT_REMARKING_CFG_SET:
               memset(&x.qos_portremarking, 0x00, sizeof(x.qos_portremarking));
               x.qos_portremarking.nPortId = cmd_arg[0];
               x.qos_portremarking.eDSCP_IngressRemarkingEnable = cmd_arg[1];
               x.qos_portremarking.bDSCP_EgressRemarkingEnable = cmd_arg[2];
               x.qos_portremarking.bPCP_IngressRemarkingEnable = cmd_arg[3];
               x.qos_portremarking.bPCP_EgressRemarkingEnable = cmd_arg[4];
               ioctl_params = (void *)&x.qos_portremarking;
               break;
       case IFX_ETHSW_QOS_QUEUE_PORT_GET:
               memset(&x.qos_queueport, 0x00, sizeof(x.qos_queueport));
               x.qos_queueport.nPortId = cmd_arg[0];
               x.qos_queueport.nTrafficClassId = cmd_arg[1];
               ioctl_params = (void *)&x.qos_queueport;
               break;
       case IFX_ETHSW_QOS_QUEUE_PORT_SET:
               memset(&x.qos_queueport, 0x00, sizeof(x.qos_queueport));
               x.qos_queueport.nQueueId = cmd_arg[0];
               x.qos_queueport.nPortId = cmd_arg[1];
               x.qos_queueport.nTrafficClassId = cmd_arg[2];
               ioctl_params = (void *)&x.qos_queueport;
               break;
       case IFX_ETHSW_QOS_WRED_CFG_GET:
               memset(&x.qos_wredcfg, 0x00, sizeof(x.qos_wredcfg));
               ioctl_params = (void *)&x.qos_wredcfg;
               break;
       case IFX_ETHSW_QOS_WRED_CFG_SET:
               memset(&x.qos_wredcfg, 0x00, sizeof(x.qos_wredcfg));
               x.qos_wredcfg.eProfile = cmd_arg[0];
               x.qos_wredcfg.nRed_Min = cmd_arg[1];
               x.qos_wredcfg.nRed_Max = cmd_arg[2];
               x.qos_wredcfg.nYellow_Min = cmd_arg[3];
               x.qos_wredcfg.nYellow_Max = cmd_arg[4];
               x.qos_wredcfg.nGreen_Min = cmd_arg[5];
               x.qos_wredcfg.nGreen_Max = cmd_arg[6];
               ioctl_params = (void *)&x.qos_wredcfg;
               break;
       case IFX_ETHSW_QOS_WRED_QUEUE_CFG_GET:
               memset(&x.qos_wredqueuecfg, 0x00, sizeof(x.qos_wredqueuecfg));
               x.qos_wredqueuecfg.nQueueId = cmd_arg[0];
               ioctl_params = (void *)&x.qos_wredqueuecfg;
               break;
       case IFX_ETHSW_QOS_WRED_QUEUE_CFG_SET:
               memset(&x.qos_wredqueuecfg, 0x00, sizeof(x.qos_wredqueuecfg));
               x.qos_wredqueuecfg.nQueueId = cmd_arg[0];
 /*              x.qos_wredqueuecfg.eProfile = cmd_arg[1]; */
               x.qos_wredqueuecfg.nRed_Min = cmd_arg[1];
               x.qos_wredqueuecfg.nRed_Max = cmd_arg[2];
               x.qos_wredqueuecfg.nYellow_Min = cmd_arg[3];
               x.qos_wredqueuecfg.nYellow_Max = cmd_arg[4];
               x.qos_wredqueuecfg.nGreen_Min = cmd_arg[5];
               x.qos_wredqueuecfg.nGreen_Max = cmd_arg[6];
               ioctl_params = (void *)&x.qos_wredqueuecfg;
               break;
       case IFX_ETHSW_QOS_SCHEDULER_CFG_GET:
               memset(&x.qos_schedulecfg, 0x00, sizeof(x.qos_schedulecfg));
               x.qos_schedulecfg.nQueueId = cmd_arg[0];
               ioctl_params = (void *)&x.qos_schedulecfg;
               break;
       case IFX_ETHSW_QOS_SCHEDULER_CFG_SET:
               memset(&x.qos_schedulecfg, 0x00, sizeof(x.qos_schedulecfg));
               x.qos_schedulecfg.nQueueId = cmd_arg[0];
               x.qos_schedulecfg.eType = cmd_arg[1];
               x.qos_schedulecfg.nWeight = cmd_arg[2];
               ioctl_params = (void *)&x.qos_schedulecfg;
               break;
       case IFX_ETHSW_QOS_STORM_CFG_SET:
               memset(&x.qos_stormcfg, 0x00, sizeof(x.qos_stormcfg));
               x.qos_stormcfg.nMeterId = cmd_arg[0];
               x.qos_stormcfg.bBroadcast = cmd_arg[1];
               x.qos_stormcfg.bMulticast = cmd_arg[2];
               x.qos_stormcfg.bUnknownUnicast = cmd_arg[3];
               ioctl_params = (void *)&x.qos_stormcfg;
               break;
       case IFX_ETHSW_QOS_STORM_CFG_GET:
               memset(&x.qos_stormcfg, 0x00, sizeof(x.qos_stormcfg));
               x.qos_stormcfg.nMeterId = cmd_arg[0];
               ioctl_params = (void *)&x.qos_stormcfg;
               break;
 /*      case IFX_ETHSW_QOS_STORM_REMOVE:
               memset(&x.qos_stormcfg, 0x00, sizeof(x.qos_stormcfg));
               x.qos_stormcfg.nMeterId = cmd_arg[0];
               ioctl_params = (void *)&x.qos_stormcfg;
               break;
*/
       case IFX_ETHSW_QOS_SHAPER_CFG_GET:
               memset(&x.qos_shappercfg, 0x00, sizeof(x.qos_shappercfg));
               x.qos_shappercfg.nRateShaperId = cmd_arg[0];
               ioctl_params = (void *)&x.qos_shappercfg;
               break;
       case IFX_ETHSW_QOS_SHAPER_CFG_SET:
               memset(&x.qos_shappercfg, 0x00, sizeof(x.qos_shappercfg));
               x.qos_shappercfg.nRateShaperId = cmd_arg[0];
               x.qos_shappercfg.bEnable = cmd_arg[1];
               x.qos_shappercfg.nCbs = cmd_arg[2];
               x.qos_shappercfg.nRate = cmd_arg[3];
               ioctl_params = (void *)&x.qos_shappercfg;
               break;
       case IFX_ETHSW_QOS_SHAPER_QUEUE_ASSIGN:
               memset(&x.qos_shapperqueue, 0x00, sizeof(x.qos_shapperqueue));
               x.qos_shapperqueue.nRateShaperId = cmd_arg[0];
               x.qos_shapperqueue.nQueueId = cmd_arg[1];
               ioctl_params = (void *)&x.qos_stormcfg;
               break;
       case IFX_ETHSW_QOS_SHAPER_QUEUE_DEASSIGN:
               memset(&x.qos_shapperqueue, 0x00, sizeof(x.qos_shapperqueue));
               x.qos_shapperqueue.nRateShaperId = cmd_arg[0];
               x.qos_shapperqueue.nQueueId = cmd_arg[1];
               ioctl_params = (void *)&x.qos_shapperqueue;
               break;

       case IFX_ETHSW_QOS_METER_CFG_GET:
               memset(&x.qos_metercfg, 0x00, sizeof(x.qos_metercfg));
               x.qos_metercfg.nMeterId = cmd_arg[0];
               ioctl_params = (void *)&x.qos_metercfg;
               break;
       case IFX_ETHSW_QOS_METER_CFG_SET:
               memset(&x.qos_metercfg, 0x00, sizeof(x.qos_metercfg));
               x.qos_metercfg.bEnable = cmd_arg[0];
               x.qos_metercfg.nMeterId = cmd_arg[1];
               x.qos_metercfg.nCbs = cmd_arg[2];
               x.qos_metercfg.nEbs = cmd_arg[3];
               x.qos_metercfg.nRate = cmd_arg[4];
               ioctl_params = (void *)&x.qos_metercfg;
               break;
       case IFX_ETHSW_QOS_METER_PORT_GET:
               memset(&x.qos_meterportget, 0x00, sizeof(x.qos_meterportget));
               ioctl_params = (void *)&x.qos_meterportget;
               break;
       case IFX_ETHSW_QOS_METER_PORT_ASSIGN:
               memset(&x.qos_meterport, 0x00, sizeof(x.qos_meterport));
               x.qos_meterport.nMeterId = cmd_arg[0];
               x.qos_meterport.eDir = cmd_arg[1];
               x.qos_meterport.nPortIngressId = cmd_arg[2];
               x.qos_meterport.nPortEgressId = cmd_arg[3];
               ioctl_params = (void *)&x.qos_meterport;
               break;
       case IFX_ETHSW_QOS_METER_PORT_DEASSIGN:
               memset(&x.qos_meterport, 0x00, sizeof(x.qos_meterport));
               x.qos_meterport.nMeterId = cmd_arg[0];
               x.qos_meterport.eDir = cmd_arg[1];
               x.qos_meterport.nPortIngressId = cmd_arg[2];
               x.qos_meterport.nPortEgressId = cmd_arg[3];
               ioctl_params = (void *)&x.qos_meterport;
               break;
       case IFX_FLOW_RESET:
               ioctl_params = 0;
               break;
       case IFX_ETHSW_HW_INIT:
               ioctl_params = 0;
               break;
       case IFX_ETHSW_ENABLE:
               ioctl_params = 0;
               break;
       case IFX_ETHSW_DISABLE:
               ioctl_params = 0;
               break;
       case IFX_FLOW_PCE_RULE_DELETE:
           memset(&x.pce_ruledelete, 0x00, sizeof(x.pce_ruledelete));
           x.pce_ruledelete.nIndex = cmd_arg[0];
          ioctl_params = (void *)&x.pce_ruledelete;
           break;
       case IFX_FLOW_PCE_RULE_READ:
           memset(&x.pce_rule, 0x00, sizeof(x.pce_rule));
           x.pce_rule.pattern.nIndex = cmd_arg[0];
          ioctl_params = (void *)&x.pce_rule;
           break;
       case IFX_FLOW_PCE_RULE_WRITE:
           memset(&x.pce_rule, 0x00, sizeof(x.pce_rule));
           /* Rule for pattern */
           x.pce_rule.pattern.nIndex = cmd_arg[0];
           x.pce_rule.pattern.bEnable = cmd_arg[1];
           x.pce_rule.pattern.bPortIdEnable = cmd_arg[2];
           x.pce_rule.pattern.nPortId = cmd_arg[3];
           x.pce_rule.pattern.bDSCP_Enable = cmd_arg[4];
           x.pce_rule.pattern.nDSCP = cmd_arg[5];
           x.pce_rule.pattern.bPCP_Enable = cmd_arg[6];
           x.pce_rule.pattern.nPCP = cmd_arg[7];
           x.pce_rule.pattern.bPktLngEnable = cmd_arg[8];
           x.pce_rule.pattern.nPktLng = cmd_arg[9];
           x.pce_rule.pattern.nPktLngRange = cmd_arg[10];

           x.pce_rule.pattern.bMAC_DstEnable = cmd_arg[11];
           if( convert_mac_adr_str((char*)cmd_arg[12], &x.pce_rule.pattern.nMAC_Dst[0]) != IFX_SUCCESS)
               return -1;

           x.pce_rule.pattern.nMAC_DstMask = cmd_arg[13];

           x.pce_rule.pattern.bMAC_SrcEnable = cmd_arg[14];
           if( convert_mac_adr_str((char*)cmd_arg[15], &x.pce_rule.pattern.nMAC_Src[0]) != IFX_SUCCESS)
               return -1;

           x.pce_rule.pattern.nMAC_SrcMask = cmd_arg[16];

           x.pce_rule.pattern.bAppDataMSB_Enable = cmd_arg[17];
           x.pce_rule.pattern.nAppDataMSB = cmd_arg[18];
           x.pce_rule.pattern.bAppMaskRangeMSB_Select = cmd_arg[19];
           x.pce_rule.pattern.nAppMaskRangeMSB = cmd_arg[20];

           x.pce_rule.pattern.bAppDataLSB_Enable = cmd_arg[21];
           x.pce_rule.pattern.nAppDataLSB = cmd_arg[22];
           x.pce_rule.pattern.bAppMaskRangeLSB_Select = cmd_arg[23];
           x.pce_rule.pattern.nAppMaskRangeLSB = cmd_arg[24];

           x.pce_rule.pattern.eDstIP_Select = cmd_arg[25];
           if( convert_ip_adr_charStr((char*)cmd_arg[26], &x.pce_rule.pattern.nDstIP.nIPv4) != IFX_SUCCESS)
              return -1;

           x.pce_rule.pattern.nDstIP_Mask = cmd_arg[27];

           x.pce_rule.pattern.eSrcIP_Select = cmd_arg[28];
           if( convert_ip_adr_charStr((char*)cmd_arg[29], &x.pce_rule.pattern.nSrcIP.nIPv4) != IFX_SUCCESS)
              return -1;

           x.pce_rule.pattern.nSrcIP_Mask = cmd_arg[30];

           x.pce_rule.pattern.bEtherTypeEnable =cmd_arg[31];
           x.pce_rule.pattern.nEtherType = cmd_arg[32];
           x.pce_rule.pattern.nEtherTypeMask = cmd_arg[33];

           x.pce_rule.pattern.bProtocolEnable = cmd_arg[34];
           x.pce_rule.pattern.nProtocol = cmd_arg[35];
           x.pce_rule.pattern.nProtocolMask = cmd_arg[36];

           x.pce_rule.pattern.bSessionIdEnable = cmd_arg[37];
           x.pce_rule.pattern.nSessionId = cmd_arg[38];

           x.pce_rule.pattern.bVid = cmd_arg[39];
           x.pce_rule.pattern.nVid = cmd_arg[40];

           /* Rule for Action */
           x.pce_rule.action.eTrafficClassAction = cmd_arg[41];
           x.pce_rule.action.nTrafficClassAlternate = cmd_arg[42];
           x.pce_rule.action.eSnoopingTypeAction = cmd_arg[43];
           x.pce_rule.action.eLearningAction = cmd_arg[44];
           x.pce_rule.action.eIrqAction = cmd_arg[45];
           x.pce_rule.action.eCrossStateAction = cmd_arg[46];
           x.pce_rule.action.eCritFrameAction = cmd_arg[47];
           x.pce_rule.action.eTimestampAction = cmd_arg[48];
           x.pce_rule.action.ePortMapAction = cmd_arg[49];
           x.pce_rule.action.nForwardPortMap = cmd_arg[50];
           x.pce_rule.action.bRemarkAction = cmd_arg[51];

           x.pce_rule.action.bRemarkPCP = cmd_arg[52];
           x.pce_rule.action.bRemarkDSCP = cmd_arg[53];
           x.pce_rule.action.bRemarkClass = cmd_arg[54];

           x.pce_rule.action.eMeterAction = cmd_arg[55];
           x.pce_rule.action.nMeterId = cmd_arg[56];

           x.pce_rule.action.bRMON_Action = cmd_arg[57];
           x.pce_rule.action.nRMON_Id = cmd_arg[58];

           x.pce_rule.action.eVLAN_Action = cmd_arg[59];
           x.pce_rule.action.nVLAN_Id = cmd_arg[60];
           x.pce_rule.action.eVLAN_CrossAction = cmd_arg[61];

          ioctl_params = (void *)&x.pce_rule;
           break;
       case IFX_ETHSW_CAP_GET:
               memset(&x.cap, 0x00, sizeof(x.cap));
               x.cap.nCapType = cmd_arg[0];
               ioctl_params = (void *)&x.cap;
               break;
       case IFX_ETHSW_VERSION_GET:
               memset(&x.Version, 0x00, sizeof(x.Version));
               x.Version.nId = cmd_arg[0];
               ioctl_params = (void *)&x.Version;
               break;
       case IFX_ETHSW_PORT_PHY_QUERY:
               memset(&x.phy_Query, 0x00, sizeof(x.phy_Query));
               x.phy_Query.nPortId = cmd_arg[0];
               ioctl_params = (void *)&x.phy_Query;
               break;
       case IFX_ETHSW_STP_PORT_CFG_GET:
               memset(&x.STP_portCfg, 0x00, sizeof(x.STP_portCfg));
               x.STP_portCfg.nPortId = cmd_arg[0];
               ioctl_params = (void *)&x.STP_portCfg;
               break;
       case IFX_ETHSW_STP_PORT_CFG_SET:
               memset(&x.STP_portCfg, 0x00, sizeof(x.STP_portCfg));
               x.STP_portCfg.nPortId = cmd_arg[0];
               x.STP_portCfg.ePortState = cmd_arg[1];
               ioctl_params = (void *)&x.STP_portCfg;
               break;
       case IFX_ETHSW_STP_BPDU_RULE_GET:
               memset(&x.STP_BPDU_Rule, 0x00, sizeof(x.STP_BPDU_Rule));
               ioctl_params = (void *)&x.STP_BPDU_Rule;
               break;
       case IFX_ETHSW_STP_BPDU_RULE_SET:
               memset(&x.STP_BPDU_Rule, 0x00, sizeof(x.STP_BPDU_Rule));
               x.STP_BPDU_Rule.eForwardPort = cmd_arg[0];
               x.STP_BPDU_Rule.nForwardPortId = cmd_arg[1];
               ioctl_params = (void *)&x.STP_BPDU_Rule;
               break;
       case IFX_ETHSW_8021X_PORT_CFG_GET:
               memset(&x.PNAC_portCfg, 0x00, sizeof(x.PNAC_portCfg));
               x.PNAC_portCfg.nPortId = cmd_arg[0];
               ioctl_params = (void *)&x.PNAC_portCfg;
               break;
       case IFX_ETHSW_8021X_PORT_CFG_SET:
               memset(&x.PNAC_portCfg, 0x00, sizeof(x.PNAC_portCfg));
               x.PNAC_portCfg.nPortId = cmd_arg[0];
               x.PNAC_portCfg.eState = cmd_arg[1];
               ioctl_params = (void *)&x.PNAC_portCfg;
               break;
       case IFX_ETHSW_8021X_EAPOL_RULE_GET:
               memset(&x.PNAC_EAPOL_Rule, 0x00, sizeof(x.PNAC_EAPOL_Rule));
               ioctl_params = (void *)&x.PNAC_EAPOL_Rule;
               break;
       case IFX_ETHSW_8021X_EAPOL_RULE_SET:
               memset(&x.PNAC_EAPOL_Rule, 0x00, sizeof(x.PNAC_EAPOL_Rule));
               x.PNAC_EAPOL_Rule.eForwardPort = cmd_arg[0];
               x.PNAC_EAPOL_Rule.nForwardPortId = cmd_arg[1];
               ioctl_params = (void *)&x.PNAC_EAPOL_Rule;
               break;
       case IFX_FLOW_RMON_EXTEND_GET:
           memset(&x.RMON_ExtendGet, 0x00, sizeof(x.RMON_ExtendGet));
           x.RMON_ExtendGet.nPortId = cmd_arg[0];
           ioctl_params = (void *)&x.RMON_ExtendGet;
           break;
#endif
       case IFX_ETHSW_CFG_GET:
           memset(&x.cfg_Data, 0x00, sizeof(x.cfg_Data));
          ioctl_params = (void *)&x.cfg_Data;
           break;
       case IFX_ETHSW_CFG_SET:
           memset(&x.cfg_Data, 0x00, sizeof(x.cfg_Data));
           //x.cfg_Data.bEnable = cmd_arg[0];
           x.cfg_Data.eMAC_TableAgeTimer = cmd_arg[0];
           x.cfg_Data.bVLAN_Aware = cmd_arg[1];
           // x.cfg_Data.eMulticastTableAgeTimer = cmd_arg[3];
           x.cfg_Data.nMaxPacketLen = cmd_arg[2];
           // x.portcfg.bLearningLimitAction = cmd_arg[3];
           x.cfg_Data.bPauseMAC_ModeSrc = cmd_arg[3];
           if( convert_mac_adr_str((char*)cmd_arg[4], &x.cfg_Data.nPauseMAC_Src[0]) != IFX_SUCCESS)
               return -1;
           // x.cfg_Data.nPauseMAC_Src[6] = cmd_arg[5];
          ioctl_params = (void *)&x.cfg_Data;
           break;
       case IFX_ETHSW_PORT_CFG_GET:
           memset(&x.portcfg, 0x00, sizeof(x.portcfg));
           x.portcfg.nPortId = cmd_arg[0];
          ioctl_params = (void *)&x.portcfg;
           break;
       case IFX_ETHSW_PORT_CFG_SET:
           memset(&x.portcfg, 0x00, sizeof(x.portcfg));
           x.portcfg.nPortId = cmd_arg[0];
           x.portcfg.eEnable = cmd_arg[1];
           x.portcfg.bUnicastUnknownDrop = cmd_arg[2];
           x.portcfg.bMulticastUnknownDrop = cmd_arg[3];
           x.portcfg.bReservedPacketDrop = cmd_arg[4];
           x.portcfg.bBroadcastDrop = cmd_arg[5];
           x.portcfg.bAging = cmd_arg[6];
           // x.portcfg.bLearningLimitAction = cmd_arg[7];
           x.portcfg.bLearningMAC_PortLock = cmd_arg[7];
           x.portcfg.nLearningLimit = cmd_arg[8]; 
           x.portcfg.ePortMonitor = cmd_arg[9];
           x.portcfg.eFlowCtrl = cmd_arg[10];
           ioctl_params = (void *)&x.portcfg;
           break;
       case IFX_ETHSW_PORT_LINK_CFG_GET:
           memset(&x.portlinkcfgGet, 0x00, sizeof(x.portlinkcfgGet));
           x.portlinkcfgGet.nPortId = cmd_arg[0];
          ioctl_params = (void *)&x.portlinkcfgGet;
           break;
       case IFX_ETHSW_PORT_LINK_CFG_SET:
           memset(&x.portlinkcfgSet, 0x00, sizeof(x.portlinkcfgSet));
           x.portlinkcfgSet.nPortId = cmd_arg[0];
           x.portlinkcfgSet.bDuplexForce = cmd_arg[1];
           x.portlinkcfgSet.eDuplex = cmd_arg[2];
           x.portlinkcfgSet.bSpeedForce = cmd_arg[3];
           x.portlinkcfgSet.eSpeed = cmd_arg[4];
           x.portlinkcfgSet.bLinkForce = cmd_arg[5];
           x.portlinkcfgSet.eLink = cmd_arg[6];
           x.portlinkcfgSet.eMII_Mode = cmd_arg[7];
           x.portlinkcfgSet.eMII_Type = cmd_arg[8];
           x.portlinkcfgSet.eClkMode = cmd_arg[9];
           ioctl_params = (void *)&x.portlinkcfgSet;
           break;
       default:
           break;
   }

   /*
    * pass data to device driver by means of ioctl command
    */

   retval=ioctl( fd, ioctl_command, ioctl_params );
  // printf("retval if ioctl [%d] is [%d]\n", ioctl_command, retval);
   if(retval != IFX_SUCCESS)
   {
      printf("IOCTL failed for ioctl command 0x%08X, returned %d\n",
         ioctl_command, retval);
      return -1;

   }

   /*
    * For ioctl commands that read back anything, print the returned data
    */

   switch(ioctl_command) {
#ifdef VERBOSE_PRINT
           case IFX_PSB6970_POWER_MANAGEMENT_GET:
              if (x.power_management.bEnable == 1)
                  printf("\tPower Management          = Enable\n");
              else
                printf("\tPower Management          = Disable\n");
#else
              if (x.power_management.bEnable == 1)
                  printf("bEnable=1\n");
              else
                printf("bEnable=0\n");
#endif
           break;
         case IFX_ETHSW_MAC_TABLE_ENTRY_READ:
#if 0
              if (x.MAC_tableRead.bLast == 0)
              {
                printf("------------------------------------------------\n");
                printf("   MAC Address       |   age    | FID | Static  \n");
                printf("------------------------------------------------\n");
                printf("  %02x:%02x:%02x:%02x:%02x:%02x  |  %4d    | %d   | %d\n",
                       x.MAC_tableRead.nMAC[0], x.MAC_tableRead.nMAC[1], x.MAC_tableRead.nMAC[2],
                       x.MAC_tableRead.nMAC[3], x.MAC_tableRead.nMAC[4], x.MAC_tableRead.nMAC[5],
                       x.MAC_tableRead.nAgeTimer, x.MAC_tableRead.nFId,
                       x.MAC_tableRead.bStaticEntry );
                 do {
                   memset(&x.MAC_tableRead, 0x00, sizeof(x.MAC_tableRead));
                   x.MAC_tableRead.bInitial = 0;
                   x.MAC_tableRead.nPortId = cmd_arg[1];
                   ioctl_params = (void *)&x.MAC_tableRead;
                   retval=ioctl( fd, ioctl_command, ioctl_params );
                   if(retval != IFX_SUCCESS)
                   {
                    printf("IOCTL failed for ioctl command 0x%08X, returned %d\n",
                     ioctl_command, retval);
                    return -1;

                   }
                   if (x.MAC_tableRead.bLast == 0)
                   {

                    printf("  %02x:%02x:%02x:%02x:%02x:%02x  |  %4d    | %d   | %d\n",
                           x.MAC_tableRead.nMAC[0], x.MAC_tableRead.nMAC[1], x.MAC_tableRead.nMAC[2],
                           x.MAC_tableRead.nMAC[3], x.MAC_tableRead.nMAC[4], x.MAC_tableRead.nMAC[5],
                           x.MAC_tableRead.nAgeTimer, x.MAC_tableRead.nFId,
                           x.MAC_tableRead.bStaticEntry );
                   }
                 } while (x.MAC_tableRead.bLast == 0);
              }
              else
              {
                  printf("There is on mac table entry\n");
              }
#endif
                            break;
         case IFX_ETHSW_VLAN_PORT_MEMBER_READ:
         	break;
         case IFX_ETHSW_RMON_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort Id                                       = %d\n", x.RMON_cnt.nPortId);
                printf("\tReceive Packet Count                          = %d\n", x.RMON_cnt.nRxGoodPkts);
                printf("\tReceive Unicast Packet Count                  = %d\n", x.RMON_cnt.nRxUnicastPkts);
                printf("\tReceive Broadcast Packet Count                = %d\n", x.RMON_cnt.nRxBroadcastPkts);
                printf("\tReceive Multicast Packet Count                = %d\n", x.RMON_cnt.nRxMulticastPkts);
                printf("\tReceive FCS Error Packet Count                = %d\n", x.RMON_cnt.nRxFCSErrorPkts);
                printf("\tReceive Undersize Good Packet Count           = %d\n", x.RMON_cnt.nRxUnderSizeGoodPkts);
                printf("\tReceive Oversize Good Packet Count            = %d\n", x.RMON_cnt.nRxOversizeGoodPkts);
                printf("\tReceive Undersize Error Packet Count          = %d\n", x.RMON_cnt.nRxUnderSizeErrorPkts);
                printf("\tReceive Good Pause Packet Count               = %d\n", x.RMON_cnt.nRxGoodPausePkts);
                printf("\tReceive Oversize Error Packet Count           = %d\n", x.RMON_cnt.nRxOversizeErrorPkts);
                printf("\tReceive Align Error Packet Count              = %d\n", x.RMON_cnt.nRxAlignErrorPkts);
                printf("\tFiltered Packet Count                         = %d\n", x.RMON_cnt.nRxFilteredPkts);
                printf("\tReceive Size 64 Packet Count                  = %d\n", x.RMON_cnt.nRx64BytePkts);
                printf("\tReceive Size 65-127 Packet Countt             = %d\n", x.RMON_cnt.nRx127BytePkts);
                printf("\tReceive Size 128-255 Packet Count             = %d\n", x.RMON_cnt.nRx255BytePkts);
                printf("\tReceive Size 256-511 Packet Count             = %d\n", x.RMON_cnt.nRx511BytePkts);
                printf("\tReceive Size 512-1023 Packet Count            = %d\n", x.RMON_cnt.nRx1023BytePkts);
                printf("\tReceive Size 1024-1522(or more)Packet Count   = %d\n", x.RMON_cnt.nRxMaxBytePkts);
                printf("\tReceive Dropped Packet Count                  = %d\n", x.RMON_cnt.nRxDroppedPkts);
                printf("\tTransmit Packet Count                         = %d\n", x.RMON_cnt.nTxGoodPkts);
                printf("\tTransmit Unicast Packet Count                 = %d\n", x.RMON_cnt.nTxUnicastPkts);
                printf("\tTransmit Broadcast Packet Count               = %d\n", x.RMON_cnt.nTxBroadcastPkts);
                printf("\tTransmit Multicast Packet Count               = %d\n", x.RMON_cnt.nTxMulticastPkts);
                printf("\tTransmit Single Collision Count               = %d\n", x.RMON_cnt.nTxSingleCollCount);
                printf("\tTransmit Multiple Collision Count             = %d\n", x.RMON_cnt.nTxMultCollCount);
                printf("\tTransmit Late Collision Count                 = %d\n", x.RMON_cnt.nTxLateCollCount);
                printf("\tTransmit Excessive Collision Count            = %d\n", x.RMON_cnt.nTxExcessCollCount);
                printf("\tTransmit Collision Count                      = %d\n", x.RMON_cnt.nTxCollCount);
                printf("\tTransmit Pause Packet Count                   = %d\n", x.RMON_cnt.nTxPauseCount);
                printf("\tTransmit Size 64 Packet Count                 = %d\n", x.RMON_cnt.nTx64BytePkts);
                printf("\tTransmit Size 65-127 Packet Count             = %d\n", x.RMON_cnt.nTx127BytePkts);
                printf("\tTransmit Size 128-255 Packet Count            = %d\n", x.RMON_cnt.nTx255BytePkts);
                printf("\tTransmit Size 256-511 Packet Count            = %d\n", x.RMON_cnt.nTx511BytePkts);
                printf("\tTransmit Size 512-1023 Packet Count           = %d\n", x.RMON_cnt.nTx1023BytePkts);
                printf("\tTransmit Size 1024-1522(or more)Packet Count  = %d\n", x.RMON_cnt.nTxMaxBytePkts);
                printf("\tTransmit Drop Packet Count                    = %d\n", x.RMON_cnt.nTxDroppedPkts);
                printf("\tEgress Queue Discard (ACM) Frame Count        = %d\n", x.RMON_cnt.nTxAcmDroppedPkts);
                value_low = x.RMON_cnt.nRxGoodBytes & 0x00000000FFFFFFFF;
                value_high = x.RMON_cnt.nRxGoodBytes >> 32;
                //printf("\tReceive Good Bytes                            = 0x%x%08x\n", value_high, value_low);
                value_low = x.RMON_cnt.nRxBadBytes & 0x00000000FFFFFFFF;
                value_high = x.RMON_cnt.nRxBadBytes >> 32;
                //printf("\tReceive Bad Bytes                             = 0x%x%08x\n", value_high, value_low);
                value_low = x.RMON_cnt.nTxGoodBytes & 0x00000000FFFFFFFF;
                value_high = x.RMON_cnt.nTxGoodBytes >> 32;
                //printf("\tTransmit Good Bytes                           = 0x%x%08x\n", value_high, value_low);
                printf("\tReceive Good Bytes Count                      = %08lld\n", x.RMON_cnt.nRxGoodBytes);
                printf("\tReceive Bad Byte Count                        = %08lld\n", x.RMON_cnt.nRxBadBytes);
                printf("\tTransmit Good Byte Count                      = %08lld\n", x.RMON_cnt.nTxGoodBytes);
#else
                printf("nPortId=%d ", x.RMON_cnt.nPortId);
                printf("nRxUnicastPkts=%d ", x.RMON_cnt.nRxUnicastPkts);
                printf("nRxBroadcastPkts=%d ", x.RMON_cnt.nRxBroadcastPkts);
                printf("nRxMulticastPkts=%d ", x.RMON_cnt.nRxMulticastPkts);
                printf("nRxFCSErrorPkts=%d ", x.RMON_cnt.nRxFCSErrorPkts);
                printf("nRxUnderSizeGoodPkts=%d ", x.RMON_cnt.nRxUnderSizeGoodPkts);
                printf("nRxOversizeGoodPkts=%d ", x.RMON_cnt.nRxOversizeGoodPkts);
                printf("nRxUnderSizeErrorPkts=%d ", x.RMON_cnt.nRxUnderSizeErrorPkts);
                printf("nRxGoodPausePkts=%d ", x.RMON_cnt.nRxGoodPausePkts);
                printf("nRxOversizeErrorPkts=%d ", x.RMON_cnt.nRxOversizeErrorPkts);
                printf("nRxAlignErrorPkts=%d ", x.RMON_cnt.nRxAlignErrorPkts);
                printf("nRxFilteredPkts=%d ", x.RMON_cnt.nRxFilteredPkts);
                printf("nRx64BytePkts=%d ", x.RMON_cnt.nRx64BytePkts);
                printf("nRx127BytePkts=%d ", x.RMON_cnt.nRx127BytePkts);
                printf("nRx255BytePkts=%d ", x.RMON_cnt.nRx255BytePkts);
                printf("nRx511BytePkts=%d ", x.RMON_cnt.nRx511BytePkts);
                printf("nRx1023BytePkts=%d ", x.RMON_cnt.nRx1023BytePkts);
                printf("nRxMaxBytePkts=%d ", x.RMON_cnt.nRxMaxBytePkts);
                printf("nTxUnicastPkts=%d ", x.RMON_cnt.nTxUnicastPkts);
                printf("nTxBroadcastPkts=%d ", x.RMON_cnt.nTxBroadcastPkts);
                printf("nTxMulticastPkts=%d ", x.RMON_cnt.nTxMulticastPkts);
                printf("nTxSingleCollCount=%d ", x.RMON_cnt.nTxSingleCollCount);
                printf("nTxMultCollCount=%d ", x.RMON_cnt.nTxMultCollCount);
                printf("nTxLateCollCount=%d ", x.RMON_cnt.nTxLateCollCount);
                printf("nTxExcessCollCount=%d ", x.RMON_cnt.nTxExcessCollCount);
                printf("nTxCollCount=%d ", x.RMON_cnt.nTxCollCount);
                printf("nTxPauseCount=%d ", x.RMON_cnt.nTxPauseCount);
                printf("nTx64BytePkts=%d ", x.RMON_cnt.nTx64BytePkts);
                printf("nTx127BytePkts=%d ", x.RMON_cnt.nTx127BytePkts);
                printf("nTx255BytePkts=%d ", x.RMON_cnt.nTx255BytePkts);
                printf("nTx511BytePkts=%d ", x.RMON_cnt.nTx511BytePkts);
                printf("nTx1023BytePkts=%d ", x.RMON_cnt.nTx1023BytePkts);
                printf("nTxMaxBytePkts=%d ", x.RMON_cnt.nTxMaxBytePkts);
                printf("nTxDroppedPkts=%d ", x.RMON_cnt.nTxDroppedPkts);
                printf("nRxDroppedPkts=%d ", x.RMON_cnt.nRxDroppedPkts);
                value_low = x.RMON_cnt.nRxGoodBytes & 0x00000000FFFFFFFF;
                value_high = x.RMON_cnt.nRxGoodBytes >> 32;
                printf("nRxGoodBytes=0x%x%08x ", value_high, value_low);
                value_low = x.RMON_cnt.nRxBadBytes & 0x00000000FFFFFFFF;
                value_high = x.RMON_cnt.nRxBadBytes >> 32;
                printf("nRxBadBytes=0x%x%08x ", value_high, value_low);
                value_low = x.RMON_cnt.nTxGoodBytes & 0x00000000FFFFFFFF;
                value_high = x.RMON_cnt.nTxGoodBytes >> 32;
                printf("nTxGoodBytes=0x%x%08x\n", value_high, value_low);
#endif
           break;
         case IFX_ETHSW_PORT_REDIRECT_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort Id                 = %d\n", x.portRedirectData.nPortId);
                printf("\tPort Redirect Egress    = %d\n", x.portRedirectData.bRedirectEgress);
                printf("\tPort Redirect Ingress   = %d\n", x.portRedirectData.bRedirectIngress);
#else
                printf("nPortId=%d ", x.portRedirectData.nPortId);
                printf("bRedirectEgress=%d ", x.portRedirectData.bRedirectEgress);
                printf("bRedirectIngress=%d\n", x.portRedirectData.bRedirectIngress);
#endif
           break;
         case IFX_ETHSW_MDIO_DATA_READ:
#ifdef VERBOSE_PRINT
                printf("\tAddress Device = %d\n", x.mdio_Data.nAddressDev);
                printf("\tAddress Register = %d\n", x.mdio_Data.nAddressReg);
                printf("\tData = 0x%x\n", x.mdio_Data.nData);
#else
                printf("nAddressDev=%d ", x.mdio_Data.nAddressDev);
                printf("nAddressReg=%d ", x.mdio_Data.nAddressReg);
                printf("nData=0x%x\n", x.mdio_Data.nData);
#endif
           break;
         case IFX_ETHSW_MDIO_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tMDIO Access Enable = %d\n", x.mdio_cfg.bMDIO_Enable);
                printf("\tClock Speed= %d\n", x.mdio_cfg.nMDIO_Speed);
#else
                printf("bMDIO_Enable=%d ", x.mdio_cfg.bMDIO_Enable);
                printf("nMDIO_Speed=%d ", x.mdio_cfg.nMDIO_Speed);
#endif
           break;
         case IFX_ETHSW_VLAN_ID_GET:
#ifdef VERBOSE_PRINT
                printf("\tVId = %d\n", x.vlan_IdGet.nVId);
                printf("\tFId = %d\n", x.vlan_IdGet.nFId);
#else
                printf("VId=%d ", x.vlan_IdGet.nVId);
                printf("FId=%d\n", x.vlan_IdGet.nFId);
#endif
           break;
         case IFX_ETHSW_VLAN_PORT_CFG_GET:
#ifdef VERBOSE_PRINT
             printf("\tVLAN PortId           = %d\n", x.vlan_portcfg.nPortId);
             printf("\tVLAN PortVId          = %d\n", x.vlan_portcfg.nPortVId);
             printf("\tVLAN Unknown Drop     = %d\n", x.vlan_portcfg.bVLAN_UnknownDrop);
             printf("\tVLAN ReAssign         = %d\n", x.vlan_portcfg.bVLAN_ReAssign);
             printf("\tVLAN Violation Member = %d\n", x.vlan_portcfg.eVLAN_MemberViolation);
             printf("\tVLAN Admit Mode       = %d\n", x.vlan_portcfg.eAdmitMode);
             printf("\tVLAN TVM              = %d\n", x.vlan_portcfg.bTVM);
#else
             printf("nPortId=%d ", x.vlan_portcfg.nPortId);
             printf("nPortVId=%d ", x.vlan_portcfg.nPortVId);
             printf("bVLAN_UnknownDrop=%d ", x.vlan_portcfg.bVLAN_UnknownDrop);
             printf("bVLAN_ReAssign=%d ", x.vlan_portcfg.bVLAN_ReAssign);
             printf("eVLAN_MemberViolation=%d ", x.vlan_portcfg.eVLAN_MemberViolation);
             printf("eAdmitMode=%d ", x.vlan_portcfg.eAdmitMode);
             printf("bTVM=%d\n", x.vlan_portcfg.bTVM);
#endif
           break;
         case IFX_ETHSW_MULTICAST_ROUTER_PORT_READ:
#ifdef VERBOSE_PRINT
#if defined(VR9)
            print_multicast_router_port();
#else
            printf("\tMULTICAST PortId           = 0x%x\n", x.multicast_RouterPortRead.nPortId);
#endif
#else
            printf("nPortId=0x%x\n", x.multicast_RouterPortRead.nPortId);
#endif
           break;
         case IFX_ETHSW_MULTICAST_TABLE_ENTRY_READ:
#if  defined(AR9) || defined(DANUBE) || defined(AMAZON_SE)
#elif defined(VR9_XX)
#ifdef VERBOSE_PRINT
                printf("-----------------------------------------------------\n");
                printf(" Port |      GDA        |      GSA      | Member Mode  \n");
                printf("-----------------------------------------------------\n");
#endif
                while (x.multicast_TableEntryRead.bLast == 0)
                {
#ifdef VERBOSE_PRINT
                    // Port field
                    printf(" %2d   |", x.multicast_TableEntryRead.nPortId);
                    // GDA field
                    if (x.multicast_TableEntryRead.eIPVersion == IFX_ETHSW_IP_SELECT_IPV4)
                    {   // IPV4
                        printf("%03d.%03d.%03d.%03d  |",
                        (x.multicast_TableEntryRead.uIP_Gda.nIPv4 >> 24) & 0xFF,
                        (x.multicast_TableEntryRead.uIP_Gda.nIPv4 >> 16) & 0xFF,
                        (x.multicast_TableEntryRead.uIP_Gda.nIPv4 >> 8)  & 0xFF,
                        x.multicast_TableEntryRead.uIP_Gda.nIPv4         & 0xFF);
                    } else
                    {   // IPV6
                        printf("%02d.%02d.%02d.%02d |",
                        x.multicast_TableEntryRead.uIP_Gda.nIPv6[3],
                        x.multicast_TableEntryRead.uIP_Gda.nIPv6[2],
                        x.multicast_TableEntryRead.uIP_Gda.nIPv6[1],
                        x.multicast_TableEntryRead.uIP_Gda.nIPv6[0]);
                    }
                    // GSA field
                    if (x.multicast_TableEntryRead.eIPVersion == IFX_ETHSW_IP_SELECT_IPV4)
                    {
                        printf("      NA       |");
                    }
                    // Member Mode field
#ifdef VR9
                    printf("   %d\n",x.multicast_TableEntryRead.eModeMember);
#else
                    printf("   NA\n");
#endif
#else
                    // Port value
                    printf("nPortId=%d ", x.multicast_TableEntryAdd.nPortId);
                    // Version value
                    printf("eIPVersion=%d ", x.multicast_TableEntryAdd.eIPVersion);
                    // GDA value
                    if (x.multicast_TableEntryRead.eIPVersion == IFX_ETHSW_IP_SELECT_IPV4)
                    {   // IPV4
                        printf("uIP_Gda=%02d.%02d.%02d.%02d ",
                        (x.multicast_TableEntryRead.uIP_Gda.nIPv4 >> 24) & 0xFF,
                        (x.multicast_TableEntryRead.uIP_Gda.nIPv4 >> 16) & 0xFF,
                        (x.multicast_TableEntryRead.uIP_Gda.nIPv4 >> 8)  & 0xFF,
                        x.multicast_TableEntryRead.uIP_Gda.nIPv4 & 0xFF);
                    } else
                    {   // IPV6
                        printf("uIP_Gda=%02d.%02d.%02d.%02d ",
                        x.multicast_TableEntryRead.uIP_Gda.nIPv6[3],
                        x.multicast_TableEntryRead.uIP_Gda.nIPv6[2],
                        x.multicast_TableEntryRead.uIP_Gda.nIPv6[1],
                        x.multicast_TableEntryRead.uIP_Gda.nIPv6[0]);
                    }
                    // GSA value
                    if (x.multicast_TableEntryRead.eIPVersion == IFX_ETHSW_IP_SELECT_IPV4)
                    {   // IPV4
                        printf("uIP_Gsa= %02d.%02d.%02d.%02d ",
                        (x.multicast_TableEntryRead.uIP_Gsa.nIPv4 >> 24) & 0xFF,
                        (x.multicast_TableEntryRead.uIP_Gsa.nIPv4 >> 16) & 0xFF,
                        (x.multicast_TableEntryRead.uIP_Gsa.nIPv4 >> 8) & 0xFF,
                        x.multicast_TableEntryRead.uIP_Gsa.nIPv4 & 0xFF);
                    } else
                    {   // IPV6
                        printf ("uIP_Gsa=%02d.%02d.%02d.%02d ",
                        x.multicast_TableEntryRemove.uIP_Gsa.nIPv6[3],
                        x.multicast_TableEntryRemove.uIP_Gsa.nIPv6[2],
                        x.multicast_TableEntryRemove.uIP_Gsa.nIPv6[1],
                        x.multicast_TableEntryRemove.uIP_Gsa.nIPv6[0]);
                    }
                    // Member Mode value
                    printf("eModeMember=%d\n",x.multicast_TableEntryRead.eModeMember);
#endif
                    memset(&x.multicast_TableEntryRead, 0x00, sizeof(x.multicast_TableEntryRead));
                    x.multicast_TableEntryRead.bInitial = 1;
                    x.multicast_TableEntryRead.nPortId = cmd_arg[0];
                    ioctl_params = (void *)&x.multicast_TableEntryRead;
                    retval=ioctl( fd, ioctl_command, ioctl_params );
                    if(retval != IFX_SUCCESS)
                    {
                        printf("IOCTL failed for ioctl command 0x%08X, returned %d\n",
                        ioctl_command, retval);
                        return -1;
                    }
                 }
#endif
           break;
         case IFX_ETHSW_MULTICAST_SNOOP_CFG_GET:
#ifdef VERBOSE_PRINT
             printf("\tMULTICAST SNOOP IGMP_Mode           = %d\n", x.multicast_SnoopCfgGet.eIGMP_Mode);
             printf("\tMULTICAST SNOOP IGMPv3 support      = %d\n", x.multicast_SnoopCfgGet.bIGMPv3);
             printf("\tMULTICAST SNOOP CrossVLAN           = %d\n", x.multicast_SnoopCfgGet.bCrossVLAN);
             printf("\tMULTICAST SNOOP ForwardPort         = %d\n", x.multicast_SnoopCfgGet.eForwardPort);
             printf("\tMULTICAST SNOOP ForwardPortId       = %d\n", x.multicast_SnoopCfgGet.nForwardPortId);
             printf("\tMULTICAST SNOOP ClassOfService      = %d\n", x.multicast_SnoopCfgGet.nClassOfService);
             printf("\tMULTICAST SNOOP Robust              = %d\n", x.multicast_SnoopCfgGet.nRobust);
             printf("\tMULTICAST SNOOP QueryInterval (HEX) = 0x%x\n", x.multicast_SnoopCfgGet.nQueryInterval);
             printf("\tMULTICAST Suppression | Aggregation = %d\n", x.multicast_SnoopCfgGet.eSuppressionAggregation);
             printf("\tMULTICAST SNOOP FastLeave           = %d\n", x.multicast_SnoopCfgGet.bFastLeave);
             printf("\tMULTICAST SNOOP LearningRouter      = %d\n", x.multicast_SnoopCfgGet.bLearningRouter);
#else
             printf("eIGMP_Mode=%d ", x.multicast_SnoopCfgGet.eIGMP_Mode);
             printf("bIGMPv3=%d ", x.multicast_SnoopCfgGet.bIGMPv3);
             printf("bCrossVLAN=%d ", x.multicast_SnoopCfgGet.bCrossVLAN);
             printf("eForwardPort=%d ", x.multicast_SnoopCfgGet.eForwardPort);
             printf("nForwardPortId=%d ", x.multicast_SnoopCfgGet.nForwardPortId);
             printf("nClassOfService=%d ", x.multicast_SnoopCfgGet.nClassOfService);
             printf("nRobust=%d ", x.multicast_SnoopCfgGet.nRobust);
             printf("nQueryInterval= 0x%x ", x.multicast_SnoopCfgGet.nQueryInterval);
             printf("x.multicast_SnoopCfgGet.eSuppressionAggregation=%dx ", x.multicast_SnoopCfgGet.eSuppressionAggregation);
             printf("bFastLeave=%d\n", x.multicast_SnoopCfgGet.bFastLeave);
             printf("bLearningRouter=%d\n", x.multicast_SnoopCfgGet.bLearningRouter);
#endif
           break;
           case IFX_ETHSW_CPU_PORT_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort Id                                 = %d\n", x.CPU_PortCfg.nPortId);
                printf("\tEnable FCS check                        = %d\n", x.CPU_PortCfg.bFcsCheck);
                printf("\tEnable FCS Enable FCS generation        = %d\n", x.CPU_PortCfg.bFcsGenerate);
                printf("\tSpecial tag enable in egress direction  = %d\n", x.CPU_PortCfg.bSpecialTagEgress);
                printf("\tSpecial tag enable in ingress direction = %d\n", x.CPU_PortCfg.bSpecialTagIngress);
                printf("\tCPU port validity                       = %d\n", x.CPU_PortCfg.bCPU_PortValid);
#else
                printf("\tnPortId                                 = %d\n", x.CPU_PortCfg.nPortId);
                printf("\tbFcsCheck                               = %d\n", x.CPU_PortCfg.bFcsCheck);
                printf("\tbFcsGenerate                            = %d\n", x.CPU_PortCfg.bFcsGenerate);
                printf("\tbSpecialTagEgress                       = %d\n", x.CPU_PortCfg.bSpecialTagEgress);
                printf("\tbSpecialTagIngress                      = %d\n", x.CPU_PortCfg.bSpecialTagIngress);
                printf("\tbCPU_PortValid                          = %d\n", x.CPU_PortCfg.bCPU_PortValid);
#endif
           break;
           case IFX_ETHSW_MONITOR_PORT_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort Id                                 = %d\n", x.monitorPortCfg.nPortId);
                printf("\tThis port is used as monitor port.      = %d\n", x.monitorPortCfg.bMonitorPort);
#else
                printf("\tnPortId                                 = %d\n", x.monitorPortCfg.nPortId);
                printf("\tbMonitorPort                            = %d\n", x.monitorPortCfg.bMonitorPort);
#endif
           break;
           case IFX_ETHSW_QOS_PORT_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort Id                  = %d\n", x.qos_portcfg.nPortId);
                printf("\tTraffic Class assignment = %d\n", x.qos_portcfg.eClassMode);
                printf("\tTraffic Class            = %d\n", x.qos_portcfg.nTrafficClass);
#else
                printf("\tnPortId                  = %d\n", x.qos_portcfg.nPortId);
                printf("\teClassMode               = %d\n", x.qos_portcfg.eClassMode);
                printf("\tnTrafficClass            = %d\n", x.qos_portcfg.nTrafficClass);
#endif
           break;
           case IFX_ETHSW_QOS_QUEUE_PORT_GET:
#ifdef VERBOSE_PRINT
                printf("\tQoS queue index          = %d\n", x.qos_queueport.nQueueId);
#else
                printf("\tnQueueId                 = %d\n", x.qos_queueport.nQueueId);
#endif
           break;
           case IFX_ETHSW_QOS_DSCP_CLASS_GET:
                   for (i=0; i<64; i++)
                        printf("\tnTrafficClass[%d] = %d\n", i, x.qos_dscpclasscfgget.nTrafficClass[i]);

           break;
           case IFX_ETHSW_QOS_PCP_CLASS_GET:
                   for (i=0; i<8; i++)
                        printf("\tnTrafficClass[%d] = %d\n", i, x.qos_pcpclasscfgget.nTrafficClass[i]);

           break;
           case IFX_ETHSW_QOS_CLASS_PCP_GET:
                   for (i=0; i<16; i++)
                        printf("\tnPCP[%d] = %d\n", i, x.qos_classpcpcfgset.nPCP[i]);

           break;
           case IFX_ETHSW_QOS_CLASS_DSCP_GET:
                   for (i=0; i<16; i++)
                        printf("\tnDSCP[%d] = %d\n", i, x.qos_classdscpcfgget.nDSCP[i]);

           break;
           case IFX_ETHSW_QOS_PORT_REMARKING_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tnPortId = 0x%x\n", x.qos_portremarking.nPortId);
                printf("\teDSCP_IngressRemarkingEnable = 0x%x\n", x.qos_portremarking.eDSCP_IngressRemarkingEnable);
                printf("\tbDSCP_EgressRemarkingEnable  = 0x%x\n", x.qos_portremarking.bDSCP_EgressRemarkingEnable);
                printf("\tbPCP_IngressRemarkingEnable  = 0x%x\n", x.qos_portremarking.bPCP_IngressRemarkingEnable);
                printf("\tbPCP_EgressRemarkingEnable   = 0x%x\n", x.qos_portremarking.bPCP_EgressRemarkingEnable);
#else
                printf("\tnPortId=%x ", x.qos_portremarking.nPortId);
                printf("\teDSCP_IngressRemarkingEnable= 0x%x ", x.qos_portremarking.eDSCP_IngressRemarkingEnable);
                printf("\tbDSCP_EgressRemarkingEnable= 0x%x ", x.qos_portremarking.bDSCP_EgressRemarkingEnable);
                printf("\tbPCP_IngressRemarkingEnable= 0x%x ", x.qos_portremarking.bPCP_IngressRemarkingEnable);
                printf("\tbPCP_EgressRemarkingEnable= 0x%x\n", x.qos_portremarking.bPCP_EgressRemarkingEnable);
#endif
           break;
         case IFX_ETHSW_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tMAC_Table Age Timer = %d\n", x.cfg_Data.eMAC_TableAgeTimer);
                printf("\tVLAN_Aware          = %d\n", x.cfg_Data.bVLAN_Aware);
                printf("\tMax Packet Len      = %d\n", x.cfg_Data.nMaxPacketLen);
                printf("\tMax Packet Len      = %d\n", x.cfg_Data.bLearningLimitAction);
                printf("\tPause MAC Mode      = %d\n", x.cfg_Data.bPauseMAC_ModeSrc);
                printf("\tPause MAC Src       = %02x:%02x:%02x:%02x:%02x:%02x\n",
                                                         x.cfg_Data.nPauseMAC_Src[0],
                                                         x.cfg_Data.nPauseMAC_Src[1],
                                                         x.cfg_Data.nPauseMAC_Src[2],
                                                         x.cfg_Data.nPauseMAC_Src[3],
                                                         x.cfg_Data.nPauseMAC_Src[4],
                                                         x.cfg_Data.nPauseMAC_Src[5]
                                                         );
#else
                printf("eMAC_TableAgeTimer=%d ", x.cfg_Data.eMAC_TableAgeTimer);
                printf("bVLAN_Aware=%d ", x.cfg_Data.bVLAN_Aware);
                printf("nMaxPacketLen=%d ", x.cfg_Data.nMaxPacketLen);
/*                printf("\tMax Packet Len      = %d\n", x.cfg_Data.bLearningLimitAction); */
                printf("bPauseMAC_ModeSrc=%d ", x.cfg_Data.bPauseMAC_ModeSrc);
                printf("nPauseMAC_Src=%02x:%02x:%02x:%02x:%02x:%02x\n",
                                                         x.cfg_Data.nPauseMAC_Src[0],
                                                         x.cfg_Data.nPauseMAC_Src[1],
                                                         x.cfg_Data.nPauseMAC_Src[2],
                                                         x.cfg_Data.nPauseMAC_Src[3],
                                                         x.cfg_Data.nPauseMAC_Src[4],
                                                         x.cfg_Data.nPauseMAC_Src[5]
                                                         );
#endif
           break;
         case IFX_ETHSW_PORT_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort Id                 = %d\n", x.portcfg.nPortId);
                printf("\tPort Enable             = %d\n", x.portcfg.eEnable);
                printf("\tUnicast Unkown Drop     = %d\n", x.portcfg.bUnicastUnknownDrop);
                printf("\tMulticast Unkown Drop   = %d\n", x.portcfg.bMulticastUnknownDrop);
                printf("\tReserved Packet Drop    = %d\n", x.portcfg.bReservedPacketDrop);
                printf("\tBroadcast Packet Drop   = %d\n", x.portcfg.bBroadcastDrop);
                printf("\tAging                   = %d\n", x.portcfg.bAging);
                printf("\tLearning Mac Port Lock  = %d\n", x.portcfg.bLearningMAC_PortLock);
                printf("\tLearning Limit          = %d\n", x.portcfg.nLearningLimit); 
                printf("\tPort Monitor            = %d\n", x.portcfg.ePortMonitor);
                printf("\tFlow Control            = %d\n", x.portcfg.eFlowCtrl);
#else
                printf("nPortId=%d ", x.portcfg.nPortId);
                printf("eEnable=%d ", x.portcfg.eEnable);
                printf("bUnicastUnknownDrop=%d ", x.portcfg.bUnicastUnknownDrop);
                printf("bMulticastUnknownDrop=%d ", x.portcfg.bMulticastUnknownDrop);
                printf("bReservedPacketDrop=%d ", x.portcfg.bReservedPacketDrop);
                printf("bBroadcastDrop=%d ", x.portcfg.bBroadcastDrop);
                printf("bAging=%d ", x.portcfg.bAging);
                printf("bLearningMAC_PortLock=%d ", x.portcfg.bLearningMAC_PortLock);
                printf("nLearningLimit=%d ", x.portcfg.nLearningLimit); 
                printf("ePortMonitor=%d\n", x.portcfg.ePortMonitor);
                printf("eFlowCtrl=%d\n", x.portcfg.eFlowCtrl);
#endif
           break;
       case IFX_ETHSW_PORT_LINK_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort Id                   = %d\n", x.portlinkcfgGet.nPortId);
                printf("\tForce Port Duplex Mode.   = %d\n", x.portlinkcfgGet.bDuplexForce);
                printf("\tPort Duplex Status.       = %d\n", x.portlinkcfgGet.eDuplex);
                printf("\tForce Link Speed.         = %d\n", x.portlinkcfgGet.bSpeedForce);
                printf("\tPort link speed status    = %d\n", x.portlinkcfgGet.eSpeed);
                printf("\tForce Link                = %d\n", x.portlinkcfgGet.bLinkForce);
                printf("\tForce link status         = %d\n", x.portlinkcfgGet.eLink);
                printf("\tSelected interface mode   = %d\n", x.portlinkcfgGet.eMII_Mode);
                printf("\tSelect if MAC or PHY mode = %d\n", x.portlinkcfgGet.eMII_Type);
                printf("\tInterface clock Mode      = %d\n", x.portlinkcfgGet.eClkMode);
#else
                printf("nPortId=%d ", x.portlinkcfgGet.nPortId);
                printf("bDuplexForce=%d ", x.portlinkcfgGet.bDuplexForce);
                printf("eDuplex=%d ", x.portlinkcfgGet.eDuplex);
                printf("bSpeedForce=%d ", x.portlinkcfgGet.bSpeedForce);
                printf("eSpeed=%d ", x.portlinkcfgGet.eSpeed);
                printf("bLinkForce=%d ", x.portlinkcfgGet.bLinkForce);
                printf("eLink=%d ", x.portlinkcfgGet.eLink);
                printf("eMII_Mode=%d", x.portlinkcfgGet.eMII_Mode);
                printf("eMII_Type=%d", x.portlinkcfgGet.eMII_Type);
                printf("eClkMode=%d\n", x.portlinkcfgGet.eClkMode);
#endif
            break;
       case IFX_ETHSW_PORT_RGMII_CLK_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort Id         = %d\n", x.portRGMII_clkcfg.nPortId);
                printf("\tDelay RX        = %d\n", x.portRGMII_clkcfg.nDelayRx);
                printf("\tDelay TX        = %d\n", x.portRGMII_clkcfg.nDelayTx);
#else
                printf("nPortId=%d ", x.portRGMII_clkcfg.nPortId);
                printf("nDelayRx=%d ", x.portRGMII_clkcfg.nDelayRx);
                printf("nDelayTx=%d\n", x.portRGMII_clkcfg.nDelayTx);
#endif
            break;
#if defined(AR9) || defined(DANUBE) || defined(AMAZON_SE)
         case IFX_PSB6970_REGISTER_GET:
#ifdef VERBOSE_PRINT
                printf("\tnRegAddr = 0x%x\n", x.register_access.nRegAddr);
                printf("\tnData = 0x%x\n", x.register_access.nData);
#else
                printf("nRegAddr=0x%x ", x.register_access.nRegAddr);
                printf("nData=0x%x\n", x.register_access.nData);
#endif
           break;
         case IFX_ETHSW_VERSION_GET:
                printf("\t%s = %s\n", x.Version.cName, x.Version.cVersion);
           break;
         case IFX_ETHSW_CAP_GET:
                printf("\t%s = %d\n", x.cap.cDesc, x.cap.nCap);
           break;
         case IFX_ETHSW_PORT_PHY_ADDR_GET:
#ifdef VERBOSE_PRINT
                printf("\tDevice address on the MDIO interface = %d\n", x.phy_addr.nAddressDev);
#else
                printf("\tnAddressDev = %d\n", x.phy_addr.nAddressDev);
#endif
           break;
         case IFX_ETHSW_PORT_PHY_QUERY:
#ifdef VERBOSE_PRINT
                printf("\tA connected PHY on this port = %d\n", x.phy_Query.bPHY_Present);
#else
                printf("\tbPHY_Present = %d\n", x.phy_Query.bPHY_Present);
#endif
           break;
           case IFX_ETHSW_STP_PORT_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort Id                      = %d\n", x.STP_portCfg.nPortId);
                printf("\tSpanning Tree Protocol state = %d\n", x.STP_portCfg.ePortState);
#else
                printf("\tnPortId                      = %d\n", x.STP_portCfg.nPortId);
                printf("\tePortState                   = %d\n", x.STP_portCfg.ePortState);
#endif
           break;
           case IFX_ETHSW_STP_BPDU_RULE_GET:
#ifdef VERBOSE_PRINT
                printf("\tFilter spanning tree packets = %d\n", x.STP_BPDU_Rule.eForwardPort);
#else
                printf("\teForwardPort                 = %d\n", x.STP_BPDU_Rule.eForwardPort);
#endif
           break;
#if 0
           case IFX_PSB6970_QOS_PORT_SHAPER_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort index                       = %d\n", x.qos_portShapterCfg.nPort);
                printf("\tPriority queue index             = %d\n", x.qos_portShapterCfg.nTrafficClass);
                printf("\tScheduler Type                   = %d\n", x.qos_portShapterCfg.eType);
                printf("\tMaximum average rate [in MBit/s] = %d\n", x.qos_portShapterCfg.nRate);
#else
                printf("\tnPor = %d\n", x.qos_portShapterCfg.nPort);
                printf("\tnTrafficClass = %d\n", x.qos_portShapterCfg.nTrafficClass);
                printf("\teType = %d\n", x.qos_portShapterCfg.eType);
                printf("\tnRate = %d\n", x.qos_portShapterCfg.nRate);
#endif
           break;
#else
           case IFX_PSB6970_QOS_PORT_SHAPER_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort index = %d\n", x.qos_portShapterCfg.nPort);
                printf("\tWFQ Algorithm = %d\n", x.qos_portShapterCfg.eWFQ_Type);
#else
                printf("\tnPort = %d\n", x.qos_portShapterCfg.nPort);
                printf("\teWFQ_Type = %d\n", x.qos_portShapterCfg.eWFQ_Type);
#endif
           break;
           case IFX_PSB6970_QOS_PORT_SHAPER_STRICT_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort index                       = %d\n", x.qos_portShapterStrictCfg.nPort);
                printf("\tPriority queue index             = %d\n", x.qos_portShapterStrictCfg.nTrafficClass);
                printf("\tMaximum average rate [in MBit/s] = %d\n", x.qos_portShapterStrictCfg.nRate);
#else
                printf("\tnPor = %d\n", x.qos_portShapterStrictCfg.nPort);
                printf("\tnTrafficClass = %d\n", x.qos_portShapterStrictCfg.nTrafficClass);
                printf("\tnRate = %d\n", x.qos_portShapterStrictCfg.nRate);
#endif
           break;
           case IFX_PSB6970_QOS_PORT_SHAPER_WFQ_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort index                       = %d\n", x.qos_portShapterWFQ_Cfg.nPort);
                printf("\tPriority queue index             = %d\n", x.qos_portShapterWFQ_Cfg.nTrafficClass);
                printf("\tMaximum average rate [in MBit/s] = %d\n", x.qos_portShapterWFQ_Cfg.nRate);
#else
                printf("\tnPor = %d\n", x.qos_portShapterWFQ_Cfg.nPort);
                printf("\tnTrafficClass = %d\n", x.qos_portShapterWFQ_Cfg.nTrafficClass);
                printf("\tnRate = %d\n", x.qos_portShapterWFQ_Cfg.nRate);
#endif
           break;

#endif
           case IFX_PSB6970_QOS_PORT_POLICER_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort index                       = %d\n", x.qos_portPolicerCfg.nPort);
                printf("\tMaximum average rate [in MBit/s] = %d\n", x.qos_portPolicerCfg.nRate);
#else
                printf("\tnPor = %d\n", x.qos_portPolicerCfg.nPort);
                printf("\tnRate = %d\n", x.qos_portPolicerCfg.nRate);
#endif
           break;
#if 0
           case IFX_ETHSW_MDIO_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tMDIO Speed                   = %d\n", x.mdio_cfg.nMDIO_Speed);
#else
                printf("\nMDIO_Speed                  = %d\n", x.mdio_cfg.nMDIO_Speed);
#endif
           break;
#endif
           case IFX_ETHSW_8021X_PORT_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort number                  = %d\n", x.PNAC_portCfg.nPortId);
                printf("\t802.1x state of the port     = %d\n", x.PNAC_portCfg.eState);
#else
                printf("\tnPortId                      = %d\n", x.PNAC_portCfg.nPortId);
                printf("\teState                       = %d\n", x.PNAC_portCfg.eState);
#endif
           break;
           case IFX_ETHSW_8021X_EAPOL_RULE_GET:
#ifdef VERBOSE_PRINT
                printf("\tEAPOL frames filtering rule  = %d\n", x.PNAC_EAPOL_Rule.eForwardPort);
#else
                printf("\teForwardPort                 = %d\n", x.PNAC_EAPOL_Rule.eForwardPort);
#endif
           break;
           case IFX_PSB6970_QOS_STORM_GET:
#ifdef VERBOSE_PRINT
                printf("\tStorm control for received boardcast packets = %d\n", x.qos_stormCfg.bBroadcast);
                printf("\tStorm control for received multicast packets = %d\n", x.qos_stormCfg.bMulticast);
                printf("\tStorm control for received unicasst packets  = %d\n", x.qos_stormCfg.bUnicast);
                printf("\t10 MBit/s link threshold (the number of the packets received during 50 ms) = %d\n", x.qos_stormCfg.nThreshold10M);
                printf("\t100 MBit/s link threshold (the number of the packets received during 50 ms) = %d\n", x.qos_stormCfg.nThreshold100M);
#else
                printf("\tbStormBroadcast              = %d\n", x.qos_stormCfg.bBroadcast);
                printf("\tbStormMulticast              = %d\n", x.qos_stormCfg.bMulticast);
                printf("\tbStormUnicast                = %d\n", x.qos_stormCfg.bUnicast);
                printf("\tnThreshold10M                = %d\n", x.qos_stormCfg.nThreshold10M);
                printf("\tnThreshold100M               = %d\n", x.qos_stormCfg.nThreshold100M);
#endif
           break;
           case IFX_PSB6970_QOS_MFC_PORT_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort index                                             = %d\n", x.qos_MfcPortCfg.nPort);
                printf("\tUse the UDP/TCP Port MFC priority classification rules = %d\n", x.qos_MfcPortCfg.bPriorityPort);
                printf("\tUse the EtherType MFC priority classification rules    = %d\n", x.qos_MfcPortCfg.bPriorityEtherType);
#else
                printf("\tnPort = %d\n", x.qos_MfcPortCfg.nPort);
                printf("\tbPriorityPort = %d\n", x.qos_MfcPortCfg.bPriorityPort);
                printf("\tbPriorityEtherType = %d\n", x.qos_MfcPortCfg.bPriorityEtherType);
#endif
           break;
           case IFX_PSB6970_QOS_MFC_ENTRY_READ:
                print_mfc_filter_table();
        break;
#endif // AR9

#ifdef VR9
         case IFX_FLOW_REGISTER_GET:
#ifdef VERBOSE_PRINT
                printf("\tnRegAddr = 0x%x\n", x.register_access.nRegAddr);
                printf("\tnData = 0x%x\n", x.register_access.nData);
#else
                printf("nRegAddr=0x%x ", x.register_access.nRegAddr);
                printf("nData=0x%x\n", x.register_access.nData);
#endif
             break;
         case IFX_ETHSW_QOS_METER_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tThe meter shaper Enable or Disable = %d\n", x.qos_metercfg.bEnable);
                printf("\tMeter index = %d\n", x.qos_metercfg.nMeterId);
                printf("\tCommitted Burst Size = %d\n", x.qos_metercfg.nCbs);
                printf("\tExcess Burst Size = %d\n", x.qos_metercfg.nEbs);
                printf("\tRate[kbit/s] = %d\n", x.qos_metercfg.nRate);
#else
                printf("bEnable=%d ", x.qos_metercfg.bEnable);
                printf("nMeterId=%d\n", x.qos_metercfg.nMeterId);
                printf("nCbs=%d ", x.qos_metercfg.nCbs);
                printf("nEbs=%d ", x.qos_metercfg.nEbs);
                printf("nRate=%d\n", x.qos_metercfg.nRate);
#endif
             break;
         case IFX_ETHSW_QOS_METER_PORT_GET:
#ifdef VERBOSE_PRINT
               MeterPort_TABLE_PRINT();
#else
                printf("nMeterId = 0x%x ", x.qos_meterport.nMeterId);
                printf("eDir= 0x%x ", x.qos_meterport.eDir);
                printf("nPortIngressId= 0x%x ", x.qos_meterport.nPortIngressId);
                printf("nPortEgressId= 0x%x\n", x.qos_meterport.nPortEgressId);
#endif
             break;
         case IFX_ETHSW_QOS_WRED_QUEUE_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("QoS queue index = 0x%x\n", x.qos_wredqueuecfg.nQueueId);
 //               printf("Drop Probability Profile = 0x%x\n", x.qos_wredqueuecfg.eProfile);
                printf("WRED Red Threshold Min = 0x%x\n", x.qos_wredqueuecfg.nRed_Min);
                printf("WRED Red Threshold Max = 0x%x\n", x.qos_wredqueuecfg.nRed_Max);
                printf("WRED Yellow Threshold Min = 0x%x\n", x.qos_wredqueuecfg.nYellow_Min);
                printf("WRED Yellow Threshold Max = 0x%x\n", x.qos_wredqueuecfg.nYellow_Max);
                printf("WRED Green Threshold Min = 0x%x\n", x.qos_wredqueuecfg.nGreen_Min);
                printf("WRED Green Threshold Max = 0x%x\n", x.qos_wredqueuecfg.nGreen_Max);
#else
                printf("nQueueId= 0x%x ", x.qos_wredqueuecfg.nQueueId);
 //               printf("eProfile= 0x%x ", x.qos_wredqueuecfg.eProfile);
                printf("nRed_Min= 0x%x ", x.qos_wredqueuecfg.nRed_Min);
                printf("nRed_Max= 0x%x ", x.qos_wredqueuecfg.nRed_Max);
                printf("nYellow_Min= 0x%x ", x.qos_wredqueuecfg.nYellow_Min);
                printf("nYellow_Max= 0x%x ", x.qos_wredqueuecfg.nYellow_Max);
                printf("nGreen_Min= 0x%x ", x.qos_wredqueuecfg.nGreen_Min);
                printf("nGreen_Max= 0x%x\n", x.qos_wredqueuecfg.nGreen_Max);
#endif
             break;
         case IFX_ETHSW_QOS_SCHEDULER_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tQoS queue index = %d\n", x.qos_schedulecfg.nQueueId);
                printf("\tScheduler Type  = 0x%x\n", x.qos_schedulecfg.eType);
                printf("\tRatio = 0x%x\n", x.qos_schedulecfg.nWeight);
#else
                printf("nQueueId=0x%x ",x.qos_schedulecfg.nQueueId);
                printf("eType=0x%x\n", x.qos_schedulecfg.eType);
                printf("Ratio= 0x%x\n", x.qos_schedulecfg.nWeight);
#endif
             break;
         case IFX_ETHSW_QOS_SHAPER_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tRate shaper index = 0x%x\n", x.qos_shappercfg.nRateShaperId);
                printf("\tEnable or Disable the rate shaperx = 0x%x\n", x.qos_shappercfg.bEnable);
                printf("\tCommitted Burst Size = %d\n", x.qos_shappercfg.nCbs);
                printf("\tRate [kbit/s]= %d\n", x.qos_shappercfg.nRate);
#else
                printf("nRateShaperId= 0x%x\n", x.qos_shappercfg.nRateShaperId);
                printf("bEnable= 0x%x ", x.qos_shappercfg.bEnable);
                printf("nCbs=%d ", x.qos_shappercfg.nCbs);
                printf("nRate=%d ", x.qos_shappercfg.nRate);
#endif
             break;
         case IFX_ETHSW_QOS_STORM_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tMeter index = 0x%x\n", x.qos_stormcfg.nMeterId);
                printf("\tbroadcast traffic= 0x%x\n", x.qos_stormcfg.bBroadcast);
                printf("\tmulticast traffic = 0x%x\n", x.qos_stormcfg.bMulticast);
                printf("\tunknown unicast traffic= 0x%x\n", x.qos_stormcfg.bUnknownUnicast);
#else
                printf("nMeterId=0x%x\n", x.qos_stormcfg.nMeterId);
                printf("bBroadcast=0x%x ", x.qos_stormcfg.bBroadcast);
                printf("bMulticast=0x%x ", x.qos_stormcfg.bMulticast);
                printf("bUnknownUnicast=0x%x ", x.qos_stormcfg.bUnknownUnicast);
#endif
             break;
         case IFX_ETHSW_8021X_PORT_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort number                  = %d\n", x.PNAC_portCfg.nPortId);
                printf("\t802.1x state of the port     = %d\n", x.PNAC_portCfg.eState);
#else
                printf("\tnPortId                      = %d\n", x.PNAC_portCfg.nPortId);
                printf("\teState                       = %d\n", x.PNAC_portCfg.eState);
#endif
             break;
         case IFX_ETHSW_8021X_EAPOL_RULE_GET:
#ifdef VERBOSE_PRINT
                printf("\t8021.x forwarding port rule       = %d\n", x.PNAC_EAPOL_Rule.eForwardPort);
                printf("\tTarget port for forwarded packets = %d\n", x.PNAC_EAPOL_Rule.nForwardPortId);
#else
                printf("\teForwardPort = %d", x.PNAC_EAPOL_Rule.eForwardPort);
                printf("\tnForwardPortId=%d\n", x.PNAC_EAPOL_Rule.nForwardPortId);
#endif
             break;
         case IFX_ETHSW_PORT_PHY_QUERY:
#ifdef VERBOSE_PRINT
                printf("\tA connected PHY on this port = %s\n",(x.phy_Query.bPHY_Present == 1)?"YES":"No");
#else
                printf("\tbPHY_Present = %d\n", x.phy_Query.bPHY_Present);
#endif
             break;
         case IFX_ETHSW_STP_PORT_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tPort Id                      = %d\n", x.STP_portCfg.nPortId);
                printf("\tSpanning Tree Protocol state = %d\n", x.STP_portCfg.ePortState);
#else
                printf("\tnPortId                      = %d\n", x.STP_portCfg.nPortId);
                printf("\tePortState                   = %d\n", x.STP_portCfg.ePortState);
#endif
             break;
         case IFX_ETHSW_STP_BPDU_RULE_GET:
#ifdef VERBOSE_PRINT
                printf("\tFilter spanning tree packets      = %d\n", x.STP_BPDU_Rule.eForwardPort);
                printf("\tTarget port for forwarded packets = %d\n", x.STP_BPDU_Rule.nForwardPortId);
#else
                printf("\teForwardPort = %d", x.STP_BPDU_Rule.eForwardPort);
                printf("\tnForwardPortId=%d\n", x.STP_BPDU_Rule.nForwardPortId);
#endif
             break;
         case IFX_ETHSW_VERSION_GET:
                printf("\t%s = %s\n", x.Version.cName, x.Version.cVersion);
             break;
         case IFX_ETHSW_CAP_GET:
                CapGet();
            break;
         case IFX_ETHSW_PORT_PHY_ADDR_GET:
#ifdef VERBOSE_PRINT
                printf("\tDevice address on the MDIO interface = %d\n", x.phy_addr.nAddressDev);
#else
                printf("\tnAddressDev = %d\n", x.phy_addr.nAddressDev);
#endif
            break;
       case IFX_FLOW_RMON_EXTEND_GET:
            for (i=0;i<24;i++)
               printf("\tRMON Counter [%d] = %d \n",i, x.RMON_ExtendGet.nTrafficFlowCnt[i]);

            break;
       case IFX_ETHSW_CPU_PORT_EXTEND_CFG_GET:
            printf("\tAdd Ethernet layer-2 header = %d\n",x.portextendcfg.eHeaderAdd);
            printf("\tRemove Ethernet layer-2 header = %d\n",x.portextendcfg.bHeaderRemove);
            printf("\tHeader data\n");
            printf("\t  Source MAC     : %02x:%02x:%02x:%02x:%02x:%02x\n",
                                        x.portextendcfg.sHeader.nMAC_Src[0],
                                        x.portextendcfg.sHeader.nMAC_Src[1],
                                        x.portextendcfg.sHeader.nMAC_Src[2],
                                        x.portextendcfg.sHeader.nMAC_Src[3],
                                        x.portextendcfg.sHeader.nMAC_Src[4],
                                        x.portextendcfg.sHeader.nMAC_Src[5]);
            printf("\t  Destination MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                                        x.portextendcfg.sHeader.nMAC_Dst[0],
                                        x.portextendcfg.sHeader.nMAC_Dst[1],
                                        x.portextendcfg.sHeader.nMAC_Dst[2],
                                        x.portextendcfg.sHeader.nMAC_Dst[3],
                                        x.portextendcfg.sHeader.nMAC_Dst[4],
                                        x.portextendcfg.sHeader.nMAC_Dst[5]);
            printf("\t  Packet EtherType Field = %d\n",x.portextendcfg.sHeader.nEthertype);
            printf("\t  VLAN Tag Priority Field = %d\n",x.portextendcfg.sHeader.nVLAN_Prio);
            printf("\t  VLAN Tag CFI = %d\n",x.portextendcfg.sHeader.nVLAN_CFI);
            printf("\t  VLAN Tag VLAN ID = %d\n",x.portextendcfg.sHeader.nVLAN_ID);
            printf("\tPAUSE frames coming = %d\n",x.portextendcfg.ePauseCtrl);
            printf("\tRemove the CRC = %d\n",x.portextendcfg.bFcsRemove);
            printf("\tPort map of WAN Ethernet switch ports = %d\n",x.portextendcfg.nWAN_Ports);
           break;
         case IFX_ETHSW_QOS_WRED_CFG_GET:
#ifdef VERBOSE_PRINT
                printf("\tDrop Probability Profile   = %d\n", x.qos_wredcfg.eProfile);
                printf("\tWRED Red Threshold Min     = 0x%x\n", x.qos_wredcfg.nRed_Min);
                printf("\tWRED Red Threshold Max     = 0x%x\n", x.qos_wredcfg.nRed_Max);
                printf("\tWRED Yellow Threshold Min  = 0x%x\n", x.qos_wredcfg.nYellow_Min);
                printf("\tWRED Yellow Threshold Max  = 0x%x\n", x.qos_wredcfg.nYellow_Max);
                printf("\tWRED Green Threshold Min   = 0x%x\n", x.qos_wredcfg.nGreen_Min);
                printf("\tWRED Green Threshold Max   = 0x%x\n", x.qos_wredcfg.nGreen_Max);
#else
                printf("\teProfile=%d ", x.qos_wredcfg.eProfile);
                printf("\tnRed_Min=0x%x ", x.qos_wredcfg.nRed_Min);
                printf("\tnRed_Max=0x%x ", x.qos_wredcfg.nRed_Max);
                printf("\tnYellow_Min=0x%x ", x.qos_wredcfg.nYellow_Min);
                printf("\tnYellow_Max=0x%x ", x.qos_wredcfg.nYellow_Max);
                printf("\tnGreen_Min=0x%x ", x.qos_wredcfg.nGreen_Min);
                printf("\tnGreen_Max=0x%x\n", x.qos_wredcfg.nGreen_Max);
#endif
            break;
         case IFX_FLOW_PCE_RULE_READ:
#ifdef VERBOSE_PRINT
                printf("\t== Pattern Table:\n");
                printf("\tnIndex                               = %d\n", x.pce_rule.pattern.nIndex);
                printf("\tIndex is used(Enabled)/ (Disabled)   = %d\n", x.pce_rule.pattern.bEnable);
                printf("\tPort ID used                         = %d\n", x.pce_rule.pattern.bPortIdEnable);
                printf("\tPort ID                              = %d\n", x.pce_rule.pattern.nPortId);
                printf("\tDSCP value used                      = %d\n", x.pce_rule.pattern.bDSCP_Enable);
                printf("\tDSCP value                           = %d\n", x.pce_rule.pattern.nDSCP);
                printf("\tPCP value used                       = %d\n", x.pce_rule.pattern.bPCP_Enable);
                printf("\tPCP value                            = %d\n", x.pce_rule.pattern.nPCP);
                printf("\tPacket length used                   = %d\n", x.pce_rule.pattern.bPktLngEnable);
                printf("\tPacket length                        = %d\n", x.pce_rule.pattern.nPktLng);
                printf("\tPacket length Range                  = %d\n", x.pce_rule.pattern.nPktLngRange);
                printf("\tDestination MAC address used         = %d\n", x.pce_rule.pattern.bMAC_DstEnable);
                printf("\tDestination MAC address              = %02x:%02x:%02x:%02x:%02x:%02x\n",
                x.pce_rule.pattern.nMAC_Dst[0], x.pce_rule.pattern.nMAC_Dst[1], x.pce_rule.pattern.nMAC_Dst[2],
                x.pce_rule.pattern.nMAC_Dst[3],x.pce_rule.pattern.nMAC_Dst[4],x.pce_rule.pattern.nMAC_Dst[5]);
                printf("\tDestination MAC address mask         = 0x%x\n", x.pce_rule.pattern.nMAC_DstMask);
                printf("\tSource MAC address used              = %d\n", x.pce_rule.pattern.bMAC_SrcEnable);
                printf("\tSource MAC address                   = %02x:%02x:%02x:%02x:%02x:%02x\n",
                x.pce_rule.pattern.nMAC_Src[0], x.pce_rule.pattern.nMAC_Src[1], x.pce_rule.pattern.nMAC_Src[2],
                x.pce_rule.pattern.nMAC_Src[3], x.pce_rule.pattern.nMAC_Src[4], x.pce_rule.pattern.nMAC_Src[5]);
                printf("\tSource MAC address mask              = 0x%x\n", x.pce_rule.pattern.nMAC_SrcMask);
                printf("\tMSB Application field used           = %d\n", x.pce_rule.pattern.bAppDataMSB_Enable);
                printf("\tMSB Application field                = %d\n", x.pce_rule.pattern.nAppDataMSB);
                printf("\tMSB Application mask/range selection = %d\n", x.pce_rule.pattern.bAppMaskRangeMSB_Select);
                printf("\tMSB Application mask/range           = %d\n", x.pce_rule.pattern.nAppMaskRangeMSB);
                printf("\tLSB Application used                 = %d\n", x.pce_rule.pattern.bAppDataLSB_Enable);
                printf("\tLSB Application field                = %d\n", x.pce_rule.pattern.nAppDataLSB);
                printf("\tLSB Application mask/range selection = %d\n", x.pce_rule.pattern.bAppMaskRangeLSB_Select);
                printf("\tLSB Application mask/range           = %d\n", x.pce_rule.pattern.nAppMaskRangeLSB);
                printf("\tDIP Selection.                       = %d\n", x.pce_rule.pattern.eDstIP_Select);
                printf("\tDIP                                  = %03d.%03d.%03d.%03d\n",
                ((x.pce_rule.pattern.nDstIP.nIPv4 >> 24 ) & 0xFF),
                ((x.pce_rule.pattern.nDstIP.nIPv4 >> 16 ) & 0xFF),
                ((x.pce_rule.pattern.nDstIP.nIPv4 >> 8 ) & 0xFF),
                (x.pce_rule.pattern.nDstIP.nIPv4 & 0xFF));
                printf("\tDIP Nibble Mask                      = 0x%x\n", x.pce_rule.pattern.nDstIP_Mask);
                printf("\tSIP Selection.                       = %d\n", x.pce_rule.pattern.eSrcIP_Select);
                printf("\tSIP                                  = %03d.%03d.%03d.%03d\n",
                ((x.pce_rule.pattern.nSrcIP.nIPv4 >> 24) & 0xFF),
                ((x.pce_rule.pattern.nSrcIP.nIPv4 >> 16) & 0xFF),
                ((x.pce_rule.pattern.nSrcIP.nIPv4 >> 8) & 0xFF),
                (x.pce_rule.pattern.nSrcIP.nIPv4 & 0xFF));
                printf("\tSIP Nibble Mask                      = 0x%x\n", x.pce_rule.pattern.nSrcIP_Mask);
                printf("\tEthertype used                       = %d\n", x.pce_rule.pattern.bEtherTypeEnable);
                printf("\tEthertype                            = %d\n", x.pce_rule.pattern.nEtherType);
                printf("\tEthertype Mask                       = 0x%x\n", x.pce_rule.pattern.nEtherTypeMask);
                printf("\tIP protocol used                     = %d\n", x.pce_rule.pattern.bProtocolEnable);
                printf("\tIP protocol                          = %d\n", x.pce_rule.pattern.nProtocol);
                printf("\tIP protocol Mask                     = 0x%x\n", x.pce_rule.pattern.nProtocolMask);
                printf("\tPPPoE used                           = %d\n", x.pce_rule.pattern.bSessionIdEnable);
                printf("\tPPPoE                                = %d\n", x.pce_rule.pattern.nSessionId);
                printf("\tVLAN used                            = %d\n", x.pce_rule.pattern.bVid);
                printf("\tVLAN                                 = %d\n", x.pce_rule.pattern.nVid);
                printf("\t== Action Table:\n");
                printf("\tAction Traffic class Group.          = %d\n", x.pce_rule.action.eTrafficClassAction);
                printf("\tAlternative Traffic class            = %d\n", x.pce_rule.action.nTrafficClassAlternate);
                printf("\tAction IGMP Snooping Group.          = %d\n", x.pce_rule.action.eSnoopingTypeAction);
                printf("\tAction Learning Group.               = %d\n", x.pce_rule.action.eLearningAction);
                printf("\tAction Interrupt Group.              = %d\n", x.pce_rule.action.eIrqAction);
                printf("\tAction Cross State Group.            = %d\n", x.pce_rule.action.eCrossStateAction);
                printf("\tAction Critical Frames Group.        = %d\n", x.pce_rule.action.eCritFrameAction);
                printf("\tAction Timestamp Group.              = %d\n", x.pce_rule.action.eTimestampAction);
                printf("\tAction Forwarding Group.             = %d\n", x.pce_rule.action.ePortMapAction);
                printf("\tTarget portmap for forwarded packets = %d\n", x.pce_rule.action.nForwardPortMap);
                printf("\tAction Remarking Group.              = %d\n", x.pce_rule.action.bRemarkAction);
                printf("\tPCP remarking enable                 = %d\n", x.pce_rule.action.bRemarkPCP);
                printf("\tDSCP remarking enable                = %d\n", x.pce_rule.action.bRemarkDSCP);
                printf("\tClass remarking enable               = %d\n", x.pce_rule.action.bRemarkClass);
                printf("\tAction Meter Group.                  = %d\n", x.pce_rule.action.eMeterAction);
                printf("\tMeter ID                             = %d\n", x.pce_rule.action.nMeterId);
                printf("\tAction RMON Group.                   = %d\n", x.pce_rule.action.bRMON_Action);
                printf("\tCounter ID                           = %d\n", x.pce_rule.action.nRMON_Id);
                printf("\tAction VLAN Group.                   = %d\n", x.pce_rule.action.eVLAN_Action);
                printf("\tAlternative VLAN Id.                 = %d\n", x.pce_rule.action.nVLAN_Id);
                printf("\tAction Cross VLAN Group.             = %d\n", x.pce_rule.action.eVLAN_CrossAction);

#else
                printf("\tbEnable=0x%x", x.pce_rule.pattern.bEnable);
                printf("\tbPortIdEnable=0x%x", x.pce_rule.pattern.bPortIdEnable);
                printf("\tnPortId=0x%x", x.pce_rule.pattern.nPortId);
                printf("\tbDSCP_Enable=0x%x", x.pce_rule.pattern.bDSCP_Enable);
                printf("\tnDSCP=0x%x", x.pce_rule.pattern.nDSCP);
                printf("\tbPCP_Enable=0x%x", x.pce_rule.pattern.bPCP_Enable);
                printf("\tnPCP=0x%x", x.pce_rule.pattern.nPCP);
                printf("\tbPktLngEnable=0x%x", x.pce_rule.pattern.bPktLngEnable);
                printf("\tnPktLng=0x%x", x.pce_rule.pattern.nPktLng);
                printf("\tnPktLngRange=0x%x", x.pce_rule.pattern.nPktLngRange);
                printf("\tbMAC_DstEnable=0x%x", x.pce_rule.pattern.bMAC_DstEnable);
                printf("\tnMAC_Dst[6]=0x%x", x.pce_rule.pattern.nMAC_Dst[6]);
                printf("\tnMAC_DstMask=0x%x", x.pce_rule.pattern.nMAC_DstMask);
                printf("\tbMAC_SrcEnable=0x%x", x.pce_rule.pattern.bMAC_SrcEnable);
                printf("\tnMAC_Src[6]=0x%x", x.pce_rule.pattern.nMAC_Src[6]);
                printf("\tnMAC_SrcMask=0x%x", x.pce_rule.pattern.nMAC_SrcMask);
                printf("\tbAppDataMSB_Enable=0x%x", x.pce_rule.pattern.bAppDataMSB_Enable);
                printf("\tnAppDataMSB=0x%x", x.pce_rule.pattern.nAppDataMSB);
                printf("\tbAppMaskRangeMSB_Select=0x%x", x.pce_rule.pattern.bAppMaskRangeMSB_Select);
                printf("\tnAppMaskRangeMSB=0x%x", x.pce_rule.pattern.nAppMaskRangeMSB);
                printf("\tbAppDataLSB_Enable=0x%x", x.pce_rule.pattern.bAppDataLSB_Enable);
                printf("\tnAppDataLSB=0x%x", x.pce_rule.pattern.nAppDataLSB);
                printf("\tbAppMaskRangeLSB_Select=0x%x", x.pce_rule.pattern.bAppMaskRangeLSB_Select);
                printf("\tnAppMaskRangeLSB=0x%x", x.pce_rule.pattern.nAppMaskRangeLSB);
                printf("\teDstIP_Select=0x%x", x.pce_rule.pattern.eDstIP_Select);
                printf("\tnDstIP=0x%x", x.pce_rule.pattern.nDstIP);
                printf("\tnDstIP_Mask=0x%x", x.pce_rule.pattern.nDstIP_Mask);
                printf("\teSrcIP_Select=0x%x", x.pce_rule.pattern.eSrcIP_Select);
                printf("\tnSrcIP=0x%x", x.pce_rule.pattern.nSrcIP);
                printf("\tnSrcIP_Mask=0x%x", x.pce_rule.pattern.nSrcIP_Mask);
                printf("\tbEtherTypeEnable=0x%x", x.pce_rule.pattern.bEtherTypeEnable);
                printf("\tnEtherType=0x%x", x.pce_rule.pattern.nEtherType);
                printf("\tnEtherTypeMask=0x%x", x.pce_rule.pattern.nEtherTypeMask);
                printf("\tbProtocolEnable=0x%x", x.pce_rule.pattern.bProtocolEnable);
                printf("\tnProtocol=0x%x", x.pce_rule.pattern.nProtocol);
                printf("\tnProtocolMask=0x%x", x.pce_rule.pattern.nProtocolMask);
                printf("\tbSessionIdEnable=0x%x", x.pce_rule.pattern.bSessionIdEnable);
                printf("\tnSessionId=0x%x", x.pce_rule.pattern.nSessionId);
                printf("\tbVid=0x%x", x.pce_rule.pattern.bVid);
                printf("\tnVid=0x%x\n", x.pce_rule.pattern.nVid);
                printf("\tnIndex =0x%x", x.pce_rule.action.nIndex);
                printf("\teTrafficClassAction=0x%x", x.pce_rule.action.eTrafficClassAction);
                printf("\tnTrafficClassAlternate=0x%x", x.pce_rule.action.nTrafficClassAlternate);
                printf("\teSnoopingTypeAction=0x%x", x.pce_rule.action.eSnoopingTypeAction);
                printf("\teLearningAction=0x%x", x.pce_rule.action.eLearningAction);
                printf("\teIrqAction=0x%x", x.pce_rule.action.eIrqAction);
                printf("\teCrossStateAction=0x%x", x.pce_rule.action.eCrossStateAction);
                printf("\teCritFrameAction=0x%x", x.pce_rule.action.eCritFrameAction);
                printf("\tePortMapAction=0x%x", x.pce_rule.action.ePortMapAction);
                printf("\tnForwardPortMap=0x%x", x.pce_rule.action.nForwardPortMap);
                printf("\tbRemarkAction=0x%x", x.pce_rule.action.bRemarkAction);
                printf("\tbRemarkPCP=0x%x", x.pce_rule.action.bRemarkPCP);

                printf("\tbRemarkDSCP=0x%x", x.pce_rule.action.bRemarkDSCP);
                printf("\tbRemarkClass=0x%x", x.pce_rule.action.bRemarkClass);
                printf("\teMeterAction=0x%x", x.pce_rule.action.eMeterAction);
                printf("\tnMeterId=0x%x", x.pce_rule.action.nMeterId);
                printf("\tbRMON_Action=0x%x", x.pce_rule.action.bRMON_Action);
                printf("\tnRMON_Id=0x%x", x.pce_rule.action.nRMON_Id);
                printf("\teVLAN_Action=0x%x", x.pce_rule.action.eVLAN_Action);
                printf("\tnVLAN_Id=0x%x", x.pce_rule.action.nVLAN_Id);
                printf("\teVLAN_CrossAction=0x%x", x.pce_rule.action.eVLAN_CrossAction);
#endif
           break;
#endif  //VR9
           default:
                break;

   }

   return retval;
}


/**
 * \brief Main function of the switch IOCTL interface
 * This function first opens the character device, checks
 * the input command, converts the input parameters and then
 * builds the ioctl command and executes it
 *
 * \param argc Count of the input commandline parameters
 *
 * \param argv Input commandline parameters
 * \return
 *   IFX_SUCCESS on successful execution of cmd
 *   IFX_ERROR on failure
 */
int main (int argc, char **argv)
{
   int retval=-1;

   /* open char device for ioctl communication */
   if((switch_fd=open(switch_dev, O_RDONLY))==-1)
   {
      printf("Error opening char device %s\n", switch_dev);
      return (IFX_ERROR);
   }

   if( (argc==1) ||
      ((argc==2) && ((strncmp(argv[1],"-h",2) == 0) || (strncmp(argv[1],"--help",6) == 0))) )
   {
      print_commands();
      close(switch_fd);
      return (IFX_SUCCESS);
   }

   /* check command passed as argv[1] from command line */
   if( (cmd_index=check_command( argv[1] )) == -1 )
   {
      printf("Invalid command passed\n");
      close(switch_fd);
      return (IFX_ERROR);
   }

   /*
    * check number of parameters passed as command line parameters
    */
   if( strlen(commands[cmd_index].param) != (argc-2) )
   {
      printf("Invalid number of parameters: is %d, should be %d\n",
         (argc-2), strlen(commands[cmd_index].param));
      print_command( cmd_index );
      close(switch_fd);
      return (IFX_ERROR);
   }

   /* convert parameter strings included in argv[2]... to cmd_arg array */
   if( convert_parameters( &argv[2], argc-2) == -1 )
   {
      printf("Invalid parameters passed\n");
      print_command(cmd_index);
      close(switch_fd);
      return (IFX_ERROR);
   }

   /* finally build and send the ioctl command */
   if(commands[cmd_index].ioctl_command != 0xFFFFFFFF)
      retval = build_ioctl_command( switch_fd, commands[cmd_index].ioctl_command );
   else
      retval = execute_user_command( commands[cmd_index].name );

   close(switch_fd);
   return (retval);

}
