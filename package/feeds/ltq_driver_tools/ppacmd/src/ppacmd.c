/******************************************************************************
**
** FILE NAME  : ppacmd.c
** PROJECT    : PPA Configuration Utility
** MODULES    : Packet Acceleration
**
** DATE     : 10 JUN 2008
** AUTHOR     : Mark Smith
** DESCRIPTION  : PPA (Routing Acceleration) User Configuration Utility
** COPYRIGHT  :        Copyright (c) 2009
**              Lantiq Deutschland GmbH
**           Am Campeon 3; 85579 Neubiberg, Germany
**
**   For licensing information, see the file 'LICENSE' in the root folder of
**   this software module.
**
** HISTORY
** $Date        $Author                $Comment
** 10 DEC 2012  Manamohan Shetty       Added the support for RTP,MIB mode and CAPWAP 
**                                     Features 
**
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <ctype.h>
#include <linux/autoconf.h>


#include "ppacmd.h"
#include "ppacmd_autotest.h"

int enable_debug = 0;
char *debug_enable_file="/var/ppa_gdb_enable";


static int g_output_to_file = 0;
static int g_all = 0;
static char g_output_filename[PPACMD_MAX_FILENAME]= {0};
#define DEFAULT_OUTPUT_FILENAME   "/var/tmp.dat"


static void ppa_print_help(void);
static int ppa_parse_simple_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata);
static void ppa_print_get_qstatus_cmd(PPA_CMD_DATA *pdata);
static int ppa_get_qstatus_do_cmd(PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata);


/*
int SaveDataToFile(char *FileName, char *data, int len )
return 0: means sucessful
return -1: fail to save data to file
*/
int SaveDataToFile(char *FileName, char *data, int len )
{
    FILE *pFile;

    if( data == NULL ) return  -1;
    
    if( strcmp(FileName, "null") == 0 ) return 0; //don't save, but remove console print also no file saving

    if( FileName == NULL || strlen(FileName) == 0 )
        FileName = DEFAULT_OUTPUT_FILENAME;

    //IFX_PPACMD_DBG("SaveDataToFile  %s ( bytes: %d from buffer %p)\n", FileName, len, data );

    pFile = fopen(FileName, "wb");

    if( pFile == NULL )
    {
        IFX_PPACMD_PRINT("SaveDataToFile: fail to open file %s\n", FileName );
        return -1;
    }

    fwrite (data , 1 , len , pFile );
    fflush(pFile);
    fclose (pFile);

    return 0;
}

static void print_session_flags( uint32_t flags)
{
    static const char *str_flag[] =
    {
        "IS_REPLY",                 //  0x00000001
        "Reserved",
        "SESSION_IS_TCP",
        "STAMPING",
        "ADDED_IN_HW",              //  0x00000010
        "NOT_ACCEL_FOR_MGM",
        "STATIC",
        "DROP",
        "VALID_NAT_IP",             //  0x00000100
        "VALID_NAT_PORT",
        "VALID_NAT_SNAT",
        "NOT_ACCELABLE",
        "VALID_VLAN_INS",           //  0x00001000
        "VALID_VLAN_RM",
        "SESSION_VALID_OUT_VLAN_INS",
        "SESSION_VALID_OUT_VLAN_RM",
        "VALID_PPPOE",              //  0x00010000
        "VALID_NEW_SRC_MAC",
        "VALID_MTU",
        "VALID_NEW_DSCP",
        "SESSION_VALID_DSLWAN_QID", //  0x00100000
        "SESSION_TX_ITF_IPOA",
        "SESSION_TX_ITF_PPPOA",
        "Reserved",
        "SRC_MAC_DROP_EN",          //  0x01000000
        "SESSION_TUNNEL_6RD",
        "SESSION_TUNNEL_DSLITE",
        "Reserved",
        "LAN_ENTRY",                //  0x10000000
        "WAN_ENTRY",
        "IPV6",
        "Reserved",
    };

    int flag;
    unsigned long bit;
    int i;


    flag = 0;
    for ( bit = 1, i = 0; bit; bit <<= 1, i++ )
    {
        if ( (flags & bit) )
        {
            if ( flag++ )
                IFX_PPACMD_PRINT( "|");
            IFX_PPACMD_PRINT( str_flag[i]);
            //IFX_PPACMD_PRINT( " ");
        }
    }

}
int stricmp(const char * p1, const char * p2)
{
    int c1, c2;

    while ( *p1 && *p2 )
    {
        c1 = *p1 >= 'A' && *p1 <= 'Z' ? *p1 + 'a' - 'A' : *p1;
        c2 = *p2 >= 'A' && *p2 <= 'Z' ? *p2 + 'a' - 'A' : *p2;
        if ( (c1 -= c2) )
            return c1;
        p1++;
        p2++;
    }

    return *p1 - *p2;
}

int strincmp(const char *p1, const char *p2, int n)
{
    int c1 = 0, c2;

    while ( n && *p1 && *p2 )
    {
        c1 = *p1 >= 'A' && *p1 <= 'Z' ? *p1 + 'a' - 'A' : *p1;
        c2 = *p2 >= 'A' && *p2 <= 'Z' ? *p2 + 'a' - 'A' : *p2;
        if ( (c1 -= c2) )
            return c1;
        p1++;
        p2++;
        n--;
    }

    return n ? *p1 - *p2 : c1;
}


/*Note:
  if type == STRING_TYPE_INTEGER, directly return the value
  if type == STRING_TYPE_IP, return value definition as below:
                       IP_NON_VALID(0): means non-valid IP
                       IP_VALID_V4(1): means valid IPV4 address
                       IP_VALID_V6(2) :means valid IPV6 address
*/

uint32_t str_convert(int type, const char *nptr, void *buff )
{
    uint32_t res;
    if( nptr == NULL )
    {
        IFX_PPACMD_PRINT("str_convert: nptr is NULL ??\n");
        return 0;
    }
    if( type == STRING_TYPE_IP )
    {
        if( (res = inet_pton(AF_INET, nptr, buff) ) == 0 )
        {
            if( (res = inet_pton(AF_INET6, nptr, buff)) == 0 )
            {
                return IP_NON_VALID;
            }
            else return IP_VALID_V6;
        }
        else return IP_VALID_V4;

        return 0;
    }
    else if( type == STRING_TYPE_INTEGER )
    {
        if( strlen(nptr) >= 2 && nptr[0] == '0' && ( nptr[1] == 'x' || nptr[1] == 'X') ) /*hex value start with 0x */
        {
            return strtoul(nptr, NULL, 16);
        }
        else  /*by default it is decimal value */
        {
            return strtoul(nptr, NULL, 10);
        }
    }
    else
    {
        IFX_PPACMD_PRINT("str_convert: wrong type parameter(%d)\n", type);
        return 0;
    }
}


static int is_digital_value(char *s)
{
    int i;
    
    if( !s ) return 0;

     if( (strlen(s) > 2 ) && ( s[0] == '0')  && ( s[1] == 'x') )
    { //hex
        for(i=2; i<strlen(s); i++ )
        { //
            if( ( s[i] >='0' &&  s[i] <='9' )  || ( s[i] >='a' &&  s[i] <='f' ) || ( s[i] >='A' &&  s[i] <='F' ) )
                continue;
            else 
                return 0;
        }
    }
    else
    { //normal value
        for(i=0; i<strlen(s); i++ )
            if( s[i] >='0' &&  s[i] <='9' )  continue;
            else
                return 0; 
    }
    return 1;
}

/*
===========================================================================================
   ppa_do_ioctl_cmd

===========================================================================================
*/
static int ppa_do_ioctl_cmd(int ioctl_cmd, void *data)
{
    int ret = PPA_CMD_OK;
    int fd  = 0;

    if ((fd = open (PPA_DEVICE, O_RDWR)) < 0)
    {
        IFX_PPACMD_PRINT ("\n [%s] : open PPA device (%s) failed. (errno=%d)\n", __FUNCTION__, PPA_DEVICE, errno);
        ret = PPA_CMD_ERR;
    }
    else
    {
        if (ioctl (fd, ioctl_cmd, data) < 0)
        {
            IFX_PPACMD_PRINT ("\n [%s] : ioctl failed for NR %d. (errno=%d(system error:%s))\n", __FUNCTION__, _IOC_NR(ioctl_cmd), errno, strerror(errno));
            ret = PPA_CMD_ERR;
        }
        close (fd);
    }
    
    return ret;
}

/*
====================================================================================
   Input conversion functions
   These sunctions convert input strings to the appropriate data types for
   the ioctl commands.
===================================================================================
*/
static void stomac(char *s,unsigned char mac_addr[])
{
    unsigned int mac[PPA_ETH_ALEN];

    sscanf(s,"%2x:%2x:%2x:%2x:%2x:%2x",&mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]);

    mac_addr[5] = mac[5];
    mac_addr[4] = mac[4];
    mac_addr[3] = mac[3];
    mac_addr[2] = mac[2];
    mac_addr[1] = mac[1];
    mac_addr[0] = mac[0];
    return;
}

/*
====================================================================================
   Generic option sets
   These option sets are shared among several commands.
===================================================================================
*/

static const char ppa_no_short_opts[] = "h";
static struct option ppa_no_long_opts[] =
{
    { 0,0,0,0 }
};

static const char ppa_if_short_opts[] = "i:fl:h";
static struct option ppa_if_long_opts[] =
{
    {"interface", required_argument,  NULL, 'i'},
    {"force", no_argument,  NULL, 'f'}, //-f is used for addlan and addwan only
    {"lower", required_argument,  NULL, 'l'}, //-l is used for manually configure its lower interface
    { 0,0,0,0 }
};

static const char ppa_mac_short_opts[] = "m:h";
static const struct option ppa_mac_long_opts[] =
{
    {"macaddr",   required_argument,  NULL, 'm'},
    { 0,0,0,0 }
};

static const char ppa_if_mac_short_opts[] = "i:m:h";
static const struct option ppa_if_mac_long_opts[] =
{
    {"interface", required_argument,  NULL, 'i'},
    {"macaddr",   required_argument,  NULL, 'm'},
    { 0,0,0,0 }
};

static const char ppa_output_short_opts[] = "o:h";

static const char ppa_if_output_short_opts[] = "i:o:h";

#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
static const char ppa_init_short_opts[] = "f:l:w:b:m:c:n:t:i:h";
#else
static const char ppa_init_short_opts[] = "f:l:w:b:m:c:n:t:h";
#endif
static const struct option ppa_init_long_opts[] =
{
    {"file",   required_argument,  NULL, 'f'},
    {"lan",    required_argument,  NULL, 'l'},
    {"wan",    required_argument,  NULL, 'w'},
    {"bridge",   required_argument,  NULL, 'b'},
    {"multicast",required_argument,  NULL, 'm'},
    {"minimal-hit",    required_argument,  NULL, 'n'},
    {"checksum", required_argument,  NULL, 'c'},
#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
    {"mib-mode", required_argument,  NULL, 'i'},
#endif
    {"help",   no_argument,    NULL, 'h'},
    { 0,0,0,0 }
};

static int ppa_do_cmd(PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata)
{
    return ppa_do_ioctl_cmd(pcmd->ioctl_cmd,pdata);
}

/*
====================================================================================
   command:   init
   description: Initialize the Packet Processing Acceleration Module
   options:   None
====================================================================================
*/

typedef enum
{
    INIT_CMD_INVALID = -1,
    INIT_CMD_COMMENT,
    INIT_CMD_SECTION,
    INIT_CMD_END,
    INIT_CMD_L3_VERIFY,
    INIT_CMD_L4_VERIFY,
    INIT_CMD_DROP_ON_ERROR,
    INIT_CMD_DROP_ON_UNICAST_MISS,
    INIT_CMD_DROP_ON_MULTICAST_MISS,
    INIT_CMD_NUM_ENTRIES,
    INIT_CMD_MULTICAST_ENTRIES,
    INIT_CMD_BRIDGE_ENTRIES,
    INIT_CMD_TCP_HITS_TO_ADD,
    INIT_CMD_UDP_HITS_TO_ADD,
    INIT_CMD_INTERFACE
} INIT_CMD;

const char *cfg_names[] =
{
    "comment",
    "section",
    "end",
    "ip-header-check",
    "tcp-udp-header-check",
    "drop-on-error",
    "unicast-drop-on-miss",
    "multicast-drop-on-miss",
    "max-unicast-sessions",
    "max-multicast-sessions",
    "max-bridging-sessions",
    "tcp-threshold",
    "udp-threshold",
    "interface"
};

typedef enum
{
    SECTION_NONE,
    SECTION_WAN,
    SECTION_LAN
} SECTION_NAME;

typedef enum
{
    ERR_MULTIPLE_SECTIONS,
    ERR_INVALID_SECTION_NAME,
    ERR_INVALID_COMMAND,
    ERR_NOT_IN_SECTION,
    ERR_IN_SECTION,
    ERR_INVALID_RANGE
} INIT_FILE_ERR;

static void ppa_print_init_help(int summary)
{
    if( summary )
    {
#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
        
        IFX_PPACMD_PRINT("init [-f <filename>] [-l <lan_num>] [-w <wan_num>] [-m <mc_num>] [-b <br_num>] [-c enable |disable checksum checking>] [ -n minimal-hit ] [-t mtu] [-i <mib-mode> ]\n");

#else
        IFX_PPACMD_PRINT("init [-f <filename>] [-l <lan_num>] [-w <wan_num>] [-m <mc_num>] [-b <br_num>] [-c enable |disable checksum checking>] [ -n minimal-hit ] [-t mtu]\n");
#endif
        IFX_PPACMD_PRINT("    -l/w/m/b: to set maximum LAN/WAN/Multicast/Bridge Acceeration entries\n");
        IFX_PPACMD_PRINT("    -c: to enable/disable checksum checking\n");
        IFX_PPACMD_PRINT("    -n: to specify minimal hit before doing acceleration\n");
        IFX_PPACMD_PRINT("    -t: to specify MTU size, default is 1500 bytes. Its value should be not over 1530-14 now\n");
#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
        IFX_PPACMD_PRINT("    -i: to set mibmode 0-Session MIB in terms of Byte,1- in terms of Packet\n");
#endif
        IFX_PPACMD_PRINT("    -f: note, if -f option is used, then no need to use other options\n");
        
    }
    else
        IFX_PPACMD_PRINT("init [-f <filename>]\n");
    return;
}

static INIT_CMD parse_init_config_line(char *buf, char **val)
{
    char *p_cmd, *p_end;
    int ndx;
    INIT_CMD ret_cmd = INIT_CMD_INVALID;
    int eol_seen = 0;

    if (buf[0] == '#')
        return INIT_CMD_COMMENT;

    p_cmd = buf;
    while (*p_cmd != '\n' && isspace(*p_cmd))   // skip leading white space while checking for eol
        p_cmd++;
    if (*p_cmd == '\n')
        return INIT_CMD_COMMENT;         // empty line
    p_end = p_cmd;                // null terminate the command
    while (!isspace(*p_end))
        p_end++;
    if (*p_end == '\n')
        eol_seen = 1;
    *p_end = '\0';

    for (ndx = 0; ndx < (sizeof(cfg_names)/sizeof(char *)); ndx++)
    {
        if ( strcasecmp(cfg_names[ndx], p_cmd ) == 0)
        {
            // return the following string if present
            if (!eol_seen)
            {
                p_cmd = p_end + 1;
                while (*p_cmd != '\n' && isspace(*p_cmd))
                    p_cmd++;
                p_end = p_cmd;
                while (!isspace(*p_end))
                    p_end++;
                *p_end = '\0';
                *val = p_cmd;
            }
            else
            {
                *val = NULL;   // no parameter present
            }
            ret_cmd = ndx;
            break;
        }
    }
    return ret_cmd;
}

static int parse_init_config_file(char *filename, PPA_CMD_INIT_INFO *pinfo)
{
    FILE *fd;
    char cmd_line[128];
    char *val;
    int linenum = 0;
    INIT_CMD cmd;
    int is_enable, num_entries, num_hits;
    SECTION_NAME curr_section = SECTION_NONE;
    PPA_VERIFY_CHECKS *pchecks;
    int seen_wan_section = 0, seen_lan_section = 0;
    //int if_index = 0;
    int num_wanifs = 0, num_lanifs = 0;
    INIT_FILE_ERR err;

    fd = fopen(filename,"r");
    if (fd != NULL)
    {
        while ( fgets(cmd_line, 128, fd) != NULL)
        {
            linenum++;
            cmd = parse_init_config_line(cmd_line, &val);
            switch(cmd)
            {
            case INIT_CMD_COMMENT:
                break;

            case INIT_CMD_SECTION:
                if (!strcasecmp("wan", val))
                {
                    if(seen_wan_section)
                    {
                        err = ERR_MULTIPLE_SECTIONS;
                        goto parse_error;
                    }
                    curr_section = SECTION_WAN;
                }
                else if (!strcasecmp("lan", val))
                {
                    if (seen_lan_section)
                    {
                        err = ERR_MULTIPLE_SECTIONS;
                        goto parse_error;
                    }
                    curr_section = SECTION_LAN;
                }
                else
                {
                    err = ERR_INVALID_SECTION_NAME;
                    goto parse_error;
                }
                break;

            case INIT_CMD_END:
                if (curr_section == SECTION_NONE)
                {
                    err = ERR_NOT_IN_SECTION;
                    goto parse_error;
                }
                if (curr_section == SECTION_WAN)
                {
                    pinfo->num_wanifs = num_wanifs;
                    seen_wan_section = 1;
                }
                else
                {
                    pinfo->num_lanifs = num_lanifs;
                    seen_lan_section = 1;
                }
                curr_section = SECTION_NONE;
                break;

            case INIT_CMD_L3_VERIFY:
            case INIT_CMD_L4_VERIFY:
            case INIT_CMD_DROP_ON_ERROR:
            case INIT_CMD_DROP_ON_UNICAST_MISS:
            case INIT_CMD_DROP_ON_MULTICAST_MISS:
                if (!strcasecmp("enable", val))
                    is_enable = 1;
                else if (!strcasecmp("disable", val))
                    is_enable = 0;
                else
                {
                    err = ERR_INVALID_COMMAND;
                    goto parse_error;
                }
                if (curr_section == SECTION_NONE)
                {
                    err = ERR_NOT_IN_SECTION;
                    goto parse_error;
                }
                if (curr_section == SECTION_LAN)
                    pchecks = &(pinfo->lan_rx_checks);
                else
                    pchecks = &(pinfo->wan_rx_checks);
                switch(cmd)
                {
                case INIT_CMD_L3_VERIFY:
                    pchecks->f_ip_verify = is_enable;
                    break;
                case INIT_CMD_L4_VERIFY:
                    pchecks->f_tcp_udp_verify = is_enable;
                    break;
                case INIT_CMD_DROP_ON_ERROR:
                    pchecks->f_tcp_udp_err_drop = is_enable;
                    break;
                case INIT_CMD_DROP_ON_UNICAST_MISS:
                    pchecks->f_drop_on_no_hit = is_enable;
                    break;
                case INIT_CMD_DROP_ON_MULTICAST_MISS:
                    pchecks->f_mc_drop_on_no_hit = is_enable;
                    break;
                default:
                    err = ERR_INVALID_COMMAND;
                    goto parse_error;
                }
                break;

            case INIT_CMD_NUM_ENTRIES:
            case INIT_CMD_BRIDGE_ENTRIES:
            case INIT_CMD_MULTICAST_ENTRIES:
                num_entries = atoi(val);
                if (num_entries > 1000 || num_entries < 0)
                {
                    err = ERR_INVALID_RANGE;
                    goto parse_error;
                }
                if (cmd == INIT_CMD_NUM_ENTRIES)
                {
                    if (curr_section == SECTION_WAN)
                        pinfo->max_wan_source_entries = num_entries;
                    else if (curr_section == SECTION_LAN)
                        pinfo->max_lan_source_entries = num_entries;
                    else
                    {
                        err = ERR_NOT_IN_SECTION;
                        goto parse_error;
                    }
                }
                else
                {
                    if (curr_section != SECTION_NONE)
                    {
                        err = ERR_IN_SECTION;
                        goto parse_error;
                    }
                    if (cmd == INIT_CMD_BRIDGE_ENTRIES)
                        pinfo->max_bridging_entries = num_entries;
                    else
                        pinfo->max_mc_entries = num_entries;
                }
                break;

            case INIT_CMD_TCP_HITS_TO_ADD:
            case INIT_CMD_UDP_HITS_TO_ADD:
                num_hits = atoi(val);
                if (num_hits < 0)
                {
                    err = ERR_INVALID_COMMAND;
                    goto parse_error;
                }
                if (cmd == INIT_CMD_TCP_HITS_TO_ADD)
                    pinfo->add_requires_min_hits = num_hits;
                else
                    pinfo->add_requires_min_hits = num_hits;
                break;

            case INIT_CMD_INTERFACE:
                if (curr_section == SECTION_NONE)
                {
                    err = ERR_NOT_IN_SECTION;
                    goto parse_error;
                }
                if (curr_section == SECTION_WAN)
                {
                    if ( num_wanifs < sizeof(pinfo->p_wanifs) / sizeof(pinfo->p_wanifs[0]) )
                    {
                        strncpy(pinfo->p_wanifs[num_wanifs].ifname, val, PPA_IF_NAME_SIZE);
                        pinfo->p_wanifs[num_wanifs].if_flags = 0;
                        num_wanifs++;
                    }
                }
                else if (curr_section == SECTION_LAN)
                {
                    if ( num_wanifs < sizeof(pinfo->p_lanifs) / sizeof(pinfo->p_lanifs[0]) )
                    {
                        strncpy(pinfo->p_lanifs[num_lanifs].ifname, val, PPA_IF_NAME_SIZE);
                        pinfo->p_lanifs[num_lanifs].if_flags = PPA_F_LAN_IF;
                        num_lanifs++;
                    }
                }
                break;

            default:
                err = ERR_INVALID_COMMAND;
                goto parse_error;
            }
        }
    }

    if( fd != NULL )
    {
        fclose(fd);
        fd = NULL;
    }
    return PPA_CMD_OK;

    // error messages
parse_error:

    switch(err)
    {
    case ERR_MULTIPLE_SECTIONS:
        IFX_PPACMD_PRINT("error: multiple section definitions - line %d\n", linenum);
        break;
    case ERR_INVALID_SECTION_NAME:
        IFX_PPACMD_PRINT("error: invalid section name - line %d\n", linenum);
        break;
    case ERR_INVALID_COMMAND:
        IFX_PPACMD_PRINT("error: invalid command - line %d\n", linenum);
        break;
    case ERR_NOT_IN_SECTION:
        IFX_PPACMD_PRINT("error: command not within valid section - line %d\n", linenum);
        break;
    case ERR_IN_SECTION:
        IFX_PPACMD_PRINT("error: command within section - line %d\n", linenum);
        break;
    case ERR_INVALID_RANGE:
        IFX_PPACMD_PRINT("error: parameter outside allowed range - line %d\n", linenum);
        break;
    }
    if( fd != NULL )
    {
        fclose(fd);
        fd = NULL;
    }
    return PPA_CMD_ERR;
}

static int ppa_parse_init_cmd(PPA_CMD_OPTS *popts,PPA_CMD_DATA *pdata)
{
    unsigned int i;
    PPA_CMD_INIT_INFO *pinfo = &pdata->init_info;
    PPA_CMD_VERSION_INFO ver;
    PPA_CMD_MAX_ENTRY_INFO max_entries;
    uint32_t f_incorrect_fw = 1;

#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
    PPA_CMD_MIB_MODE_INFO    var_mib_mode;   /*!< MIB mode configuration parameter */  
#endif
    // Default PPA Settings
    pinfo->lan_rx_checks.f_ip_verify     = 0;
    pinfo->lan_rx_checks.f_tcp_udp_verify  = 0;
    pinfo->lan_rx_checks.f_tcp_udp_err_drop  = 0;
    pinfo->lan_rx_checks.f_drop_on_no_hit  = 0;
    pinfo->lan_rx_checks.f_mc_drop_on_no_hit = 0;

    pinfo->wan_rx_checks.f_ip_verify     = 0;
    pinfo->wan_rx_checks.f_tcp_udp_verify  = 0;
    pinfo->wan_rx_checks.f_tcp_udp_err_drop  = 0;
    pinfo->wan_rx_checks.f_drop_on_no_hit  = 0;
    pinfo->wan_rx_checks.f_mc_drop_on_no_hit = 0;

    pinfo->num_lanifs = 0;
    memset(pinfo->p_lanifs, 0, sizeof(pinfo->p_lanifs));
    pinfo->num_wanifs = 0;
    memset(pinfo->p_wanifs, 0, sizeof(pinfo->p_wanifs));

    pinfo->max_lan_source_entries  = 0;
    pinfo->max_wan_source_entries  = 0;
    pinfo->max_mc_entries      = 0;
    pinfo->max_bridging_entries  = 0;
    pinfo->add_requires_min_hits   = 0;

    if( ppa_do_ioctl_cmd(PPA_CMD_GET_VERSION, &ver ) != PPA_CMD_OK )
        return -EIO;

    if( ppa_do_ioctl_cmd(PPA_CMD_GET_MAX_ENTRY, &max_entries) != PPA_CMD_OK )
    {
        return -EIO;
    }

    pinfo->max_lan_source_entries = max_entries.entries.max_lan_entries;
    pinfo->max_wan_source_entries = max_entries.entries.max_wan_entries;
    pinfo->max_mc_entries     = max_entries.entries.max_mc_entries;
    pinfo->max_bridging_entries   = max_entries.entries.max_bridging_entries;

    if( ver.ppe_fw_ver[0].family ||  ver.ppe_fw_ver[1].family ) f_incorrect_fw = 0;
    else f_incorrect_fw = 1;
    if( f_incorrect_fw )
    {
        IFX_PPACMD_PRINT("Wrong PPE FW version:family-%u, itf-%u\n", (unsigned int)ver.ppe_fw_ver[0].family, (unsigned int)ver.ppe_fw_ver[0].itf);
        return   PPA_CMD_ERR;
    }

    // Override any default setting from configuration file (if specified)
    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'f':
            if ( parse_init_config_file( popts->optarg, &pdata->init_info) )
            {
                IFX_PPACMD_PRINT("%s: error reading PPA configuration file: %s\n", PPA_CMD_NAME, popts->optarg);
                return PPA_CMD_ERR;
            }
            break;

        case 'l':
            if(  str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL ) < pinfo->max_lan_source_entries )
                pinfo->max_lan_source_entries = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL );
            break;

        case 'w':
            if( str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL ) < pinfo->max_wan_source_entries )
                pinfo->max_wan_source_entries = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL );
            break;

        case 'm':
            if( str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL ) < pinfo->max_mc_entries )
                pinfo->max_mc_entries = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL );
            break;

        case 'b':
            if( str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL ) < pinfo->max_bridging_entries)
                pinfo->max_bridging_entries = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL );
            break;

        case 'n':
            pinfo->add_requires_min_hits = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL );
            break;

#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
        case 'i':
            //var_mib_mode.mib_mode = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL );
            pinfo->mib_mode = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL );

           /* if( ppa_do_ioctl_cmd(PPA_CMD_SET_MIB_MODE, &var_mib_mode) != PPA_CMD_OK )
            {
                return -EIO;
            }*/
            break;
#endif
        case 't':
            pinfo->mtu= str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL );
            break;

        case 'c':
            if( strcmp(popts->optarg, "enable" ) == 0)
            {
                pinfo->lan_rx_checks.f_ip_verify     = 1;
                pinfo->lan_rx_checks.f_tcp_udp_verify  = 1;

                pinfo->wan_rx_checks.f_ip_verify     = 1;
                pinfo->wan_rx_checks.f_tcp_udp_verify  = 1;
            }
            else if( strcmp(popts->optarg, "disable" ) == 0)
            {
                pinfo->lan_rx_checks.f_ip_verify     = 0;
                pinfo->lan_rx_checks.f_tcp_udp_verify  = 0;

                pinfo->wan_rx_checks.f_ip_verify     = 0;
                pinfo->wan_rx_checks.f_tcp_udp_verify  = 0;
            }
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    IFX_PPACMD_DBG("LAN SETTINGS\n");
    IFX_PPACMD_DBG("     IP Verify: %s\n", pinfo->lan_rx_checks.f_ip_verify ? "enable":"disable");
    IFX_PPACMD_DBG("    UDP Verify: %s\n", pinfo->lan_rx_checks.f_tcp_udp_verify ? "enable":"disable");
    IFX_PPACMD_DBG("TCP/UDP Error Drop: %s\n", pinfo->lan_rx_checks.f_tcp_udp_err_drop ? "enable":"disable");
    IFX_PPACMD_DBG("     DROP on HIT: %s\n", pinfo->lan_rx_checks.f_drop_on_no_hit ? "enable":"disable");
    IFX_PPACMD_DBG("  MC Drop on HIT: %s\n", pinfo->lan_rx_checks.f_mc_drop_on_no_hit ? "enable":"disable");

    IFX_PPACMD_DBG("WAN SETTINGS\n");
    IFX_PPACMD_DBG("     IP Verify: %s\n", pinfo->wan_rx_checks.f_ip_verify ? "enable":"disable");
    IFX_PPACMD_DBG("    UDP Verify: %s\n", pinfo->wan_rx_checks.f_tcp_udp_verify ? "enable":"disable");
    IFX_PPACMD_DBG("TCP/UDP Error Drop: %s\n", pinfo->wan_rx_checks.f_tcp_udp_err_drop ? "enable":"disable");
    IFX_PPACMD_DBG("     DROP on HIT: %s\n", pinfo->wan_rx_checks.f_drop_on_no_hit ? "enable":"disable");
    IFX_PPACMD_DBG("  MC Drop on HIT: %s\n", pinfo->wan_rx_checks.f_mc_drop_on_no_hit ? "enable":"disable");

    IFX_PPACMD_DBG("INTERFACES\n");
    IFX_PPACMD_DBG("Number of LAN IF: %lu\n", pinfo->num_lanifs);
    for ( i = 0; i < pinfo->num_lanifs; i++ )
        IFX_PPACMD_DBG("  %s (%08lX)\n", pinfo->p_lanifs[i].ifname, pinfo->p_lanifs[i].if_flags);
    IFX_PPACMD_DBG("Number of WAN IF: %lu\n",pinfo->num_wanifs);
    for ( i = 0; i < pinfo->num_wanifs; i++ )
        IFX_PPACMD_DBG("  %s %08lX)\n", pinfo->p_wanifs[i].ifname, pinfo->p_wanifs[i].if_flags);

    IFX_PPACMD_DBG("OTHER\n");
    IFX_PPACMD_DBG("   Max. LAN Entries: %lu\n", pinfo->max_lan_source_entries);
    IFX_PPACMD_DBG("   Max. WAN Entries: %lu\n", pinfo->max_wan_source_entries);
    IFX_PPACMD_DBG("   Max. MC Entries: %lu\n", pinfo->max_mc_entries);
    IFX_PPACMD_DBG("   Max. Bridge Entries: %lu\n", pinfo->max_bridging_entries);
    IFX_PPACMD_DBG("   Min. Hits: %lu\n", pinfo->add_requires_min_hits);
    IFX_PPACMD_DBG("   mtu: %lu\n", pinfo->mtu);

    return PPA_CMD_OK;
}

static void ppa_print_init_fake_cmd(PPA_CMD_DATA *pdata)
{
    /* By default, we will enable ppa LAN/WAN acceleratation */
    PPA_CMD_ENABLE_INFO  enable_info;

    memset( &enable_info, 0, sizeof(enable_info)) ;

    enable_info.lan_rx_ppa_enable = 1;
    enable_info.wan_rx_ppa_enable = 1;

    if( ppa_do_ioctl_cmd(PPA_CMD_ENABLE, &enable_info ) != PPA_CMD_OK )
    {
        IFX_PPACMD_PRINT("ppacmd control to enable lan/wan failed\n");
        return ;
    }
}



/*
====================================================================================
   command:   exit
   description: Remove the Packet Processing Acceleration Module
   options:   None
====================================================================================
*/
static void ppa_print_exit_help(int summary)
{
    IFX_PPACMD_PRINT("exit\n");
    return;
}

static int ppa_parse_exit_cmd(PPA_CMD_OPTS *popts,PPA_CMD_DATA *pdata)
{
    if (popts->opt != 0)
        return PPA_CMD_ERR;

    IFX_PPACMD_DBG("EXIT COMMAND\n");

    return PPA_CMD_OK;
}

/*
====================================================================================
   command:   control
   description: Enable and Disable Packet Processing Acceleration for WAN and/or LAN
        interfaces.
   options:   Enable and/or Disable parameters
====================================================================================
*/

static const struct option ppa_control_long_opts[] =
{
    {"enable-lan",  no_argument, NULL, OPT_ENABLE_LAN},
    {"disable-lan", no_argument, NULL, OPT_DISABLE_LAN},
    {"enable-wan",  no_argument, NULL, OPT_ENABLE_WAN},
    {"disable-wan", no_argument, NULL, OPT_DISABLE_WAN},
    { 0,0,0,0 }
};

static void ppa_print_control_help(int summary)
{
    IFX_PPACMD_PRINT("control {--enable-lan|--disable-lan} {--enable-wan|--disable-wan} \n");
    return;
}

static int ppa_parse_control_cmd(PPA_CMD_OPTS *popts,PPA_CMD_DATA *pdata)
{
    unsigned int lan_opt = 0, wan_opt = 0;

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case OPT_ENABLE_LAN:
            pdata->ena_info.lan_rx_ppa_enable = 1;
            lan_opt++;
            break;

        case OPT_DISABLE_LAN:
            pdata->ena_info.lan_rx_ppa_enable = 0;
            lan_opt++;
            break;

        case OPT_ENABLE_WAN:
            pdata->ena_info.wan_rx_ppa_enable = 1;
            wan_opt++;
            break;

        case OPT_DISABLE_WAN:
            pdata->ena_info.wan_rx_ppa_enable = 0;
            wan_opt++;
            break;
        }
        popts++;
    }

    /* Allow only one of the parameters for LAN or WAN to be specified */
    if (wan_opt > 1 || lan_opt > 1)
        return PPA_CMD_ERR;

    if (wan_opt ==0 &&  lan_opt == 0) /*sgh add: without this checking, all lan/wan acceleration will be disabled if user run command "ppacmd control" without any parameter */
        return PPA_CMD_ERR;

    IFX_PPACMD_DBG("PPA CONTROL: LAN = %s   WAN = %s\n", pdata->ena_info.lan_rx_ppa_enable ? "enable" : "disable",
            pdata->ena_info.wan_rx_ppa_enable ? "enable" : "disable");

    return PPA_CMD_OK;
}

/*
====================================================================================
   command:   status
   description: Display Packet Processing Acceleration status for WAN and/or LAN
        interfaces.

   options:   None
====================================================================================
*/

static void ppa_print_status_help(int summary)
{
    IFX_PPACMD_PRINT("status [-o outfile] \n");
    return;
}

static int ppa_parse_status_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int out_opt = 0;

    while(popts->opt)
    {
        if (popts->opt == 'o')
        {
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            out_opt++;
        }
        else
        {
            return PPA_CMD_ERR;
        }
    }

    if (out_opt > 1)
        return PPA_CMD_ERR;

    IFX_PPACMD_DBG("PPA STATUS\n");

    return PPA_CMD_OK;
}

static void ppa_print_status(PPA_CMD_DATA *pdata)
{
    if( pdata->ena_info.flags == 0 )
        IFX_PPACMD_PRINT("PPA not initialized yet\n");
    else
    {
        PPA_CMD_MAX_ENTRY_INFO max_entries;
#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
        PPA_CMD_MIB_MODE_INFO mode_info;
#endif
#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
        PPA_CMD_DATA cmd_info;
#endif
        
        memset( &max_entries, 0, sizeof(max_entries) );
        IFX_PPACMD_PRINT("  LAN Acceleration: %s.\n", pdata->ena_info.lan_rx_ppa_enable ? "enabled": "disabled");
        IFX_PPACMD_PRINT("  WAN Acceleration: %s.\n", pdata->ena_info.wan_rx_ppa_enable ? "enabled": "disabled");

        if( ppa_do_ioctl_cmd(PPA_CMD_GET_MAX_ENTRY, &max_entries) == PPA_CMD_OK )
        {
            IFX_PPACMD_PRINT("  LAN max entries:%d(Collision:%d)\n", (unsigned int)max_entries.entries.max_lan_entries, (unsigned int)max_entries.entries.max_lan_collision_entries);
            IFX_PPACMD_PRINT("  WAN max entries:%d(Collision:%d)\n", (unsigned int)max_entries.entries.max_wan_entries, (unsigned int)max_entries.entries.max_wan_collision_entries);
            IFX_PPACMD_PRINT("  LAN hash index number:%d, bucket number per index:%d)\n", (unsigned int)max_entries.entries.max_lan_hash_index_num, (unsigned int)max_entries.entries.max_lan_hash_bucket_num);
            IFX_PPACMD_PRINT("  WAN hash index number:%d, bucket number per index:%d)\n", (unsigned int)max_entries.entries.max_wan_hash_index_num, (unsigned int)max_entries.entries.max_wan_hash_bucket_num);
            IFX_PPACMD_PRINT("  MC max entries:%d\n", (unsigned int)max_entries.entries.max_mc_entries);
            IFX_PPACMD_PRINT("  Bridge max entries:%d\n", (unsigned int)max_entries.entries.max_bridging_entries);
            IFX_PPACMD_PRINT("  IPv6 address max entries:%d\n", (unsigned int)max_entries.entries.max_ipv6_addr_entries);
            IFX_PPACMD_PRINT("  PPE FW max queue:%d\n", (unsigned int)max_entries.entries.max_fw_queue);
            IFX_PPACMD_PRINT("  6RD max entries:%d\n", (unsigned int)max_entries.entries.max_6rd_entries);
            IFX_PPACMD_PRINT("  MF Flow max entries:%d\n", (unsigned int)max_entries.entries.max_mf_flow);
        }

#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
        if( ppa_do_ioctl_cmd(PPA_CMD_GET_MIB_MODE, &mode_info) == PPA_CMD_OK )
        {
            if(mode_info.mib_mode == 1)
                  IFX_PPACMD_PRINT("  Unicast/Multicast Session Mib in Packet\n");
            else
                  IFX_PPACMD_PRINT("  Unicast/Multicast Session Mib in Byte\n");
        }
#endif

#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG

         //get number of capwap tunnel 
         cmd_info.count_info.flag = 0;
         if( ppa_do_ioctl_cmd(PPA_CMD_GET_MAXCOUNT_CAPWAP, &cmd_info ) != PPA_CMD_OK )
            return;
         IFX_PPACMD_PRINT("  Maximum CAPWAP tunnel: %d\n",cmd_info.count_info.count);
    
#endif 
    }
}

/*
====================================================================================
   command:   setmac
   description: Set Ethernet MAC address
   options:   None
====================================================================================
*/

static void ppa_print_set_mac_help(int summary)
{
    IFX_PPACMD_PRINT("setmac -i <ifname> -m <macaddr> \n");
    return;
}

static int ppa_parse_set_mac_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int if_opt = 0, mac_opt = 0;

    while(popts->opt)
    {
        if (popts->opt == 'i')
        {
            strncpy(pdata->if_mac.ifname,popts->optarg,PPA_IF_NAME_SIZE);
            pdata->if_mac.ifname[PPA_IF_NAME_SIZE-1] = 0;
            if_opt++;
        }
        else if (popts->opt == 'm')
        {
            stomac(popts->optarg,pdata->if_mac.mac);
            mac_opt++;
        }
        else
        {
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if (mac_opt != 1 || if_opt != 1)
        return PPA_CMD_ERR;

    IFX_PPACMD_DBG("PPA SET MAC: %s  =  %02x:%02x:%02x:%02x:%02x:%02x\n", pdata->if_mac.ifname,
            pdata->if_mac.mac[0],
            pdata->if_mac.mac[1],
            pdata->if_mac.mac[2],
            pdata->if_mac.mac[3],
            pdata->if_mac.mac[4],
            pdata->if_mac.mac[5]);
    return PPA_CMD_OK;
}

/*
====================================================================================
  Generic add/delete/get interface functions.
  The add/delete WAN interface commands share the same data structures and command
  options so they are combined into one set of functions and shared by each.
====================================================================================
*/

static int ppa_parse_add_del_if_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    int  opt = 0;

    memset( pdata->if_info.ifname_lower, 0, sizeof(pdata->if_info.ifname_lower) );
    while (popts->opt)
    {
        if (popts->opt == 'i')
        {
            strncpy(pdata->if_info.ifname,popts->optarg,PPA_IF_NAME_SIZE);
            pdata->if_info.ifname[PPA_IF_NAME_SIZE-1] = 0;
            opt ++;
        }
        else if (popts->opt == 'f')
        {
            pdata->if_info.force_wanitf_flag=1;
        }
        else if (popts->opt == 'l')
        {
            strncpy(pdata->if_info.ifname_lower,popts->optarg,PPA_IF_NAME_SIZE-1);
            pdata->if_info.ifname_lower[PPA_IF_NAME_SIZE-1] = 0;   
        }
        
        popts++;
    }

    if( opt != 1 )
        return PPA_CMD_ERR;

    IFX_PPACMD_DBG("PPA ADD/DEL IF: %s with flag=%x\n", pdata->if_info.ifname, pdata->if_info.force_wanitf_flag);

    return PPA_CMD_OK;
}

static void ppa_print_lan_netif_cmd(PPA_CMD_DATA *pdata)
{
    int i;
    if( !g_output_to_file )
    {
        for(i=0; i<pdata->all_if_info.num_ifinfos; i++ )
            IFX_PPACMD_PRINT("[%2d] %15s with acc_rx/acc_tx %llu:%llu\n", i, pdata->all_if_info.ifinfo[i].ifname, pdata->all_if_info.ifinfo[i].acc_rx, pdata->all_if_info.ifinfo[i].acc_tx );
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->all_if_info), sizeof(pdata->all_if_info) );
    }

}
static void ppa_print_get_lan_netif_cmd(PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
        IFX_PPACMD_PRINT("LAN IF: ---\n");
    ppa_print_lan_netif_cmd(pdata);
}

static void ppa_print_get_wan_netif_cmd(PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
        IFX_PPACMD_PRINT("WAN IF: ---\n");
    ppa_print_lan_netif_cmd(pdata);
}

/*
====================================================================================
   command:   addwan
        delwan
        getwan
   description: Add WAN interface to PPA
   options:
====================================================================================
*/

static void ppa_print_add_wan_help(int summary)
{
    IFX_PPACMD_PRINT("addwan -i <ifname>\n");
    if( summary )
    {
       IFX_PPACMD_PRINT("addwan -i <ifname> -f\n");
       IFX_PPACMD_PRINT("    Note:  -f is used to force change WAN interface in PPE FW level. Be careful to use it !!\n");
       IFX_PPACMD_PRINT("           -l is used to manually configure its lower interface in case auto-searching failed !!\n");
    }
    return;
}

static void ppa_print_del_wan_help(int summary)
{
    IFX_PPACMD_PRINT("delwan -i <ifname>\n");
    return;
}

static void ppa_print_get_wan_help(int summary)
{
    IFX_PPACMD_PRINT("getwan [-o outfile]\n");
    return;
}

/*
====================================================================================
   command:    addlan
         dellan
         getlan
   description:
   options:
====================================================================================
*/

static void ppa_print_add_lan_help(int summary)
{
    IFX_PPACMD_PRINT("addlan -i <ifname>\n");
    if( summary )
    {
        
        IFX_PPACMD_PRINT("addlan -i <ifname> -f\n");
        IFX_PPACMD_PRINT("    Note:  -f is used to force change LAN interface in PPE FW level. Be careful to use it !!\n");
        IFX_PPACMD_PRINT("           -l is used to manually configure its lower interface in case auto-searching failed !!\n");
    }
    return;
}

static void ppa_print_del_lan_help(int summary)
{
    IFX_PPACMD_PRINT("dellan -i <ifname>\n");
    return;
}

static void ppa_print_get_lan_help(int summary)
{
    IFX_PPACMD_PRINT("getlan [ -o outfile ]\n");
    return;
}

/*
====================================================================================
   command:   getmac
   description: Get Ethernet MAC address
   options:   interface for MAC address
====================================================================================
*/

static void ppa_print_get_mac_help(int summary)
{
    IFX_PPACMD_PRINT("getmac -i <ifname> [-o outfile] \n");
    return;
}

static int ppa_parse_get_mac_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int if_opt = 0, out_opt = 0;

    while (popts->opt)
    {
        if (popts->opt == 'i')
        {
            strncpy(pdata->if_info.ifname,popts->optarg,PPA_IF_NAME_SIZE);
            pdata->if_info.ifname[PPA_IF_NAME_SIZE-1] = 0;
            if_opt++;
        }
        else if (popts->opt == 'o')
        {
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            out_opt++;
        }
        else
        {
            return PPA_CMD_ERR;
        }
        popts++;
    }

    // Each parameter must be specified just once.
    if (out_opt  > 1 || if_opt != 1)
        return PPA_CMD_ERR;

    IFX_PPACMD_DBG("PPA GET MAC: %s\n", pdata->if_info.ifname);

    return PPA_CMD_OK;
}

static void ppa_print_get_mac_cmd(PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
    {
        IFX_PPACMD_PRINT("%02x:%02x:%02x:%02x:%02x:%02x\n", pdata->if_mac.mac[0],pdata->if_mac.mac[1],pdata->if_mac.mac[2],
                         pdata->if_mac.mac[3],pdata->if_mac.mac[4],pdata->if_mac.mac[5] );
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->if_mac), sizeof(pdata->if_mac) );
    }
}

/*
====================================================================================
   command:   addbr
   description:
   options:
====================================================================================
*/

static void ppa_add_mac_entry_help(int summary)
{
    IFX_PPACMD_PRINT("addbr -m <macaddr> -i <ifname> \n");
    return;
}

static int ppa_parse_add_mac_entry_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int if_opt = 0, mac_opt = 0;

    while (popts->opt)
    {
        if (popts->opt == 'i')
        {
            strncpy(pdata->mac_entry.ifname,popts->optarg,PPA_IF_NAME_SIZE);
            pdata->mac_entry.ifname[PPA_IF_NAME_SIZE-1] = 0;
            if_opt++;
        }
        else if (popts->opt == 'm')
        {
            stomac(popts->optarg,pdata->mac_entry.mac_addr);
            mac_opt++;
        }
        else
        {
            return PPA_CMD_ERR;
        }
        popts++;
    }

    // Each parameter must be specified just once.
    if (mac_opt != 1 || if_opt != 1)
        return PPA_CMD_ERR;

    IFX_PPACMD_DBG("PPA ADD MAC: %s  =  %02x:%02x:%02x:%02x:%02x:%02x\n", pdata->mac_entry.ifname,
            pdata->mac_entry.mac_addr[0],
            pdata->mac_entry.mac_addr[1],
            pdata->mac_entry.mac_addr[2],
            pdata->mac_entry.mac_addr[3],
            pdata->mac_entry.mac_addr[4],
            pdata->mac_entry.mac_addr[5]);


    return PPA_CMD_OK;
}

/*
====================================================================================
   command:   delbr
   description:
   options:
====================================================================================
*/

static void ppa_del_mac_entry_help(int summary)
{
    IFX_PPACMD_PRINT("delbr -m <macaddr> \n");
    return;
}

static int ppa_parse_del_mac_entry_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int mac_opt = 0;

    while (popts->opt)
    {
        if (popts->opt == 'm')
        {
            stomac(popts->optarg,pdata->mac_entry.mac_addr);
            mac_opt++;
        }
        else
        {
            return PPA_CMD_ERR;
        }
        popts++;
    }

    // MAC parameter must be specified just once.
    if (mac_opt != 1)
        return PPA_CMD_ERR;


    IFX_PPACMD_DBG("PPA DEL MAC:  %02x:%02x:%02x:%02x:%02x:%02x\n", pdata->mac_entry.mac_addr[0],
            pdata->mac_entry.mac_addr[1],
            pdata->mac_entry.mac_addr[2],
            pdata->mac_entry.mac_addr[3],
            pdata->mac_entry.mac_addr[4],
            pdata->mac_entry.mac_addr[5]);
    return PPA_CMD_OK;
}

/*
====================================================================================
   command:   setvif
   description: Set interface VLAN configuration.
   options:
====================================================================================
*/

static const char ppa_set_vlan_if_cfg_short_opts[] = "i:V:c:O:h";

static const struct option ppa_set_vlan_if_cfg_long_opts[] =
{
    {"interface",     required_argument,  NULL,  'i'},
    {"vlan-type",     required_argument,  NULL,  'V'},
    {"tag-control",     required_argument,  NULL,  'c'},
    {"outer-tag-control", required_argument,  NULL,  'O'},
    {"vlan-aware",    no_argument,    NULL,  OPT_VLAN_AWARE},
    {"outer-tag-control", no_argument,    NULL,  OPT_OUTER_VLAN_AWARE},
    { 0,0,0,0 }
};

static void ppa_set_vlan_if_cfg_help(int summary)
{
    IFX_PPACMD_PRINT("setvif -i <ifname> -V <vlan-type> -c <inner-tag-control> -O <outer-tag-control>\n");
    if( summary )
    {
        IFX_PPACMD_PRINT("    <vlan-type>   := {src-ip-addr|eth-type|ingress-vid|port} \n");
        IFX_PPACMD_PRINT("    <tag-control> := {insert|remove|replace|none} \n");

        IFX_PPACMD_PRINT("    This command is for A4/D4 only at present\n");
    }
    return;
}

static int ppa_parse_set_vlan_if_cfg_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int vlan_type_opt = 0, in_tag_opt = 0, out_tag_opt = 0;
    unsigned int in_aware_opt = 0, out_aware_opt = 0, if_opt = 0;

    while (popts->opt)
    {
        switch(popts->opt)
        {
        case 'V':
            if (strcmp("src-ip-addr",popts->optarg) == 0)
                pdata->br_vlan.vlan_cfg.src_ip_based_vlan = 1;
            else if (strcmp("eth-type",popts->optarg) == 0)
                pdata->br_vlan.vlan_cfg.eth_type_based_vlan = 1;
            else if (strcmp("ingress-vid",popts->optarg) == 0)
                pdata->br_vlan.vlan_cfg.vlanid_based_vlan = 1;
            else if (strcmp("port",popts->optarg) == 0)
                pdata->br_vlan.vlan_cfg.port_based_vlan = 1;
            else
                return PPA_CMD_ERR;
            vlan_type_opt++;
            break;

        case 'c':
            if (strcmp("insert",popts->optarg) == 0)
                pdata->br_vlan.vlan_tag_ctrl.insertion = 1;
            else if (strcmp("remove",popts->optarg) == 0)
                pdata->br_vlan.vlan_tag_ctrl.remove = 1;
            else if (strcmp("replace",popts->optarg) == 0)
                pdata->br_vlan.vlan_tag_ctrl.replace = 1;
            else if (strcmp("none",popts->optarg) == 0)
                pdata->br_vlan.vlan_tag_ctrl.unmodified = 1;
            else
                return PPA_CMD_ERR;
            in_tag_opt++;
            break;

        case 'O':
            if (strcmp("insert",popts->optarg) == 0)
                pdata->br_vlan.vlan_tag_ctrl.out_insertion = 1;
            else if (strcmp("remove",popts->optarg) == 0)
                pdata->br_vlan.vlan_tag_ctrl.out_remove = 1;
            else if (strcmp("replace",popts->optarg) == 0)
                pdata->br_vlan.vlan_tag_ctrl.out_replace = 1;
            else if (strcmp("none",popts->optarg) == 0)
                pdata->br_vlan.vlan_tag_ctrl.out_unmodified = 1;
            else
                return PPA_CMD_ERR;
            out_tag_opt++;
            break;

        case 'i':
            strncpy(pdata->br_vlan.if_name,popts->optarg,PPA_IF_NAME_SIZE);
            pdata->br_vlan.if_name[PPA_IF_NAME_SIZE-1] = 0;
            if_opt++;
            break;

        case OPT_VLAN_AWARE:
            pdata->br_vlan.vlan_cfg.vlan_aware = 1;
            in_aware_opt++;
            break;

        case OPT_OUTER_VLAN_AWARE:
            pdata->br_vlan.vlan_cfg.out_vlan_aware = 1;
            out_aware_opt++;
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if (   /*vlan_type_opt > 1 ||*/ if_opt     != 1
                                    || in_tag_opt  > 1 || out_tag_opt   > 1
                                    || in_aware_opt  > 1 || out_aware_opt > 1)
        return PPA_CMD_ERR;


    // Set default values is not specified in command line
    if (vlan_type_opt == 0)
        pdata->br_vlan.vlan_cfg.port_based_vlan = 1;

    if (in_tag_opt == 0)
        pdata->br_vlan.vlan_tag_ctrl.unmodified = 1;

    if (out_tag_opt == 0)
        pdata->br_vlan.vlan_tag_ctrl.out_unmodified = 1;

    IFX_PPACMD_DBG("VLAN TYPE:%s\n", pdata->br_vlan.if_name);
    IFX_PPACMD_DBG("  SRC IP VLAN: %s\n", pdata->br_vlan.vlan_cfg.src_ip_based_vlan ? "enable" : "disable");
    IFX_PPACMD_DBG("  ETH TYPE VLAN: %s\n", pdata->br_vlan.vlan_cfg.eth_type_based_vlan ? "enable" : "disable");
    IFX_PPACMD_DBG("     VID VLAN: %s\n", pdata->br_vlan.vlan_cfg.vlanid_based_vlan ? "enable" : "disable");
    IFX_PPACMD_DBG("PORT BASED VLAN: %s\n", pdata->br_vlan.vlan_cfg.port_based_vlan ? "enable" : "disable");

    IFX_PPACMD_DBG("TAG CONTROL\n");
    IFX_PPACMD_DBG("    INSERT: %s\n", pdata->br_vlan.vlan_tag_ctrl.insertion ? "enable" : "disable");
    IFX_PPACMD_DBG("    REMOVE: %s\n", pdata->br_vlan.vlan_tag_ctrl.remove ? "enable" : "disable");
    IFX_PPACMD_DBG("     REPLACE: %s\n", pdata->br_vlan.vlan_tag_ctrl.replace ? "enable" : "disable");
    IFX_PPACMD_DBG("  OUT INSERT: %s\n",pdata->br_vlan.vlan_tag_ctrl.out_insertion ? "enable" : "disable");
    IFX_PPACMD_DBG("  OUT REMOVE: %s\n", pdata->br_vlan.vlan_tag_ctrl.out_remove ? "enable" : "disable");
    IFX_PPACMD_DBG("   OUT REPLACE: %s\n", pdata->br_vlan.vlan_tag_ctrl.out_replace ? "enable" : "disable");
    IFX_PPACMD_DBG("  VLAN AWARE: %s\n", pdata->br_vlan.vlan_cfg.vlan_aware ? "enable" : "disable");
    IFX_PPACMD_DBG("OUT VLAN AWARE: %s\n", pdata->br_vlan.vlan_cfg.out_vlan_aware ? "enable" : "disable");

    return PPA_CMD_OK;
}

/*
====================================================================================
   command:
   description:
   options:
====================================================================================
*/

static void ppa_get_vlan_if_cfg_help(int summary)
{
    IFX_PPACMD_PRINT("getvif -i <ifname> [-o outfile] \n");
    if( summary ) IFX_PPACMD_PRINT("    This command is for A4/D4 only at present\n");
    return;
}

static int ppa_parse_get_vlan_if_cfg_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int out_opts = 0, if_opts = 0;

    while (popts->opt)
    {
        if (popts->opt == 'i')
        {
            strcpy(pdata->br_vlan.if_name, popts->optarg);
            if_opts++;
        }
        else if (popts->opt == 'o')
        {
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            out_opts++;
        }
        else
        {
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if (out_opts > 1 || if_opts != 1)
        return PPA_CMD_ERR;

    IFX_PPACMD_DBG("PPA GET VLAN CFG: %s\n", pdata->br_vlan.if_name);

    return PPA_CMD_OK;
}


static void ppa_print_get_vif(PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
    {
        //vlan_tag_ctrl, &cmd_info.br_vlan.vlan_cfg, c
        IFX_PPACMD_PRINT("%s: ", pdata->br_vlan.if_name);
        if(  pdata->br_vlan.vlan_cfg.eth_type_based_vlan )
            IFX_PPACMD_PRINT(" ether-type based, ");
        if(  pdata->br_vlan.vlan_cfg.src_ip_based_vlan)
            IFX_PPACMD_PRINT(" src-ip based, ");
        if(  pdata->br_vlan.vlan_cfg.vlanid_based_vlan )
            IFX_PPACMD_PRINT(" vlan id based, ");
        if(  pdata->br_vlan.vlan_cfg.port_based_vlan )
            IFX_PPACMD_PRINT(" port based,");

        IFX_PPACMD_PRINT( "%s, ", pdata->br_vlan.vlan_cfg.vlan_aware ? "inner vlan aware":"inner vlan no");
        IFX_PPACMD_PRINT( "%s, ", pdata->br_vlan.vlan_cfg.out_vlan_aware ? "out vlan aware":"outlvan vlan no");

        if( pdata->br_vlan.vlan_tag_ctrl.unmodified )
            IFX_PPACMD_PRINT("inner-vlan unmodified, ");
        else  if( pdata->br_vlan.vlan_tag_ctrl.insertion )
            IFX_PPACMD_PRINT("inner-vlan insert, ");
        else  if( pdata->br_vlan.vlan_tag_ctrl.remove )
            IFX_PPACMD_PRINT("inner-vlan remove, ");
        else  if( pdata->br_vlan.vlan_tag_ctrl.replace )
            IFX_PPACMD_PRINT("inner-vlan replace, ");


        if( pdata->br_vlan.vlan_tag_ctrl.out_unmodified )
            IFX_PPACMD_PRINT("out-vlan unmodified, ");
        else  if( pdata->br_vlan.vlan_tag_ctrl.out_insertion )
            IFX_PPACMD_PRINT("out-vlan insert, ");
        else  if( pdata->br_vlan.vlan_tag_ctrl.out_remove )
            IFX_PPACMD_PRINT("out-vlan remove, ");
        else  if( pdata->br_vlan.vlan_tag_ctrl.out_replace )
            IFX_PPACMD_PRINT("out-vlan replace, ");

        IFX_PPACMD_PRINT("\n");

    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->br_vlan),  sizeof(pdata->br_vlan)  );
    }
}

/*
====================================================================================
   command:   addvfilter
   description:
   options:
====================================================================================
*/
typedef struct vlan_ctrl
{
    char* cmd_str;  //command
    char op; //qid
} vlan_ctrl;
vlan_ctrl vlan_ctrl_list[]= {{"none", 0},{"remove", 1},{"insert", 2},{"replace", 3} };

static const char ppa_add_vlan_filter_short_opts[] = "t:V:i:a:e:o:q:d:c:r:h";
static const struct option ppa_add_vlan_filter_long_opts[] =
{
    {"vlan-tag",  required_argument,  NULL, 't'},
    {"ingress-vid", required_argument,  NULL, 'V'},
    {"interface",  required_argument,  NULL, 'i'},
    {"src-ipaddr",  required_argument,  NULL, 'a'},
    {"eth-type",  required_argument,  NULL, 'e'},
    {"out-vlan-id", required_argument,  NULL, 'o'},
    {"dest_qos",  required_argument,  NULL, 'q'},
    {"dst-member",  required_argument,  NULL, 'd'},
    {"inner-vctrl",  required_argument,  NULL, 'r'},
    {"outer-vctrl",  required_argument,  NULL, 'c'},
    { 0,0,0,0 }
};

static void ppa_add_vlan_filter_help(int summary)
{
    if( !summary )
    {
        //only display part of parameter since there are too many parameters
        IFX_PPACMD_PRINT("addvfilter {-i <ifname>|-a <src-ip-addr>|-e <eth-type>|-V <vlan-id>} ...\n");
    }
    else
    {
        IFX_PPACMD_PRINT("addvfilter {-i <ifname>|-a <src-ip-addr>|-e <eth-type>|-V <vlan-id>} -t <vlan-tag>\n");
        IFX_PPACMD_PRINT("    -o <out_vlan_id> -q <queue_id> -d <member-list> -c <in-tag-control> -r <out-tag-control>\n");
        IFX_PPACMD_PRINT("    parameter c/r: for tag based vlan filter only\n");
        IFX_PPACMD_PRINT("    <tag-control: none | remove | insert | replace\n");
        IFX_PPACMD_PRINT("    This command is for A4/D4 only at present\n");
    }
    return;
}

static int ppa_parse_add_vlan_filter_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int i, j, tag_opts = 0, match_opts = 0, out_vlan_id_opts=0, vlan_if_member_opts=0, qid_opts=0,inner_vlan_ctrl_opts=0, out_vlan_ctrl_opts=0;
    uint32_t vlan_ctrl;

    memset( &pdata->vlan_filter, 0, sizeof(pdata->vlan_filter) );
    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 't':  /*inner vlan: for all kinds of vlan filters */
            pdata->vlan_filter.vlan_filter_cfg.vlan_info.vlan_vci = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            tag_opts++;
            break;

        case 'i': /*port based vlan filter: for comparing  */
            strncpy(pdata->vlan_filter.vlan_filter_cfg.match_field.match_field.ifname, popts->optarg, PPA_IF_NAME_SIZE-1);
            pdata->vlan_filter.vlan_filter_cfg.match_field.match_flags = PPA_F_VLAN_FILTER_IFNAME;
            match_opts++;
            break;

        case 'a': /*ip based vlan filter: for comparing*/
            inet_aton(popts->optarg,&pdata->vlan_filter.vlan_filter_cfg.match_field.match_field.ip_src);
            pdata->vlan_filter.vlan_filter_cfg.match_field.match_flags = PPA_F_VLAN_FILTER_IP_SRC;
            match_opts++;
            break;

        case 'e': /*protocol based vlan filter: for comparing */
            pdata->vlan_filter.vlan_filter_cfg.match_field.match_field.eth_protocol = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            pdata->vlan_filter.vlan_filter_cfg.match_field.match_flags = PPA_F_VLAN_FILTER_ETH_PROTO;
            match_opts++;
            break;

        case 'V': /*vlan tag based vlan filter: for comparing */
            pdata->vlan_filter.vlan_filter_cfg.match_field.match_field.ingress_vlan_tag = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            pdata->vlan_filter.vlan_filter_cfg.match_field.match_flags = PPA_F_VLAN_FILTER_VLAN_TAG;
            match_opts++;
            break;

        case 'o': /*outer vlan: for all kinds of vlan filters */
            pdata->vlan_filter.vlan_filter_cfg.vlan_info.out_vlan_id= str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            out_vlan_id_opts++;
            break;

        case 'd': /*member list: for all kinds of vlan filters */
            if( vlan_if_member_opts < PPA_MAX_IFS_NUM )
            {
                strncpy(pdata->vlan_filter.vlan_filter_cfg.vlan_info.vlan_if_membership[vlan_if_member_opts].ifname, popts->optarg, sizeof(pdata->vlan_filter.vlan_filter_cfg.vlan_info.vlan_if_membership[vlan_if_member_opts].ifname) );

                vlan_if_member_opts++;
                pdata->vlan_filter.vlan_filter_cfg.vlan_info.num_ifs = vlan_if_member_opts;

            }
            break;

        case 'q': /*qid: for all kinds of vlan filters */
            pdata->vlan_filter.vlan_filter_cfg.vlan_info.qid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            qid_opts++;
            break;

        case 'c': //inner vlan ctrl: only for vlan tag based vlan filter
        case 'r': //out vlan ctrl:: only for vlan tag based vlan filter
            for(i=0; i<sizeof(vlan_ctrl_list)/sizeof(vlan_ctrl_list[0]); i++ )
            {
                if( strcmp( vlan_ctrl_list[i].cmd_str, popts->optarg ) == 0 )
                {
                    if( popts->opt == 'c' )
                    {
                        vlan_ctrl = vlan_ctrl_list[i].op << 2;
                        pdata->vlan_filter.vlan_filter_cfg.vlan_info.inner_vlan_tag_ctrl = 0;
                        for(j=0; j<8; j++)
                        {
                            pdata->vlan_filter.vlan_filter_cfg.vlan_info.inner_vlan_tag_ctrl |= vlan_ctrl << ( 4 * j );
                        }
                        inner_vlan_ctrl_opts++;
                        break;
                    }
                    else
                    {
                        vlan_ctrl = (vlan_ctrl_list[i].op);
                        pdata->vlan_filter.vlan_filter_cfg.vlan_info.out_vlan_tag_ctrl = 0;
                        for(j=0; j<8; j++)
                        {
                            pdata->vlan_filter.vlan_filter_cfg.vlan_info.out_vlan_tag_ctrl |= vlan_ctrl << ( 4 * j);
                        }
                        out_vlan_ctrl_opts++;
                        break;
                    }
                }
            }
            break;

        default:
            IFX_PPACMD_PRINT("not known parameter: %c\n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    /* Check that match field is not defined more than once and VLAN tag is specified */
    if ( ( match_opts != 1) || (tag_opts  != 1) || (out_vlan_id_opts !=1)  || (vlan_if_member_opts == 0)  )
    {
        if( match_opts != 1)
            IFX_PPACMD_PRINT( "match_opts wrong:%d\n", match_opts);
        else     if( tag_opts != 1)
            IFX_PPACMD_PRINT( "tag_opts wrong:%d\n", tag_opts);
        else     if( out_vlan_id_opts != 1)
            IFX_PPACMD_PRINT( "out_vlan_id_opts wrong:%d\n", out_vlan_id_opts);
        else     if( vlan_if_member_opts != 1)
            IFX_PPACMD_PRINT( "vlan_if_member_opts wrong:%d\n", vlan_if_member_opts);


        return PPA_CMD_ERR;
    }

    if( qid_opts == 0 )
    {
        pdata->vlan_filter.vlan_filter_cfg.vlan_info.qid = PPA_INVALID_QID;
    }


    if( pdata->vlan_filter.vlan_filter_cfg.match_field.match_flags == PPA_F_VLAN_FILTER_VLAN_TAG )
    {
        if( (inner_vlan_ctrl_opts != 1) || (out_vlan_ctrl_opts != 1) )
        {
            IFX_PPACMD_PRINT("vlan control wrong: inner_vlan_ctrl_opts=%d, out_vlan_ctrl_opts=%d\n", inner_vlan_ctrl_opts , out_vlan_ctrl_opts);
            return PPA_CMD_ERR;
        }
    }
    else
    {
        if(( inner_vlan_ctrl_opts != 0) ||( out_vlan_ctrl_opts != 0 ) )
        {
            IFX_PPACMD_PRINT("vlan control wrong 2: inner_vlan_ctrl_opts=%d, out_vlan_ctrl_opts=%d\n", inner_vlan_ctrl_opts , out_vlan_ctrl_opts);
            return PPA_CMD_ERR;
        }
    }


    IFX_PPACMD_DBG("INNER VLAN TAG: 0x%x\n", (unsigned int) pdata->vlan_filter.vlan_filter_cfg.vlan_info.vlan_vci);
    IFX_PPACMD_DBG("OUT VLAN TAG: 0x%x\n", (unsigned int) pdata->vlan_filter.vlan_filter_cfg.vlan_info.out_vlan_id);

    IFX_PPACMD_DBG("MATCH FIELD\n");
    switch(pdata->vlan_filter.vlan_filter_cfg.match_field.match_flags)
    {
    case PPA_F_VLAN_FILTER_VLAN_TAG:
        IFX_PPACMD_DBG("VLAN TAG: %04lX\n", pdata->vlan_filter.vlan_filter_cfg.match_field.match_field.ingress_vlan_tag);
        IFX_PPACMD_DBG("INNER VLAN CTRL: %s\n", vlan_ctrl_list[ (pdata->vlan_filter.vlan_filter_cfg.vlan_info.inner_vlan_tag_ctrl >> 2 ) & 0x3].cmd_str);
        IFX_PPACMD_DBG("OUT   VLAN CTRL: %s\n", vlan_ctrl_list[ (pdata->vlan_filter.vlan_filter_cfg.vlan_info.out_vlan_tag_ctrl ) & 0x3].cmd_str);
        break;
    case PPA_F_VLAN_FILTER_IFNAME:
        IFX_PPACMD_DBG( "IF NAME: %s\n", pdata->vlan_filter.vlan_filter_cfg.match_field.match_field.ifname);
        break;
    case PPA_F_VLAN_FILTER_IP_SRC:
        IFX_PPACMD_DBG("IP SRC: %08lX\n", pdata->vlan_filter.vlan_filter_cfg.match_field.match_field.ip_src);
        break;
    case PPA_F_VLAN_FILTER_ETH_PROTO:
        IFX_PPACMD_DBG("ETH TYPE: %04lX\n",pdata->vlan_filter.vlan_filter_cfg.match_field.match_field.eth_protocol);
        break;
    }

    for(i=0; i< pdata->vlan_filter.vlan_filter_cfg.vlan_info.num_ifs; i++ )
    {
        IFX_PPACMD_DBG("Dest member[%d]=%s\n", i, pdata->vlan_filter.vlan_filter_cfg.vlan_info.vlan_if_membership[i].ifname);
    }
    return PPA_CMD_OK;
}

/*
====================================================================================
   command:   delvfilter
   description:
   options:
====================================================================================
*/

static const char ppa_del_vlan_filter_short_opts[] = "V:i:a:e:h";

static const struct option ppa_del_vlan_filter_long_opts[] =
{
    {"ingress-vid", required_argument,  NULL, 'V'},
    {"interface",   required_argument,  NULL, 'i'},
    {"src-ipaddr",  required_argument,  NULL, 'a'},
    {"eth-type",  required_argument,  NULL, 'e'},
    { 0,0,0,0 }
};

static void ppa_del_vlan_filter_help(int summary)
{
    IFX_PPACMD_PRINT("delvfilter {-i <ifname>|-a <src-ip-addr>|-e <eth-type>|-V <vlan-id>} \n");
    if( summary )   IFX_PPACMD_PRINT("    This command is for A4/D4 only at present\n");
    return;
}

static int ppa_parse_del_vlan_filter_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int match_opts = 0;

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'i': /*port based vlan filter: for comparing  */
            strncpy(pdata->vlan_filter.vlan_filter_cfg.match_field.match_field.ifname, popts->optarg, PPA_IF_NAME_SIZE);
            pdata->vlan_filter.vlan_filter_cfg.match_field.match_flags = PPA_F_VLAN_FILTER_IFNAME;
            match_opts++;
            break;

        case 'a': /*ip based vlan filter: for comparing*/
            inet_aton(popts->optarg, &pdata->vlan_filter.vlan_filter_cfg.match_field.match_field.ip_src);
            pdata->vlan_filter.vlan_filter_cfg.match_field.match_flags = PPA_F_VLAN_FILTER_IP_SRC;
            match_opts++;
            break;

        case 'e': /*protocol based vlan filter: for comparing */
            pdata->vlan_filter.vlan_filter_cfg.match_field.match_field.eth_protocol = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            pdata->vlan_filter.vlan_filter_cfg.match_field.match_flags = PPA_F_VLAN_FILTER_ETH_PROTO;
            match_opts++;
            break;

        case 'V': /*vlan tag based vlan filter: for comparing */
            pdata->vlan_filter.vlan_filter_cfg.match_field.match_field.ingress_vlan_tag = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            pdata->vlan_filter.vlan_filter_cfg.match_field.match_flags = PPA_F_VLAN_FILTER_VLAN_TAG;
            match_opts++;
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    /* Check that match field is not defined more than once and VLAN tag is specified */
    if ( match_opts != 1)
        return PPA_CMD_ERR;


    switch(pdata->vlan_filter.vlan_filter_cfg.match_field.match_flags)
    {
    case PPA_F_VLAN_FILTER_VLAN_TAG:
        IFX_PPACMD_DBG("VLAN TAG: %04lX\n", pdata->vlan_filter.vlan_filter_cfg.match_field.match_field.ingress_vlan_tag);
        break;
    case PPA_F_VLAN_FILTER_IFNAME:
        IFX_PPACMD_DBG(" IF NAME: %s\n", pdata->vlan_filter.vlan_filter_cfg.match_field.match_field.ifname);
        break;
    case PPA_F_VLAN_FILTER_IP_SRC:
        IFX_PPACMD_DBG("  IP SRC: %08lX\n", pdata->vlan_filter.vlan_filter_cfg.match_field.match_field.ip_src);
        break;
    case PPA_F_VLAN_FILTER_ETH_PROTO:
        IFX_PPACMD_DBG("ETH TYPE: %04lX\n",pdata->vlan_filter.vlan_filter_cfg.match_field.match_field.eth_protocol);
        break;
    }
    return PPA_CMD_OK;
}

/*
====================================================================================
   command:   getvfiltercount
   description: get vlan fitlers counter
   options:
====================================================================================
*/
static void ppa_get_vfilter_count_help(int summary)
{
    IFX_PPACMD_PRINT("getvfilternum [-o file ]\n");
    if( summary ) IFX_PPACMD_PRINT("    This command is for A4/D4 only at present\n");
    return;
}

static int ppa_parse_get_vfilter_count(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    int res;

    res =   ppa_parse_simple_cmd( popts, pdata );

    if( res != PPA_CMD_OK ) return res;


    pdata->count_info.flag = 0;

    return PPA_CMD_OK;
}

/*
====================================================================================
   command:   getfilters
   description: get all vlan fitlers information
   options:
====================================================================================
*/

static void ppa_get_all_vlan_filter_help(int summary)
{
    IFX_PPACMD_PRINT("getvfilters [-o file ]\n");
    if( summary ) IFX_PPACMD_PRINT("    This command is for A4/D4 only at present\n");
    return;
}

static int ppa_get_all_vlan_filter_cmd (PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata)
{
    PPA_CMD_VLAN_ALL_FILTER_CONFIG *psession_buffer;
    PPA_CMD_DATA cmd_info;
    int res = PPA_CMD_OK, i, j, size;
    uint32_t flag = PPA_CMD_GET_ALL_VLAN_FILTER_CFG;
    unsigned char bfCorrectType = 0;

    //get session count first before malloc memroy
    cmd_info.count_info.flag = 0;
    if( ppa_do_ioctl_cmd(PPA_CMD_GET_COUNT_VLAN_FILTER, &cmd_info ) != PPA_CMD_OK )
        return -EIO;

    if( cmd_info.count_info.count == 0 )
    {
        IFX_PPACMD_DBG("vfilter count=0\n");
        if( g_output_to_file )
            SaveDataToFile(g_output_filename, (char *)(&cmd_info.count_info),  sizeof(cmd_info.count_info)  );
        return PPA_CMD_OK;
    }

    //malloc memory and set value correctly
    size = sizeof(PPA_CMD_COUNT_INFO) + sizeof(PPA_CMD_VLAN_FILTER_CONFIG) * ( 1 + cmd_info.count_info.count ) ;
    psession_buffer = (PPA_CMD_VLAN_ALL_FILTER_CONFIG *) malloc( size );
    if( psession_buffer == NULL )
    {
        IFX_PPACMD_PRINT("Malloc %d bytes failed\n", size );
        return PPA_CMD_NOT_AVAIL;
    }

    memset( psession_buffer, 0, sizeof(size) );

    psession_buffer->count_info.count = cmd_info.count_info.count;
    psession_buffer->count_info.flag = 0;

    //get session information
    if( (res = ppa_do_ioctl_cmd(flag, psession_buffer ) != PPA_CMD_OK ) )
    {
        free( psession_buffer );
        return res;
    }

    IFX_PPACMD_DBG("Vfilter count=%u. \n", (unsigned int)psession_buffer->count_info.count);


    if( !g_output_to_file )
    {
        for(i=0; i<psession_buffer->count_info.count; i++ )
        {
            IFX_PPACMD_PRINT("[%02d] ", i );
            if( psession_buffer->filters[i].vlan_filter_cfg.match_field.match_flags == PPA_F_VLAN_FILTER_VLAN_TAG )
            {
                bfCorrectType = 1;
                IFX_PPACMD_PRINT("Vlan tag based:vlan %u. Qos:%u. ",  (unsigned int)psession_buffer->filters[i].vlan_filter_cfg.match_field.match_field.ingress_vlan_tag, (unsigned int)psession_buffer->filters[i].vlan_filter_cfg.vlan_info.qid);
            }
            else  if( psession_buffer->filters[i].vlan_filter_cfg.match_field.match_flags == PPA_F_VLAN_FILTER_IFNAME  )
            {
                bfCorrectType = 1;
                IFX_PPACMD_PRINT("Port based: %s. Qos:%d. ",  psession_buffer->filters[i].vlan_filter_cfg.match_field.match_field.ifname, psession_buffer->filters[i].vlan_filter_cfg.vlan_info.qid);
            }
            else if( psession_buffer->filters[i].vlan_filter_cfg.match_field.match_flags == PPA_F_VLAN_FILTER_IP_SRC)
            {
                bfCorrectType = 1;
                IFX_PPACMD_PRINT("Src ip based: %u.%u.%u.%u. Qos:%d. ", NIPQUAD( psession_buffer->filters[i].vlan_filter_cfg.match_field.match_field.ip_src) , psession_buffer->filters[i].vlan_filter_cfg.vlan_info.qid);
            }
            else if( psession_buffer->filters[i].vlan_filter_cfg.match_field.match_flags == PPA_F_VLAN_FILTER_ETH_PROTO)
            {
                bfCorrectType = 1;
                IFX_PPACMD_PRINT("Ether type based: %04x. Qos:%u. ",  (unsigned int)psession_buffer->filters[i].vlan_filter_cfg.match_field.match_field.eth_protocol, (unsigned int)psession_buffer->filters[i].vlan_filter_cfg.vlan_info.qid);
            }

            if( bfCorrectType )
            {
                IFX_PPACMD_PRINT("Inner/Out VLAN:%03X/%03x ", (unsigned int)psession_buffer->filters[i].vlan_filter_cfg.vlan_info.vlan_vci, (unsigned int)psession_buffer->filters[i].vlan_filter_cfg.vlan_info.out_vlan_id );

                IFX_PPACMD_PRINT("Dst members:");
                for(j=0; j<psession_buffer->filters[i].vlan_filter_cfg.vlan_info.num_ifs ; j++ )
                {
                    if( i == 0 )
                        IFX_PPACMD_PRINT("%s", psession_buffer->filters[i].vlan_filter_cfg.vlan_info.vlan_if_membership[j].ifname);
                    else
                        IFX_PPACMD_PRINT(",%s", psession_buffer->filters[i].vlan_filter_cfg.vlan_info.vlan_if_membership[j].ifname);
                }
                IFX_PPACMD_PRINT(".");

                if (psession_buffer->filters[i].vlan_filter_cfg.match_field.match_flags == PPA_F_VLAN_FILTER_VLAN_TAG )
                {
                    IFX_PPACMD_PRINT("  Inner/Out vlan control:%s/%s ",
                                     vlan_ctrl_list[ (psession_buffer->filters[i].vlan_filter_cfg.vlan_info.inner_vlan_tag_ctrl >> 2) & 0x3].cmd_str,
                                     vlan_ctrl_list[ (psession_buffer->filters[i].vlan_filter_cfg.vlan_info.out_vlan_tag_ctrl >> 0) & 0x3].cmd_str);
                }
            }
            IFX_PPACMD_PRINT("\n");
        }
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(psession_buffer),  size  );
    }

    free( psession_buffer );
    return PPA_CMD_OK;
}

/*multicast bridging/routing */
static const char ppa_mc_add_short_opts[] = "s:g:l:w:f:t:i:h"; //need to further implement add/remove/modify vlan and enable new dscp and its value
static const struct option ppa_mc_add_long_opts[] =
{
    {"bridging_flag",  required_argument,  NULL, 's'},  /*0-means routing, 1 means bridging */
    /*{"multicat mac address", required_argument,  NULL, 'm'}, */  /*for bridging only as ritech suggest. I may not help at present */
    {"multicast group",   required_argument,  NULL, 'g'},
    {"down interface",  required_argument,  NULL, 'l'},
    {"up interface",  required_argument,  NULL, 'w'},
    {"source_ip",  required_argument,  NULL, 'i'},
    {"ssm_flag",  required_argument,  NULL, 'f'},
    {"qid",  required_argument,  NULL, 't'},
    { 0,0,0,0 }
};

static unsigned int is_ip_zero(IP_ADDR_C *ip)
{
    if(ip->f_ipv6){
		return ((ip->ip.ip6[0] | ip->ip.ip6[1] | ip->ip.ip6[2] | ip->ip.ip6[3]) == 0);
	}else{
		return (ip->ip.ip == 0);
	}
}

static unsigned int ip_equal(IP_ADDR_C *dst_ip, IP_ADDR_C *src_ip)
{
	if(dst_ip->f_ipv6){
		return (((dst_ip->ip.ip6[0] ^ src_ip->ip.ip6[0] ) |
			     (dst_ip->ip.ip6[1] ^ src_ip->ip.ip6[1] ) |
			     (dst_ip->ip.ip6[2] ^ src_ip->ip.ip6[2] ) |
			     (dst_ip->ip.ip6[3] ^ src_ip->ip.ip6[3] )) == 0);
	}else{
		return ( (dst_ip->ip.ip ^ src_ip->ip.ip) == 0);
	}
}


static void ppa_add_mc_help( int summary)
{
    if( !summary )
    {
        IFX_PPACMD_PRINT("addmc -g ip-group -l <down interface> -w<up-interface> -i< src-ip> -f<ssm_flag> ...\n");
    }
    else
    {
        IFX_PPACMD_PRINT("addmc [-s <bridging_flag>] -g ip-group -l <down interface> -w<up-interface> -i< src-ip> -f<ssm_flag>\n");
        IFX_PPACMD_PRINT("      option -s: must be 1 for multicast bridging. For multicast routing, it can be 0 or skipp this option \n");
        IFX_PPACMD_PRINT("      option -l: if not set this parameter, it means to delete the multicast group \n");
        IFX_PPACMD_PRINT("      option -f: only dummy parameter now for IGMP V3\n");
    }
    return;
}

static int ppa_parse_add_mc_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int lan_if_opts=0, g_opts=0;
	int ret;

    memset(pdata, 0, sizeof(*pdata) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 's':
            pdata->mc_add_info.bridging_flag = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            IFX_PPACMD_DBG("addmc  mode: %s\n", pdata->mc_add_info.bridging_flag ? "bridging":"routing" );
            break;

#ifdef PPA_MC_FUTURE_USE
        case 'm':
            stomac(popts->optarg, pdata->mc_add_info.mac);
            IFX_PPACMD_DBG("addmc mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                    pdata->mc_add_info.mac[0], pdata->mc_add_info.mac[1], pdata->mc_add_info.mac[2],
                    pdata->mc_add_info.mac[3], pdata->mc_add_info.mac[4], pdata->mc_add_info.mac[5]);
            break;
#endif

        case 'g':   /*Support IPv4 and IPv6 */
            ret = str_convert(STRING_TYPE_IP, popts->optarg, pdata->mc_add_info.mc.mcast_addr.ip.ip6);
            if(ret == IP_NON_VALID){
                IFX_PPACMD_DBG("Multicast group ip is not a valid IPv4 or IPv6 IP address: %s\n", popts->optarg);
				break;
            }else if(ret == IP_VALID_V6){
                IFX_PPACMD_DBG("MLD GROUP IP: "NIP6_FMT"\n", NIP6(pdata->mc_add_info.mc.mcast_addr.ip.ip6));
                pdata->mc_add_info.mc.mcast_addr.f_ipv6 = 1;
            }
            
            g_opts ++;
            break;

        case 'l':
            if( lan_if_opts > PPA_MAX_MC_IFS_NUM ) return PPA_CMD_ERR; //not to accelerated since too many lan interrace join the same group

            if( lan_if_opts < PPA_MAX_MC_IFS_NUM )
            {
                strncpy(pdata->mc_add_info.lan_ifname[lan_if_opts], popts->optarg, PPA_IF_NAME_SIZE);
                pdata->mc_add_info.lan_ifname[lan_if_opts][PPA_IF_NAME_SIZE-1] = 0;
                IFX_PPACMD_DBG("addmc lan if:%s,lan_if_opts:%d\n", pdata->mc_add_info.lan_ifname[lan_if_opts], lan_if_opts+1 );
                lan_if_opts++;
            }
            break;

        case 'w':
            strncpy(pdata->mc_add_info.src_ifname, popts->optarg, sizeof(pdata->mc_add_info.src_ifname)-1);
            pdata->mc_add_info.src_ifname[sizeof(pdata->mc_add_info.src_ifname)-1] = 0;
            IFX_PPACMD_DBG("addmc wan if:%s\n", pdata->mc_add_info.src_ifname);
            break;

        case 'i': // Src IP, Support IPv4 & IPv6

            ret = str_convert(STRING_TYPE_IP, popts->optarg, pdata->mc_add_info.mc.source_ip.ip.ip6);
            if(ret == IP_NON_VALID){
                IFX_PPACMD_DBG("Multicast group ip is not a valid IPv4 or IPv6 IP address: %s\n", popts->optarg);
				break;
            }else if(ret == IP_VALID_V6){
                IFX_PPACMD_DBG("Source IP: "NIP6_FMT"\n", NIP6(pdata->mc_add_info.mc.mcast_addr.ip.ip6));
                pdata->mc_add_info.mc.source_ip.f_ipv6 = 1;
            }
            break;

        case 'f':
            pdata->mc_add_info.mc.SSM_flag = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);;
            IFX_PPACMD_DBG("addmc SSM_flag:%d \n",pdata->mc_add_info.mc.SSM_flag);
            break;

        case 't':     //future only
            pdata->mc_add_info.mc.mc_extra.dslwan_qid_remark = 1;
            pdata->mc_add_info.mc.mc_extra.dslwan_qid = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);
            IFX_PPACMD_DBG("addmc qid:%d (flag=%d)\n",pdata->mc_add_info.mc.mc_extra.dslwan_qid, pdata->mc_add_info.mc.mc_extra.dslwan_qid_remark);
            break;

        default:
            IFX_PPACMD_PRINT("mc_add not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }


    /* Check that match field is not defined more than once and VLAN tag is specified */
    if( g_opts != 1 )  return  PPA_CMD_ERR;

    pdata->mc_add_info.num_ifs = lan_if_opts;


    return PPA_CMD_OK;
}

/*capwap configuration*/
#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
//static const char ppa_capwap_add_short_opts[] = "c:i:p:q:d:s:o:l:u:e:x:y:r:w:t:m:h"; 
static const char ppa_capwap_add_short_opts[] = "p:i:c:q:e:u:o:l:s:d:x:y:r:w:t:m:h"; 
static const struct option ppa_capwap_add_long_opts[] =
{
    {"directpath_portid",  required_argument,  NULL, 'p'},  
    {"interface_name",  required_argument,  NULL, 'i'},  
    {"phy_port_id",   required_argument,  NULL, 'c'},
    {"qid",  required_argument,  NULL, 'q'},
    {"dst_mac",  required_argument,  NULL, 'e'},
    {"src_mac",  required_argument,  NULL, 'u'},
    {"tos",  required_argument,  NULL, 'o'},
    {"ttl",  required_argument,  NULL, 'l'},
    {"src_ip",  required_argument,  NULL, 's'},
    {"dst_ip",  required_argument,  NULL, 'd'},
    {"src_port",  required_argument,  NULL, 'x'},
    {"dst_port",  required_argument,  NULL, 'y'},
    {"RID",  required_argument,  NULL, 'r'},
    {"WBID",  required_argument,  NULL, 'w'},
    {"T",  required_argument,  NULL, 't'},
    {"max fragment size",  required_argument,  NULL, 'm'},
    { 0,0,0,0 }
};



static void ppa_add_capwap_help( int summary)
{
    if( !summary )
    {
        IFX_PPACMD_PRINT("addcapwap -p<directpath_portid> -i<interface_name> -c<physical port id> -q<PPE FW QOS qid> -e<destination mac > -u<source mac> -o<TOS> -l<TTL> -s<source ip> -d<destination ip> -x<UDP source port> -y<UDP Dst port> -r<RID> -w<WBID> -t<T Flag> -m<Max fragment size>...\n");
    }
    else
    {
        IFX_PPACMD_PRINT("addcapwap -p<directpath_portid> -i<interface_name> -c<physical port id> -q<PPE FW QOS qid> -e<destination mac > -u<source mac> -o<TOS> -l<TTL> -s<source ip> -d<destination ip> -x<UDP source port> -y<UDP Dst port> -r<RID> -w<WBID> -t<T Flag> -m<Max fragment size>...\n");

        IFX_PPACMD_PRINT("      option -p: Directpath portId,valid range 3 to 7\n");
        IFX_PPACMD_PRINT("      option -i: Interface name, for example -i eth0 \n");
        IFX_PPACMD_PRINT("      option -c: Physical PortId, this is an optional parameter either interface name or portId\n");
        IFX_PPACMD_PRINT("      option -q: PPE FW QoS Queue Id\n");
        IFX_PPACMD_PRINT("      option -e: Destination mac\n");
        IFX_PPACMD_PRINT("      option -u: Source mac\n");
        IFX_PPACMD_PRINT("      option -o: IPv4 header TOS, this is an optional parameter, default value is 0\n");
        IFX_PPACMD_PRINT("      option -l: IPv4 header TTL, this is an optional parameter, default value is 1\n");
        IFX_PPACMD_PRINT("      option -s: IPv4 source IP\n");
        IFX_PPACMD_PRINT("      option -d: IPv4 destination IP\n");
        IFX_PPACMD_PRINT("      option -x: UDP source port\n");
        IFX_PPACMD_PRINT("      option -y: UDP destination port\n");
        IFX_PPACMD_PRINT("      option -r: CAPWAP RID\n");
        IFX_PPACMD_PRINT("      option -w: CAPWAP WBID\n");
        IFX_PPACMD_PRINT("      option -t: CAPWAP T Flag\n");
        IFX_PPACMD_PRINT("      option -m: Maximum Fragment size, default value is 1518\n");
    }
    return;
}

static int ppa_parse_add_capwap_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int lan_if_opts=0;
	 int ret;

    memset(pdata, 0, sizeof(*pdata) );

    pdata->capwap_add_info.tos = DEFAULT_TOS;
    pdata->capwap_add_info.ttl = DEFAULT_TTL;
    pdata->capwap_add_info.max_frg_size = DEFAULT_MAX_FRG_SIZE;
    
    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'p':
            pdata->capwap_add_info.directpath_portid = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);
            IFX_PPACMD_DBG("addcapwap directpath portid:%d\n",pdata->capwap_add_info.directpath_portid);
            break;


        case 'i':
            if( lan_if_opts > PPA_MAX_CAPWAP_IFS_NUM ) return PPA_CMD_ERR; 

            if( lan_if_opts < PPA_MAX_CAPWAP_IFS_NUM )
            {
                strncpy(pdata->capwap_add_info.lan_ifname[lan_if_opts], popts->optarg, PPA_IF_NAME_SIZE);
                pdata->capwap_add_info.lan_ifname[lan_if_opts][PPA_IF_NAME_SIZE-1] = 0;
                IFX_PPACMD_DBG("addcapwap lan if:%s,lan_if_opts:%d\n", pdata->capwap_add_info.lan_ifname[lan_if_opts], lan_if_opts+1 );
                lan_if_opts++;
            }
            break;

        case 'c':
            if( lan_if_opts > PPA_MAX_CAPWAP_IFS_NUM ) return PPA_CMD_ERR; 

            if( lan_if_opts < PPA_MAX_CAPWAP_IFS_NUM )
            {
                pdata->capwap_add_info.phy_port_id[lan_if_opts] = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
                IFX_PPACMD_DBG("addcapwap phy port Id :%d,lan_if_opts:%d\n",
                                pdata->capwap_add_info.phy_port_id[lan_if_opts], lan_if_opts+1 );
                lan_if_opts++;
            }
            break;

        case 'q':
            pdata->capwap_add_info.qid = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);
            IFX_PPACMD_DBG("addcapwap queue id:%d \n",pdata->capwap_add_info.qid);
            break;

        case 'e':
            stomac(popts->optarg, pdata->capwap_add_info.dst_mac);
            IFX_PPACMD_DBG("addcapwap destination mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            pdata->capwap_add_info.dst_mac[0],
                            pdata->capwap_add_info.dst_mac[1],
                            pdata->capwap_add_info.dst_mac[2],
                            pdata->capwap_add_info.dst_mac[3],
                            pdata->capwap_add_info.dst_mac[4],
                            pdata->capwap_add_info.dst_mac[5]);
            break;

        case 'u':
            stomac(popts->optarg, pdata->capwap_add_info.src_mac);
            IFX_PPACMD_DBG("addcapwap source mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            pdata->capwap_add_info.src_mac[0],
                            pdata->capwap_add_info.src_mac[1],
                            pdata->capwap_add_info.src_mac[2],
                            pdata->capwap_add_info.src_mac[3],
                            pdata->capwap_add_info.src_mac[4],
                            pdata->capwap_add_info.src_mac[5]);
            break;

        case 'o':
            pdata->capwap_add_info.tos = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);
            IFX_PPACMD_DBG("addcapwap TOS:%d \n",pdata->capwap_add_info.tos);
            break;

        case 'l':
            pdata->capwap_add_info.ttl = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);
            IFX_PPACMD_DBG("addcapwap TTL:%d \n",pdata->capwap_add_info.ttl);
            break;

        case 's': // Src IP, Support IPv4 & IPv6

            ret = str_convert(STRING_TYPE_IP, popts->optarg, pdata->capwap_add_info.source_ip.ip.ip6);
            if(ret == IP_NON_VALID){
                IFX_PPACMD_DBG("Source ip is not a valid IPv4 IP address: %s\n", popts->optarg);
				break;
            }else if(ret == IP_VALID_V6){
                IFX_PPACMD_DBG("Source IP: "NIP6_FMT"\n", NIP6(pdata->capwap_add_info.source_ip.ip.ip6));
                //IFX_PPACMD_DBG("Source IP: "NIPQUAD_FMT"\n", NIPQUAD(pdata->capwap_add_info.source_ip.ip.ip6));
                pdata->capwap_add_info.source_ip.f_ipv6 = 1;
            }
            break;

        case 'd': // Dst IP, Support IPv4 & IPv6

            ret = str_convert(STRING_TYPE_IP, popts->optarg, pdata->capwap_add_info.dest_ip.ip.ip6);
            if(ret == IP_NON_VALID){
                IFX_PPACMD_DBG("Destination ip is not a valid IPv4 IP address: %s\n", popts->optarg);
				break;
            //}else if(ret == IP_VALID_V4){
            }else if(ret == IP_VALID_V6){
                IFX_PPACMD_DBG("Destnation IP: "NIP6_FMT"\n", NIP6(pdata->capwap_add_info.dest_ip.ip.ip6));
                //IFX_PPACMD_DBG("Destnation IP: "NIPQUAD_FMT"\n", NIPQUAD(pdata->capwap_add_info.dest_ip.ip.ip6));
                pdata->capwap_add_info.dest_ip.f_ipv6 = 1;
            }
            break;

        case 'x':
            pdata->capwap_add_info.source_port = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);
            IFX_PPACMD_DBG("addcapwap UDP source port:%d \n",pdata->capwap_add_info.source_port);
            break;

        case 'y':
            pdata->capwap_add_info.dest_port = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);
            IFX_PPACMD_DBG("addcapwap UDP destination port:%d \n",pdata->capwap_add_info.dest_port);
            break;

        case 'r':
            pdata->capwap_add_info.rid = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);
            IFX_PPACMD_DBG("addcapwap RID:%d \n",pdata->capwap_add_info.rid);
            break;

        case 'w':
            pdata->capwap_add_info.wbid = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);
            IFX_PPACMD_DBG("addcapwap WBID:%d \n",pdata->capwap_add_info.wbid);
            break;

        case 't':
            pdata->capwap_add_info.t_flag = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);
            IFX_PPACMD_DBG("addcapwap T flag:%d \n",pdata->capwap_add_info.t_flag);
            break;

        case 'm':
            pdata->capwap_add_info.max_frg_size = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);
            IFX_PPACMD_DBG("addcapwap T flag:%d \n",pdata->capwap_add_info.max_frg_size);
            break;

        default:
            IFX_PPACMD_PRINT("capwap_add not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    pdata->capwap_add_info.num_ifs = lan_if_opts;

    return PPA_CMD_OK;
}


//delcapwap
static const char ppa_capwap_del_short_opts[] = "p:s:d:h"; 
static const struct option ppa_capwap_del_long_opts[] =
{
    {"directpath_portid",  required_argument,  NULL, 'p'},  
    {"src_ip",  required_argument,  NULL, 's'},
    {"dst_ip",  required_argument,  NULL, 'd'},
    { 0,0,0,0 }
};



static void ppa_del_capwap_help( int summary)
{
    if( !summary )
    {
        IFX_PPACMD_PRINT("delcapwap -p<directpath_portid> -s<source ip> -d<destination ip> ...\n");
    }
    else
    {
        IFX_PPACMD_PRINT("delcapwap -p<directpath_portid> -s<source ip> -d<destination ip> ...\n");

        IFX_PPACMD_PRINT("      option -p: Directpath portId,valid range 3 to 7\n");
        IFX_PPACMD_PRINT("      option -s: IPv4 source IP address\n");
        IFX_PPACMD_PRINT("      option -d: IPv4 destination IP address\n");
    }
    return;
}

static int ppa_parse_del_capwap_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
	 int ret;

    memset(pdata, 0, sizeof(*pdata) );

    
    while(popts->opt)
    {
        switch(popts->opt)
        {
           case 'p':
            pdata->capwap_add_info.directpath_portid = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);
            IFX_PPACMD_DBG("addcapwap directpath portid:%d\n",pdata->capwap_add_info.directpath_portid);
            break;


           case 's': // Src IP, Support IPv4 

            ret = str_convert(STRING_TYPE_IP, popts->optarg, pdata->capwap_add_info.source_ip.ip.ip6);
            if(ret == IP_NON_VALID){
                IFX_PPACMD_DBG("Source ip is not a valid IPv4 IP address: %s\n", popts->optarg);
				break;
            }else if(ret == IP_VALID_V6){
                //IFX_PPACMD_DBG("Source IP: "NIPQUAD_FMT"\n", NIPQUAD(pdata->capwap_add_info.source_ip.ip.ip6));
                IFX_PPACMD_DBG("Source IP: "NIP6_FMT"\n", NIP6(pdata->capwap_add_info.source_ip.ip.ip6));
                pdata->capwap_add_info.source_ip.f_ipv6 = 1;
            }
            break;

        case 'd': // Dst IP, Support IPv4 

            ret = str_convert(STRING_TYPE_IP, popts->optarg, pdata->capwap_add_info.dest_ip.ip.ip6);
            if(ret == IP_NON_VALID){
                IFX_PPACMD_DBG("Destination ip is not a valid IPv4 IP address: %s\n", popts->optarg);
				break;
            }else if(ret == IP_VALID_V6){
                //IFX_PPACMD_DBG("Destnation IP: "NIPQUAD_FMT"\n", NIPQUAD(pdata->capwap_add_info.dest_ip.ip.ip6));
                IFX_PPACMD_DBG("Destnation IP: "NIP6_FMT"\n", NIP6(pdata->capwap_add_info.dest_ip.ip.ip6));
                pdata->capwap_add_info.dest_ip.f_ipv6 = 1;
            }
            break;

              default:
            IFX_PPACMD_PRINT("capwap_del not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    return PPA_CMD_OK;
}

// getcapwap
static void ppa_get_capwap_help( int summary)
{
    IFX_PPACMD_PRINT("getcapwap [-o file ]\n"); 
    return;
}

static const char ppa_capwap_get_short_opts[] = "o"; 
static const struct option ppa_capwap_get_long_opts[] =
{
    {"save-to-file",      required_argument,  NULL, 'o'},
    { 0,0,0,0 }
};

static int ppa_parse_get_capwap_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    int ret;
    memset(pdata, 0, sizeof(*pdata) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        
        case 'o':
            g_output_to_file = 1;
            strncpy(g_output_filename, popts->optarg, PPACMD_MAX_FILENAME);
            break;

        default:
            IFX_PPACMD_PRINT("getcapwap doesn't support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    return PPA_CMD_OK;
}

static int ppa_get_capwap_cmd (PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata)
{
   
    PPA_CMD_CAPWAP_GROUPS_INFO *psession_buffer;
    
    PPA_CMD_DATA cmd_info;
    
    int res = PPA_CMD_OK, i, j, size;
    uint32_t flag = PPA_CMD_GET_CAPWAP_GROUPS;
    char str_srcip[64], str_dstip[64];

    //get number of capwap tunnel 
    cmd_info.count_info.flag = 0;
    if( ppa_do_ioctl_cmd(PPA_CMD_GET_COUNT_CAPWAP, &cmd_info ) != PPA_CMD_OK )
        return -EIO;

    if( cmd_info.count_info.count == 0 )
    {
        IFX_PPACMD_PRINT("CAPWAP tunnel count=0\n");
        if( g_output_to_file )
            SaveDataToFile(g_output_filename, (char *)(&cmd_info.count_info),  sizeof(cmd_info.count_info)  );
        return PPA_CMD_OK;
    }

    //malloc memory and set value correctly
    size = sizeof(PPA_CMD_COUNT_INFO) + sizeof(PPA_CMD_CAPWAP_INFO) * ( 1 + cmd_info.count_info.count ) ;
    psession_buffer = (PPA_CMD_CAPWAP_GROUPS_INFO *) malloc( size );
    if( psession_buffer == NULL )
    {
        IFX_PPACMD_PRINT("Malloc %d bytes failed\n", size );
        return PPA_CMD_NOT_AVAIL;
    }

    memset( psession_buffer, 0, sizeof(size) );

    psession_buffer->count_info.count = cmd_info.count_info.count;
    psession_buffer->count_info.flag = 0;


    //get session information
    if( (res = ppa_do_ioctl_cmd(flag, psession_buffer ) != PPA_CMD_OK ) )
    {
        free( psession_buffer );
        return res;
    }

    IFX_PPACMD_DBG("CAPWAP Tunnel total count=%u. \n", (unsigned int)psession_buffer->count_info.count);

    if( !g_output_to_file )
    {
        for(i=0; i<psession_buffer->count_info.count; i++ )
        { 
            
           if(!psession_buffer->capwap_list[i].source_ip.f_ipv6)
               sprintf(str_srcip, NIPQUAD_FMT, NIPQUAD(psession_buffer->capwap_list[i].source_ip.ip.ip));
           else
               sprintf(str_srcip, NIP6_FMT, NIP6(psession_buffer->capwap_list[i].source_ip.ip.ip6));

           if(!psession_buffer->capwap_list[i].dest_ip.f_ipv6)
               sprintf(str_dstip, NIPQUAD_FMT, NIPQUAD(psession_buffer->capwap_list[i].dest_ip.ip.ip));
           else
               sprintf(str_dstip, NIP6_FMT, NIP6(psession_buffer->capwap_list[i].dest_ip.ip.ip6));

           
           IFX_PPACMD_PRINT("[%03u] CAPWAP Tunnel:Src IP: %s  Dst IP: %s\t", psession_buffer->capwap_list[i].directpath_portid, str_srcip,str_dstip);

          
           IFX_PPACMD_PRINT(" Upstream Dst Port: ");
           for(j=0; j<psession_buffer->capwap_list[i].num_ifs; j++)
           {
               if( j == 0 )
               {
                  if(psession_buffer->capwap_list[i].lan_ifname[j][0] != '\0')
                     IFX_PPACMD_PRINT("%s", psession_buffer->capwap_list[i].lan_ifname[j]);
                  else
                     IFX_PPACMD_PRINT("%d", psession_buffer->capwap_list[i].phy_port_id[j]);
               }
               else
                  if(psession_buffer->capwap_list[i].lan_ifname[j][0] != '\0')
                     IFX_PPACMD_PRINT("/%s", psession_buffer->capwap_list[i].lan_ifname[j]);
                  else
                     IFX_PPACMD_PRINT("/%d", psession_buffer->capwap_list[i].phy_port_id[j]);
           }

           if( (psession_buffer->capwap_list[i].dest_ip.f_ipv6) || (psession_buffer->capwap_list[i].source_ip.f_ipv6))
           {
               IFX_PPACMD_PRINT("\t CAPWAP ds MIB: %u  \t CAPWAP us MIB: %u \n",
                             (unsigned int)psession_buffer->capwap_list[i].ds_mib,
                             (unsigned int)psession_buffer->capwap_list[i].us_mib);
               IFX_PPACMD_PRINT("\n");
               IFX_PPACMD_PRINT("\n");

               continue;
           }

           IFX_PPACMD_PRINT("\t Dst mac: %02x:%02x:%02x:%02x:%02x:%02x",
                           psession_buffer->capwap_list[i].dst_mac[0],
                           psession_buffer->capwap_list[i].dst_mac[1],
                           psession_buffer->capwap_list[i].dst_mac[2],
                           psession_buffer->capwap_list[i].dst_mac[3],
                           psession_buffer->capwap_list[i].dst_mac[4],
                           psession_buffer->capwap_list[i].dst_mac[5]);

           IFX_PPACMD_PRINT("\t Source mac: %02x:%02x:%02x:%02x:%02x:%02x",
                           psession_buffer->capwap_list[i].src_mac[0],
                           psession_buffer->capwap_list[i].src_mac[1],
                           psession_buffer->capwap_list[i].src_mac[2],
                           psession_buffer->capwap_list[i].src_mac[3],
                           psession_buffer->capwap_list[i].src_mac[4],
                           psession_buffer->capwap_list[i].src_mac[5]);


          IFX_PPACMD_PRINT("\t qid: %2u \t TOS: %2u \t TTL: %2u \t UDP Source port: %2u \t UDP Dest port: %2u \t CAPWAP RID: %2u \t CAPWAP WBID: %2u \t CAPWAP T Flag: %2u \t CAPWAP Max Fragment size: %u \t CAPWAP ds MIB: %u  \t CAPWAP us MIB: %u \n",
                             (unsigned int)psession_buffer->capwap_list[i].qid,
                             (unsigned int)psession_buffer->capwap_list[i].tos,
                             (unsigned int)psession_buffer->capwap_list[i].ttl,
                             (unsigned int)psession_buffer->capwap_list[i].source_port,
                             (unsigned int)psession_buffer->capwap_list[i].dest_port,
                             (unsigned int)psession_buffer->capwap_list[i].rid,
                             (unsigned int)psession_buffer->capwap_list[i].wbid,
                             (unsigned int)psession_buffer->capwap_list[i].t_flag,
                             (unsigned int)psession_buffer->capwap_list[i].max_frg_size,
                             (unsigned int)psession_buffer->capwap_list[i].ds_mib,
                             (unsigned int)psession_buffer->capwap_list[i].us_mib);

           IFX_PPACMD_PRINT("\n");
           IFX_PPACMD_PRINT("\n");
        }
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(psession_buffer),  size  );
    }

    free( psession_buffer );
    return PPA_CMD_OK;
}

#endif



/* rtp sampling */
#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE

static const char ppa_rtp_set_short_opts[] = "g:i:r:"; 
static const struct option ppa_rtp_set_long_opts[] =
{
    {"multicast group",   required_argument,  NULL, 'g'},
    {"source_ip",  required_argument,  NULL, 'i'},
    {"rtp_flag",  required_argument,  NULL, 'r'},
    { 0,0,0,0 }
};


static void ppa_set_rtp_help( int summary)
{
    if( !summary )
    {
        IFX_PPACMD_PRINT("setrtp -g <mc-group> [-i<src-ip>] -r<rtp_flag> ...\n");
    }
    else
    {
        IFX_PPACMD_PRINT("setrtp -g <mc-group> [-i<src-ip>] -r<rtp_flag>\n");
        IFX_PPACMD_PRINT("      option -g: multicast group ip\n");
        IFX_PPACMD_PRINT("      option -i: source ip, this parameter is optional\n");
        IFX_PPACMD_PRINT("      option -r: RTP sequence number update by PPE FW, 1-enable 0-disable\n");
    }
    return;
}

static int ppa_parse_set_rtp_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int lan_if_opts=0, g_opts=0;
	int ret;

    memset(pdata, 0, sizeof(*pdata) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'g':   /*Support IPv4 and IPv6 */
            ret = str_convert(STRING_TYPE_IP, popts->optarg, pdata->mc_entry.mcast_addr.ip.ip6);
            if(ret == IP_NON_VALID){
                IFX_PPACMD_DBG("Multicast group ip is not a valid IPv4 or IPv6 IP address: %s\n", popts->optarg);
				break;
            }else if(ret == IP_VALID_V6){
                IFX_PPACMD_DBG("MLD GROUP IP: "NIP6_FMT"\n", NIP6(pdata->mc_entry.mcast_addr.ip.ip6));
                pdata->mc_entry.mcast_addr.f_ipv6 = 1;
            }
            
            g_opts ++;
            break;

        case 'i': // Src IP, Support IPv4 & IPv6

            ret = str_convert(STRING_TYPE_IP, popts->optarg, pdata->mc_entry.source_ip.ip.ip6);
            if(ret == IP_NON_VALID){
                IFX_PPACMD_DBG("Multicast group ip is not a valid IPv4 or IPv6 IP address: %s\n", popts->optarg);
				break;
            }else if(ret == IP_VALID_V6){
                IFX_PPACMD_DBG("Source IP: "NIP6_FMT"\n",NIP6(pdata->mc_entry.source_ip.ip.ip6));
                pdata->mc_entry.source_ip.f_ipv6 = 1;
            }
            break;

        case 'r':
            pdata->mc_entry.RTP_flag = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);;
            IFX_PPACMD_DBG("RTP_flag:%d \n",pdata->mc_entry.RTP_flag);
            break;

        default:
            IFX_PPACMD_PRINT("setrtp does not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( g_opts != 1 )  return  PPA_CMD_ERR;

    return PPA_CMD_OK;
}


#endif

/* mib mode */
#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE

static const char ppa_mib_mode_short_opts[] = "i:"; 
static void ppa_set_mib_mode_help( int summary)
{
    if( !summary )
    {
        IFX_PPACMD_PRINT("setmibmode -i <mib-mode> ...\n");
    }
    else
    {
        IFX_PPACMD_PRINT("setmibmode -i <mib-mode>\n");
        IFX_PPACMD_PRINT("      option -i: Mib mode, 0-Byte 1-Packet\n");
    }
    return;
}

static int ppa_parse_set_mib_mode(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
	int ret;

    memset(pdata, 0, sizeof(*pdata) );

    switch(popts->opt)
    {
   
        case 'i':
            pdata->mib_mode_info.mib_mode = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);;
            IFX_PPACMD_DBG("mib_mode:%d \n",pdata->mib_mode_info.mib_mode);
            break;

        default:
            IFX_PPACMD_PRINT("setmibmode does not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
    }


    return PPA_CMD_OK;
}
#endif


/*
====================================================================================
   command:   getmcextra
   description: get multicast extra information, like vlan/dscp
   options:
====================================================================================
*/

static const char    ppa_get_mc_extra_short_opts[] = "g:o:h"; //need to further implement add/remove/modify vlan and enable new dscp and its value
static const struct option ppa_get_mc_extra_long_opts[] =
{
    {"multicast group",   required_argument,  NULL, 'g'},
    {"save-to-file",    required_argument,  NULL, 'o'},

    { 0,0,0,0 }
};
static void ppa_get_mc_extra_help( int summary)
{
    IFX_PPACMD_PRINT("getmcextra -g <multicast group > [-o file ]\n"); // [ -m <multicast-mac-address (for bridging only)>]
    return;
}

static int ppa_parse_get_mc_extra_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int g_opt=0;

    memset(pdata, 0, sizeof(*pdata) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'g':
            inet_aton(popts->optarg, &pdata->mc_entry.mcast_addr);
            IFX_PPACMD_DBG("getmcextra  group ip: %d.%d.%d.%d\n", NIPQUAD( (pdata->mc_entry.mcast_addr.ip.ip)) );
            g_opt ++;
            break;

        case 'o':
            g_output_to_file = 1;
            strncpy(g_output_filename, popts->optarg, PPACMD_MAX_FILENAME);
            break;

        default:
            IFX_PPACMD_PRINT("getmcextra not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( g_opt != 1)  return PPA_CMD_ERR;

    return PPA_CMD_OK;
}

static void ppa_print_get_mc_extra_cmd(PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
    {
        IFX_PPACMD_PRINT("multicast group %d.%d.%d.%d extra properties:\n", NIPQUAD(pdata->mc_entry.mcast_addr));
        if(pdata->mc_entry. mc_extra.dscp_remark )
            IFX_PPACMD_PRINT("  new_dscp=%04x. ", (unsigned int)pdata->mc_entry.mc_extra.new_dscp );
        else IFX_PPACMD_PRINT("Not new dscp editing. ");

        if(pdata->mc_entry.mc_extra.vlan_insert )
            IFX_PPACMD_PRINT("  New  inner vlan =%04x. ", (unsigned int)(pdata->mc_entry.mc_extra.vlan_prio<<13) + (pdata->mc_entry.mc_extra.vlan_cfi<<12) +  pdata->mc_entry.mc_extra.vlan_id);
        else if(pdata->mc_entry.mc_extra.vlan_remove )
            IFX_PPACMD_PRINT("Remove inner vlan.  ");
        else IFX_PPACMD_PRINT("No inner vlan editing. ");

        if(pdata->mc_entry.mc_extra.out_vlan_insert )
            IFX_PPACMD_PRINT("New outvlan=%04x.  ", (unsigned int)pdata->mc_entry.mc_extra.out_vlan_tag );
        else if(pdata->mc_entry.mc_extra.out_vlan_remove )
            IFX_PPACMD_PRINT("Remove out vlan.  ");
        else IFX_PPACMD_PRINT("No out vlan editing. ");

        if( pdata->mc_entry.mc_extra.dslwan_qid_remark )
            IFX_PPACMD_PRINT("qid: %d\n", pdata->mc_entry.mc_extra.dslwan_qid );

        IFX_PPACMD_PRINT("\n");

    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->mc_entry), sizeof(pdata->mc_entry) );
    }

}


/*
====================================================================================
   command:   getmcnum
   description: get multicast groups count
   options:
====================================================================================
*/

static void ppa_get_mc_count_help(int summary)
{
    IFX_PPACMD_PRINT("getmcnum [-o file ]\n");
    return;
}

static int ppa_parse_get_mc_count_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    int res;

    res =   ppa_parse_simple_cmd( popts, pdata );

    if( res != PPA_CMD_OK ) return res;


    pdata->count_info.flag = 0;

    return PPA_CMD_OK;
}
/*
====================================================================================
   command:   getmcgroups
   description: get all multicast groups information
   options:
====================================================================================
*/

static void ppa_get_mc_groups_help( int summary)
{
    IFX_PPACMD_PRINT("getmcgroups -g <multicast group > -s <source ip> -f <ssm flag> [-o file ]\n"); // [ -m <multicast-mac-address (for bridging only)>]
    return;
}

static const char ppa_get_mc_group_short_opts[] = "g:s:f:o"; //need to further implement add/remove/modify vlan and enable new dscp and its value
static const struct option ppa_get_mc_group_long_opts[] =
{
    {"multicast group",   required_argument,  NULL, 'g'},
    {"source_ip",         required_argument,  NULL, 's'},
    {"ssm_flag",          required_argument,  NULL, 'f'},
    {"save-to-file",      required_argument,  NULL, 'o'},
    { 0,0,0,0 }
};

static int ppa_parse_get_mc_group_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    int ret;
    memset(pdata, 0, sizeof(*pdata) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        
        case 'g':   /*Support IPv4 and IPv6 */
            ret = str_convert(STRING_TYPE_IP, popts->optarg, pdata->mc_entry.mcast_addr.ip.ip6);
            if(ret == IP_NON_VALID){
                IFX_PPACMD_DBG("Multicast group ip is not a valid IPv4 or IPv6 IP address: %s\n", popts->optarg);
				break;
            }else if(ret == IP_VALID_V6){
                IFX_PPACMD_DBG("MLD GROUP IP: "NIP6_FMT"\n", NIP6(pdata->mc_entry.mcast_addr.ip.ip6));
                pdata->mc_entry.mcast_addr.f_ipv6= 1;
            }
            
            break;

        case 's': // Src IP, Support IPv4 & IPv6

            ret = str_convert(STRING_TYPE_IP, popts->optarg, pdata->mc_entry.source_ip.ip.ip6);
            if(ret == IP_NON_VALID){
                IFX_PPACMD_DBG("Multicast group ip is not a valid IPv4 or IPv6 IP address: %s\n", popts->optarg);
				break;
            }else if(ret == IP_VALID_V6){
                IFX_PPACMD_DBG("Source IP: "NIP6_FMT"\n", NIP6(pdata->mc_entry.mcast_addr.ip.ip6));
                pdata->mc_entry.source_ip.f_ipv6 = 1;
            }
            break;

        case 'f':
            pdata->mc_entry.SSM_flag = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);;
            IFX_PPACMD_DBG("addmc SSM_flag:%d \n",pdata->mc_entry.SSM_flag);
            break;

        case 'o':
            g_output_to_file = 1;
            strncpy(g_output_filename, popts->optarg, PPACMD_MAX_FILENAME);
            break;

        default:
            IFX_PPACMD_PRINT("mc_get_mc_group not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    return PPA_CMD_OK;
}

static int ppa_get_mc_groups_cmd (PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata)
{
    PPA_CMD_MC_GROUPS_INFO *psession_buffer;
    PPA_CMD_DATA cmd_info;
    int res = PPA_CMD_OK, i, j;
	uint32_t size;
    uint32_t flag = PPA_CMD_GET_MC_GROUPS;
    char str_srcip[64], str_dstip[64];

    //get session count first before malloc memroy
    cmd_info.count_info.flag = 0;
    if( ppa_do_ioctl_cmd(PPA_CMD_GET_COUNT_MC_GROUP, &cmd_info ) != PPA_CMD_OK )
        return -EIO;

    if( cmd_info.count_info.count == 0 )
    {
        IFX_PPACMD_PRINT("MC groups count=0\n");
        if( g_output_to_file )
            SaveDataToFile(g_output_filename, (char *)(&cmd_info.count_info),  sizeof(cmd_info.count_info)  );
        return PPA_CMD_OK;
    }

    //malloc memory and set value correctly
    size = sizeof(PPA_CMD_COUNT_INFO) + sizeof(PPA_CMD_MC_GROUP_INFO) * ( 1 + cmd_info.count_info.count ) ;
    psession_buffer = (PPA_CMD_MC_GROUPS_INFO *) malloc( size );
    if( psession_buffer == NULL )
    {
        IFX_PPACMD_PRINT("Malloc %d bytes failed\n", size );
        return PPA_CMD_NOT_AVAIL;
    }
    IFX_PPACMD_DBG("Get buffer size=%ul\n", size);
    memset( psession_buffer, 0, size);

    psession_buffer->count_info.count = cmd_info.count_info.count;
    psession_buffer->count_info.flag = 0;

    IFX_PPACMD_DBG("specified dst ip: %d.%d.%d.%d, src ip: %d.%d.%d.%d \n", 
        NIPQUAD(pdata->mc_entry.mcast_addr.ip.ip),
        NIPQUAD(pdata->mc_entry.source_ip.ip.ip));

    //get session information
    if( (res = ppa_do_ioctl_cmd(flag, psession_buffer ) != PPA_CMD_OK ) )
    {
        free( psession_buffer );
        return res;
    }

    IFX_PPACMD_DBG("MC groups total count=%u. \n", (unsigned int)psession_buffer->count_info.count);

#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
            PPA_CMD_MIB_MODE_INFO mode_info;
            char str_mib[12];
            if( ppa_do_ioctl_cmd(PPA_CMD_GET_MIB_MODE, &mode_info) == PPA_CMD_OK )
            {
                if(mode_info.mib_mode == 1)
                        strcpy(str_mib," packet");
                else
                        strcpy(str_mib," byte");
            }
            else
            {
                free( psession_buffer );
                return PPA_CMD_ERR;
            }
#endif


    if( !g_output_to_file )
    {
        for(i=0; i<psession_buffer->count_info.count; i++ )
        { 
            if(!is_ip_zero(&pdata->mc_entry.mcast_addr)){
                if(!ip_equal(&pdata->mc_entry.mcast_addr,&psession_buffer->mc_group_list[i].mc.mcast_addr)
                    || (pdata->mc_entry.SSM_flag == 1 
                          && !ip_equal(&pdata->mc_entry.source_ip,&psession_buffer->mc_group_list[i].mc.source_ip)))
                          continue;
            }
            if(!psession_buffer->mc_group_list[i].mc.mcast_addr.f_ipv6){
                    if(psession_buffer->mc_group_list[i].mc.source_ip.ip.ip == 0){
                        sprintf(str_srcip, "%s", "ANY");
                    }else{
                        sprintf(str_srcip, NIPQUAD_FMT, NIPQUAD(psession_buffer->mc_group_list[i].mc.source_ip.ip.ip));
                    }
                    //format like: [002] Dst: 239.  2.  2.  3  Src: 1.1.1.1 \n\t (route) qid  0 vlan 0000/04x From  nas0 to  eth0 ADDED_IN_HW|VALID_PPPOE|VALID_NEW_SRC_MAC
                    
                    IFX_PPACMD_PRINT("[%03u] MC GROUP:%3u.%3u.%3u.%3u Src IP: %s \n\t (%s) qid(%2u) dscp(%2u) vlan (%04x/%04x) mib (%llu:%llu(cpu:hw mib in%s)) ", i,
                             NIPQUAD((psession_buffer->mc_group_list[i].mc.mcast_addr.ip.ip)), str_srcip,
                             psession_buffer->mc_group_list[i].bridging_flag ? "bridge":"route ",
                             (unsigned int)psession_buffer->mc_group_list[i].mc.mc_extra.dslwan_qid,
                             (unsigned int)psession_buffer->mc_group_list[i].mc.mc_extra.new_dscp,
                             (unsigned int)( psession_buffer->mc_group_list[i].mc.mc_extra.vlan_prio << 13) |( psession_buffer->mc_group_list[i].mc.mc_extra.vlan_cfi << 12) | psession_buffer->mc_group_list[i].mc.mc_extra.vlan_id,
                             (unsigned int)psession_buffer->mc_group_list[i].mc.mc_extra.out_vlan_tag,
                             (unsigned long long)psession_buffer->mc_group_list[i].mc.mips_bytes,
                             (unsigned long long)psession_buffer->mc_group_list[i].mc.hw_bytes,
                             str_mib
                             );

#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
                    if( psession_buffer->mc_group_list[i].mc.RTP_flag == 1)
                    {
                        IFX_PPACMD_PRINT("rtp pkt cnt(%u) rtp seq num(%u) ", 
                             psession_buffer->mc_group_list[i].mc.rtp_pkt_cnt,
                             psession_buffer->mc_group_list[i].mc.rtp_seq_num
                             );
                     }
#endif
            }else{
                if(is_ip_zero(&psession_buffer->mc_group_list[i].mc.source_ip)){
                    sprintf(str_srcip, "%s", "ANY");
                }else{
                    sprintf(str_srcip, NIP6_FMT, NIP6(psession_buffer->mc_group_list[i].mc.source_ip.ip.ip6));
                }
                sprintf(str_dstip, NIP6_FMT, NIP6(psession_buffer->mc_group_list[i].mc.mcast_addr.ip.ip6));

                IFX_PPACMD_PRINT("[%03u] MC GROUP:%s Src IP: %s \n\t (%s) qid(%2u) dscp(%2u) vlan (%04x/%04x) mib (%llu:%llu(mips:hw)) ", i,
                             str_dstip, str_srcip,
                             psession_buffer->mc_group_list[i].bridging_flag ? "bridge":"route ",
                             (unsigned int)psession_buffer->mc_group_list[i].mc.mc_extra.dslwan_qid,
                             (unsigned int)psession_buffer->mc_group_list[i].mc.mc_extra.new_dscp,
                             (unsigned int)( psession_buffer->mc_group_list[i].mc.mc_extra.vlan_prio << 13) |( psession_buffer->mc_group_list[i].mc.mc_extra.vlan_cfi << 12) | psession_buffer->mc_group_list[i].mc.mc_extra.vlan_id,
                             (unsigned int)psession_buffer->mc_group_list[i].mc.mc_extra.out_vlan_tag,
                             (unsigned long long)psession_buffer->mc_group_list[i].mc.mips_bytes,
                             (unsigned long long)psession_buffer->mc_group_list[i].mc.hw_bytes
                             );
#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
                    if( psession_buffer->mc_group_list[i].mc.RTP_flag == 1)
                    {
                        IFX_PPACMD_PRINT("rtp pkt cnt(%u) rtp seq num(%u)", 
                             psession_buffer->mc_group_list[i].mc.rtp_pkt_cnt,
                             psession_buffer->mc_group_list[i].mc.rtp_seq_num
                             );
                     }
#endif

                
            }

            if( strlen(psession_buffer->mc_group_list[i].src_ifname ) == 0 )
                IFX_PPACMD_PRINT("From N/A ");
            else
            {
                IFX_PPACMD_PRINT("From  %s ", psession_buffer->mc_group_list[i].src_ifname);
            }

            if( psession_buffer->mc_group_list[i].num_ifs ==0 ||psession_buffer->mc_group_list[i].lan_ifname[0] == 0 )
                IFX_PPACMD_PRINT("to N/A");
            else
            {
                IFX_PPACMD_PRINT("to  ");
                for(j=0; j<psession_buffer->mc_group_list[i].num_ifs; j++)
                {
                    if( j == 0 )
                        IFX_PPACMD_PRINT("%s", psession_buffer->mc_group_list[i].lan_ifname[j]);
                    else
                        IFX_PPACMD_PRINT("/%s", psession_buffer->mc_group_list[i].lan_ifname[j]);
                }
            }
            IFX_PPACMD_PRINT("\n\t ");
            print_session_flags( psession_buffer->mc_group_list[i].mc.flags );
            IFX_PPACMD_PRINT("\n");
        }
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(psession_buffer),  size  );
    }

    free( psession_buffer );
    return PPA_CMD_OK;
}


/*
====================================================================================
   command:   setmcextra
   description: set multicast extra information, like vlan/dscp
   options:
====================================================================================
*/

static const char    ppa_set_mc_extra_short_opts[] = "g:q:i:o:t:h"; //need to further implement add/remove/modify vlan and enable new dscp and its value
static const struct option ppa_set_mc_extra_long_opts[] =
{
    {"multicast group",   required_argument,  NULL, 'g'},
    {"dscp",   required_argument,  NULL, 'q'},
    {"inner vlan",   required_argument,  NULL, 'i'},
    {"outer vlan",   required_argument,  NULL, 'o'},
    {"qid",   required_argument,  NULL, 't'},

    { 0,0,0,0 }
};
static void ppa_set_mc_extra_help( int summary)
{
    if( !summary )
    {
        IFX_PPACMD_PRINT("setmcextra -g <multicast group> [-q <new dscp_value>] [-i <insert:<vlan_tag>|remove>] ...\n");
    }
    else
    {
        IFX_PPACMD_PRINT("setmcextra -g <multicast group> [-q <new dscp_value>] [-i <insert:<vlan_tag>|remove>] [-o <insert:<vlan_tag>|remove] \n");
        IFX_PPACMD_PRINT("      dscp: 4 bits ( Only for A5/D5)\n");
        IFX_PPACMD_PRINT("      vlan_tag:  16 bits: priority(3 bits), cfi(1 bit) and vlan id (10bits) \n");
    }
    return;
}

static int ppa_parse_set_mc_extra_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int g_opt=0, q_opt=0, inner_vlan_opt=0, out_vlan_opt=0;
    char *insert_format="insert:";
    char *remove_format="remove";
    uint32_t vlanid ;

    memset(pdata, 0, sizeof(*pdata) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'g': //multicast group
            inet_aton(popts->optarg, ( struct in_addr *)&pdata->mc_entry.mcast_addr);
            g_opt ++;
            break;

        case 'q': //dscp
            pdata->mc_entry.mc_extra.new_dscp = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            pdata->mc_entry.mc_extra.dscp_remark = 1;
            pdata->mc_entry.flags |= PPA_F_SESSION_NEW_DSCP;
            q_opt ++;
            break;

        case 'i': //inner vlan
            if( strncmp( popts->optarg, insert_format, strlen(insert_format) ) == 0 )
            {
                char *p = popts->optarg +strlen(insert_format);
                vlanid = str_convert(STRING_TYPE_INTEGER,  p, NULL);
                if( vlanid == 0 ) return PPA_CMD_ERR;

                pdata->mc_entry.mc_extra.vlan_insert= 1;
                pdata->mc_entry.mc_extra.vlan_prio = ( vlanid  >> 13 ) & 7;
                pdata->mc_entry.mc_extra.vlan_cfi = ( vlanid  >> 12 ) & 1;
                pdata->mc_entry.mc_extra.vlan_id = vlanid & 0xFFF;
            }
            else if( strncmp( popts->optarg, remove_format, strlen(remove_format) ) == 0 )
            {
                pdata->mc_entry.mc_extra.vlan_remove = 1;
            }
            inner_vlan_opt ++;
            pdata->mc_entry.flags |= PPA_F_SESSION_VLAN;
            break;

        case 'o': //outer vlan
            if( strncmp( popts->optarg, insert_format, strlen(insert_format) ) == 0 )
            {
                char *p = popts->optarg +strlen(insert_format);
                vlanid = str_convert(STRING_TYPE_INTEGER,  p, NULL);
                if( vlanid == 0 ) return PPA_CMD_ERR;

                pdata->mc_entry.mc_extra.out_vlan_insert= 1;
                pdata->mc_entry.mc_extra.out_vlan_tag = vlanid;

            }
            else if( strncmp( popts->optarg, remove_format, strlen(remove_format) ) == 0 )
            {
                pdata->mc_entry.mc_extra.out_vlan_remove = 1;
            }
            out_vlan_opt ++;
            pdata->mc_entry.flags |= PPA_F_SESSION_OUT_VLAN;
            break;

        case 't':   //future only
        {
            pdata->mc_entry.mc_extra.dslwan_qid_remark = 1;
            pdata->mc_entry.mc_extra.dslwan_qid = str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);
        }

        default:
            IFX_PPACMD_PRINT("set_mc_extra not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( g_opt != 1 || inner_vlan_opt >1 || out_vlan_opt > 1 )
        return PPA_CMD_ERR;

    IFX_PPACMD_DBG("setmcextra: %d.%d.%d.%d\n", NIPQUAD( (pdata->mc_entry.mcast_addr.ip.ip)) );

    if( pdata->mc_entry.mc_extra.dscp_remark )
        IFX_PPACMD_DBG("  New dscp: 0x%x\n", pdata->mc_entry.mc_extra.new_dscp );

    if( pdata->mc_entry.mc_extra.vlan_insert )
    {
        vlanid = ( pdata->mc_entry.mc_extra.vlan_prio<< 13 ) + (pdata->mc_entry.mc_extra.vlan_cfi  << 12) + pdata->mc_entry.mc_extra.vlan_id;
        IFX_PPACMD_DBG("  New inner vlan: %04x\n", (unsigned int)vlanid );
    }
    else if( pdata->mc_entry.mc_extra.vlan_remove)
    {
        IFX_PPACMD_DBG("  Remove inner vlan. \n");
    }


    if( pdata->mc_entry.mc_extra.out_vlan_insert )
    {
        IFX_PPACMD_DBG("  New outer vlan: %04x\n", (unsigned int)pdata->mc_entry.mc_extra.out_vlan_tag);
    }
    else if( pdata->mc_entry.mc_extra.out_vlan_remove)
    {
        IFX_PPACMD_DBG("  Remove outer vlan. \n");
    }

    if( pdata->mc_entry.mc_extra.dslwan_qid_remark )
        IFX_PPACMD_DBG("  New dslwan_qid:%0u\n", (unsigned int)pdata->mc_entry.mc_extra.dslwan_qid);

    return PPA_CMD_OK;
}

/*
====================================================================================
   command:   getlansessionum/getwansessionnum
   description: get LAN/WAN session count
   options:
====================================================================================
*/
static void ppa_get_lan_session_count_help( int summary)
{
    IFX_PPACMD_PRINT("getlansessionnum [-w | n | a ] [-o file ]\n");
    
    if( summary )
    {
        IFX_PPACMD_PRINT("  -w: all accelerated sessions in PPA\n");
        IFX_PPACMD_PRINT("  -n: all non-accelerated sessions in PPA\n");
        IFX_PPACMD_PRINT("  -a: all sessions in PPA\n");
    }
    return;
}

static void ppa_get_wan_session_count_help( int summary)
{
    IFX_PPACMD_PRINT("getwansessionnum [-w | n | a ] [-o file ]\n");
    if( summary )
    {
        IFX_PPACMD_PRINT("  -w: all accelerated sessions in PPA\n");
        IFX_PPACMD_PRINT("  -n: all non-accelerated sessions in PPA\n");
        IFX_PPACMD_PRINT("  -a: all sessions in PPA\n");
    }
    return;
}

static const char ppa_get_simple_short_opts[] = "o:ah";
static const struct option ppa_get_simple_long_opts[] =
{
    {"save-to-file",   required_argument,  NULL, 'o'},
    {"all",   no_argument,  NULL, 'a'},
    { 0,0,0,0 }
};

/*this is a simple template parse command. At most there is only one parameter for saving result to file */
static int ppa_parse_simple_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'o':
            g_output_to_file = 1;
            strncpy(g_output_filename, popts->optarg, PPACMD_MAX_FILENAME);
            break;

        case 'a':
            g_all = 1;
            break;

        case 'w': //only for getlan/wassessionnum
            pdata->session_info.count_info.flag = SESSION_ADDED_IN_HW;
            break;
        
        case 'n': //only for getlan/wassessionnum
            pdata->session_info.count_info.flag = SESSION_NON_ACCE_MASK;
            break;
        
        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    return PPA_CMD_OK;
}

static int ppa_parse_get_lan_session_count(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    int res;

    pdata->count_info.flag =0;
    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'w':
            pdata->count_info.flag = SESSION_ADDED_IN_HW;
            break;

        case 'n':
            pdata->count_info.flag = SESSION_NON_ACCE_MASK;
            break;       

        case 'a':
            pdata->count_info.flag = 0;
            break;

        case 'o':
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    return PPA_CMD_OK;
}


static int ppa_parse_get_wan_session_count(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    int res;

    pdata->count_info.flag =0;
    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'w':
            pdata->count_info.flag = SESSION_ADDED_IN_HW;
            break;

        case 'n':
            pdata->count_info.flag = SESSION_NON_ACCE_MASK;
            break;       

        case 'a':
            pdata->count_info.flag = 0;
            break;

        case 'o':
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    return PPA_CMD_OK;
}


/*
====================================================================================
   command:   getlansessions/getwansessions
   description: get LAN/WAN all session detail information
   options:
====================================================================================
*/

static const char ppa_get_session_short_opts[] = "wnafd:o:h";
static void ppa_get_lan_sessions_help( int summary)
{
    IFX_PPACMD_PRINT("getlansessions [-w] [-a ] [-n ] [ -f ] [-o file ] [-d delay]\n");
    if( summary )
    {
        IFX_PPACMD_PRINT("  -w: all accelerated sessions in PPA\n");
        IFX_PPACMD_PRINT("  -n: all non-accelerated sessions in PPA\n");
        IFX_PPACMD_PRINT("  -a: all sessions in PPA\n");
        IFX_PPACMD_PRINT("  -f: Set SESSION_BYTE_STAMPING flag for testing purpose\n");
        IFX_PPACMD_PRINT("  -d: sleep time in seconds. For testing purpose\n");
    }
    return;
}

static const char ppa_get_session_count_short_opts[] = "wnao:h";
static int delay_in_second =0;

static void ppa_get_wan_sessions_help( int summary)
{
    IFX_PPACMD_PRINT("getwansessions [-w] [-a ] [-n ] [ -f ] [-o file ] [-d delay]\n");
    if( summary )
    {
        IFX_PPACMD_PRINT("  -w: all accelerated sessions in PPA\n");
        IFX_PPACMD_PRINT("  -n: all non-accelerated sessions in PPA\n");
        IFX_PPACMD_PRINT("  -a: all sessions in PPA\n");
        IFX_PPACMD_PRINT("  -f: Set SESSION_BYTE_STAMPING flag for testing purpose\n");
        IFX_PPACMD_PRINT(" -d: sleep time in seconds. For testing purpose\n");
    }
    return;
}

static int ppa_parse_get_session_cmd(PPA_CMD_OPTS *popts,PPA_CMD_DATA *pdata)
{
    pdata->session_info.count_info.flag = 0;  //default get all lan or wan sessions

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'w':
            pdata->session_info.count_info.flag = SESSION_ADDED_IN_HW;
            break;

        case 'n':
            pdata->session_info.count_info.flag = SESSION_NON_ACCE_MASK;
            break;

        case 'a':
            pdata->session_info.count_info.flag = 0;
            break;

        case 'f':
            pdata->session_info.count_info.stamp_flag |= SESSION_BYTE_STAMPING;
            break;

       case 'd':
            delay_in_second= str_convert(STRING_TYPE_INTEGER,  popts->optarg, NULL);
            break;     

        case 'o':
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    return PPA_CMD_OK;
}

static PPA_CMD_SESSIONS_INFO *ppa_get_sessions_malloc (uint32_t session_flag, uint32_t flag, uint32_t stamp_flag, uint32_t hash_index)
{
    PPA_CMD_SESSIONS_INFO *psession_buffer;
    PPA_CMD_DATA cmd_info;
    int res = PPA_CMD_OK, i, size;

    memset( &cmd_info, 0, sizeof(cmd_info) );
    cmd_info.count_info.flag = session_flag;
    cmd_info.count_info.stamp_flag = 0;
    cmd_info.count_info.hash_index = hash_index;

    if( flag == PPA_CMD_GET_LAN_SESSIONS )
    {
        if( ppa_do_ioctl_cmd(PPA_CMD_GET_COUNT_LAN_SESSION, &cmd_info ) != PPA_CMD_OK )
            return NULL;
    }
    else if( flag == PPA_CMD_GET_WAN_SESSIONS )
    {
        if( ppa_do_ioctl_cmd(PPA_CMD_GET_COUNT_WAN_SESSION, &cmd_info ) != PPA_CMD_OK )
            return NULL;
    }    
    else if( flag == PPA_CMD_GET_LAN_WAN_SESSIONS )
    {
        IFX_PPACMD_DBG("PPA_CMD_GET_LAN_WAN_SESSIONS\n");
        if( ppa_do_ioctl_cmd(PPA_CMD_GET_COUNT_LAN_WAN_SESSION, &cmd_info ) != PPA_CMD_OK )
            return NULL;
    }
    
    if( cmd_info.count_info.count == 0 )
    {
        IFX_PPACMD_DBG("session count=0. \n");
        if( g_output_to_file )
            SaveDataToFile(g_output_filename, (char *)(&cmd_info.count_info),  sizeof(cmd_info.count_info)  );
        return NULL;
    }

    //malloc memory and set value correctly
    size = sizeof(PPA_CMD_SESSIONS_INFO) + sizeof(PPA_CMD_SESSION_ENTRY) * ( cmd_info.count_info.count + 1 );
    psession_buffer = (PPA_CMD_SESSIONS_INFO *) malloc( size );
    if( psession_buffer == NULL )
    {
        IFX_PPACMD_PRINT("failed to get %d bytes buffer\n", psession_buffer );
        return NULL;
    }
    
    memset( psession_buffer, 0, sizeof(size) );

    psession_buffer->count_info.count = cmd_info.count_info.count;
    psession_buffer->count_info.flag = session_flag;
    psession_buffer->count_info.stamp_flag = stamp_flag;
    psession_buffer->count_info.hash_index = hash_index;
    //get session information
    if( delay_in_second ) sleep(delay_in_second);
    if( (res = ppa_do_ioctl_cmd(flag, psession_buffer ) != PPA_CMD_OK ) )
    {
        free( psession_buffer );
        IFX_PPACMD_PRINT("ppa_get_sessions_malloc failed for ioctl not succeed.\n");
        return NULL;
    }

    
    return psession_buffer;
}


static int ppa_get_sessions (PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata, uint32_t flag)
{
    PPA_CMD_SESSIONS_INFO *psession_buffer;
    PPA_CMD_DATA cmd_info;
    int res = PPA_CMD_OK, i, size;
    unsigned int hash_index = 0;
    unsigned int session_no=0;

    for(hash_index=0; hash_index<SESSION_LIST_HASH_TABLE_SIZE; hash_index++)
    {
        psession_buffer = ppa_get_sessions_malloc(pdata->session_info.count_info.flag, flag, pdata->session_info.count_info.stamp_flag, hash_index+1);
        if( !psession_buffer )
        {
            continue;
        }
        
        if( !g_output_to_file )
        {
            IFX_PPACMD_PRINT("\nSession Information: %5u in PPA Hash[%02u] -------\n", (unsigned int)psession_buffer->count_info.count, hash_index );
#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
            PPA_CMD_MIB_MODE_INFO mode_info;
            char str_mib[12];
            if( ppa_do_ioctl_cmd(PPA_CMD_GET_MIB_MODE, &mode_info) == PPA_CMD_OK )
            {
                if(mode_info.mib_mode == 1)
                        strcpy(str_mib," packet");
                else
                        strcpy(str_mib," byte");
            }
            else
            {
                free( psession_buffer );
                return PPA_CMD_ERR;
            }
#endif

            for(i=0; i<psession_buffer->count_info.count; i++ )
            {
                //print format: <packet index> <packet-type>  <rx interface name> (source ip : port) -> <tx interface name> ( dst_ip : dst_port ) nat ( nat_ip: nat_port )
                IFX_PPACMD_PRINT("[%03d]", i + session_no);

                if( psession_buffer->session_list[i].ip_proto == 0x11 ) //UDP
                    IFX_PPACMD_PRINT("udp");
                else if( psession_buffer->session_list[i].ip_proto == 6)
                    IFX_PPACMD_PRINT("tcp");
                else
                    IFX_PPACMD_PRINT("---");
#ifdef CONFIG_IFX_PPA_IPv6_ENABLE
                if( psession_buffer->session_list[i].flags & SESSION_IS_IPV6 )

#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
                    IFX_PPACMD_PRINT(": %8s (%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%5d) -> %8s (%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%5d) NAT (%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%5d) (%llu:%llu(hw mib in %s )) @session0x%08x with coll %lu pri %lu\n",
                                     psession_buffer->session_list[i].rx_if_name, NIP6(psession_buffer->session_list[i].src_ip.ip6), psession_buffer->session_list[i].src_port,
                                     psession_buffer->session_list[i].tx_if_name, NIP6(psession_buffer->session_list[i].dst_ip.ip6), psession_buffer->session_list[i].dst_port,
                                     NIP6(psession_buffer->session_list[i].nat_ip.ip6),
                                     psession_buffer->session_list[i].nat_port,
                                     (unsigned long long)psession_buffer->session_list[i].mips_bytes, (unsigned long long)psession_buffer->session_list[i].hw_bytes,str_mib,
                                     (unsigned int) psession_buffer->session_list[i].session, (unsigned int) psession_buffer->session_list[i].collision_flag, (unsigned int) psession_buffer->session_list[i].priority);


#else
                    IFX_PPACMD_PRINT(": %8s (%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%5d) -> %8s (%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%5d) NAT (%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%5d) (%llu:%llu(hw)) @session0x%08x with coll %lu pri %lu\n",
                                     psession_buffer->session_list[i].rx_if_name, NIP6(psession_buffer->session_list[i].src_ip.ip6), psession_buffer->session_list[i].src_port,
                                     psession_buffer->session_list[i].tx_if_name, NIP6(psession_buffer->session_list[i].dst_ip.ip6), psession_buffer->session_list[i].dst_port,
                                     NIP6(psession_buffer->session_list[i].nat_ip.ip6), psession_buffer->session_list[i].nat_port,
                                     (unsigned long long)psession_buffer->session_list[i].mips_bytes, (unsigned long long)psession_buffer->session_list[i].hw_bytes,
                                     (unsigned int) psession_buffer->session_list[i].session, (unsigned int) psession_buffer->session_list[i].collision_flag, (unsigned int) psession_buffer->session_list[i].priority);
                else
#endif
#endif

#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
            IFX_PPACMD_PRINT(": %8s (%3d.%3d.%3d.%3d/%5d) -> %8s (%3d.%3d.%3d.%3d/%5d) NAT (%3d.%3d.%3d.%3d/%5d) (%llu:%llu(hw mib in %s))  @session0x%08x with coll %u pri %lu\n",
                                     psession_buffer->session_list[i].rx_if_name, NIPQUAD(psession_buffer->session_list[i].src_ip), psession_buffer->session_list[i].src_port,
                                     psession_buffer->session_list[i].tx_if_name, NIPQUAD(psession_buffer->session_list[i].dst_ip), psession_buffer->session_list[i].dst_port,
                                     NIPQUAD(psession_buffer->session_list[i].nat_ip), psession_buffer->session_list[i].nat_port, (unsigned long long)psession_buffer->session_list[i].mips_bytes, (unsigned long long)psession_buffer->session_list[i].hw_bytes,str_mib,
                                     (unsigned int) psession_buffer->session_list[i].session, (unsigned int) psession_buffer->session_list[i].collision_flag, (unsigned int) psession_buffer->session_list[i].priority);


#else
                    IFX_PPACMD_PRINT(": %8s (%3d.%3d.%3d.%3d/%5d) -> %8s (%3d.%3d.%3d.%3d/%5d) NAT (%3d.%3d.%3d.%3d/%5d) (%llu:%llu(hw))  @session0x%08x with coll %u pri %lu\n",
                                     psession_buffer->session_list[i].rx_if_name, NIPQUAD(psession_buffer->session_list[i].src_ip), psession_buffer->session_list[i].src_port,
                                     psession_buffer->session_list[i].tx_if_name, NIPQUAD(psession_buffer->session_list[i].dst_ip), psession_buffer->session_list[i].dst_port,
                                     NIPQUAD(psession_buffer->session_list[i].nat_ip), psession_buffer->session_list[i].nat_port,
                                     (unsigned long long)psession_buffer->session_list[i].mips_bytes, (unsigned long long)psession_buffer->session_list[i].hw_bytes,
                                     (unsigned int) psession_buffer->session_list[i].session, (unsigned int) psession_buffer->session_list[i].collision_flag, (unsigned int) psession_buffer->session_list[i].priority);

#endif
            }
        }
        else
        {
            SaveDataToFile(g_output_filename, (char *)(psession_buffer),  size  );
        }

        free( psession_buffer );
    }
    return PPA_CMD_OK;
}



static int ppa_get_lan_sessions_cmd (PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata)
{
    return ppa_get_sessions(pcmd, pdata, PPA_CMD_GET_LAN_SESSIONS);
}

static int ppa_get_wan_sessions_cmd (PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata)
{
    return ppa_get_sessions(pcmd, pdata, PPA_CMD_GET_WAN_SESSIONS);
}

#define MAX_SESSION_HASH_TABLE 2
typedef struct PPA_SESSION_STATISTIC_ITEM_LIST
{
    int32_t in_hash_num;
    int32_t acc_in_hash_num;  /* accelerated session number which put in the hash index's bucket */
    int32_t acc_in_collision_num;/* accelerated session number which have same hash index, but sit in collision table */
    int32_t not_acc_for_mgm_num; /* not accelerated session number for session managemnet purpuse */
    int32_t add_hw_fail_num;
    int32_t waiting_acc_sessions_num;
    struct list_head  add_hw_fail_list_head;
    int32_t other_not_acc_num; /* non_accelerated session number with unknow reasons */
    struct list_head  other_not_acc_list_head;
}PPA_SESSION_STATISTIC_ITEM_LIST;

typedef struct PPA_SESSION_STATISTIC
{
    int32_t hash_table_offset[MAX_SESSION_HASH_TABLE];
    int32_t hash_table_entries_size[MAX_SESSION_HASH_TABLE];
    int32_t max_table_hash_index_num[MAX_SESSION_HASH_TABLE];
    int32_t max_hash_bucket_per_index[MAX_SESSION_HASH_TABLE];
    int32_t max_collision[MAX_SESSION_HASH_TABLE];
    int32_t table_items_num;  //includes all tables items
    int32_t table_num;

    int32_t in_hash_num ;
    int32_t acc_in_hash_num ;
    int32_t waiting_session ;
    int32_t acc_in_collision_num ;
    int32_t add_hw_fail_num ;
    int32_t other_not_acc_num ;
    int32_t free_hash ; 
    uint32_t free_in_collision;

    int32_t valid_hash_num;
    int32_t failed_hash_num; //the session cannot be accelerated now for special reason, like cannot get its mac address and so on
    struct list_head  failed_hash_list_head;
    int32_t non_accelerable_sess_num; //canot acclerate session at all for PPA limitations
    struct list_head  non_accelerable_sess_list_head;
    int32_t not_acc_for_mgm_num;  //controlled by session management
    struct list_head  not_acc_for_mgm_list_head;


    PPA_SESSION_STATISTIC_ITEM_LIST table[1]; //dummy size. shared by all PPE FW session tables (LAN/WAN)
  
}PPA_SESSION_STATISTIC;


typedef struct PPA_SESSION_STATIS_ENTRY_INFO
{
    struct list_head hlist;
    void *session_addr;
}PPA_SESSION_STATIS_ENTRY_INFO;

static void ppa_free_session_mgm_list(struct list_head *hlist_head)
{       
    struct list_head *pos, *q;
    PPA_SESSION_STATIS_ENTRY_INFO *item;

    list_for_each_safe(pos, q, hlist_head)
    {
         item = list_entry(pos, struct PPA_SESSION_STATIS_ENTRY_INFO, hlist);
         list_del(pos);
         free(item);
    }
}

static int display_non_lan_wan_session_num = 0;
static int ppa_acceleration_session_hash_summary()
{
    PPA_CMD_SESSION_ENTRY *pp_item=NULL;
    PPA_SESSION_STATISTIC *session_statistics = NULL;
    int32_t index;
    PPA_SESSION_STATIS_ENTRY_INFO *item=NULL;
    int32_t i,j, hash_index;
    uint32_t buf_size;
    PPA_CMD_SESSIONS_INFO *sessions;
    PPA_CMD_MAX_ENTRY_INFO max_entries={0};

    //Get PPE HASH property
    if( ppa_do_ioctl_cmd(PPA_CMD_GET_MAX_ENTRY, &max_entries) != PPA_CMD_OK )
    {
        IFX_PPACMD_PRINT("PPA_CMD_GET_MAX_ENTRY failed\n");
        return PPA_CMD_ERR;
    }   
    IFX_PPACMD_DBG("max_lan_entries=%u\n", max_entries.entries.max_lan_entries);
    IFX_PPACMD_DBG("max_wan_entries=%u\n", max_entries.entries.max_wan_entries);
    IFX_PPACMD_DBG("max_lan_collision_entries=%u\n", max_entries.entries.max_lan_collision_entries);
    IFX_PPACMD_DBG("max_wan_collision_entries=%u\n", max_entries.entries.max_wan_collision_entries);
    IFX_PPACMD_DBG("session_hash_table_num=%u\n", max_entries.entries.session_hash_table_num);
    IFX_PPACMD_DBG("max_lan_hash_index_num=%u\n", max_entries.entries.max_lan_hash_index_num);
    IFX_PPACMD_DBG("max_wan_hash_index_num=%u\n", max_entries.entries.max_wan_hash_index_num);
    IFX_PPACMD_DBG("max_lan_hash_bucket_num=%u\n", max_entries.entries.max_lan_hash_bucket_num);
    IFX_PPACMD_DBG("max_wan_hash_bucket_num=%u\n", max_entries.entries.max_wan_hash_bucket_num);
    if( max_entries.entries.session_hash_table_num > MAX_SESSION_HASH_TABLE )
    {
        IFX_PPACMD_DBG("wrong session_hash_table_num %d\n", max_entries.entries.session_hash_table_num);
        goto END;
    }
    buf_size = sizeof(PPA_SESSION_STATISTIC) + sizeof(PPA_SESSION_STATISTIC_ITEM_LIST) * ( max_entries.entries.max_lan_hash_index_num + max_entries.entries.max_wan_hash_index_num) ;
    session_statistics = malloc( buf_size );
    if( !session_statistics ) 
    {
        IFX_PPACMD_PRINT( "ppa_malloc fail to allocat memory of %d bytes\n", buf_size);
        goto END;
    }
    IFX_PPACMD_DBG("Get session_statistics buffer buf_size=%d with hash index %d/%d support\n", buf_size, max_entries.entries.max_lan_hash_index_num , max_entries.entries.max_wan_hash_index_num );
    memset( session_statistics, 0, buf_size );
   
    session_statistics->table_num = max_entries.entries.session_hash_table_num;
    for(i=0; i<session_statistics->table_num; i++) 
    {
        if( i == 0 )
        {
            session_statistics->max_table_hash_index_num[i] = max_entries.entries.max_lan_hash_index_num;
            session_statistics->max_hash_bucket_per_index[i] = max_entries.entries.max_lan_hash_bucket_num;
            session_statistics->max_collision[i] = max_entries.entries.max_lan_collision_entries;
            session_statistics->table_items_num += max_entries.entries.max_lan_entries;
            session_statistics->hash_table_offset[i] = 0;
        }
        else if( i == 1 )
        {
            session_statistics->max_table_hash_index_num[i] = max_entries.entries.max_wan_hash_index_num;            
            session_statistics->max_hash_bucket_per_index[i] = max_entries.entries.max_wan_hash_bucket_num;            
            session_statistics->max_collision[i] = max_entries.entries.max_wan_collision_entries;
            session_statistics->table_items_num += max_entries.entries.max_wan_entries;
            session_statistics->hash_table_offset[i] = session_statistics->max_table_hash_index_num[0];
        }
    }     
  
    IFX_PPACMD_PRINT(" Table  #Index   #Bucket  #Collision  Total\n");
    for(i=0; i<session_statistics->table_num; i++)
    {
        char table_flag[10]="     "; //for future GRX500
        
        if( session_statistics->table_num > 1 )
        {
            if(i== 0 ) 
                strcpy(table_flag, "(LAN)");
            else if (i== 1 ) 
                strcpy(table_flag, "(WAN)"); 
        }
        IFX_PPACMD_PRINT(" %d%s %4d     %4d      %4d      %4d(%d + %d)\n", 
            i, table_flag,
            session_statistics->max_table_hash_index_num[i], 
            session_statistics->max_hash_bucket_per_index[i], 
            session_statistics->max_collision[i],
            session_statistics->max_table_hash_index_num[i] * session_statistics->max_hash_bucket_per_index[i] + session_statistics->max_collision[i],
            session_statistics->max_table_hash_index_num[i] * session_statistics->max_hash_bucket_per_index[i], 
            session_statistics->max_collision[i]
            );
    }    
    //initialize list
    IFX_PPACMD_DBG( "initialize list\n");
    INIT_LIST_HEAD(&session_statistics->failed_hash_list_head);
    INIT_LIST_HEAD(&session_statistics->non_accelerable_sess_list_head);
    INIT_LIST_HEAD(&session_statistics->not_acc_for_mgm_list_head);
    for(i=0; i<session_statistics->table_num; i++) 
    {
        int table_offset = session_statistics->hash_table_offset[i];
       
        for(j=0; j<session_statistics->max_table_hash_index_num[i]; j++ )
        {
            INIT_LIST_HEAD(&session_statistics->table[table_offset + j].add_hw_fail_list_head);
            INIT_LIST_HEAD(&session_statistics->table[table_offset + j].other_not_acc_list_head);
        }
    }
    IFX_PPACMD_DBG( "initialize done\n");

    for(hash_index=0; hash_index<SESSION_LIST_HASH_TABLE_SIZE; hash_index++ )
    {
        sessions = ppa_get_sessions_malloc(0, PPA_CMD_GET_LAN_WAN_SESSIONS, 0, hash_index+1);

        IFX_PPACMD_DBG( "Get session list: %d\n", sessions ? sessions->count_info.count: 0);
        //loop the session list
        if( sessions ) 
        {
            for( i=0; i<sessions->count_info.count; i ++)
            {
                if( !item )
                {
                    item = malloc(sizeof(PPA_SESSION_STATIS_ENTRY_INFO) );
                    if( !item ) 
                    {
                        IFX_PPACMD_DBG("ppa_malloc fail\n");
                        goto END;
                    }
                    INIT_LIST_HEAD(&item->hlist);
                }

                pp_item = &sessions->session_list[i];        
                
                if( pp_item->flags & SESSION_NOT_ACCELABLE )
                {  //this session cannot be accelerated at all, for example, hairping NAT, CPE local session,...
                    session_statistics->non_accelerable_sess_num ++;                
                    item->session_addr = (void *)pp_item;
                    list_add(&item->hlist,  &session_statistics->non_accelerable_sess_list_head);
                    item = NULL;
                    continue;
                }
                
                if( ! pp_item->flag2 & SESSION_FLAG2_HASH_INDEX_DONE )
                {  //not valid hash, for example for IPV6 session, if IP6 table full already, then new IP6 cannot be added and this sessinon cannot get its hash index at all
                    session_statistics->failed_hash_num ++;                                
                    item->session_addr = (void *)pp_item; 
                    list_add(&item->hlist,  &session_statistics->failed_hash_list_head);
                    item = NULL;
                    continue;
                }

                //check tash index validility
                if( (pp_item->hash_table_id >= MAX_SESSION_HASH_TABLE) && (pp_item->hash_table_id >= session_statistics->table_num) )
                {
                    IFX_PPACMD_DBG("wrong hash_table_id(%d) for session 0x%p\n", pp_item->hash_table_id, pp_item);
                    goto END;
                }
                if( pp_item->hash_index >= session_statistics->max_table_hash_index_num[pp_item->hash_table_id])
                {
                    IFX_PPACMD_DBG("wrong hash_index(%d) for session 0x%p. It should less than %d\n", 
                        pp_item->hash_index, 
                        pp_item, 
                        session_statistics->max_table_hash_index_num[pp_item->hash_table_id]);
                    goto END;
                }            
                index = session_statistics->hash_table_offset[pp_item->hash_table_id] + pp_item->hash_index;
                if( index >= session_statistics->table_items_num )
                {
                    IFX_PPACMD_DBG("wrong index(%d=%d + %d) for session 0x%p\n", pp_item->hash_table_id,
                            session_statistics->hash_table_offset[pp_item->hash_table_id],
                            pp_item->hash_index,
                            pp_item);
                    goto END;
                }

                session_statistics->table[index].in_hash_num ++;
                
                if( pp_item->flags & SESSION_NOT_ACCEL_FOR_MGM )
                {  
                    session_statistics->table[index].not_acc_for_mgm_num ++;
                    item->session_addr = (void *)pp_item;                
                    list_add(&item->hlist,  &session_statistics->not_acc_for_mgm_list_head);
                    item = NULL;
                    continue;
                }            
                if ( pp_item->flags & SESSION_ADDED_IN_HW )
                {                          
                    if( pp_item->collision_flag ) 
                        session_statistics->table[index].acc_in_collision_num ++;
                    else
                        session_statistics->table[index].acc_in_hash_num ++;

                    continue;
                }
            
                //can be accelerated, but not in acceleration path
                session_statistics->table[index].waiting_acc_sessions_num ++;
                
                if( pp_item->flag2 & SESSION_FLAG2_ADD_HW_FAIL )
                {                
                    session_statistics->table[index].add_hw_fail_num ++;    
                    
                    item->session_addr = (void *)pp_item;
                    list_add(&item->hlist,  &session_statistics->table[index].add_hw_fail_list_head);
                    item = NULL;
                    IFX_PPACMD_DBG("session_statistics->table[%d].add_hw_fail_num=%d", index, session_statistics->table[index].add_hw_fail_num);
                    continue;
                }

                //other non-acceleation reason, like mac address fail or interface not added into ppa yet.
                session_statistics->table[index].other_not_acc_num ++;                
                item->session_addr = (void *)pp_item;
                list_add(&item->hlist,  &session_statistics->table[index].other_not_acc_list_head);
                item = NULL;           
            }
        }
        if( sessions ) 
        {
            free(sessions);
            sessions = NULL;
        }
    }

    for(i=0; i<session_statistics->table_num; i++)
    {
        int table_offset = session_statistics->hash_table_offset[i];
        int collision_num = 0;

        int in_hash_num = 0;
        int acc_in_hash_num = 0;
        int waiting_session = 0;
        int acc_in_collision_num = 0;
        int add_hw_fail_num = 0;
        int not_acc_for_mgm_num = 0;
        int other_not_acc_num = 0;
        int free_hash = 0;
        int free_in_collision =0;
        
        IFX_PPACMD_PRINT("\nTable %d\n", i);
        IFX_PPACMD_PRINT("   Index Sessions  hash_usage  coll_usage  sess_fail sess_waiting others_fail  free\n");
        for(j=0; j<session_statistics->max_table_hash_index_num[i]; j++ )
        {
            in_hash_num += session_statistics->table[table_offset+j].in_hash_num;
            acc_in_hash_num += session_statistics->table[table_offset+j].acc_in_hash_num;
            waiting_session += session_statistics->table[table_offset+j].add_hw_fail_num + session_statistics->table[table_offset+j].other_not_acc_num;
            acc_in_collision_num += session_statistics->table[table_offset+j].acc_in_collision_num;
            add_hw_fail_num += session_statistics->table[table_offset+j].add_hw_fail_num;
            not_acc_for_mgm_num += session_statistics->table[table_offset+j].not_acc_for_mgm_num;
            other_not_acc_num += session_statistics->table[table_offset+j].other_not_acc_num;
            free_hash += session_statistics->max_hash_bucket_per_index[i] - session_statistics->table[table_offset+j].acc_in_hash_num;
            
            
            IFX_PPACMD_PRINT("  [%5d]  %4d        %4d        %4d        %4d        %4d        %4d    %4d\n", j, 
                session_statistics->table[table_offset+j].in_hash_num,
                session_statistics->table[table_offset+j].acc_in_hash_num,
                session_statistics->table[table_offset+j].acc_in_collision_num,
                session_statistics->table[table_offset+j].add_hw_fail_num,
                session_statistics->table[table_offset+j].not_acc_for_mgm_num,
                session_statistics->table[table_offset+j].other_not_acc_num,
                session_statistics->max_hash_bucket_per_index[i] - session_statistics->table[table_offset+j].acc_in_hash_num ); 
            
            collision_num += session_statistics->table[table_offset+j].acc_in_collision_num;
            if( session_statistics->table[table_offset+j].acc_in_hash_num > session_statistics->max_hash_bucket_per_index[i] )
            {
                IFX_PPACMD_PRINT("why acceleated (%d) is more than its bucket(%d)\n", 
                                session_statistics->table[table_offset+j].acc_in_hash_num,
                                session_statistics->max_hash_bucket_per_index[i] );
                                
            }
        }
        
        IFX_PPACMD_PRINT("  [total]  %4d        %4d        %4d        %4d        %4d        %4d    %4d\n",
                in_hash_num,
                acc_in_hash_num,
                acc_in_collision_num,
                add_hw_fail_num,
                not_acc_for_mgm_num,
                other_not_acc_num,
                free_hash ); 
        IFX_PPACMD_PRINT("  collision(used/free): %d/%d\n", 
                        collision_num,  
                        session_statistics->max_collision[i] - collision_num );
        
        if( collision_num > session_statistics->max_collision[i] )
        {
            IFX_PPACMD_PRINT("why acceleated (%d) is more than its bucket(%d)\n", 
                            collision_num,
                            session_statistics->max_collision[i] );
                            
        }

        //update statistics
        session_statistics->in_hash_num += in_hash_num;
        session_statistics->acc_in_hash_num += acc_in_hash_num;
        session_statistics->waiting_session += waiting_session;
        session_statistics->acc_in_collision_num += acc_in_collision_num;
        session_statistics->add_hw_fail_num += add_hw_fail_num;
        session_statistics->not_acc_for_mgm_num += not_acc_for_mgm_num;
        session_statistics->other_not_acc_num += other_not_acc_num;
        session_statistics->free_hash += free_hash;
        session_statistics->free_in_collision += session_statistics->max_collision[i] - collision_num; 
    }
    IFX_PPACMD_PRINT("\n");
    IFX_PPACMD_PRINT("   -sum-   %4d        %4d        %4d        %4d        %4d        %4d    %4d\n",
                session_statistics->in_hash_num,
                session_statistics->acc_in_hash_num,
                session_statistics->acc_in_collision_num,
                session_statistics->add_hw_fail_num,
                session_statistics->not_acc_for_mgm_num,
                session_statistics->other_not_acc_num,
                session_statistics->free_hash ); 
    IFX_PPACMD_PRINT("         Acclerated    %4d\n", session_statistics->acc_in_hash_num + session_statistics->acc_in_collision_num);
    IFX_PPACMD_PRINT("         Free          %4d\n", session_statistics->free_hash + session_statistics->free_in_collision);
    if( display_non_lan_wan_session_num )
    {
        PPA_CMD_DATA tmp_cmd_info;        
        if( ppa_do_ioctl_cmd(PPA_CMD_GET_COUNT_NONE_LAN_WAN_SESSION, &tmp_cmd_info ) == PPA_CMD_OK ) 
        {        
            IFX_PPACMD_PRINT("         Non LAN/WAN   %4d (unknown session for netif not added into PPA)\n", tmp_cmd_info.count_info.count );        
        }
        else
        {
            IFX_PPACMD_PRINT("Failed to get PPA_CMD_GET_COUNT_NONE_LAN_WAN_SESSION\n");
        }
    }

END: 
    if( session_statistics )
    {
        ppa_free_session_mgm_list(&session_statistics->failed_hash_list_head);
        ppa_free_session_mgm_list(&session_statistics->non_accelerable_sess_list_head);
        ppa_free_session_mgm_list(&session_statistics->not_acc_for_mgm_list_head);     
        for(i=0; i<session_statistics->table_num; i++) 
        {
            int table_offset = session_statistics->hash_table_offset[i];
            for(j=0; j<session_statistics->max_table_hash_index_num[i]; j++ )
            {
                ppa_free_session_mgm_list(&session_statistics->table[table_offset + j].add_hw_fail_list_head);
                ppa_free_session_mgm_list(&session_statistics->table[table_offset + j].other_not_acc_list_head);
            }
        }        
        free(session_statistics);
        session_statistics = NULL;
    }
    if( sessions )
    {
        free(sessions);
    }    

    return PPA_CMD_OK;
}

static int ppa_session_hash_summary()
{
    PPA_CMD_SESSION_SUMMARY_INFO *ppa_sum;
    PPA_CMD_COUNT_INFO count_info={0};
    int i, j,len, num=0;

    if( ppa_do_ioctl_cmd(PPA_CMD_GET_COUNT_PPA_HASH, &count_info ) != PPA_CMD_OK )
        return PPA_CMD_ERR;
    if( count_info.count <= 0 )
    {
        IFX_PPACMD_PRINT("Why PPA Hash Max Index num is not valid:%d\n", count_info.count);
        return PPA_CMD_ERR;
    }
    IFX_PPACMD_DBG("count_info.count: %d\n", count_info.count);
    len = sizeof(PPA_CMD_SESSION_SUMMARY_INFO) + sizeof(PPA_SESSION_SUMMARY) * count_info.count;
    ppa_sum =(PPA_CMD_SESSION_SUMMARY_INFO *) malloc( len );
    if( ppa_sum == NULL )
    {
        IFX_PPACMD_PRINT("Failed to malloc memory: %d bytes\n", len);
        return PPA_CMD_ERR;
    }
    memset(ppa_sum, 0, len);
    ppa_sum->hash_max_num = count_info.count;
    if( ppa_do_ioctl_cmd(PPA_CMD_GET_PPA_HASH_SUMMARY, ppa_sum ) != PPA_CMD_OK )
        return PPA_CMD_ERR;
    IFX_PPACMD_PRINT("\nPPA Hash Usage with %d hash index\n", count_info.count);
    for(i=0; i<count_info.count;  )
    {
        #define NUM_PER_LINE 30
         if( (i % NUM_PER_LINE ) == 0 )
            IFX_PPACMD_PRINT("[%5d]:", i);
         for(j=0; j<NUM_PER_LINE; j++ )
         {
             if( j + i >= count_info.count ) break;
             IFX_PPACMD_PRINT("%2u ", ppa_sum->hash_info.count[i+j]);
             num += ppa_sum->hash_info.count[i+j];             
         }
         IFX_PPACMD_PRINT("\n");
         i+= NUM_PER_LINE;         
    }
    IFX_PPACMD_PRINT("[total]:%4u\n", num);
    if( ppa_sum ) 
        free(ppa_sum);
    return PPA_CMD_OK;
}

static const char ppa_summary_short_opts[] = "wpah";
static void ppa_summary_help( int summary)
{
    IFX_PPACMD_PRINT("summary [-w] [-p ]\n");
    if( summary )
    {
        IFX_PPACMD_PRINT("  -a: all summeries\n");
        IFX_PPACMD_PRINT("  -w: acceleration session hash summary ( default )\n");
        IFX_PPACMD_PRINT("  -p: ppa session hash summary\n");
    }
    return;
}

static int ppa_parse_summary_cmd(PPA_CMD_OPTS *popts,PPA_CMD_DATA *pdata)
{
#define SUMMARY_PPE_SESSION 1
#define SUMMARY_PPA_SESSION 2

    uint32_t flag = 0;
    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'a':
            flag = ~0;
            break;
            
        case 'w':
            flag |= SUMMARY_PPE_SESSION;            
            break;

        case 'p':
            flag |= SUMMARY_PPA_SESSION;            
            break;

        case 'n':
            display_non_lan_wan_session_num = 1;
            break;

        case 'o':
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( flag == 0 ) 
        flag = SUMMARY_PPE_SESSION;

    if( flag & SUMMARY_PPE_SESSION )
        ppa_acceleration_session_hash_summary();
    if( flag & SUMMARY_PPA_SESSION )
        ppa_session_hash_summary();       
    
        
    return PPA_CMD_OK;


}


#ifdef NEED_DELSESSION // in fact, no need to delete session but instead we use modify sesion to add/del session from acceleration path
/*
====================================================================================
   command:   delsession
   description: delete a routing session
   options:
====================================================================================
*/
static const char ppa_del_session_short_opts[] = "a:f:t:s:d:p:h";
static const struct option ppa_del_session_long_opts[] =
{
    {"session address",   required_argument,  NULL, 'a'},
    {"src-ip",   required_argument,  NULL, 'f'},
    {"dst-ip",   required_argument,  NULL, 't'},
    {"src-port",   required_argument,  NULL, 's'},
    {"dst-port",   required_argument,  NULL, 'd'},
    {"proto",   required_argument,  NULL, 'p'},

    { 0,0,0,0 }
};

static void ppa_del_session_help( int summary)
{
    IFX_PPACMD_PRINT("delsession [-a ppa-session-address] [-f src-ip] [-t dst-ip ] [-s src-port ] [-d dst-port] [-p proto]\n");
    if( summary )
    {
        IFX_PPACMD_PRINT("  Note 1: source/destionation ip and port is named before NAT\n");
        IFX_PPACMD_PRINT("  Note 2: below combination should be provided\n");
        IFX_PPACMD_PRINT("    :   delsession -a xxxx \n");
        IFX_PPACMD_PRINT("    :      if session address is -1, it means to match all related ppa sessionn\n");
        IFX_PPACMD_PRINT("    : or\n");
        IFX_PPACMD_PRINT("    :   delsession -f xxx -t xxx -s xxx -d xxx -p udp/tcp \n");
        IFX_PPACMD_PRINT("    :      if src-ip/dst-ip is 0.0.0.0, it means to match all related ip\n");
        IFX_PPACMD_PRINT("    :      if src-port/dst-port is 0, it means to match all related port\n");
        IFX_PPACMD_PRINT("  Note 3: -p proto support udp/tcp/all. Default is udp only\n");
    }
    return;
}

static int ppa_parse_del_session_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int a_opt=0, f_opt=0, t_opt=0, s_opt=0,d_opt=0, p_opt=0;
    unsigned int ip1_type=0, ip2_type=0;

    memset( &pdata->session_info, 0, sizeof(pdata->session_info) );
    pdata->session_info.session_list[0].ip_proto = IFX_IPPROTO_UDP;

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'a':
            pdata->session_info.session_list[0].session = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            a_opt ++;
            break;

        case 'f':
            if( (ip1_type = str_convert(STRING_TYPE_IP, popts->optarg, (void *)&pdata->session_info.session_list[0].src_ip)) == IP_NON_VALID )
            {
                IFX_PPACMD_PRINT("Wrong source ip:%s\n", popts->optarg);
                return PPA_CMD_ERR;
            };
            if( ip1_type == IP_VALID_V6 )
                pdata->session_info.session_list[0].flags |= SESSION_IS_IPV6;

            f_opt ++;
            break;

        case 't':
            if( (ip2_type = str_convert(STRING_TYPE_IP, popts->optarg, (void *)&pdata->session_info.session_list[0].dst_ip)) == IP_NON_VALID )
            {
                IFX_PPACMD_PRINT("Wrong Dst ip:%s\n", popts->optarg);
                return PPA_CMD_ERR;
            };
            if( ip2_type == IP_VALID_V6 )
                pdata->session_info.session_list[0].flags |= SESSION_IS_IPV6;
            t_opt ++;
            break;

        case 's':
            pdata->session_info.session_list[0].src_port=  str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            s_opt ++;
            break;

        case 'd':
            pdata->session_info.session_list[0].dst_port=  str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            d_opt ++;
            break;

        case 'p':
            if( stricmp(popts->optarg, "tcp") == 0 )
            {
                pdata->session_info.session_list[0].ip_proto= IFX_IPPROTO_TCP;
            }
            else if( stricmp(popts->optarg, "all") == 0 )
            {
                pdata->session_info.session_list[0].ip_proto= 0;
            }
            else
            {
                pdata->session_info.session_list[0].ip_proto= IFX_IPPROTO_UDP;
            }
            p_opt ++;
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }
    if( a_opt == 0 && f_opt==0 && t_opt==0 && s_opt==0 &&d_opt ==0)  /*no parameter is provided */
    {
        IFX_PPACMD_PRINT("delsession [-a ppa-session-address] [-f src-ip] [-t dst-ip ] [-s src-port ] [-d dst-port] [-p proto]\n");
        return PPA_CMD_ERR;
    }
    if( a_opt && (f_opt|t_opt|s_opt|d_opt) )
    {
        IFX_PPACMD_PRINT("wrong input: -a and -f/t/s/d canot be used together.\n");
        return PPA_CMD_ERR;
    }
    if( a_opt > 1)
    {
        IFX_PPACMD_PRINT("wrong input: -a.\n");
        return PPA_CMD_ERR;
    }
    if( f_opt > 1 || t_opt > 1 || s_opt > 1 || d_opt > 1 )
    {
        IFX_PPACMD_PRINT("wrong input: -f/t/s/d.\n");
        return PPA_CMD_ERR;
    }
    if( ip1_type != ip2_type )
    {
        IFX_PPACMD_PRINT("srcip and dst-ip should match, ie, both are IPV4 or IPV6 address\n");
        return PPA_CMD_ERR;
    }
#ifndef CONFIG_IFX_PPA_IPv6_ENABLE
    if( ip1_type == IP_VALID_V6 )
    {
        IFX_PPACMD_PRINT("ppacmd not support IPV6. Pls recompile ppacmd\n");
        return PPA_CMD_ERR;
    }
#endif

    if( a_opt == 1)
    {
        IFX_PPACMD_DBG("delete session via ppa session address: 0x%08x\n", (unsigned int) pdata->session_info.session_list[0].session );
    }
    else
    {
#ifdef CONFIG_IFX_PPA_IPv6_ENABLE
        if( pdata->session_info.session_list[0].flags & SESSION_IS_IPV6 )
        {
            IFX_PPACMD_DBG("delete session via ppa session tuple:%s from %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%5d to %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%5d\n",
                    (pdata->session_info.session_list[0].ip_proto == IFX_IPPROTO_TCP)?"tcp":"udp",
                    NIP6(pdata->session_info.session_list[0].src_ip.ip6), pdata->session_info.session_list[0].src_port,
                    NIP6(pdata->session_info.session_list[0].dst_ip.ip6), pdata->session_info.session_list[0].dst_port);

        }
        else
#endif
        {
            IFX_PPACMD_DBG("delete session via ppa session tuple:%s from %u.%u.%u.%u/%5d to %u.%u.%u.%u/%5d\n",
                    (pdata->session_info.session_list[0].ip_proto == IFX_IPPROTO_TCP)?"tcp":"udp",
                    NIPQUAD(pdata->session_info.session_list[0].src_ip), pdata->session_info.session_list[0].src_port,
                    NIPQUAD(pdata->session_info.session_list[0].dst_ip), pdata->session_info.session_list[0].dst_port);
        }
    }
    return PPA_CMD_OK;

}
#endif

/*
====================================================================================
   command:   addsession
   description: delete a routing session
   options:
====================================================================================
*/
static const char ppa_add_session_short_opts[] = "f:t:s:d:p:i:w:x:y:n:m:o:r:c:q:z:h";
static const struct option ppa_add_session_long_opts[] =
{
    {"src-ip",   required_argument,  NULL, 'f'},
    {"dst-ip",   required_argument,  NULL, 't'},
    {"src-port",   required_argument,  NULL, 's'},
    {"dst-port",   required_argument,  NULL, 'd'},
    {"proto",   required_argument,  NULL, 'p'},
    {"dest_ifid",   required_argument,  NULL, 'i'},
    {"wan-flag",   required_argument,  NULL, 'w'},
    {"src-mac",   required_argument,  NULL, 'x'},
    {"dst-mac",   required_argument,  NULL, 'y'},


    {"nat-ip",   required_argument,  NULL, 'n'},
    {"nat-port",   required_argument,  NULL, 'm'},
    {"new_dscp",   required_argument,  NULL, 'o'},

    {"in_vlan_id",   required_argument,  NULL, 'r'},
    {"out_vlan_tag",   required_argument,  NULL, 'c'},
    {"qid",   required_argument,  NULL, 'q'},
    {"pppoe-id",   required_argument,  NULL, 'z'},

    { 0,0,0,0 }
};

#define DEFAULT_SRC_IP "192.168.168.100"
#define DEFAULT_DST_IP "192.168.0.100"
#define DEFAULT_SRC_PORT "1024"
#define DEFAULT_DST_PORT "1024"
#define DEFAULT_SRC_MAC "00:11:22:33:44:11"
#define DEFAULT_DST_MAC "00:11:22:33:44:22"
static void ppa_add_session_help( int summary)
{
    IFX_PPACMD_PRINT("addsession\n");
    if( summary )
    {
        IFX_PPACMD_PRINT("addsession [-f src-ip] [-t dst-ip ] [-s src-port ] [-d dst-port] [-p proto] [-i dest_ifid] [-w wan-flag]\n");
        IFX_PPACMD_PRINT("         [-x src-mac] [-y dst-mac]\n");
        IFX_PPACMD_PRINT("         [-n nat-ip] [-m nat-port ] [-o new_dscp ] \n");
        IFX_PPACMD_PRINT("         [-r in_vlan_id] [-c out_vlan_tag ] [-q queue-id ] [-z pppoe-id] \n");
        IFX_PPACMD_PRINT("  Note:  This commands is only for test purpose !!!\n");
        IFX_PPACMD_PRINT("        a) [-f src-ip]: default is %s\n", DEFAULT_SRC_IP);
        IFX_PPACMD_PRINT("        b) [-t dst-ip]: default is %s\n", DEFAULT_DST_IP);
        IFX_PPACMD_PRINT("        c) [-s src-port ]: default is %s\n", DEFAULT_SRC_PORT);
        IFX_PPACMD_PRINT("        d) [-d dst-port ]: default is %s\n", DEFAULT_DST_PORT);
        IFX_PPACMD_PRINT("        e) [-p proto]: default is udp.  Supported value are  udp and tcp\n");
        IFX_PPACMD_PRINT("        f) [-i dest_ifid]: default is 1, ie, eth1\n");
        IFX_PPACMD_PRINT("        g) [-w wan-flag]: default is lan, supported value are lan and wan\n");
        IFX_PPACMD_PRINT("        h) [-i dest_ifid]: default is 1, ie, eth1\n");
        IFX_PPACMD_PRINT("        i) [-x src-mac]: default is %s\n", DEFAULT_SRC_MAC );
        IFX_PPACMD_PRINT("        j) [-y dst_mac]: default is %s\n", DEFAULT_DST_MAC);
        IFX_PPACMD_PRINT("        l) [-n nat-ip]: default is 0, ie no NAT\n");
        IFX_PPACMD_PRINT("        l) [-m nat-port]: default is 0, ie, no NAT\n");
        IFX_PPACMD_PRINT("        m) [-o new_dscp]: default is 0, ie, no dscp change\n");
        IFX_PPACMD_PRINT("        n) [-r in_vlan_id]: default is 0, ie, no inner vlan change\n");
        IFX_PPACMD_PRINT("        o) [-c out_vlan_id]: default is 0, ie, no out vlan change\n");
        IFX_PPACMD_PRINT("        p) [-q queue-id]: default is 0\n");
        IFX_PPACMD_PRINT("        q) [-q pppoe-id]: default is 0, ie, no pppoe header insert\n");
    }
    return;
}

static int ppa_parse_add_session_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int i_opt=0, f_opt=0, t_opt=0, s_opt=0,d_opt=0, p_opt=0, w_opt=0, x_opt=0, y_opt=0, n_opt=0,m_opt=0, o_opt=0, r_opt=0, c_opt=0,q_opt=0,z_opt=0;
    unsigned int ip1_type=0, ip2_type=0, ip3_type=0;

    memset( &pdata->detail_session_info, 0, sizeof(pdata->detail_session_info) );
    pdata->detail_session_info.ip_proto = IFX_IPPROTO_UDP;
    pdata->detail_session_info.flags = SESSION_LAN_ENTRY | SESSION_VALID_OUT_VLAN_RM | SESSION_VALID_OUT_VLAN_RM | SESSION_VALID_MTU | SESSION_VALID_NEW_SRC_MAC;

    ip1_type = str_convert(STRING_TYPE_IP, DEFAULT_SRC_IP, (void *)&pdata->detail_session_info.src_ip);
    if( ip1_type == IP_VALID_V6)
        pdata->detail_session_info.flags |= SESSION_IS_IPV6;
    ip2_type = str_convert(STRING_TYPE_IP, DEFAULT_DST_IP, (void *)&pdata->detail_session_info.dst_ip);
    if( ip2_type == IP_VALID_V6 )
        pdata->detail_session_info.flags |= SESSION_IS_IPV6;

    pdata->detail_session_info.src_port= str_convert(STRING_TYPE_INTEGER, DEFAULT_SRC_PORT, NULL);
    pdata->detail_session_info.dst_port = str_convert(STRING_TYPE_INTEGER, DEFAULT_DST_PORT, NULL);
    stomac(DEFAULT_SRC_MAC, pdata->detail_session_info.src_mac );
    stomac(DEFAULT_DST_MAC, pdata->detail_session_info.dst_mac );


    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'f':
            if( (ip1_type = str_convert(STRING_TYPE_IP, popts->optarg, (void *)&pdata->detail_session_info.src_ip)) == IP_NON_VALID )
            {
                IFX_PPACMD_PRINT("Wrong source ip:%s\n", popts->optarg);
                return PPA_CMD_ERR;
            };
            if( ip1_type == IP_VALID_V6 )
                pdata->detail_session_info.flags |= SESSION_IS_IPV6;

            f_opt ++;
            break;

        case 't':
            if( (ip2_type = str_convert(STRING_TYPE_IP, popts->optarg, (void *)&pdata->detail_session_info.dst_ip)) == IP_NON_VALID )
            {
                IFX_PPACMD_PRINT("Wrong Dst ip:%s\n", popts->optarg);
                return PPA_CMD_ERR;
            };
            if( ip2_type == IP_VALID_V6 )
                pdata->detail_session_info.flags |= SESSION_IS_IPV6;
            t_opt ++;
            break;

        case 's':
            pdata->detail_session_info.src_port=  str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            s_opt ++;
            break;

        case 'd':
            pdata->detail_session_info.dst_port=  str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            d_opt ++;
            break;

        case 'p':
            if( stricmp(popts->optarg, "tcp") == 0 )
            {
                pdata->detail_session_info.ip_proto= IFX_IPPROTO_TCP;
            }
            else
            {
                pdata->detail_session_info.ip_proto= IFX_IPPROTO_UDP;
            }
            p_opt ++;
            break;
        case 'i':
            pdata->detail_session_info.dest_ifid=  str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            i_opt ++;
            break;
        case 'w':
            if( stricmp(popts->optarg, "wan") == 0 )
                pdata->detail_session_info.flags |= SESSION_WAN_ENTRY;
            else
                pdata->detail_session_info.flags |= SESSION_LAN_ENTRY;
            w_opt ++;
            break;
        case 'x':  //src-mac
            stomac(popts->optarg, pdata->detail_session_info.src_mac );
            pdata->detail_session_info.flags |= SESSION_VALID_NEW_SRC_MAC;
            x_opt ++;
            break;
        case 'y': //dst_mac
            stomac(popts->optarg, pdata->detail_session_info.dst_mac );
            y_opt ++;
            break;
        case 'n': //nat-ip
            if( (ip3_type = str_convert(STRING_TYPE_IP, popts->optarg, (void *)&pdata->detail_session_info.nat_ip)) == IP_NON_VALID )
            {
                IFX_PPACMD_PRINT("Wrong nat ip:%s\n", popts->optarg);
                return PPA_CMD_ERR;
            };
            if( ip3_type == IP_VALID_V6 )
                pdata->detail_session_info.flags |= SESSION_IS_IPV6;
            pdata->detail_session_info.flags |= SESSION_VALID_NAT_IP;
            n_opt ++;
            break;
        case 'm': //nat-port
            pdata->detail_session_info.nat_port=  str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            pdata->detail_session_info.flags |= SESSION_VALID_NAT_PORT;
            m_opt ++;
            break;
        case 'o': //new_dscp
            pdata->detail_session_info.new_dscp =  str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            pdata->detail_session_info.new_dscp = SESSION_VALID_NEW_DSCP;
            o_opt ++;
            break;
        case 'r':  //in_vlan_id
            pdata->detail_session_info.in_vci_vlanid =  str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            pdata->detail_session_info.flags |= SESSION_VALID_VLAN_INS;
            r_opt ++;
            break;
        case 'c':  //out_vlan_tag
            pdata->detail_session_info.out_vlan_tag =  str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            pdata->detail_session_info.flags |= SESSION_VALID_OUT_VLAN_INS;
            c_opt ++;
            break;
        case 'q': //qid
            pdata->detail_session_info.qid =  str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            q_opt ++;
            break;
        case 'z': //pppoe-id
            pdata->detail_session_info.pppoe_session_id =  str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            pdata->detail_session_info.flags |= SESSION_VALID_PPPOE;
            z_opt ++;
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( f_opt>1|| t_opt>1|| s_opt>1||d_opt>1|| p_opt>1|| w_opt>1|| x_opt>1||
            y_opt>1|| n_opt>1||m_opt>1|| o_opt>1|| r_opt>1|| c_opt>1||q_opt>1||z_opt>1)  /*too many parameters are provided */
    {
        IFX_PPACMD_PRINT("too many same paramemter are provided\n");
        return PPA_CMD_ERR;
    }
    if( f_opt==0&& t_opt==0&& s_opt==0&&d_opt==0&& p_opt==0&& w_opt==0&& x_opt==0&&
            y_opt==0&& n_opt==0&&m_opt==0&& o_opt==0&& r_opt==0&& c_opt==0&&q_opt==0&&z_opt==0)  /*too many parameters are provided */
    {
        IFX_PPACMD_PRINT("At least provide one parameter\n");
        return PPA_CMD_ERR;
    }
    if( f_opt > 1 || t_opt > 1 || s_opt > 1 || d_opt > 1 )
    {
        IFX_PPACMD_PRINT("wrong input: -f/t/s/d.\n");
        return PPA_CMD_ERR;
    }
    if( ip1_type != ip2_type || ( ip3_type && ip3_type != ip1_type )  )
    {
        IFX_PPACMD_PRINT("src-ip, dst-ip, nap-ip should match, ie, both are IPV4 or IPV6 address\n");
        return PPA_CMD_ERR;
    }

#ifndef CONFIG_IFX_PPA_IPv6_ENABLE
    if( ip1_type == IP_VALID_V6 )
    {
        IFX_PPACMD_PRINT("ppacmd not support IPV6. Pls recompile ppacmd\n");
        return PPA_CMD_ERR;
    }
#endif

#ifdef CONFIG_IFX_PPA_IPv6_ENABLE
    if( pdata->detail_session_info.flags & SESSION_IS_IPV6 )
    {
        IFX_PPACMD_DBG("add session via ppa session tuple:%s from %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%5d to %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%5d\n",
                (pdata->detail_session_info.ip_proto == IFX_IPPROTO_TCP)?"tcp":"udp",
                NIP6(pdata->detail_session_info.src_ip.ip6), pdata->detail_session_info.src_port,
                NIP6(pdata->detail_session_info.dst_ip.ip6), pdata->detail_session_info.dst_port);

    }
    else
#endif
    {
        IFX_PPACMD_DBG("add session via ppa session tuple:%s from %u.%u.%u.%u/%5d to %u.%u.%u.%u/%5d\n",
                (pdata->detail_session_info.ip_proto == IFX_IPPROTO_TCP)?"tcp":"udp",
                NIPQUAD(pdata->detail_session_info.src_ip), pdata->detail_session_info.src_port,
                NIPQUAD(pdata->detail_session_info.dst_ip), pdata->detail_session_info.dst_port);
    }

    return PPA_CMD_OK;

}

/**** addsession --- end */

/*
====================================================================================
   command:   modifysession
   description: modify a routing session
   options:
====================================================================================
*/
static const char ppa_modify_session_short_opts[] = "a:f:m:p:h";
static const struct option ppa_modify_session_long_opts[] =
{
    {"session address",   required_argument,  NULL, 'a'},
    {"session address",   required_argument,  NULL, 'f'},
    {"acceleration mode",   required_argument,  NULL, 'm'},
    {"pppoe",  required_argument,  NULL, 'p'},
    {"help",   no_argument,  NULL, 'h'},
    { 0,0,0,0 }
};

static void ppa_modify_session_help( int summary)
{
    IFX_PPACMD_PRINT("modifysession [-a ppa-session-address] [ -f 0 | 1 | 2 ] [-m 0 | 1] \n");
    if( summary )
    {
        IFX_PPACMD_PRINT("  Note 1: -f: 1 to match LAN session only, 2 to match WAN session only, and 0 match LAN/WAN both\n");
        IFX_PPACMD_PRINT("  Note 2: -m: 0 to disable acceleration for this specified session and 1 to enable acceleration\n");
        IFX_PPACMD_PRINT("  Note 3: -p: none zero id is to add or replace pppoe session id ( for test purpose only)\n");
        IFX_PPACMD_PRINT("            : otherwise no pppoe action ( for test purpose only)\n");

    }
    return;
}

static int ppa_parse_modify_session_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int a_opt=0, m_opt=0, f_opt=0, p_opt=0;
    uint32_t tmp_f = 0;

    memset( &pdata->session_extra_info, 0, sizeof(pdata->session_extra_info) );
    pdata->session_extra_info.lan_wan_flags = SESSION_WAN_ENTRY | SESSION_LAN_ENTRY;

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'a':
            pdata->session_extra_info.session = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            a_opt ++;
            break;

        case 'm':
            pdata->session_extra_info.flags |= PPA_F_ACCEL_MODE;
            if( str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL ) )
                pdata->session_extra_info.session_extra.accel_enable = 1;
            else
                pdata->session_extra_info.session_extra.accel_enable = 0;
            m_opt ++;
            break;

        case 'f':
            tmp_f= str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL );
            if( tmp_f == 1 ) pdata->session_extra_info.lan_wan_flags = SESSION_LAN_ENTRY;
            else if( tmp_f == 2 ) pdata->session_extra_info.lan_wan_flags = SESSION_WAN_ENTRY;
            else if( tmp_f == 0 ) pdata->session_extra_info.lan_wan_flags = SESSION_WAN_ENTRY | SESSION_LAN_ENTRY;
            else
            {
                IFX_PPACMD_PRINT("Wrong flag:%d\n", tmp_f);
                return PPA_CMD_ERR;
            }
            break;
            
        case 'p':
            pdata->session_extra_info.flags |= PPA_F_PPPOE;
            pdata->session_extra_info.session_extra.pppoe_id = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL );
            p_opt ++;
            IFX_PPACMD_DBG("pppoe_id:%d\n", pdata->session_extra_info.session_extra.pppoe_id);
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( ( a_opt != 1 ) || ( m_opt != 1 && p_opt != 1 ) )
    {
        ppa_modify_session_help(1);
        return PPA_CMD_ERR;
    }

    IFX_PPACMD_DBG("session address=0x%x\n", (unsigned int)pdata->session_extra_info.session  );
    IFX_PPACMD_DBG("Flag=0x%x\n", (unsigned int)pdata->session_extra_info.flags );
    if( pdata->session_extra_info.flags & PPA_F_ACCEL_MODE)
    {
        IFX_PPACMD_DBG("  %s\n", pdata->session_extra_info.session_extra.accel_enable ?"/s acceleration":"disable acceleration" );
    }
    if( pdata->session_extra_info.flags & PPA_F_PPPOE)
    {
        IFX_PPACMD_DBG("  %s with id=0x%d\n", pdata->session_extra_info.session_extra.pppoe_id? "new pppoe session id":"remove pppoe", pdata->session_extra_info.session_extra.pppoe_id );
    }

    return PPA_CMD_OK;
}

/*
====================================================================================
   command:   setsessiontimer
   description: set routing session polling timer
   options:
====================================================================================
*/
static const char ppa_set_session_timer_short_opts[] = "a:t:h";
static const struct option ppa_set_session_timer_long_opts[] =
{
    {"session address",   required_argument,  NULL, 'a'},
    {"timter",   required_argument,  NULL, 't'},
    {"help",   no_argument,  NULL, 'h'},
    { 0,0,0,0 }
};

static void ppa_set_session_timer_help( int summary)
{
    IFX_PPACMD_PRINT("setsessiontimer [-a ppa-session-address] [-t polling_timer_in_seconds] \n");
    if( summary )
    {
        IFX_PPACMD_PRINT("Note 1: by default, session address is 0, ie, to set ppa routing session polling timer only\n");
    }
    return;
}

static int ppa_parse_set_session_timer_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int a_opt=0, t_opt=0;

    memset( &pdata->session_timer_info, 0, sizeof(pdata->session_timer_info) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'a':
            pdata->session_timer_info.session = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            a_opt ++;
            break;

        case 't':
            pdata->session_timer_info.timer_in_sec = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL );
            t_opt ++;
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( t_opt !=1 )
    {
        ppa_set_session_timer_help(1);
        return PPA_CMD_ERR;
    }

    IFX_PPACMD_DBG("session address=%x\n", (unsigned int) pdata->session_timer_info.session  );
    IFX_PPACMD_DBG("timer=%d\n", (unsigned int)pdata->session_timer_info.timer_in_sec);

    return PPA_CMD_OK;
}


/*
====================================================================================
   command:   getsessiontimer
   description: get routing session polling timer
   options:
====================================================================================
*/
static const char ppa_get_session_timer_short_opts[] = "s:h";
static const struct option ppa_get_session_timer_long_opts[] =
{
    {"save",   required_argument,  NULL, 'o'},
    {"help",   no_argument,  NULL, 'h'},
    { 0,0,0,0 }
};

static void ppa_get_session_timer_help( int summary)
{
    IFX_PPACMD_PRINT("getsessiontimer [-o save_file] \n");
    return;
}

static int ppa_parse_get_session_timer_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    memset( &pdata->session_timer_info, 0, sizeof(pdata->session_timer_info) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case  'o':
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    return PPA_CMD_OK;
}


static void ppa_print_get_session_timer(PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
    {
        IFX_PPACMD_PRINT("PPA Routing poll timer:%d\n", (unsigned int)pdata->session_timer_info.timer_in_sec );
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->session_timer_info),  sizeof(pdata->session_timer_info)  );
    }
}


/*
====================================================================================
   command:   delvfilter
   description: delete one vlan filter
   options:
====================================================================================
*/

/*delete all vfilter */
static void ppa_del_all_vfilter_help( int summary)
{
    IFX_PPACMD_PRINT("delallvfilter\n"); // [ -m <multicast-mac-address (for bridging only)>]
    if( summary )
        IFX_PPACMD_PRINT("    This command is for A4/D4 only at present\n");
    return;
}

/*
====================================================================================
   command:   getversion
   description: get ppa/ppe driver/ppe fw version
   options:
====================================================================================
*/


static void ppa_get_version_help( int summary)
{
    IFX_PPACMD_PRINT("getversion [ -o file ] [ -a ]\n"); // [ -m <multicast-mac-address (for bridging only)>]
    return;
}

PPA_VERSION ppa_api_ver;
PPA_VERSION ppa_stack_al_ver;
PPA_VERSION ppe_hal_ver;
PPA_VERSION ppe_fw_ver;
char *ppe_fw_ver_family_str[] = /*Platfrom of PPE FW */
{
    "Reserved",
    "Danube",
    "Twinpass",
    "Amazon-SE",
    "Reserved",
    "AR9",
    "GR9",
    "VR9",
    "AR10",
    "VRX318",
};
char *ppe_fw_ver_interface_str[] =
{
    "D4", // 0
    "D5", // 1
    "A4", // 2
    "E4", // 3
    "A5", // 4
    "E5",  // 5
    "Res",  // 6: reserved
    "D5"  // 7: in VR9 D5+Bonding fw, it is set to this value. So here we set it to D5
};

char *ppe_fw_ver_package_str[] = 
{
	"Reserved - 0",
	"A1",
	"B1 - PTM_BONDING",
	"E1",
	"A5",
	"D5",
	"D5v2",
	"E5",
};

char *ppe_drv_ver_family_str[] =   /* Network processor platfrom */
{
    "Reserved",  //bit 0, ie, value 0x1
    "Danube",  //bit 1, ie, value 0x2
    "Twinpass",  //bit 2, ie, value 0x4
    "Amazon-SE", //bit 3, ie, value 0x8
    "Reserved",  //bit 4, ie, value 0x10
    "AR9",       //bit 5  , ie, value 0x20
    "VR9",      //bit 6, ie, value 0x40
    "AR10",     //bit 7, ie, value 080
};

//get the non-zero high bit index ( from zero )
static int ppa_get_platform_id(uint32_t family)
{
    /*very one bit repreents one platform */
    int i;

    for(i=31; i>=0; i--)
        if( family & (1<<i) ) return i;

    return 0;
}

typedef enum {
    DSL_ATM_MODE=1,
    DSL_PTM_MODE
};
static void ppa_print_get_version_cmd(PPA_CMD_DATA *pdata)
{
    int i;
    int k, num,id, dsl_atm_ptm=0;
 
    if( !g_output_to_file )
    {
        IFX_PPACMD_PRINT("PPA SubSystem version info:v%u.%u.%u-%u\n", (unsigned int)pdata->ver.ppa_subsys_ver.major, (unsigned int)pdata->ver.ppa_subsys_ver.mid, (unsigned int)pdata->ver.ppa_subsys_ver.minor, (unsigned int)pdata->ver.ppa_subsys_ver.tag );
        IFX_PPACMD_PRINT("  PPA ppacmd utility tool info:%u.%u.%u\n", PPACMD_VERION_MAJOR, PPACMD_VERION_MID, PPACMD_VERION_MINOR);
        IFX_PPACMD_PRINT("  PPA API driver info:%u.%u.%u.%u.%u.%u.%u\n", (unsigned int)pdata->ver.ppa_api_ver.family, (unsigned int)pdata->ver.ppa_api_ver.type, (unsigned int)pdata->ver.ppa_api_ver.itf, (unsigned int)pdata->ver.ppa_api_ver.mode, (unsigned int)pdata->ver.ppa_api_ver.major, (unsigned int)pdata->ver.ppa_api_ver.mid, (unsigned int)pdata->ver.ppa_api_ver.minor);
        IFX_PPACMD_PRINT("  PPA Stack Adaption Layer driver info:%u.%u.%u.%u.%u.%u.%u\n", (unsigned int)pdata->ver.ppa_stack_al_ver.family, (unsigned int)pdata->ver.ppa_stack_al_ver.type, (unsigned int)pdata->ver.ppa_stack_al_ver.itf, (unsigned int)pdata->ver.ppa_stack_al_ver.mode, (unsigned int)pdata->ver.ppa_stack_al_ver.major, (unsigned int)pdata->ver.ppa_stack_al_ver.mid, (unsigned int)pdata->ver.ppa_stack_al_ver.minor);
        IFX_PPACMD_PRINT("  PPE HAL driver info:%u.%u.%u.%u.%u.%u.%u\n", (unsigned int)pdata->ver.ppe_hal_ver.family, (unsigned int)pdata->ver.ppe_hal_ver.type, (unsigned int)pdata->ver.ppe_hal_ver.itf, (unsigned int)pdata->ver.ppe_hal_ver.mode, (unsigned int)pdata->ver.ppe_hal_ver.major, (unsigned int)pdata->ver.ppe_hal_ver.mid, (unsigned int)pdata->ver.ppe_hal_ver.minor);
        id=ppa_get_platform_id(pdata->ver.ppe_hal_ver.family);
        if( id < NUM_ENTITY(ppe_drv_ver_family_str) )
        {
            IFX_PPACMD_PRINT("  PPE Driver Platform:%s\n", ppe_drv_ver_family_str[id] );
        }
        else
        {
            IFX_PPACMD_PRINT("PPE Driver Platform :unknown (0x%x)\n",  (unsigned int)pdata->ver.ppa_stack_al_ver.family);
        }
        num = 0;
        for(k=0; k<2; k++ )
        {
            if( pdata->ver.ppe_fw_ver[k].family == 0 )
            {
                if( k ==0 && pdata->ver.ppe_fw_ver[1].family != 0 )
                { // For VR9 D5 Ethernet wan mode, only PPE FW 1 is working, PE FW 0 not running at all
                    //note, so far only VR9 E5 are using two PPE. So here we harded some information here for E5 ethernet WAN mode
                    IFX_PPACMD_PRINT("  PPE0 FW Mode: %s family %s (not running now)\n", ppe_fw_ver_family_str[7], ppe_fw_ver_interface_str[5]);
                }
                continue;
            }
            IFX_PPACMD_PRINT("  PPE%d FW version:%u.%u.%u.%u.%u\n", k, (unsigned int)pdata->ver.ppe_fw_ver[k].family, (unsigned int)pdata->ver.ppe_fw_ver[k].type, (unsigned int)pdata->ver.ppe_fw_ver[k].major, (unsigned int)pdata->ver.ppe_fw_ver[k].mid, (unsigned int)pdata->ver.ppe_fw_ver[k].minor);

            IFX_PPACMD_PRINT("  PPE%d FW Mode:", k);
            if( pdata->ver.ppe_fw_ver[k].family < NUM_ENTITY(ppe_fw_ver_family_str) )
            {
                IFX_PPACMD_PRINT("%s family ", ppe_fw_ver_family_str[pdata->ver.ppe_fw_ver[k].family] );
            }
            else
            {
                IFX_PPACMD_PRINT("unknown family(%d)",  (unsigned int)pdata->ver.ppe_fw_ver[k].family);
            }
			if(pdata->ver.ppe_fw_ver[k].type < NUM_ENTITY(ppe_fw_ver_package_str)){
				IFX_PPACMD_PRINT(" %s", ppe_fw_ver_package_str[pdata->ver.ppe_fw_ver[k].type] );
			}else{			     
				IFX_PPACMD_PRINT(" unknown package(%d) with ppe_fw_ver_package_str[%d] ", (unsigned int)pdata->ver.ppe_fw_ver[k].type,  NUM_ENTITY(ppe_fw_ver_package_str));
			}
            if( k == 0 ) { //adapt for VRX318 from ppe_fw_ver_package_str
                if( pdata->ver.ppe_fw_ver[k+1].type == 2 /*B1-PTM Bonding: E1 w/ Bonding */ || 
                    pdata->ver.ppe_fw_ver[k+1].type == 3 /*E1 w/o bonding */ )
                    dsl_atm_ptm=DSL_PTM_MODE; //PTM
                else if( pdata->ver.ppe_fw_ver[k+1].type == 1 /*A1*/||
                         pdata->ver.ppe_fw_ver[k+1].type == 10 /*support wrong value in current VRX318 */)
                    dsl_atm_ptm=DSL_ATM_MODE; //PTM                
            }    
            IFX_PPACMD_PRINT("\n");
            IFX_PPACMD_DBG("PPE%d package=%d with dsl_atm_ptm=%d\n", k, pdata->ver.ppe_fw_ver[k+1].type, dsl_atm_ptm );

            if( num ) continue; //only print once for below messages
            num ++;

            IFX_PPACMD_DBG("  wan_port_map=%u mixed=%u\n", (unsigned int)pdata->ver.ppa_wan_info.wan_port_map, (unsigned int)pdata->ver.ppa_wan_info.mixed );

            //display wan interfaces in PPE FW
            IFX_PPACMD_PRINT("  PPE WAN interfaces: ");
            for(i=0; i<8; i++)
            {
                if( pdata->ver.ppa_wan_info.wan_port_map & (1<<i) )
                {
                    char dsl_mode[10]={0};
                    if( dsl_atm_ptm == DSL_ATM_MODE ) strcpy(dsl_mode, "(ATM)");
                    else if( dsl_atm_ptm == DSL_PTM_MODE ) strcpy(dsl_mode, "(PTM)");
                    if( i == 0 || i== 1 ) IFX_PPACMD_PRINT("eth%d ", i);
                    else if( i >=2  && i<6 ) IFX_PPACMD_PRINT("ext%d ", i-2);
                    else if( i ==6 || i==7 ) IFX_PPACMD_PRINT("DSL%s ", dsl_mode);
                }
            }
            IFX_PPACMD_PRINT("\n");

            //display wan vlan range if it is mixed mode
            if( pdata->ver.ppa_wan_info.mixed )
            {
                //get and display wan vlan range
                PPA_CMD_VLAN_RANGES *psession_buffer;
                PPA_CMD_DATA cmd_info;
                int res = PPA_CMD_OK, i, size;
                uint32_t flag = PPA_CMD_WAN_MII0_VLAN_RANGE_GET;

                //get session count first before malloc memroy
                cmd_info.count_info.flag = 0;
                if( ppa_do_ioctl_cmd(PPA_CMD_GET_COUNT_WAN_MII0_VLAN_RANGE, &cmd_info ) != PPA_CMD_OK )
                    return ;

                if( cmd_info.count_info.count == 0 )
                {
                    IFX_PPACMD_DBG("wan mii vlan range count=%d\n", (unsigned int)cmd_info.count_info.count);
                    return ;
                }

                //malloc memory and set value correctly
                size = sizeof(PPA_CMD_COUNT_INFO) + sizeof(PPA_VLAN_RANGE) * ( 1 + cmd_info.count_info.count ) ;
                psession_buffer = (PPA_CMD_VLAN_RANGES *) malloc( size );
                if( psession_buffer == NULL )
                {
                    IFX_PPACMD_PRINT("Malloc %d bytes failed\n", size );
                    return ;
                }

                memset( psession_buffer, 0, sizeof(size) );
                psession_buffer->count_info.count = cmd_info.count_info.count;
                psession_buffer->count_info.flag = 0;
                //get session information
                if( (res = ppa_do_ioctl_cmd(flag, psession_buffer ) != PPA_CMD_OK ) )
                {
                    free( psession_buffer );
                    return;
                }
                IFX_PPACMD_PRINT("  Mixed mode (WAN VLAN range): ");
                for(i=0; i<psession_buffer->count_info.count; i++ )
                {
                    IFX_PPACMD_PRINT("0x%03x to 0x%03x", (unsigned int)psession_buffer->ranges[i].start_vlan_range, (unsigned int)psession_buffer->ranges[i].end_vlan_range );
                    if( i != psession_buffer->count_info.count - 1 ) //not last one
                        IFX_PPACMD_PRINT(",");
                }
                IFX_PPACMD_PRINT("\n");
                free( psession_buffer );
            }
        }
        IFX_PPACMD_PRINT("  PPA IPV6:%s(ppe:%s/ppa:%s)\n", pdata->ver.ppe_fw_feature.ipv6_en & pdata->ver.ppa_feature.ipv6_en ? "enabled":"disabled",
                         pdata->ver.ppe_fw_feature.ipv6_en ? "enabled":"disabled",
                         pdata->ver.ppa_feature.ipv6_en ? "enabled":"disabled" );
#ifdef PPACMD_DEBUG_QOS
        IFX_PPACMD_PRINT("  PPA QOS:%s(ppe:%s/ppa:%s)\n", pdata->ver.ppe_fw_feature.qos_en & pdata->ver.ppa_feature.qos_en ? "enabled":"disabled",
                         pdata->ver.ppe_fw_feature.qos_en ? "enabled":"disabled",
                         pdata->ver.ppa_feature.qos_en ? "enabled":"disabled" );
#endif


        if( g_all )
        {
            int ioctl_cmd;
            PPA_CMD_DATA tmp_pdata;
			
            ioctl_cmd =PPA_CMD_GET_STATUS;
            memset( &tmp_pdata, 0, sizeof(tmp_pdata) );
            if( ppa_do_ioctl_cmd( ioctl_cmd, &tmp_pdata) == IFX_SUCCESS ) ppa_print_status( &tmp_pdata);
            
#ifdef CONFIG_IFX_PPA_QOS
            ioctl_cmd =PPA_CMD_GET_QOS_STATUS;
            memset( &tmp_pdata, 0, sizeof(tmp_pdata) );
            ppa_get_qstatus_do_cmd(NULL, &tmp_pdata );
#endif
            system( "[ -e /proc/driver/ifx_cgu/clk_setting ] && cat /proc/driver/ifx_cgu/clk_setting > /tmp/clk.txt" );
            system( "[ -e /tmp/clk.txt ] && cat /tmp/clk.txt && rm /tmp/clk.txt" );


        }
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->ver),  sizeof(pdata->ver)  );
    }
}

/*get bridge mac count */
static void ppa_get_br_count_help( int summary)
{
    IFX_PPACMD_PRINT("getbrnum [-o file ]\n");
    return;
}

static int ppa_parse_get_br_count(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    int res =   ppa_parse_simple_cmd( popts, pdata );

    if( res != PPA_CMD_OK ) return res;


    pdata->count_info.flag = 0;

    return PPA_CMD_OK;
}

static void ppa_print_get_count_cmd(PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
    {
        IFX_PPACMD_PRINT("The count is %u\n", (unsigned int)pdata->count_info.count );
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->count_info), sizeof(pdata->count_info) );
    }

}


/*get all bridge mac information */
static void ppa_get_all_br_help( int summary)
{
    IFX_PPACMD_PRINT("getbrs  [-o file ]\n"); // [ -m <multicast-mac-address (for bridging only)>]
    return;
}

static int ppa_get_all_br_cmd (PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata)
{
    PPA_CMD_ALL_MAC_INFO *psession_buffer;
    PPA_CMD_DATA cmd_info;
    int res = PPA_CMD_OK, i, size;
    uint32_t flag = PPA_CMD_GET_ALL_MAC;

    //get session count first before malloc memroy
    cmd_info.count_info.flag = 0;
    if( ppa_do_ioctl_cmd(PPA_CMD_GET_COUNT_MAC, &cmd_info ) != PPA_CMD_OK )
        return -EIO;

    if( cmd_info.count_info.count == 0 )
    {
        IFX_PPACMD_DBG("bridge mac count=0\n");
        if( g_output_to_file )
            SaveDataToFile(g_output_filename, (char *)(&cmd_info.count_info),  sizeof(cmd_info.count_info)  );
        return PPA_CMD_OK;
    }

    //malloc memory and set value correctly
    size = sizeof(PPA_CMD_COUNT_INFO) + sizeof(PPA_CMD_MAC_ENTRY) * ( 1 + cmd_info.count_info.count ) ;
    psession_buffer = (PPA_CMD_ALL_MAC_INFO *) malloc( size );
    if( psession_buffer == NULL )
    {
        IFX_PPACMD_PRINT("Malloc %d bytes failed\n", size );
        return PPA_CMD_NOT_AVAIL;
    }
    memset( psession_buffer, 0, sizeof(size) );

    psession_buffer->count_info.count = cmd_info.count_info.count;
    psession_buffer->count_info.flag = 0;

    //get session information
    if( (res = ppa_do_ioctl_cmd(flag, psession_buffer ) != PPA_CMD_OK ) )
    {
        free( psession_buffer );
        return res;
    }

    IFX_PPACMD_DBG("bridge mac count=%u. \n", (unsigned int)psession_buffer->count_info.count);


    if( !g_output_to_file )
    {
        for(i=0; i<psession_buffer->count_info.count; i++ )
        {
            //format like: [002] 239.  2.  2.  3 (route) qid  0 vlan 0000/04x From  nas0 to  eth0 ADDED_IN_HW|VALID_PPPOE|VALID_NEW_SRC_MAC
            IFX_PPACMD_PRINT("[%03d] %02x:%02x:%02x:%02x:%02x:%02x %s\n", i, psession_buffer->session_list[i].mac_addr[0],
                             psession_buffer->session_list[i].mac_addr[1],
                             psession_buffer->session_list[i].mac_addr[2],
                             psession_buffer->session_list[i].mac_addr[3],
                             psession_buffer->session_list[i].mac_addr[4],
                             psession_buffer->session_list[i].mac_addr[5],
                             psession_buffer->session_list[i].ifname );
        }
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(psession_buffer),  size  );
    }

    free( psession_buffer );
    return PPA_CMD_OK;
}


/*GET ALL WAN MII0 VLAND  Rage */
static void ppa_get_wan_mii0_vlan_range_count_help(int summary)
{
    IFX_PPACMD_PRINT("getvlanrangenum [-o file ]\n");
    return;
}



/*Add WAN MII0 VLAN RANGE */
static void ppa_add_wan_mii0_vlan_range_help( int summary)
{
    IFX_PPACMD_PRINT("addvlanrange -s <start_vlan_id> -e <end_vlan_id>\n");
    if( summary ) IFX_PPACMD_PRINT("addvlanrange is used to add wan interface's vlan range. At present only 1 range is supported\n");
    return;
}

static int ppa_parse_add_wan_mii0_vlan_range_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int s_opt=0, e_opt=0;

    memset(pdata, 0, sizeof(*pdata) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 's': //multicast group
            pdata->wan_vlanid_range.start_vlan_range = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            s_opt ++;
            break;

        case 'e': //dscp
            pdata->wan_vlanid_range.end_vlan_range = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            e_opt ++;
            break;


        default:
            IFX_PPACMD_PRINT("ppa_parse_add_wan_mii0_vlan_range_cmd not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( s_opt != 1 || e_opt != 1 )
        return PPA_CMD_ERR;

    IFX_PPACMD_DBG("ppa_parse_add_wan_mii0_vlan_range_cmd: from 0x%x to 0x%x\n", (unsigned int)pdata->wan_vlanid_range.start_vlan_range, (unsigned int)pdata->wan_vlanid_range.end_vlan_range );

    return PPA_CMD_OK;
}

static const char    ppa_add_wan_mii0_vlan_range_short_opts[] = "s:e:h"; //need to further implement add/remove/modify vlan and enable new dscp and its value
static const struct option ppa_add_wan_mii0_vlan_range_long_opts[] =
{
    {"start",   required_argument,  NULL, 's'},
    {"end",   required_argument,  NULL, 'd'},
    { 0,0,0,0 }
};

static void ppa_get_wan_mii0_all_vlan_range_help(int summary)
{
    IFX_PPACMD_PRINT("getvlanranges [-o file ]\n");
    return;
}

static int ppa_wan_mii0_all_vlan_range_cmd (PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata)
{
    PPA_CMD_VLAN_RANGES *psession_buffer;
    PPA_CMD_DATA cmd_info;
    int res = PPA_CMD_OK, i, size;
    uint32_t flag = PPA_CMD_WAN_MII0_VLAN_RANGE_GET;

    //get session count first before malloc memroy
    cmd_info.count_info.flag = 0;
    if( ppa_do_ioctl_cmd(PPA_CMD_GET_COUNT_WAN_MII0_VLAN_RANGE, &cmd_info ) != PPA_CMD_OK )
        return -EIO;

    if( cmd_info.count_info.count == 0 )
    {
        IFX_PPACMD_DBG("wan mii vlan range count=%d\n", (unsigned int)cmd_info.count_info.count);
        if( g_output_to_file )
            SaveDataToFile(g_output_filename, (char *)(&cmd_info.count_info),  sizeof(cmd_info.count_info)  );
        return PPA_CMD_OK;
    }

    //malloc memory and set value correctly
    size = sizeof(PPA_CMD_COUNT_INFO) + sizeof(PPA_VLAN_RANGE) * ( 1 + cmd_info.count_info.count ) ;
    psession_buffer = (PPA_CMD_VLAN_RANGES *) malloc( size );
    if( psession_buffer == NULL )
    {
        IFX_PPACMD_PRINT("Malloc %d bytes failed\n", size );
        return PPA_CMD_NOT_AVAIL;
    }

    memset( psession_buffer, 0, sizeof(size) );

    psession_buffer->count_info.count = cmd_info.count_info.count;
    psession_buffer->count_info.flag = 0;

    //get session information
    if( (res = ppa_do_ioctl_cmd(flag, psession_buffer ) != PPA_CMD_OK ) )
    {
        free( psession_buffer );
        return res;
    }

    IFX_PPACMD_DBG("wanmii0 vlan range count=%d. \n", (unsigned int)psession_buffer->count_info.count);


    if( !g_output_to_file )
    {
        for(i=0; i<psession_buffer->count_info.count; i++ )
        {
            IFX_PPACMD_PRINT("[%02d] from %03x to %03x\n", i, (unsigned int)psession_buffer->ranges[i].start_vlan_range, (unsigned int)psession_buffer->ranges[i].end_vlan_range );
        }
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(psession_buffer),  size  );
    }

    free( psession_buffer );
    return PPA_CMD_OK;
}

static void ppa_get_sizeof_help(int summary)
{
    IFX_PPACMD_PRINT("size\n");
    if( summary)
        IFX_PPACMD_PRINT("note: get PPA main structure's internal size"); //purposely don't display this command since it is not for customer
    return;
}

static void ppa_print_get_sizeof_cmd( PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
    {
        IFX_PPACMD_PRINT("one unicast session size =%d \n", (unsigned int)pdata->size_info.rout_session_size);
        IFX_PPACMD_PRINT("one unicast session size in ioctl =%d \n", sizeof(PPA_CMD_SESSION_ENTRY));
        IFX_PPACMD_PRINT("one multicast session size =%d\n", (unsigned int)pdata->size_info.mc_session_size );
        IFX_PPACMD_PRINT("one bridge session size =%d \n", (unsigned int)pdata->size_info.br_session_size );
        IFX_PPACMD_PRINT("netif size =%d\n", (unsigned int)pdata->size_info.netif_size );

    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->size_info), sizeof(pdata->size_info) );
    }

    return;
}

/*ppacmd setbr: set bridge mac address learning hook enable/disable---begin*/
static void ppa_set_br_help( int summary)
{
    IFX_PPACMD_PRINT("setbr [-f 0/1]\n");
    return;
}
static const char ppa_set_br_short_opts[] = "f:h";
static const struct option ppa_set_br_long_opts[] =
{
    {"flag",   required_argument,  NULL, 'f'},
    { 0,0,0,0 }
};

static int ppa_set_br_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    memset( pdata, 0, sizeof(PPA_CMD_BRIDGE_ENABLE_INFO) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case  'f':
            pdata->br_enable_info.bridge_enable= str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);;
            break;

        default:
            IFX_PPACMD_PRINT("ppa_set_br_cmd not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    return PPA_CMD_OK;
}

/*ppacmd setbr: set bridge mac address learning hook enable/disable ---end*/

/*ppacmd getbrstatus: get bridge mac address learning hook status: enabled/disabled---begin*/
static void ppa_get_br_status_help( int summary)
{
    IFX_PPACMD_PRINT("getbrstatus [-o outfile]\n");
    return;
}
static const char ppa_get_br_status_short_opts[] = "o:h";
static const struct option ppa_get_br_status_long_opts[] =
{
    {"save-to-file",   required_argument,  NULL, 'o'},
    { 0,0,0,0 }
};
static int ppa_get_br_status_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    memset( pdata, 0, sizeof(PPA_CMD_BRIDGE_ENABLE_INFO) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case  'o':
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            break;

        default:
            IFX_PPACMD_PRINT("ppa_get_br_status_cmd not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    return PPA_CMD_OK;
}
static void ppa_print_get_br_status_cmd(PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
    {
        IFX_PPACMD_PRINT("The bridge mac learning hook is %s\n", pdata->br_enable_info.bridge_enable ? "enabled":"disabled");
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->br_enable_info), sizeof(pdata->br_enable_info) );
    }
}

int get_portid(char *ifname)
{
    PPA_CMD_PORTID_INFO portid;

    memset( &portid, 0, sizeof(portid));
    strncpy( portid.ifname, ifname, sizeof(portid.ifname));

    if( ppa_do_ioctl_cmd(PPA_CMD_GET_PORTID, &portid ) != PPA_CMD_OK )
    {
        IFX_PPACMD_PRINT("ppacmd get portid failed\n");
        return -1;
    }

    return portid.portid;
}

/*ppacmd ppa_get_br_status_cmd:   ---end*/

#ifdef CONFIG_IFX_PPA_QOS

static void ppa_get_qstatus_help( int summary)
{
    IFX_PPACMD_PRINT("getqstatus [-o outfile]\n");
    return;
}
static const char ppa_get_qstatus_short_opts[] = "o:h";
static int ppa_parse_get_qstatus_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA* pdata)
{
    int  out_opt=0;

    memset( pdata, 0, sizeof(PPA_CMD_QOS_STATUS_INFO) );

    //note, must set below max_buffer_size
    //pdata->qos_status_info.qstat.max_buffer_size = NUM_ENTITY( pdata->qos_status_info.qstat.mib);
    while(popts->opt)
    {
        if (popts->opt == 'o')
        {
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            out_opt++;
        }
        else
        {
            return PPA_CMD_ERR;
        }
    }

    if (out_opt > 1)
        return PPA_CMD_ERR;

    IFX_PPACMD_DBG("PPA getqstatus\n");

    return PPA_CMD_OK;
}

static int ppa_get_qstatus_do_cmd(PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata)
{
    int i;
    int res=PPA_CMD_ERR;

    for(i=0; i<IFX_MAX_PORT_NUM; i++ )
    {
        pdata->qos_status_info.qstat.qos_queue_portid = i;
        if( ppa_do_ioctl_cmd(PPA_CMD_GET_QOS_STATUS, &pdata->qos_status_info ) == PPA_CMD_OK )
        {          
            if( pdata->qos_status_info.qstat.res == IFX_SUCCESS )
            {
                ppa_print_get_qstatus_cmd(pdata);
                res = PPA_CMD_OK;
            }            
        }
    }
    return res;
}
static void ppa_print_get_qstatus_cmd(PPA_CMD_DATA *pdata)
{
    unsigned long long rate[IFX_MAX_QOS_QUEUE_NUM];
    unsigned long long rate_L1[IFX_MAX_QOS_QUEUE_NUM];
    
    if( g_output_to_file)
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->qos_status_info), sizeof(pdata->qos_status_info) );
        return ;
    }

    if( pdata->qos_status_info.qstat.qos_queue_portid == -1 )
    {
        /*qos is not enabled */
        IFX_PPACMD_PRINT("Note: PPA QOS is not supported for wan_itf=%x  mixed_itf=%x\n", (unsigned int)pdata->qos_status_info.qstat.wan_port_map, (unsigned int)pdata->qos_status_info.qstat.wan_mix_map);
        return ;
    }

    IFX_PPACMD_PRINT("\nPort[%2d]\n  qos     : %s(For VR9 E5 VDSL WAN mode, this flag is not used)\n  wfq     : %s\n  Rate shaping: %s\n\n",
                     pdata->qos_status_info.qstat.qos_queue_portid,
                     pdata->qos_status_info.qstat.eth1_qss ?"enabled":"disabled",
                     pdata->qos_status_info.qstat.wfq_en?"enabled":"disabled",
                     pdata->qos_status_info.qstat.shape_en ?"enabled":"disabled");

    IFX_PPACMD_PRINT("  Ticks  =%u,  overhd  =%u,     qnum=%u  @0x%x\n",
                     (unsigned int)pdata->qos_status_info.qstat.time_tick,
                     (unsigned int)pdata->qos_status_info.qstat.overhd_bytes,
                     (unsigned int)pdata->qos_status_info.qstat.eth1_eg_qnum,
                     (unsigned int)pdata->qos_status_info.qstat.tx_qos_cfg_addr);

    IFX_PPACMD_PRINT("  PPE clk=%u MHz, basic tick=%u\n",(unsigned int)pdata->qos_status_info.qstat.pp32_clk/1000000, (unsigned int)pdata->qos_status_info.qstat.basic_time_tick );

#ifdef CONFIG_IFX_PPA_QOS_WFQ
    IFX_PPACMD_PRINT("\n  wfq_multiple : %08u @0x%x", (unsigned int)pdata->qos_status_info.qstat.wfq_multiple, (unsigned int)pdata->qos_status_info.qstat.wfq_multiple_addr);
    IFX_PPACMD_PRINT("\n  strict_weight: %08u @0x%x\n", (unsigned int)pdata->qos_status_info.qstat.wfq_strict_pri_weight, (unsigned int)pdata->qos_status_info.qstat.wfq_strict_pri_weight_addr);
#endif

    if ( pdata->qos_status_info.qstat.eth1_eg_qnum && pdata->qos_status_info.qstat.max_buffer_size  )
    {
        uint32_t i;
        uint32_t times = (pdata->qos_status_info.qstat.eth1_eg_qnum > pdata->qos_status_info.qstat.max_buffer_size) ? pdata->qos_status_info.qstat.max_buffer_size:pdata->qos_status_info.qstat.eth1_eg_qnum;

        IFX_PPACMD_PRINT("\n  Cfg :  T   R   S -->  Bit-rate(kbps)    Weight --> Level     Address     d/w    tick_cnt   b/S\n");

        for ( i = 0; i < times; i++ )
        {
            IFX_PPACMD_PRINT("\n    %2u:  %03u  %05u  %05u   %07u      %08u   %03u    @0x%x   %08u  %03u   %05u\n", (unsigned int)i,
                             (unsigned int)pdata->qos_status_info.qstat.queue_internal[i].t,
                             (unsigned int)pdata->qos_status_info.qstat.queue_internal[i].r,
                             (unsigned int)pdata->qos_status_info.qstat.queue_internal[i].s,
                             (unsigned int)pdata->qos_status_info.qstat.queue_internal[i].bit_rate_kbps,
                             (unsigned int)pdata->qos_status_info.qstat.queue_internal[i].w,
                             (unsigned int)pdata->qos_status_info.qstat.queue_internal[i].weight_level,
                             (unsigned int)pdata->qos_status_info.qstat.queue_internal[i].reg_addr,
                             (unsigned int)pdata->qos_status_info.qstat.queue_internal[i].d,
                             (unsigned int)pdata->qos_status_info.qstat.queue_internal[i].tick_cnt,
                             (unsigned int)pdata->qos_status_info.qstat.queue_internal[i].b);
        }

        //QOS Note: For ethernat wan mode only one port rateshaping.  For E5 ptm mode, we have 4 gamma interface port rateshaping configuration
        if( pdata->qos_status_info.qstat.qos_queue_portid == 7 ){//PTM wan mode
            for( i = 0; i < 4; i ++){
                IFX_PPACMD_PRINT("\n  p[%d]:  %03u  %05u  %05u   %07u                        @0x%x   %08u  %03u   %05u\n",
                             i, (unsigned int)pdata->qos_status_info.qstat.ptm_qos_port_rate_shaping[i].t,
                             (unsigned int)pdata->qos_status_info.qstat.ptm_qos_port_rate_shaping[i].r,
                             (unsigned int)pdata->qos_status_info.qstat.ptm_qos_port_rate_shaping[i].s,
                             (unsigned int)pdata->qos_status_info.qstat.ptm_qos_port_rate_shaping[i].bit_rate_kbps,
                             (unsigned int)pdata->qos_status_info.qstat.ptm_qos_port_rate_shaping[i].reg_addr,
                             (unsigned int)pdata->qos_status_info.qstat.ptm_qos_port_rate_shaping[i].d,
                             (unsigned int)pdata->qos_status_info.qstat.ptm_qos_port_rate_shaping[i].tick_cnt,
                             (unsigned int)pdata->qos_status_info.qstat.ptm_qos_port_rate_shaping[i].b);
            }
        }
        else if( pdata->qos_status_info.qstat.qos_queue_portid & 3){
            IFX_PPACMD_PRINT("\n  port:  %03u  %05u  %05u   %07u                        @0x%x   %08u  %03u   %05u\n",
                             (unsigned int)pdata->qos_status_info.qstat.qos_port_rate_internal.t,
                             (unsigned int)pdata->qos_status_info.qstat.qos_port_rate_internal.r,
                             (unsigned int)pdata->qos_status_info.qstat.qos_port_rate_internal.s,
                             (unsigned int)pdata->qos_status_info.qstat.qos_port_rate_internal.bit_rate_kbps,
                             (unsigned int)pdata->qos_status_info.qstat.qos_port_rate_internal.reg_addr,
                             (unsigned int)pdata->qos_status_info.qstat.qos_port_rate_internal.d,
                             (unsigned int)pdata->qos_status_info.qstat.qos_port_rate_internal.tick_cnt,
                             (unsigned int)pdata->qos_status_info.qstat.qos_port_rate_internal.b);

        }
        
        //print debug if necessary
        for ( i = 0; i <times ; i++ )
        {
            if( pdata->qos_status_info.qstat.mib[i].mib.tx_diff_jiffy == 0 ) 
            {
                IFX_PPACMD_PRINT("Error, why tx_diff_jiffy[%2u] is zero\n", i);                
                pdata->qos_status_info.qstat.mib[i].mib.tx_diff_jiffy = 1;
            }
            rate[i]=pdata->qos_status_info.qstat.mib[i].mib.tx_diff * 8 * pdata->qos_status_info.qstat.mib[i].mib.sys_hz/pdata->qos_status_info.qstat.mib[i].mib.tx_diff_jiffy;
            rate_L1[i]=pdata->qos_status_info.qstat.mib[i].mib.tx_diff_L1 * 8 * pdata->qos_status_info.qstat.mib[i].mib.sys_hz/pdata->qos_status_info.qstat.mib[i].mib.tx_diff_jiffy;
        }
        
        IFX_PPACMD_DBG("\n  Info : Rate  tx_diff_bytes tx_diff_jiffy   HZ\n");
        for ( i = 0; i < times ; i++ )
        {            
            IFX_PPACMD_DBG("  %2u:    %010llu %010llu %010llu %010u\n",(unsigned int)i,rate[i],
                             pdata->qos_status_info.qstat.mib[i].mib.tx_diff,
                             pdata->qos_status_info.qstat.mib[i].mib.tx_diff_jiffy,
                             pdata->qos_status_info.qstat.mib[i].mib.sys_hz);            
        }

        IFX_PPACMD_PRINT("\n  MIB : rx_pkt/rx_bytes      tx_pkt/tx_bytes         cpu_small/total_drop  fast_small/total_drop  tx rate/L1(bps/sec)   address\n");
        for ( i = 0; i < times ; i++ )
        {            
            IFX_PPACMD_PRINT("  %2u: %010u/%010u  %010u/%010u  %010u/%010u  %010u/%010u  %010u/%010u @0x%x\n",
                             (unsigned int)i,
                             (unsigned int)pdata->qos_status_info.qstat.mib[i].mib.total_rx_pkt,
                             (unsigned int)pdata->qos_status_info.qstat.mib[i].mib.total_rx_bytes,
                             (unsigned int)pdata->qos_status_info.qstat.mib[i].mib.total_tx_pkt,
                             (unsigned int)pdata->qos_status_info.qstat.mib[i].mib.total_tx_bytes,
                             (unsigned int)pdata->qos_status_info.qstat.mib[i].mib.cpu_path_small_pkt_drop_cnt,
                             (unsigned int)pdata->qos_status_info.qstat.mib[i].mib.cpu_path_total_pkt_drop_cnt,
                             (unsigned int)pdata->qos_status_info.qstat.mib[i].mib.fast_path_small_pkt_drop_cnt,
                             (unsigned int)pdata->qos_status_info.qstat.mib[i].mib.fast_path_total_pkt_drop_cnt,
                             (unsigned int)rate[i],
                             (unsigned int)rate_L1[i],
                             (unsigned int)pdata->qos_status_info.qstat.mib[i].reg_addr                            
                            ) ;

        }

        //QOS queue descriptor
        IFX_PPACMD_PRINT("\n  Desc: threshold  num  base_addr  rd_ptr   wr_ptr\n");
        for(i=0; i<times; i++)
        {
            IFX_PPACMD_PRINT("  %2u: 0x%02x     0x%02x   0x%04x   0x%04x   0x%04x  @0x%x\n",
                             (unsigned int)i,
                             (unsigned int)pdata->qos_status_info.qstat.desc_cfg_interanl[i].threshold,
                             (unsigned int)pdata->qos_status_info.qstat.desc_cfg_interanl[i].length,
                             (unsigned int)pdata->qos_status_info.qstat.desc_cfg_interanl[i].addr,
                             (unsigned int)pdata->qos_status_info.qstat.desc_cfg_interanl[i].rd_ptr,
                             (unsigned int)pdata->qos_status_info.qstat.desc_cfg_interanl[i].wr_ptr,
                             (unsigned int)pdata->qos_status_info.qstat.desc_cfg_interanl[i].reg_addr );
        }

    }
    else
    {
        IFX_PPACMD_PRINT("Note: PPA QOS is disabled for wan_itf=%x  mixed_itf=%x\n",
                         (unsigned int)pdata->qos_status_info.qstat.wan_port_map,
                         (unsigned int)pdata->qos_status_info.qstat.wan_mix_map);
    }


}


/*ppacmd getqnum: get eth1 queue number  ---begin*/
static void ppa_get_qnum_help( int summary)
{
    IFX_PPACMD_PRINT("getqnum [-p portid] [-i ifname] [-o outfile]\n");
    if (summary )
        IFX_PPACMD_PRINT("note: one or only one of portid or ifname must be specified\n");
    return;
}
static const char ppa_get_qnum_short_opts[] = "p:o:i:h";
static const struct option ppa_get_qnum_long_opts[] =
{
    {"portid",   required_argument,  NULL, 'p'},
    {"interface",   required_argument,  NULL, 'i'},
    {"save-to-file",   required_argument,  NULL, 'o'},
    { 0,0,0,0 }
};
static int ppa_parse_get_qnum_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA* pdata)
{
    int p_opt=0, i_opt=0;
    memset( pdata, 0, sizeof(PPA_CMD_QUEUE_NUM_INFO) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'p':
            pdata->qnum_info.portid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            \
            p_opt ++;
            break;

        case 'i':
            if( get_portid(popts->optarg) >= 0 )
            {
                pdata->qnum_info.portid = get_portid(popts->optarg);
                i_opt ++;
            }
            else
            {
                IFX_PPACMD_PRINT("The portid of %s is not exist.\n", popts->optarg);
                return PPA_CMD_ERR;
            }
            break;

        case  'o':
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( p_opt + i_opt != 1)
    {
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
        return PPA_CMD_ERR;
    }
    IFX_PPACMD_DBG("ppa_parse_get_qnum_cmd: portid=%d, queue_num=%d\n", (unsigned int)pdata->qnum_info.portid, (unsigned int)pdata->qnum_info.queue_num);

    return PPA_CMD_OK;
}
static void ppa_print_get_qnum_cmd(PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
    {
        IFX_PPACMD_PRINT("The queue number( of port id %d )  is %d\n", (unsigned int)pdata->qnum_info.portid, (unsigned int)pdata->qnum_info.queue_num);
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->qnum_info), sizeof(pdata->qnum_info) );
    }
}
/*ppacmd getqnum: get eth1 queue number  ---end*/


/*ppacmd getmib: get qos mib counter  ---begin*/
static void ppa_get_qmib_help( int summary)
{
    IFX_PPACMD_PRINT("getqmib [-p portid] [-i ifname] <-q queuid> [-o outfile]\n");
    if( summary )
    {
        IFX_PPACMD_PRINT("note: one or only one of portid or ifname must be specified\n");
        IFX_PPACMD_PRINT("    if queueid is not provided, then it will get all queue's mib coutners\n");
    }
    return;
}
static const char ppa_get_qmib_short_opts[] = "p:i:q:o:h";
static const struct option ppa_get_qmib_long_opts[] =
{
    {"portid",   required_argument,  NULL, 'p'},
    {"ifname",   required_argument,  NULL, 'i'},
    {"queueid",   required_argument,  NULL, 'q'},
    {"save-to-file",   required_argument,  NULL, 'o'},
    { 0,0,0,0 }
};
static int ppa_parse_get_qmib_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA* pdata)
{
    int p_opt=0, i_opt=0;

    memset( pdata, 0, sizeof(PPA_CMD_QOS_MIB_INFO) );
    pdata->qos_mib_info.queueid = -1;

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'p':
            pdata->qos_mib_info.portid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            p_opt ++;
            break;

        case 'i':
            if( get_portid(popts->optarg) >= 0 )
            {
                pdata->qos_mib_info.portid = get_portid(popts->optarg);
                i_opt ++;
            }
            else
            {
                IFX_PPACMD_PRINT("The portid of %s is not exist.\n", popts->optarg);
                return PPA_CMD_ERR;
            }
            break;

        case 'q':
            pdata->qos_mib_info.queueid= str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            break;

        case  'o':
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( p_opt + i_opt != 1)
    {
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
        return PPA_CMD_ERR;
    }

    IFX_PPACMD_DBG("ppa_parse_get_qmib_cmd: portid=%d, queueid=%d\n", (unsigned int)pdata->qos_mib_info.portid, (unsigned int)pdata->qos_mib_info.queueid);

    return PPA_CMD_OK;
}

static int ppa_get_qmib_do_cmd(PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata)
{
    PPA_CMD_QUEUE_NUM_INFO qnum_info;
    PPA_CMD_QOS_MIB_INFO mib_info;
    int i, start_i, end_i=0;
    int res;

    memset( &qnum_info, 0, sizeof(qnum_info) );
    qnum_info.portid = pdata->qos_mib_info.portid;
    if( (res = ppa_do_ioctl_cmd(PPA_CMD_GET_QOS_QUEUE_MAX_NUM, &qnum_info ) != PPA_CMD_OK ) )
    {
        IFX_PPACMD_PRINT("ioctl PPA_CMD_GET_QOS_QUEUE_MAX_NUM fail\n");
        return IFX_FAILURE;
    }
    if( pdata->qos_mib_info.queueid != -1 ) // set queueid already, then use it
    {
        start_i = pdata->qos_mib_info.queueid;
        if( start_i >= qnum_info.queue_num -1 )
            start_i = qnum_info.queue_num -1;
        end_i = start_i + 1;
        IFX_PPACMD_PRINT("Need to read queue %d's mib counter\n", start_i);
    }
    else
    {
        start_i = 0;
        end_i = qnum_info.queue_num;
        IFX_PPACMD_PRINT("Need to read mib counter from queue %d to %d\n", start_i, end_i-1 );
    }

    for(i=start_i; i<end_i; i++)
    {
        memset( &mib_info, 0, sizeof(mib_info) );
        mib_info.portid = pdata->qos_mib_info.portid;
        mib_info.queueid = i;

        if( (res = ppa_do_ioctl_cmd(PPA_CMD_GET_QOS_MIB, &mib_info) == PPA_CMD_OK ) )
        {
            if( !g_output_to_file  )
            {
                if( i== start_i )
                    IFX_PPACMD_PRINT("MIB  rx_pkt/rx_bytes     tx_pkt/tx_bytes    cpu_small_drop/cpu_drop  fast_small_drop/fast_drop_cnt\n");

                IFX_PPACMD_PRINT("  %2d: 0x%08x/0x%08x  0x%08x/0x%08x  0x%08x/0x%08x  0x%08x/0x%08x\n", i,
                                 (unsigned int)mib_info.mib.total_rx_pkt, (unsigned int)mib_info.mib.total_rx_bytes,
                                 (unsigned int)mib_info.mib.total_tx_pkt, (unsigned int)mib_info.mib.total_tx_bytes,
                                 (unsigned int)mib_info.mib.cpu_path_small_pkt_drop_cnt, (unsigned int)mib_info.mib.cpu_path_total_pkt_drop_cnt,
                                 (unsigned int)mib_info.mib.fast_path_small_pkt_drop_cnt, (unsigned int)mib_info.mib.fast_path_total_pkt_drop_cnt );
            }
            else
            {
                /*note, currently only last valid flow info is saved to file and all other flow informations are all overwritten */
                SaveDataToFile(g_output_filename, (char *)(&mib_info), sizeof(mib_info) );
            }
        }
    }
    return IFX_SUCCESS;
}

/*ppacmd getqmib: get eth1 queue number  ---end*/

#ifdef CONFIG_IFX_PPA_QOS_WFQ
/*ppacmd setctrlwfq ---begin*/
static void ppa_set_ctrl_wfq_help( int summary)
{
    IFX_PPACMD_PRINT("setctrlwfq [-p portid] [-i ifname] <-c enable | disable> [-m manual_wfq]\n");
    if( summary )
    {
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
        IFX_PPACMD_PRINT("note: manual_wfq 0      -- default weight\n");
        IFX_PPACMD_PRINT("note: manual_wfq 1      -- use user specified weight directly\n");
        IFX_PPACMD_PRINT("note: manual_wfq other_value-- use user specified mapping \n");
    }
    return;
}
static const char ppa_set_ctrl_wfq_short_opts[] = "p:c:i:m:h";
static const struct option ppa_set_ctrl_wfq_long_opts[] =
{
    {"portid",  required_argument,  NULL, 'p'},
    {"ifname",   required_argument,  NULL, 'i'},
    {"control",   required_argument,  NULL, 'c'},
    {"manual",   required_argument,  NULL, 'm'},
    { 0,0,0,0 }
};
static int ppa_parse_set_ctrl_wfq_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int c_opt=0;
    int p_opt=0, i_opt=0;

    memset( pdata, 0, sizeof(PPA_CMD_QOS_CTRL_INFO) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'p':
            pdata->qos_ctrl_info.portid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            p_opt ++;
            break;

        case 'i':
            if( get_portid(popts->optarg) >= 0 )
            {
                pdata->qos_ctrl_info.portid = get_portid(popts->optarg);
                i_opt ++;
            }
            else
            {
                IFX_PPACMD_PRINT("The portid of %s is not exist.\n", popts->optarg);
                return PPA_CMD_ERR;
            }
            break;

        case 'c':
            if( strcmp("enable" , popts->optarg) == 0 )
                pdata->qos_ctrl_info.enable = 1;
            else
                pdata->qos_ctrl_info.enable = 0;
            c_opt = 1;
            break;

        case 'm':
            pdata->qos_ctrl_info.flags= str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL); //use manual set WFQ weight
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( c_opt == 0 )
    {
        IFX_PPACMD_PRINT("Need enter control option\n");
        return PPA_CMD_ERR;
    }

    if( p_opt + i_opt != 1)
    {
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
        return PPA_CMD_ERR;
    }

    IFX_PPACMD_DBG("ppa_parse_set_ctrl_wfq_cmd: portid=%d, ctrl=%s\n", (unsigned int)pdata->qos_ctrl_info.portid, pdata->qos_ctrl_info.enable ? "enabled":"disable");

    return PPA_CMD_OK;
}
/*ppacmd setctrlwfq ---end*/

/*ppacmd getctrlwfq ---begin*/
static void ppa_get_ctrl_wfq_help( int summary)
{
    IFX_PPACMD_PRINT("getctrlwfq [-p portid] [-i ifname] [-o outfile]\n");
    if( summary )
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
    return;
}
static const char ppa_get_ctrl_wfq_short_opts[] = "p:i:o:h";
static const struct option ppa_get_ctrl_wfq_long_opts[] =
{
    {"portid",   required_argument,  NULL, 'p'},
    {"ifname",   required_argument,  NULL, 'i'},
    {"save-to-file",   required_argument,  NULL, 'o'},
    { 0,0,0,0 }
};
static int ppa_parse_get_ctrl_wfq_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    int p_opt=0, i_opt=0;
    memset( pdata, 0, sizeof(PPA_CMD_QOS_CTRL_INFO) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'p':
            pdata->qos_ctrl_info.portid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            p_opt ++;
            break;

        case 'i':
            if( get_portid(popts->optarg) >= 0 )
            {
                pdata->qos_ctrl_info.portid = get_portid(popts->optarg);
                i_opt ++;
            }
            else
            {
                IFX_PPACMD_PRINT("The portid of %s is not exist.\n", popts->optarg);
                return PPA_CMD_ERR;
            }
            break;

        case  'o':
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( p_opt + i_opt != 1)
    {
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
        return PPA_CMD_ERR;
    }
    IFX_PPACMD_DBG("ppa_parse_get_ctrl_wfq_cmd: portid=%d, ctrl=%s\n", (unsigned int)pdata->qos_ctrl_info.portid, pdata->qos_ctrl_info.enable? "enabled":"disabled");

    return PPA_CMD_OK;
}
static void ppa_print_get_ctrl_wfq_cmd(PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
    {
        IFX_PPACMD_PRINT("The wfq of port id %d is %s\n", (unsigned int)pdata->qos_ctrl_info.portid, pdata->qos_ctrl_info.enable ? "enabled":"disabled");
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->qos_ctrl_info), sizeof(pdata->qos_ctrl_info) );
    }
}
/*ppacmd getctrlwfq ---end*/



/*ppacmd setwfq ---begin*/
static void ppa_set_wfq_help( int summary)
{
    IFX_PPACMD_PRINT("setwfq [-p portid] [-i ifname] <-q queuid> <-w weight-level>\n");
    if( summary )
    {
        IFX_PPACMD_PRINT("weight-level is from 0 ~ 100. 0/100 means lowest/highest strict priority queue\n");
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
    }
    return;
}
static const char ppa_set_wfq_short_opts[] = "p:q:w:i:h";
static const struct option ppa_set_wfq_long_opts[] =
{
    {"portid",   required_argument,  NULL, 'p'},
    {"ifname",   required_argument,  NULL, 'i'},
    {"queuid",   required_argument,  NULL, 'q'},
    {"weight-level",   required_argument,  NULL, 'w'},
    { 0,0,0,0 }
};

static int ppa_parse_set_wfq_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA*pdata)
{
    unsigned int w_opt=0, q_opt=0;
    int p_opt=0, i_opt=0;
    memset( pdata, 0, sizeof(PPA_CMD_WFQ_INFO) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'p':
            pdata->qos_wfq_info.portid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            p_opt ++;
            break;
        case 'i':
            if( get_portid(popts->optarg) >= 0 )
            {
                pdata->qos_wfq_info.portid = get_portid(popts->optarg);
                i_opt ++;
            }
            else
            {
                IFX_PPACMD_PRINT("The portid of %s is not exist.\n", popts->optarg);
                return PPA_CMD_ERR;
            }
            break;
        case 'q':
            pdata->qos_wfq_info.queueid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            q_opt = 1;
            break;
        case 'w':
            pdata->qos_wfq_info.weight = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            w_opt = 1;
            break;
        default:
            IFX_PPACMD_PRINT("ppa_parse_set_wfq_cmd not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( q_opt == 0 )
    {
        IFX_PPACMD_PRINT("Need enter queueid\n");
        return PPA_CMD_ERR;
    }
    if( w_opt == 0 )
    {
        IFX_PPACMD_PRINT("Need enter weight\n");
        return PPA_CMD_ERR;
    }
    if( p_opt + i_opt != 1)
    {
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
        return PPA_CMD_ERR;
    }
    IFX_PPACMD_DBG("ppa_parse_set_wfq_cmd: portid=%d, queueid=%d, weight=%d\n", (unsigned int)pdata->qos_wfq_info.portid, (unsigned int)pdata->qos_wfq_info.queueid, (unsigned int)pdata->qos_wfq_info.weight);

    return PPA_CMD_OK;
}
/*ppacmd setwfq ---end*/

/*ppacmd resetwfq ---begin*/
static void ppa_reset_wfq_help( int summary)
{
    IFX_PPACMD_PRINT("resetwfq [-p portid] [-i ifname] <-q queuid> \n");
    if( summary )
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
    return;
}
static const char ppa_reset_wfq_short_opts[] = "p:q:i:h";
static const struct option ppa_reset_wfq_long_opts[] =
{
    {"portid",   required_argument,  NULL, 'p'},
    {"ifname",   required_argument,  NULL, 'i'},
    {"queuid",   required_argument,  NULL, 'q'},
    { 0,0,0,0 }
};
static int ppa_parse_reset_wfq_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int q_opt=0;
    int p_opt=0, i_opt=0;
    memset( pdata, 0, sizeof(PPA_CMD_WFQ_INFO) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'p':
            pdata->qos_wfq_info.portid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            p_opt ++;
            break;
        case 'i':
            if( get_portid(popts->optarg) >= 0 )
            {
                pdata->qos_wfq_info.portid = get_portid(popts->optarg);
                i_opt ++;
            }
            else
            {
                IFX_PPACMD_PRINT("The portid of %s is not exist.\n", popts->optarg);
                return PPA_CMD_ERR;
            }
            break;
        case 'q':
            pdata->qos_wfq_info.queueid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            q_opt = 1;
            break;
        default:
            IFX_PPACMD_PRINT("ppa_parse_reset_wfq_cmd not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( q_opt == 0 )
    {
        IFX_PPACMD_PRINT("Need enter queueid\n");
        return PPA_CMD_ERR;
    }
    if( p_opt + i_opt != 1)
    {
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
        return PPA_CMD_ERR;
    }

    IFX_PPACMD_DBG("ppa_parse_reset_wfq_cmd: portid=%d, queueid=%d\n", (unsigned int)pdata->qos_wfq_info.portid, (unsigned int)pdata->qos_wfq_info.queueid);

    return PPA_CMD_OK;
}
/*ppacmd resetwfq ---end*/

/*ppacmd getwfq ---begin*/
static void ppa_get_wfq_help( int summary)
{
    IFX_PPACMD_PRINT("getwfq [-p portid] [-i ifname] <-q queuid> [-o outfile]\n");
    if( summary )
    {
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
        IFX_PPACMD_PRINT("    if queueid is not provided, then it will get all queue's wfq\n");
    }
    return;
}
static const char ppa_get_wfq_short_opts[] = "p:q:o:i:h";
static const struct option ppa_get_wfq_long_opts[] =
{
    {"portid",   required_argument,  NULL, 'p'},
    {"ifname",   required_argument,  NULL, 'i'},
    {"queuid",   required_argument,  NULL, 'q'},
    {"save-to-file",   required_argument,  NULL, 'o'},
    { 0,0,0,0 }
};
static int ppa_parse_get_wfq_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int q_opt=0;
    int p_opt=0, i_opt=0;

    memset( pdata, 0, sizeof(PPA_CMD_WFQ_INFO) );
    pdata->qos_wfq_info.queueid = -1;

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'p':
            pdata->qos_wfq_info.portid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            p_opt ++;
            break;
        case 'i':
            if( get_portid(popts->optarg) >= 0 )
            {
                pdata->qos_wfq_info.portid = get_portid(popts->optarg);
                i_opt ++;
            }
            else
            {
                IFX_PPACMD_PRINT("The portid of %s is not exist.\n", popts->optarg);
                return PPA_CMD_ERR;
            }
            break;
        case 'q':
            pdata->qos_wfq_info.queueid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            q_opt = 1;
            break;
        case  'o':
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( p_opt + i_opt != 1)
    {
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
        return PPA_CMD_ERR;
    }

    IFX_PPACMD_DBG("ppa_parse_get_wfq_cmd: portid=%d, queueid=%d\n", (unsigned int)pdata->qos_wfq_info.portid, (unsigned int)pdata->qos_wfq_info.queueid);

    return PPA_CMD_OK;
}

static int ppa_get_wfq_do_cmd(PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata)
{
    PPA_CMD_QUEUE_NUM_INFO qnum_info;
    PPA_CMD_WFQ_INFO  info;
    int i, start_i, end_i=0;
    int res;

    memset( &qnum_info, 0, sizeof(qnum_info) );
    qnum_info.portid = pdata->qos_wfq_info.portid;
    if( (res = ppa_do_ioctl_cmd(PPA_CMD_GET_QOS_QUEUE_MAX_NUM, &qnum_info ) != PPA_CMD_OK ) )
    {
        IFX_PPACMD_PRINT("ioctl PPA_CMD_GET_QOS_QUEUE_MAX_NUM fail\n");
        return IFX_FAILURE;
    }

    if( pdata->qos_wfq_info.queueid != -1 ) // set queuid already, then use it
    {
        start_i = pdata->qos_wfq_info.queueid;
        if( start_i >= qnum_info.queue_num -1 )
            start_i = qnum_info.queue_num -1;
        end_i = start_i + 1;
        IFX_PPACMD_DBG("Need to read wfq from queue %d \n", start_i);
    }
    else
    {
        start_i = 0;
        end_i = qnum_info.queue_num;
        IFX_PPACMD_DBG("Need to read wfq from queue %d to %d\n", start_i, end_i-1 );
    }

    for(i=start_i; i<end_i; i++)
    {
        memset( &info, 0, sizeof(info) );
        info.portid = pdata->qos_wfq_info.portid;
        info.queueid = i;

        if( (res = ppa_do_ioctl_cmd(PPA_CMD_GET_QOS_WFQ, &info) == PPA_CMD_OK ) )
        {
            if( !g_output_to_file  )
            {
                IFX_PPACMD_PRINT("  queue %2d wfq rate: %d\n", i, (unsigned int)info.weight );
            }
            else
            {
                /*note, currently only last valid flow info is saved to file and all other flow informations are all overwritten */
                SaveDataToFile(g_output_filename, (char *)(&info), sizeof(info) );
            }
        }
    }
    return IFX_SUCCESS;
}

/*ppacmd getwfq ---end*/
#endif  //enod of CONFIG_IFX_PPA_QOS_WFQ


#ifdef CONFIG_IFX_PPA_QOS_RATE_SHAPING
/*ppacmd setctrlrate ---begin*/
static void ppa_set_ctrl_rate_help( int summary)
{
    IFX_PPACMD_PRINT("setctrlrate [-p portid] [-i ifname] <-c enable | disable>\n");
    if( summary )
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
    return;
}
static const char ppa_set_ctrl_rate_short_opts[] = "p:c:i:h";
static const struct option ppa_set_ctrl_rate_long_opts[] =
{
    {"portid",  required_argument,  NULL, 'p'},
    {"ifname",   required_argument,  NULL, 'i'},
    {"control",   required_argument,  NULL, 'c'},
    { 0,0,0,0 }
};
static int ppa_parse_set_ctrl_rate_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int c_opt=0;
    int p_opt=0, i_opt=0;
    memset( pdata, 0, sizeof(PPA_CMD_QOS_CTRL_INFO) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'p':
            pdata->qos_ctrl_info.portid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            p_opt ++;
            break;
        case 'i':
            if( get_portid(popts->optarg) >= 0 )
            {
                pdata->qos_ctrl_info.portid = get_portid(popts->optarg);
                i_opt ++;
            }
            else
            {
                IFX_PPACMD_PRINT("The portid of %s is not exist.\n", popts->optarg);
                return PPA_CMD_ERR;
            }
            break;
        case 'c':
            if( strcmp("enable" , popts->optarg) == 0 )
                pdata->qos_ctrl_info.enable = 1;
            else
                pdata->qos_ctrl_info.enable = 0;
            c_opt = 1;
            break;
        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( c_opt == 0 )
    {
        IFX_PPACMD_PRINT("Need enter control option\n");
        return PPA_CMD_ERR;
    }
    if( p_opt + i_opt != 1)
    {
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
        return PPA_CMD_ERR;
    }

    IFX_PPACMD_DBG("ppa_parse_set_ctrl_rate_cmd: portid=%d, ctrl=%s\n", (unsigned int)pdata->qos_ctrl_info.portid, pdata->qos_ctrl_info.enable ? "enabled":"disable");

    return PPA_CMD_OK;
}
/*ppacmd setctrlrate ---end*/

/*ppacmd getctrlrate ---begin*/
static void ppa_get_ctrl_rate_help( int summary)
{
    IFX_PPACMD_PRINT("getctrlrate [-p portid] [-i ifname] [-o outfile]\n");
    if( summary )
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
    return;
}
static const char ppa_get_ctrl_rate_short_opts[] = "p:i:h";
static const struct option ppa_get_ctrl_rate_long_opts[] =
{
    {"portid",   required_argument,  NULL, 'p'},
    {"ifname",   required_argument,  NULL, 'i'},
    {"save-to-file",   required_argument,  NULL, 'o'},
    { 0,0,0,0 }
};
static int ppa_parse_get_ctrl_rate_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    int p_opt=0, i_opt=0;

    memset( pdata, 0, sizeof(PPA_CMD_QOS_CTRL_INFO) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'p':
            pdata->qos_ctrl_info.portid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            p_opt ++;
            break;
        case 'i':
            if( get_portid(popts->optarg) >= 0 )
            {
                pdata->qos_ctrl_info.portid = get_portid(popts->optarg);
                i_opt ++;
            }
            else
            {
                IFX_PPACMD_PRINT("The portid of %s is not exist.\n", popts->optarg);
                return PPA_CMD_ERR;
            }
            break;
        case  'o':
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }
    if( p_opt + i_opt != 1)
    {
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
        return PPA_CMD_ERR;
    }
    IFX_PPACMD_DBG("ppa_parse_get_ctrl_rate_cmd: portid=%d\n", (unsigned int)pdata->qos_ctrl_info.portid);

    return PPA_CMD_OK;
}
static void ppa_print_get_ctrl_rate_cmd(PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
    {
        IFX_PPACMD_PRINT("The rate of port id %d is %s\n", (unsigned int)pdata->qos_ctrl_info.portid, pdata->qos_ctrl_info.enable ? "enabled":"disabled");
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->qos_ctrl_info), sizeof(pdata->qos_ctrl_info) );
    }
}
/*ppacmd getctrlrate ---end*/

/*ppacmd setrate ---begin*/
static void ppa_set_rate_help( int summary)
{
    IFX_PPACMD_PRINT("setrate [-p portid] [-i ifname] [-q queuid | -g gamma_itf_id ] <-r rate> <-b burst>\n");
    if( summary )
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
    return;
}
static const char ppa_set_rate_short_opts[] = "p:i:q:g:r:b:h";
static const struct option ppa_set_rate_long_opts[] =
{
    {"portid",   required_argument,  NULL,   'p'},
    {"ifname",   required_argument,  NULL,   'i'},
    {"queuid",   required_argument,  NULL,   'q'},
	{"gammaitf", required_argument,  NULL,   'g'},
    {"rate",     required_argument,  NULL,   'r'},
    {"burst",    required_argument,  NULL,   'b'},
    { 0,0,0,0 }
};

static int ppa_set_rate_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA*pdata)
{
    unsigned int r_opt=0, q_opt=0;
    int p_opt=0, i_opt=0;
    memset( pdata, 0, sizeof(PPA_CMD_RATE_INFO) );

    q_opt = 1;  //by default if no queue id is specified, it will be regarded as port based rate shaping.
    pdata->qos_rate_info.queueid = -1;
    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'p':
            pdata->qos_rate_info.portid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            p_opt ++;
            break;
        case 'i':
            if( get_portid(popts->optarg) >= 0 )
            {
                pdata->qos_rate_info.portid = get_portid(popts->optarg);
                i_opt ++;
            }
            else
            {
                IFX_PPACMD_PRINT("The portid of %s is not exist.\n", popts->optarg);
                return PPA_CMD_ERR;
            }
            break;
        case 'q':
            pdata->qos_rate_info.queueid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            q_opt ++;
            break;
		case 'g':
		    pdata->qos_rate_info.queueid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            pdata->qos_rate_info.queueid = ~ pdata->qos_rate_info.queueid; //if queueid is bigger than max allowed queueid, it is regarded as port id
			q_opt ++;
			break;
        case 'r':
            pdata->qos_rate_info.rate    = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            r_opt = 1;
            break;
        case 'b':
            pdata->qos_rate_info.burst   = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            r_opt = 1;
            break;
        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( q_opt > 2 )
    {
        IFX_PPACMD_PRINT("Queue id and gamma interface id cannot both be set id\n");
        return PPA_CMD_ERR;
    }
    if( q_opt == 0){//no queue id and gamma interface id setting, default for port 0 setting
        pdata->qos_rate_info.queueid = ~0;
    }
    if( r_opt == 0 )
    {
        IFX_PPACMD_PRINT("Need enter rate\n");
        return PPA_CMD_ERR;
    }
    if( p_opt + i_opt != 1)
    {
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
        return PPA_CMD_ERR;
    }
    IFX_PPACMD_DBG("ppa_set_rate_cmd: portid=%d, queueid=%d,rate=%d burst=%d\n", 
	         (unsigned int)pdata->qos_rate_info.portid, 
			 (unsigned int)pdata->qos_rate_info.queueid, 
			 (unsigned int)pdata->qos_rate_info.rate, 
			 (unsigned int)pdata->qos_rate_info.burst);

    return PPA_CMD_OK;
}
/*ppacmd setrate ---end*/

/*ppacmd resetrate ---begin*/
static void ppa_reset_rate_help( int summary)
{
    IFX_PPACMD_PRINT("resetrate [-p portid] [-i ifname] [ <-q queuid> | -g <gamma interface id> ] \n");
    if( summary )
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
    return;
}
static const char ppa_reset_rate_short_opts[] = "p:q:i:h";
static const struct option ppa_reset_rate_long_opts[] =
{
    {"portid",   required_argument,  NULL, 'p'},
    {"ifname",   required_argument,  NULL, 'i'},
    {"queuid",   required_argument,  NULL, 'q'},
    { 0,0,0,0 }
};
static int ppa_parse_reset_rate_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int q_opt=0;
    int p_opt=0, i_opt=0;
    memset( pdata, 0, sizeof(PPA_CMD_RATE_INFO) );
    pdata->qos_rate_info.burst = -1;

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'p':
            pdata->qos_rate_info.portid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            p_opt ++;
            break;
        case 'i':
            if( get_portid(popts->optarg) >= 0 )
            {
                pdata->qos_rate_info.portid = get_portid(popts->optarg);
                i_opt ++;
            }
            else
            {
                IFX_PPACMD_PRINT("The portid of %s is not exist.\n", popts->optarg);
                return PPA_CMD_ERR;
            }
            break;
        case 'q':
            pdata->qos_rate_info.queueid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            q_opt ++ ;
            break;
            
        case 'g':
		    pdata->qos_rate_info.queueid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            pdata->qos_rate_info.queueid = ~ pdata->qos_rate_info.queueid; //if queueid is bigger than max allowed queueid, it is regarded as port id
			q_opt ++;
			break;
        default:
            IFX_PPACMD_PRINT("ppa_parse_reset_rate_cmd not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( q_opt > 2 )
    {
        IFX_PPACMD_PRINT("Queue id and gamma interface id cannot both be set id\n");
        return PPA_CMD_ERR;
    }

    if( q_opt == 0 )
    {
        IFX_PPACMD_PRINT("Need enter queueid\n");
        return PPA_CMD_ERR;
    }
    if( p_opt + i_opt != 1)
    {
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
        return PPA_CMD_ERR;
    }

    IFX_PPACMD_DBG("ppa_parse_reset_rate_cmd: portid=%d, queueid=%d\n", (unsigned int)pdata->qos_rate_info.portid, (unsigned int)pdata->qos_rate_info.queueid);

    return PPA_CMD_OK;
}
/*ppacmd resetrate ---end*/

/*ppacmd getrate ---begin*/
static void ppa_get_rate_help( int summary)
{
    IFX_PPACMD_PRINT("getrate [-p portid] [-i ifname] <-q queuid> [-o outfile]\n");
    if( summary )
    {
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
        IFX_PPACMD_PRINT("    if queueid is not provided, then get all queue's rate information\n");
    }
    return;
}
static const char ppa_get_rate_short_opts[] = "p:q:o:i:h";
static const struct option ppa_get_rate_long_opts[] =
{
    {"portid",   required_argument,  NULL, 'p'},
    {"ifname",   required_argument,  NULL, 'i'},
    {"queuid",   required_argument,  NULL, 'q'},
    {"save-to-file",   required_argument,  NULL, 'o'},
    { 0,0,0,0 }
};
static int ppa_parse_get_rate_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int q_opt=0;
    int p_opt=0, i_opt=0;
    memset( pdata, 0, sizeof(PPA_CMD_RATE_INFO) );
    pdata->qos_rate_info.queueid = -1;

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'p':
            pdata->qos_rate_info.portid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            p_opt ++;
            break;
        case 'i':
            if( get_portid(popts->optarg) >= 0 )
            {
                pdata->qos_rate_info.portid = get_portid(popts->optarg);
                i_opt ++;
            }
            else
            {
                IFX_PPACMD_PRINT("The portid of %s is not exist.\n", popts->optarg);
                return PPA_CMD_ERR;
            }
            break;
        case 'q':
            pdata->qos_rate_info.queueid = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            q_opt = 1;
            break;
        case 'o':
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            break;

        default:
            IFX_PPACMD_PRINT("ppa_parse_get_rate_cmd not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( p_opt + i_opt != 1)
    {
        IFX_PPACMD_PRINT("note: one or only one of portid and ifname must be specified\n");
        return PPA_CMD_ERR;
    }

    IFX_PPACMD_DBG("ppa_parse_get_rate_cmd: portid=%d, queueid=%d\n", (unsigned int)pdata->qos_rate_info.portid, (unsigned int)pdata->qos_rate_info.queueid);

    return PPA_CMD_OK;
}

static int ppa_get_rate_do_cmd(PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata)
{
    PPA_CMD_QUEUE_NUM_INFO qnum_info;
    PPA_CMD_RATE_INFO  info;
    int i=0, j=0, start_i, end_i=0;
    int res;

    memset( &qnum_info, 0, sizeof(qnum_info) );
    qnum_info.portid = pdata->qos_rate_info.portid;
    if( (res = ppa_do_ioctl_cmd(PPA_CMD_GET_QOS_QUEUE_MAX_NUM, &qnum_info ) != PPA_CMD_OK ) )
    {
        IFX_PPACMD_PRINT("ioctl PPA_CMD_GET_QOS_QUEUE_MAX_NUM fail\n");
        return IFX_FAILURE;
    }

    if( pdata->qos_rate_info.queueid!= -1 ) // set index already, then use it
    {
        memset( &info, 0, sizeof(info) );
        info.portid = pdata->qos_rate_info.portid;
        info.queueid = pdata->qos_rate_info.queueid;

        if( (res = ppa_do_ioctl_cmd(PPA_CMD_GET_QOS_RATE, &info) == PPA_CMD_OK ) )
        {
            if( !g_output_to_file  )
            {
                IFX_PPACMD_PRINT("       Rate     Burst\n");

                if( pdata->qos_rate_info.queueid < qnum_info.queue_num )
                    IFX_PPACMD_PRINT("   queue %2d:  %08d(kbps) 0x%04d\n", (unsigned int)info.queueid, (unsigned int)info.rate, (unsigned int)info.burst );
                else
                    IFX_PPACMD_PRINT("   port    :  %08d(kbps) 0x%04d\n", (unsigned int)info.rate, (unsigned int)info.burst );
            }
            else
            {
                /*note, currently only last valid flow info is saved to file and all other flow informations are all overwritten */
                SaveDataToFile(g_output_filename, (char *)(&info), sizeof(info) );
            }
            j++;
        }
    }
    else
    {
        start_i = 0;
        end_i = qnum_info.queue_num;
        IFX_PPACMD_DBG("Need to read rate shaping from queue %d to %d\n", start_i, end_i-1 );

        for(i=start_i; i<end_i; i++)
        {
            memset( &info, 0, sizeof(info) );
            info.portid = pdata->qos_rate_info.portid;
            info.queueid = i;

            if( (res = ppa_do_ioctl_cmd(PPA_CMD_GET_QOS_RATE, &info) == PPA_CMD_OK ) )
            {
                if( !g_output_to_file  )
                {
                    if( i == start_i )
                        IFX_PPACMD_PRINT("       Rate     Burst\n");

                    IFX_PPACMD_PRINT("   queue %2d:  %07d(kbps) 0x%04d\n", i, (unsigned int)info.rate, (unsigned int)info.burst );
                }
                else
                {
                    /*note, currently only last valid flow info is saved to file and all other flow informations are all overwritten */
                    SaveDataToFile(g_output_filename, (char *)(&info), sizeof(info) );
                }
                j++;
            }
        }

        if( pdata->qos_rate_info.queueid == -1 )
        {
            memset( &info, 0, sizeof(info) );
            info.portid = pdata->qos_rate_info.portid;
            info.queueid = qnum_info.queue_num;

            if( (res = ppa_do_ioctl_cmd(PPA_CMD_GET_QOS_RATE, &info) == PPA_CMD_OK ) )
            {
                if( !g_output_to_file  )
                {
                    IFX_PPACMD_PRINT("   port   :   %07d(kbps) 0x%04d\n", (unsigned int)info.rate, (unsigned int)info.burst );
                }
                else
                {
                    /*note, currently only last valid flow info is saved to file and all other flow informations are all overwritten */
                    SaveDataToFile(g_output_filename, (char *)(&info), sizeof(info) );
                }
                j++;
            }
        }
    }




    return IFX_SUCCESS;
}

/*ppacmd getrate ---end*/
#endif  //enod of CONFIG_IFX_PPA_QOS_RATE_SHAPING


#endif /*end if CONFIG_IFX_PPA_QOS */


/*ppacmd gethook: get all exported hook number  ---begin*/
static void ppa_get_hook_count_help( int summary)
{
    IFX_PPACMD_PRINT("gethooknum [-o outfile]\n");
    return;
}

static const char ppa_get_hook_short_opts[] = "o:h";
static const struct option ppa_get_hook_long_opts[] =
{
    {"save-to-file",   required_argument,  NULL, 'o'},
    { 0,0,0,0 }
};

static void ppa_print_get_hook_count_cmd(PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
    {
        IFX_PPACMD_PRINT("There are %u exported hook \n", (unsigned int) pdata->count_info.count);
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->count_info), sizeof(pdata->count_info) );
    }
}
/*ppacmd gethook: get all exported hook number  ---end*/

/*ppacmd gethooklist: get all exported hook list  ---begin*/
static void ppa_get_hook_list_help( int summary)
{
    IFX_PPACMD_PRINT("gethooklist [-o outfile]\n");
    return;
}

static const char ppa_get_hook_list_short_opts[] = "o:h";
static const struct option ppa_get_hook_list_long_opts[] =
{
    {"save-to-file",   required_argument,  NULL, 'o'},
    { 0,0,0,0 }
};
static int ppa_get_hook_list_cmd(PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata)
{
    PPA_CMD_DATA cmd_info;
    PPA_CMD_HOOK_LIST_INFO *phook_list;
    int res = PPA_CMD_OK, i, size;

    //get session count first before malloc memroy
    cmd_info.count_info.flag = 0;
    cmd_info.count_info.count = 0;
    if( ppa_do_ioctl_cmd(PPA_CMD_GET_HOOK_COUNT, &cmd_info ) != PPA_CMD_OK )
    {
        IFX_PPACMD_DBG("ppa_do_ioctl_cmd: PPA_CMD_GET_HOOK_COUNT failed\n");
        return -EIO;
    }

    if( cmd_info.count_info.count <= 0 )
    {
        IFX_PPACMD_DBG("Note, there is no exported PPA hooks at all\n");
        return -EIO;
    }
    IFX_PPACMD_DBG("There are %u hook mapped\n", (unsigned int) cmd_info.count_info.count );

    //malloc memory and set value correctly
    size = sizeof(PPA_CMD_HOOK_LIST_INFO) + cmd_info.count_info.count * sizeof(PPA_HOOK_INFO);
    phook_list = (PPA_CMD_HOOK_LIST_INFO *) malloc( size );
    if( phook_list == NULL )
    {
        IFX_PPACMD_DBG("Malloc failed in ppa_get_hook_list_cmd\n");
        return PPA_CMD_NOT_AVAIL;
    }

    phook_list->hook_count = cmd_info.count_info.count;
    phook_list->flag = 0;

    //get session information
    if( (res = ppa_do_ioctl_cmd(PPA_CMD_GET_HOOK_LIST, phook_list ) != PPA_CMD_OK ) )
    {
        free( phook_list );
        IFX_PPACMD_DBG("ppa_get_hook_list_cmd ioctl  failed\n");
        return res;
    }

    if( !g_output_to_file )
    {
        for(i=0; i<phook_list->hook_count; i++)
        {
            if( phook_list->list[i].used_flag )
                IFX_PPACMD_PRINT("%02d: %-50s( hook address  0x%x: %s)\n", i,  phook_list->list[i].hookname,(unsigned int) phook_list->list[i].hook_addr,  phook_list->list[i].hook_flag?"enabled":"disabled");
            else
                IFX_PPACMD_PRINT("%02d: free item\n", i);
        }
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(phook_list), size );
    }

    free( phook_list );
    return PPA_CMD_OK;
}
/*ppacmd gethooklist: get all exported hook list  ---end*/

/*ppacmd sethook: set hook to enable/dsiable  ---begin*/
static void ppa_set_hook_help( int summary)
{
    IFX_PPACMD_PRINT("sethook <-n hookname > <-c enable | disable >\n");
    return;
}

static const char ppa_set_hook_short_opts[] = "n:c:h";
static const struct option ppa_set_hook_long_opts[] =
{
    {"hookname",   required_argument,  NULL, 'n'},
    {"control",   required_argument,  NULL, 'c'},
    { 0,0,0,0 }
};

static int ppa_parse_set_hook_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int hookname_opt = 0, control_opt = 0;

    memset( &pdata->hook_control_info, 0, sizeof(pdata->hook_control_info)) ;

    while(popts->opt)
    {
        if (popts->opt == 'n')
        {
            strncpy((char * )pdata->hook_control_info.hookname,  popts->optarg, sizeof(pdata->hook_control_info.hookname) );
            pdata->hook_control_info.hookname[sizeof(pdata->hook_control_info.hookname)-1] = 0;
            hookname_opt++;
        }
        else if (popts->opt == 'c')
        {
            if( stricmp( popts->optarg, "enable" ) == 0 )
                pdata->hook_control_info.enable = 1;
            control_opt++;
        }
        else
        {
            return PPA_CMD_ERR;
        }

        popts++;
    }

    if (hookname_opt != 1 || control_opt >=2)
        return PPA_CMD_ERR;

    IFX_PPACMD_DBG("sethook: try to %s  %s \n", pdata->hook_control_info.enable ? "enable":"disable", pdata->hook_control_info.hookname);
    return PPA_CMD_OK;
}
/*ppacmd sethook: set hook to enable/dsiable  ---end*/

/*ppacmd r ( getmem ) ---begin*/
static void ppa_get_mem_help( int summary)
{
    if( !summary )
        IFX_PPACMD_PRINT("r <-a address> [-n repeat] [-f offset] [-s size] [-o outfile]\n");
    else
    {
        IFX_PPACMD_PRINT("r <-a address> [-n repeat] [-f offset] [-s size] [-o outfile]\n");
        IFX_PPACMD_PRINT("    Note: offset + size must equal to 32\n");
        IFX_PPACMD_PRINT("    Note: address must align to 4 bytes address and repeation is based on 4 bytes address\n");
    }
    return;
}

static const char ppa_get_mem_short_opts[] = "a:f:s:p:n:o:h";
static const struct option ppa_get_mem_long_opts[] =
{
    {"address",   required_argument,  NULL, 'a'},
    {"offset",   required_argument,  NULL, 'f'},
    {"size",   required_argument,  NULL, 's'},
    {"repeat",   required_argument,  NULL, 'n'},
    {"save-to-file",   required_argument,  NULL, 'o'},
    { 0,0,0,0 }
};

static int ppa_parse_get_mem_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int a_opt=0, f_opt=0, s_opt=0, n_opt=0;

    memset( &pdata->read_mem_info, 0, sizeof(pdata->read_mem_info) );
    pdata->read_mem_info.repeat = 1; /*default read only */
    pdata->read_mem_info.size = 32;  /*default read 32 bits */
    pdata->read_mem_info.shift= 32 -pdata->read_mem_info.size ;

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'a':
            pdata->read_mem_info.addr = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            a_opt ++;
            break;
        case 'f':
            pdata->read_mem_info.shift = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            if( pdata->read_mem_info.size >= 32 - pdata->read_mem_info.shift )
                pdata->read_mem_info.size = 32 - pdata->read_mem_info.shift;
            f_opt ++;
            break;
        case 's':
            pdata->read_mem_info.size = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            if( pdata->read_mem_info.shift > 32 - pdata->read_mem_info.size )
                pdata->read_mem_info.shift= 32 - pdata->read_mem_info.size;
            s_opt ++;
            break;
        case 'n':
            pdata->read_mem_info.repeat = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            n_opt ++;
            break;
        case  'o':
            g_output_to_file = 1;
            strncpy(g_output_filename,popts->optarg,PPACMD_MAX_FILENAME);
            break;

        default:
            IFX_PPACMD_PRINT("r not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( a_opt != 1 || f_opt >= 2 || s_opt >= 2 || n_opt >= 2)
    {
        IFX_PPACMD_PRINT("Wrong parameter\n");
        return PPA_CMD_ERR;
    }
    if( pdata->read_mem_info.size == 0 )
    {
        pdata->read_mem_info.size = 32 - pdata->read_mem_info.shift;
    }
    if( pdata->read_mem_info.size > 32 )
    {
        IFX_PPACMD_PRINT("Wrong size value: %d ( it should be 1 ~ 32 )\n", (unsigned int)pdata->read_mem_info.size);
        return PPA_CMD_ERR;
    }
    if( pdata->read_mem_info.shift >= 32 )
    {
        IFX_PPACMD_PRINT("Wrong shift value: %d ( it should be 0 ~ 31 )\n", (unsigned int)pdata->read_mem_info.shift);
        return PPA_CMD_ERR;
    }

    if( pdata->read_mem_info.shift + pdata->read_mem_info.size > 32 )
    {
        IFX_PPACMD_PRINT("Wrong shift/size value: %d/%d (The sum should <= 32 ))\n", (unsigned int)pdata->read_mem_info.shift, (unsigned int)pdata->read_mem_info.size);
        return PPA_CMD_ERR;
    }

    IFX_PPACMD_DBG("r: address=0x%x, offset=%d, size=%d, repeat=%d\n", (unsigned int )pdata->read_mem_info.addr, (unsigned int )pdata->read_mem_info.shift, (unsigned int )pdata->read_mem_info.size, (unsigned int )pdata->read_mem_info.repeat  );

    return PPA_CMD_OK;
}

static int ppa_get_mem_cmd(PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata)
{
    PPA_CMD_READ_MEM_INFO *buffer;
    int i, size;
    int num_per_line=4;  //4 double words

    size = sizeof(pdata->read_mem_info) + sizeof(pdata->read_mem_info.buffer) * pdata->read_mem_info.repeat ;
    buffer = (PPA_CMD_READ_MEM_INFO *) malloc( size );
    if( buffer == NULL )
    {
        IFX_PPACMD_PRINT("Malloc %d bytes failed\n", size );
        return PPA_CMD_NOT_AVAIL;
    }
    memset( buffer, 0, sizeof(size) );

    memcpy( (void *)buffer, &pdata->read_mem_info, sizeof(pdata->read_mem_info) );
    buffer->flag = 0;

    //get session information
    if(  ppa_do_ioctl_cmd(PPA_CMD_READ_MEM, buffer ) != PPA_CMD_OK )
    {
        free( buffer );
        return IFX_FAILURE;
    }

    if( !g_output_to_file )
    {
        IFX_PPACMD_PRINT("The value from address 0x%x(0x%x):----------------------------", (unsigned int)buffer->addr, (unsigned int)buffer->addr_mapped);
        int tmp_filling_size = buffer->addr%16;
        if( tmp_filling_size != 0 )
        {
            IFX_PPACMD_PRINT("\n%08x: ", (unsigned int )buffer->addr/16 * 16 );
            for(i=0; i<tmp_filling_size/4; i++)
            {
                if( buffer->size <= 4 )
                {
                    IFX_PPACMD_PRINT( "  ");   // 2
                }
                else if( buffer->size <=8 )
                {
                    IFX_PPACMD_PRINT( "   " ); // 3
                }
                else if( buffer->size <=12 )
                {
                    IFX_PPACMD_PRINT( "    "); // 4
                }
                else if( buffer->size <=16 )
                {
                    IFX_PPACMD_PRINT( "     "); // 5
                }
                else if( buffer->size <=20 )
                {
                    IFX_PPACMD_PRINT( "      "); // 6
                }
                else if( buffer->size <=24 )
                {
                    IFX_PPACMD_PRINT( "       "); // 7
                }
                else if( buffer->size <=28 )
                {
                    IFX_PPACMD_PRINT( "         "); // 8
                }
                else
                {
                    IFX_PPACMD_PRINT( "         "); // 9
                }
            }
        }

        for(i=0;  i<buffer->repeat; i++ )
        {
            if( ( i+ tmp_filling_size/4) %num_per_line == 0 )
                IFX_PPACMD_PRINT("\n%08x: ", (unsigned int )buffer->addr + i * 4 );

            if( buffer->size <= 4 )
            {
                IFX_PPACMD_PRINT( "%01x ", (unsigned int )buffer->buffer[i]  );
            }
            else if( buffer->size <=8 )
            {
                IFX_PPACMD_PRINT( "%02x ", (unsigned int )buffer->buffer[i]  );
            }
            else if( buffer->size <=12 )
            {
                IFX_PPACMD_PRINT( "%03x ", (unsigned int )buffer->buffer[i]  );
            }
            else if( buffer->size <=16 )
            {
                IFX_PPACMD_PRINT( "%04x ", (unsigned int )buffer->buffer[i]  );
            }
            else if( buffer->size <=20 )
            {
                IFX_PPACMD_PRINT( "%05x ", (unsigned int )buffer->buffer[i]  );
            }
            else if( buffer->size <=24 )
            {
                IFX_PPACMD_PRINT( "%06x ", (unsigned int )buffer->buffer[i]  );
            }
            else if( buffer->size <=28 )
            {
                IFX_PPACMD_PRINT( "%07x ", (unsigned int )buffer->buffer[i] );
            }
            else
            {
                IFX_PPACMD_PRINT( "%08x ", (unsigned int )buffer->buffer[i]  );
            }
        }
        IFX_PPACMD_PRINT("\n" );
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->read_mem_info), sizeof(pdata->read_mem_info) );
    }

    free( buffer );
    return IFX_SUCCESS;
}
/*ppacmd r ( getmem ) ---end*/

/*ppacmd w ( setmem ) ---begin*/
static void ppa_set_mem_help( int summary)
{
    if( !summary )
        IFX_PPACMD_PRINT("w <-a address>  <-v value> [-n repeat] [-f offset] [-s size] \n");
    else
    {
        IFX_PPACMD_PRINT("w <-a address>  <-v value> [-n repeat] [-f offset] [-s size] \n");
        IFX_PPACMD_PRINT("    Note: offset + size must equal to 32\n");
        IFX_PPACMD_PRINT("    Note: address must align to 4 bytes address and repeation is based on 4 bytes address\n");
    }
    return;
}
static const char ppa_set_mem_short_opts[] = "a:f:s:v:n:p:h";
static const struct option ppa_set_mem_long_opts[] =
{
    {"address",   required_argument,  NULL, 'a'},
    {"offset",   required_argument,  NULL, 'f'},
    {"size",   required_argument,  NULL, 's'},
    {"repeat",   required_argument,  NULL, 'n'},
    {"value",   required_argument,  NULL, 'v'},
    { 0,0,0,0 }
};
static int ppa_parse_set_mem_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int a_opt=0, f_opt=0, s_opt=0, n_opt=0, v_opt=0;

    memset( &pdata->set_mem_info, 0, sizeof(pdata->set_mem_info) );
    pdata->set_mem_info.repeat = 1; /*default read only */
    pdata->set_mem_info.size = 32;  /*default read 32 bits */
    pdata->set_mem_info.shift= 0;
    pdata->set_mem_info.value= 0;

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'a':
            pdata->set_mem_info.addr = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            a_opt ++;
            break;
        case 'f':
            pdata->set_mem_info.shift = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            if( pdata->set_mem_info.size > 32 - pdata->set_mem_info.shift )
                pdata->set_mem_info.size = 32 - pdata->set_mem_info.shift;
            f_opt ++;
            break;
        case 's':
            pdata->set_mem_info.size = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            if( pdata->set_mem_info.shift > 32 - pdata->set_mem_info.size )
                pdata->set_mem_info.shift= 32 - pdata->set_mem_info.size;
            s_opt ++;
            break;
        case 'n':
            pdata->set_mem_info.repeat = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            n_opt ++;
            break;
        case  'v':
            pdata->set_mem_info.value = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            v_opt ++;
            break;
        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( a_opt != 1 || f_opt >= 2 || s_opt >= 2 || n_opt >= 2 || v_opt != 1)
    {
        IFX_PPACMD_PRINT("Wrong parameter\n");
        return PPA_CMD_ERR;
    }
    if( pdata->set_mem_info.size == 0 )
    {
        pdata->set_mem_info.size = 32 - pdata->set_mem_info.shift;
    }
    if(  pdata->set_mem_info.size > 32 )
    {
        IFX_PPACMD_PRINT("Wrong size value: %d ( it should be 1 ~ 32 )\n", (unsigned int)pdata->set_mem_info.size);
        return PPA_CMD_ERR;
    }
    if( pdata->set_mem_info.shift >= 32 )
    {
        IFX_PPACMD_PRINT("Wrong shift value: %d ( it should be 0 ~ 31 )\n", (unsigned int)pdata->set_mem_info.shift);
        return PPA_CMD_ERR;
    }

    if( pdata->set_mem_info.shift + pdata->set_mem_info.size > 32)
    {
        IFX_PPACMD_PRINT("Wrong shift/size value: %d/%d (The sum should <= 32 ))\n", (unsigned int)pdata->set_mem_info.shift, (unsigned int)pdata->set_mem_info.size);
        return PPA_CMD_ERR;
    }

    IFX_PPACMD_DBG("w: address=0x%x, offset=%d, size=%d, value=0x%x, repeat=%d\n", (unsigned int )pdata->set_mem_info.addr, (unsigned int )pdata->set_mem_info.shift, (unsigned int )pdata->set_mem_info.size, (unsigned int ) pdata->set_mem_info.value , (unsigned int )  pdata->set_mem_info.repeat );

    return PPA_CMD_OK;
}

static void ppa_print_set_mem_cmd( PPA_CMD_DATA *pdata)
{
    IFX_PPACMD_PRINT("Write memory from address 0x%x(0x%x) to 0x%x(0x%x) with value 0x%x\n",
                     (unsigned int)pdata->set_mem_info.addr,
                     (unsigned int)pdata->set_mem_info.addr_mapped,
                     (unsigned int)(pdata->set_mem_info.addr+pdata->set_mem_info.repeat),
                     (unsigned int)(pdata->set_mem_info.addr_mapped+pdata->set_mem_info.repeat),
                     (unsigned int)pdata->set_mem_info.value );

    return;
}

/*ppacmd w ( setmem ) ---end*/


/*ppacmd sethaldbg (  ) ---begin*/
static void ppa_set_hal_dbg_help( int summary)
{
    IFX_PPACMD_PRINT("sethaldbg -f < 1 | 0>\n");
    return;
}
static const char ppa_set_hal_dbg_short_opts[] = "f:h";
static const struct option ppa_set_hal_dbg_long_opts[] =
{
    {"flag",   required_argument,  NULL, 'f'},
    { 0,0,0,0 }
};
static int ppa_parse_set_hal_flag_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int f_opt=0;

    memset( &pdata->genernal_enable_info, 0, sizeof(pdata->genernal_enable_info) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'f':
            pdata->genernal_enable_info.enable = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            f_opt ++;
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( f_opt != 1 )
    {
        IFX_PPACMD_PRINT("Wrong parameter\n");
        return PPA_CMD_ERR;
    }

    IFX_PPACMD_DBG("sethaldbg: enable=0x%x\n",  (unsigned int )  pdata->genernal_enable_info.enable);

    return PPA_CMD_OK;
}

/*ppacmd sethaldbg ---end*/

#if defined(CONFIG_IFX_PPA_MFE) && CONFIG_IFX_PPA_MFE

/*ppacmd controlmf: enable multiple field  ---begin*/
static void ppa_set_mf_help( int summary)
{
    IFX_PPACMD_PRINT("setctrlmf <-c enable|disable > \n");
    return;
}
static const char ppa_set_mf_short_opts[] = "c:h";
static const struct option ppa_set_mf_long_opts[] =
{
    {"control",   required_argument,  NULL, 'c'},
    { 0,0,0,0 }
};
static int ppa_parse_set_mf_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int c_opt=0;

    memset( pdata, 0, sizeof(PPA_CMD_ENABLE_MULTIFIELD_INFO ) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'c':
            if( stricmp(popts->optarg, "enable") == 0 )
                pdata->mf_ctrl_info.enable_multifield = 1;
            c_opt ++;
            break;

        default:
            IFX_PPACMD_PRINT("controlmf not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( c_opt != 1)
    {
        IFX_PPACMD_PRINT("Wrong parameter\n");
        return PPA_CMD_ERR;
    }

    IFX_PPACMD_DBG("To %s multiple filed feature.\n", pdata->mf_ctrl_info.enable_multifield ? "enable":"disable");

    return PPA_CMD_OK;
}
/*ppacmd controlmf: enable multiple field  ---end*/

/*ppacmd getmf: get multiple field status: enabled/disabled ---begin*/
static void ppa_get_mf_status_help( int summary)
{
    IFX_PPACMD_PRINT("getctrlmf  [-o outfile]> \n");
    return;
}

static void ppa_print_get_mf_cmd(PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
    {
        IFX_PPACMD_PRINT("The multiple filed feature is %s\n", pdata->mf_ctrl_info.enable_multifield ? "enabled": "disabled");
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->mf_ctrl_info), sizeof(pdata->mf_ctrl_info) );
    }

}
/*ppacmd getmf: get multiple field status: enabled/disabled --end*/


/*ppacmd getmfnum: get max multiple field flow number: ---------------------------------begin*/
static void ppa_get_mf_count_help( int summary)
{
    IFX_PPACMD_PRINT("getmfnum [-a]  [-o outfile]>\n");
    if( summary )
        IFX_PPACMD_PRINT("    note: if -a is specified, it is to get the supported maximum flow entries,otherwise get the mf flow only\n");    
    return;
}

/*this is a simple template parse command. At most there is only one parameter for saving result to file */
static int ppa_parse_get_mf_count_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    memset( &pdata->count_info, 0, sizeof(pdata->count_info));
    pdata->count_info.flag = SESSION_ADDED_IN_HW;
    
    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'o':
            g_output_to_file = 1;
            strncpy(g_output_filename, popts->optarg, PPACMD_MAX_FILENAME);
            break;

        case 'a':
            pdata->count_info.flag =0;
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    return PPA_CMD_OK;
}

static void ppa_print_get_mf_count_cmd(PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
    {
        if( pdata->count_info.flag == SESSION_ADDED_IN_HW )
            IFX_PPACMD_PRINT("The multiple filed flow count is %u\n", (unsigned int) pdata->count_info.count);
        else
            IFX_PPACMD_PRINT("The maximum supported multiple filed flow count is %u\n", (unsigned int) pdata->count_info.count);
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->count_info), sizeof(pdata->count_info) );
    }

}
/*ppacmd getmfnum: get max multiple field flow number: ---------------------------------end*/

void ppa_mf_flow_display(PPA_MULTIFIELD_FLOW_INFO *flow_list, int32_t index, int32_t index_in_mfe)
{
    uint32_t full_id, mask;
    PPA_MULTIFIELD_FLOW_INFO *flow = NULL;

    flow = &flow_list[index];
    if( index_in_mfe >= 0 )
        IFX_PPACMD_PRINT("  Multiple field flow[%d] information: \n", index_in_mfe );
    else
        IFX_PPACMD_PRINT("  Multiple field flow information: \n");
    if( flow->cfg0.vlan_info.bfauto )
    {
        IFX_PPACMD_PRINT("  Acclerate packet From %s to %s via queue %d %s\n", flow->cfg0.vlan_info.vlan_info_auto.rx_ifname, flow->cfg0.vlan_info.vlan_info_auto.tx_ifname, flow->cfg0.queue_id, flow->cfg0.fwd_cpu ?"fwd to CPU":"as original Destination");
        IFX_PPACMD_PRINT("  Flow classification info:\n\n");
    }
    else
    {
        IFX_PPACMD_PRINT("  Acclerate packet From port id %d to %d via queue %d %s\n", flow->cfg0.vlan_info.vlan_info_manual.rx_if_id, flow->cfg0.vlan_info.vlan_info_manual.tx_if_id, flow->cfg0.queue_id, flow->cfg0.fwd_cpu ?"fwd to CPU":"as original Destination");
        if( flow->cfg0.vlan_info.vlan_info_manual.action_out_vlan_insert )
        {
            VLAN_ID_CONBINE(full_id, flow->cfg0.vlan_info.vlan_info_manual.new_out_vlan_pri, flow->cfg0.vlan_info.vlan_info_manual.new_out_vlan_cfi, flow->cfg0.vlan_info.vlan_info_manual.new_out_vlan_vid );
            full_id |= (flow->cfg0.vlan_info.vlan_info_manual.new_out_vlan_tpid << 16 );
            IFX_PPACMD_PRINT("  New out vlan id 0x%04x\n", (unsigned int)full_id );
        }
        if( flow->cfg0.vlan_info.vlan_info_manual.action_in_vlan_insert )
        {
            VLAN_ID_CONBINE(full_id, flow->cfg0.vlan_info.vlan_info_manual.new_in_vlan_pri, flow->cfg0.vlan_info.vlan_info_manual.new_in_vlan_cfi, flow->cfg0.vlan_info.vlan_info_manual.new_in_vlan_vid );
            IFX_PPACMD_PRINT("  New in vlan id 0x%04x\n", (unsigned int)full_id );
        }
        if( flow->cfg0.vlan_info.vlan_info_manual.action_out_vlan_remove ||flow->cfg0.vlan_info.vlan_info_manual.action_in_vlan_remove)
            IFX_PPACMD_PRINT("  Remove: %s %s \n", flow->cfg0.vlan_info.vlan_info_manual.action_out_vlan_remove?"outer vlan":"", flow->cfg0.vlan_info.vlan_info_manual.action_in_vlan_remove?"inner vlan":"");

        IFX_PPACMD_PRINT("  Flow classification info:\n\n");
        IFX_PPACMD_PRINT("  vlan number: 0x%01x/0x%01x\n", flow->cfg0.vlan_info.vlan_info_manual.is_vlan, flow->cfg0.vlan_info.vlan_info_manual.is_vlan_mask);
        VLAN_ID_CONBINE(full_id, flow->cfg0.vlan_info.vlan_info_manual.out_vlan_pri, flow->cfg0.vlan_info.vlan_info_manual.out_vlan_cfi, flow->cfg0.vlan_info.vlan_info_manual.out_vlan_vid );
        VLAN_ID_CONBINE(mask,   flow->cfg0.vlan_info.vlan_info_manual.out_vlan_pri_mask, flow->cfg0.vlan_info.vlan_info_manual.out_vlan_cfi_mask, flow->cfg0.vlan_info.vlan_info_manual.out_vlan_vid_mask);
        IFX_PPACMD_PRINT("  match out vlan id 0x%04x/0x%04x\n", (unsigned int)full_id, (unsigned int)mask );

        VLAN_ID_CONBINE(full_id, flow->cfg0.vlan_info.vlan_info_manual.in_vlan_pri, flow->cfg0.vlan_info.vlan_info_manual.in_vlan_cfi, flow->cfg0.vlan_info.vlan_info_manual.in_vlan_vid );
        VLAN_ID_CONBINE(mask,   flow->cfg0.vlan_info.vlan_info_manual.in_vlan_pri_mask, flow->cfg0.vlan_info.vlan_info_manual.in_vlan_cfi_mask, flow->cfg0.vlan_info.vlan_info_manual.in_vlan_vid_mask);
        IFX_PPACMD_PRINT("  match in vlan id 0x%04x/0x%04x\n", (unsigned int)full_id, (unsigned int)mask );
    }

    IFX_PPACMD_PRINT("  DSCP: 0x%x/0x%x\n", flow->cfg0.dscp, flow->cfg0.dscp_mask );
    IFX_PPACMD_PRINT("  Ether Type: 0x%x/0x%x\n", flow->cfg0.ether_type, flow->cfg0.ether_type_mask);
    IFX_PPACMD_PRINT("  IPV4 flag: 0x%x/0x%x\n", flow->cfg0.ipv4, flow->cfg0.ipv4_mask);
    IFX_PPACMD_PRINT("  IPV6 flag: 0x%x/0x%x\n", flow->cfg0.ipv6, flow->cfg0.ipv6_mask);
    IFX_PPACMD_PRINT("  L3 off0: 0x%x/0x%x\n", flow->cfg0.l3_off0, flow->cfg0.l3_off0_mask);
    IFX_PPACMD_PRINT("  L3 off1: 0x%x/0x%x\n", flow->cfg0.l3_off1, flow->cfg0.l3_off1_mask);
    IFX_PPACMD_PRINT("  Source ip: 0x%x(%u.%u.%u.%u)/0x%x\n", (unsigned int)flow->cfg0.s_ip, NIPQUAD(flow->cfg0.s_ip),(unsigned int)flow->cfg0.s_ip_mask);
    IFX_PPACMD_PRINT("  Packet length: 0x%x/0x%x\n", (unsigned int)flow->cfg0.pkt_length, (unsigned int)flow->cfg0.pkt_length_mask);
    IFX_PPACMD_PRINT("  pppoe flag: 0x%x/0x%x\n", (unsigned int)flow->cfg0.pppoe_session, (unsigned int)flow->cfg0.pppoe_session_mask);
    IFX_PPACMD_PRINT("\n");
}

/*ppacmd addmfflow: add a multiple field flow   --------------------------------------------------begin*/
static void ppa_mf_flow_help( char *cmd, int summary)
{
    IFX_PPACMD_PRINT("%s -a <-r rx-ifname > <-t tx-ifname> [-q queue-id] [-z fwd_to_cpu]--- ( Auto-Learning)\n", cmd);
    IFX_PPACMD_PRINT("%s  <-r rx-ifname | rx-port-id > <-t tx-ifname | tx-port-id > [-q queue-id] [-z fwd_to_cpu] ... --- ( Manually specify 16 keys ) \n", cmd);
    IFX_PPACMD_PRINT("         [-n vlan-number(2 bits)/mask] [-o match-out-vlan(32 bits)/mask] [-i match-in-vlan(16 bits)/mask]\n");
    IFX_PPACMD_PRINT("         [-c insert:<new_out_vlan_id(32bits)>|remove ] [ -m insert <new_inner_vlan(16 bits)>|remove ] \n");
    IFX_PPACMD_PRINT("         [-e ether-type(16bits)/mask] [-p pppoe-session(1bit)/mask] [-d dscp(tos)/maks] [-v  ipversion (4|6)] \n");
    IFX_PPACMD_PRINT("         [-f source-ip(32 bits)/mask] [-u l3_off0(8 bits)/mask] [-w l3_off1(8 bits)/mask ] [-k packet_leng(8 bits)/mask]  \n");

    if( summary )
    {
        IFX_PPACMD_PRINT("     Note\n");
        IFX_PPACMD_PRINT("       1. Use either -r/t or -s/d\n");
        IFX_PPACMD_PRINT("       2. For mask, 0 mean to compare this corresponding bit, 1 mean don't compare the bit. \n");
        IFX_PPACMD_PRINT("                           If mask is not provided, it will be set to 0 instead\n");
        IFX_PPACMD_PRINT("       3. Vlan-number: 2 bits \n");
        IFX_PPACMD_PRINT("       4. Packet length: less than and equal compare after L2 packet length divided 64 \n");
        IFX_PPACMD_PRINT("       5. src-ip/mask:  src-ip can use string format ( like 192.1.2.3)  or hex value ( like 0xa0010203\n");
        IFX_PPACMD_PRINT("       6. new_out_vlan_id: 4 bytes ( 2 bytes TPID + 2 bytes TCI ), like 0x81000002 \n");
        IFX_PPACMD_PRINT("       7. new_inner_vlan: 2 bytes TCI, like 0x0002 \n");
        IFX_PPACMD_PRINT("       8. TCI format: priority (3 bits), cfi (1 bit) and vid (12 bits), over all 16 bits \n");
        
    }

    return;
}

static void ppa_add_mf_flow_help( int summary)
{
    ppa_mf_flow_help("addmf", summary);
    return;
}


static const char ppa_add_mf_flow_short_opts[] = "ar:t:n:o:i:c:m:x:g:e:p:j:v:f:u:w:z:k:q:h";
static const struct option ppa_add_mf_flow_long_opts[] =
{
    /*simple auto way to add multiple field flow, like eth1.50 bridge to eth0 */
    {"auto-vlan",   no_argument,  NULL, 'a'},
    {"rx-ifname",   required_argument,  NULL, 'r'}, //rx
    {"tx-ifname",   required_argument,  NULL, 't'},  //tx

    /*manual way to provide detail information */
    {"vlan-num",   required_argument,  NULL, 'n'},   // ??? why need it, difficult for transparent bridging

    {"match-out-vlan",   required_argument,  NULL, 'o'},  //vlanid/pri/cfi and its mask
    {"match-in-vlan",   required_argument,  NULL, 'i'},    //vlanid and its mask

    {"out-vlan-control",   required_argument,  NULL, 'c'},  // out vlan action: remove/insert
    {"in-vlan-control",   required_argument,  NULL, 'm'},   // inner vlan action: remove/insert

    /*all below parameter shared together by simple auto way and manual way */
    {"ether-type",   required_argument,  NULL, 'e'},   //include ethernet type and its mask
    {"pppoe-session", required_argument,  NULL, 'p'},
    {"dscp",   required_argument,  NULL, 'd'},   //dscp and(tos)/its mask
    {"ipver",   required_argument,  NULL, 'v'},   //match ipv4 or ipv6
    {"src-ip",   required_argument,  NULL, 'f'},   //include source ip and its mask
    {"l3_off0",   required_argument,  NULL, 'u'},   //include l3_off0 and its mask
    {"l3_off1",   required_argument,  NULL, 'w'},  //include l3_off1 and its mask
    {"fwd_cpu",   required_argument,  NULL, 'z'},   // forward to CPU or keep original destionation
    {"pkt_len",   required_argument,  NULL, 'k'},   // packet length
    {"queuid",   required_argument,  NULL, 'q'},   //queue id
    {"help",   required_argument,  NULL, 'h'},   //queue id
    { 0,0,0,0 }
};

static void ppa_get_value_mask(char *s, char **value, char**mask)
{
    char *splitter="/";

    *value = NULL;
    *mask = NULL;

    *value=strtok(s, splitter);
    if( *value )
    {
        *mask = strtok( NULL,splitter);
    }
}


static int ppa_parse_add_mf_flow_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int  r_opt=0, s_opt=0, n_opt=0, o_opt=0, i_opt=0, c_opt=0, m_opt=0, g_opt=0, e_opt=0, p_opt=0, t_opt=0, r4_opt=0, r6_opt=0, x_opt=0, f_opt=0, u_opt=0, w_opt=0,z_opt=0, q_opt=0, v_opt=0, d_opt=0;
    char *value;
    char *mask;
    uint32_t temp;

    memset( pdata, 0, sizeof(pdata->mf_flow_info ) );
    pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.is_vlan_mask = -1;
    pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.out_vlan_pri_mask= -1;
    pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.out_vlan_cfi_mask = -1;
    pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.out_vlan_vid_mask = -1;
    pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.in_vlan_pri_mask= -1;
    pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.in_vlan_cfi_mask = -1;
    pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.in_vlan_vid_mask = -1;
    pdata->mf_flow_info.flow.cfg0.ether_type_mask = -1;
    pdata->mf_flow_info.flow.cfg0.pppoe_session_mask = -1;
    pdata->mf_flow_info.flow.cfg0.dscp_mask = -1;
    pdata->mf_flow_info.flow.cfg0.ipv4_mask = -1;
    pdata->mf_flow_info.flow.cfg0.ipv6_mask = -1;
    pdata->mf_flow_info.flow.cfg0.s_ip_mask = -1;
    pdata->mf_flow_info.flow.cfg0.l3_off0_mask = -1;
    pdata->mf_flow_info.flow.cfg0.l3_off1_mask = -1;
    pdata->mf_flow_info.flow.cfg0.pkt_length_mask = -1;

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'a': //auto VLAN
            pdata->mf_flow_info.flow.cfg0.vlan_info.bfauto= 1;
            break;

        case 'r':  //rx interface name or port id
            if( is_digital_value( popts->optarg) ) 
                pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.rx_if_id = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            else 
            {
                strncpy( pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_auto.rx_ifname, popts->optarg, sizeof(pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_auto.rx_ifname) );
            }
            r_opt ++;
            break;

        case 't': //tx interface name or port id
            if( is_digital_value( popts->optarg) ) pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.tx_if_id = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            else 
            {
                strncpy( pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_auto.tx_ifname, popts->optarg, sizeof(pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_auto.rx_ifname) );
            }
            t_opt ++;
            break;       

        case 'n': //vlan tag number
            ppa_get_value_mask( popts->optarg, &value, &mask);
            if( value == NULL )
            {
                IFX_PPACMD_PRINT("Wrong vlan tag num/mask\n");
                return IFX_FAILURE;
            }
            pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.is_vlan = str_convert(STRING_TYPE_INTEGER, value, NULL);
            if( mask  == NULL )
                pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.is_vlan_mask = 0;
            else
                pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.is_vlan_mask = str_convert(STRING_TYPE_INTEGER, mask, NULL);
            n_opt ++;
            break;

        case 'o': //match out vlan ( prio 3 bits, cfi 1 bits and 12 bits vlan id )/and mask
            ppa_get_value_mask( popts->optarg, &value, &mask);
            if( value == NULL )
            {
                IFX_PPACMD_PRINT("Wrong out vlan tag/mask\n");
                return IFX_FAILURE;
            }
            temp = str_convert(STRING_TYPE_INTEGER, value, NULL);
            VLAN_ID_SPLIT(temp, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.out_vlan_pri, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.out_vlan_cfi, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.out_vlan_vid);
            if( mask  == NULL )
            {
                pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.out_vlan_pri_mask= 0;
                pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.out_vlan_cfi_mask = 0;
                pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.out_vlan_vid_mask = 0;
            }
            else
            {
                temp = str_convert(STRING_TYPE_INTEGER, mask, NULL);
                VLAN_ID_SPLIT(temp, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.out_vlan_pri_mask, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.out_vlan_cfi_mask, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.out_vlan_vid_mask );
            }
            o_opt ++;
            break;

        case 'i': //match inner vlan ( prio 3 bits, cfi 1 bits and 12 bits vlan id )/and mask
            ppa_get_value_mask( popts->optarg, &value, &mask);
            if( value == NULL )
            {
                IFX_PPACMD_PRINT("Wrong out vlan tag/mask\n");
                return IFX_FAILURE;
            }
            temp = str_convert(STRING_TYPE_INTEGER, value, NULL);
            VLAN_ID_SPLIT(temp, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.in_vlan_pri , pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.in_vlan_cfi, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.in_vlan_vid );
            if( mask  == NULL )
            {
                pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.in_vlan_pri_mask= 0;
                pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.in_vlan_cfi_mask = 0;
                pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.in_vlan_vid_mask = 0;
            }
            else
            {
                temp = str_convert(STRING_TYPE_INTEGER, mask, NULL);
                VLAN_ID_SPLIT(temp, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.in_vlan_pri_mask, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.in_vlan_cfi_mask, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.in_vlan_vid_mask  );
            }
            i_opt ++;
            break;

        case 'c': //out vlan action: remove or insert
            if( strincmp(popts->optarg, "insert", strlen("insert")) == 0 )
            {
                pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.action_out_vlan_insert = 1;
                temp = str_convert(STRING_TYPE_INTEGER,  &popts->optarg[strlen("insert:")], NULL);
                VLAN_ID_SPLIT( temp, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.new_out_vlan_pri, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.new_out_vlan_cfi, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.new_out_vlan_vid );
                pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.new_out_vlan_tpid = temp >> 16;
            }
            else if( stricmp(popts->optarg, "remove") == 0 )
                pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.action_out_vlan_remove= 1;
            else
            {
                IFX_PPACMD_PRINT("Wrong out vlan control. It should be insert or remove\n");
                return IFX_FAILURE;
            }
            c_opt ++;
            break;

        case 'm': //inner vlan action: remove or insert
            if( strincmp(popts->optarg, "insert", strlen("insert")) == 0 )
            {
                pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.action_in_vlan_insert = 1;
                temp = str_convert(STRING_TYPE_INTEGER,  &popts->optarg[strlen("insert:")], NULL);
                VLAN_ID_SPLIT( temp, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.new_in_vlan_pri, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.new_in_vlan_cfi, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.new_in_vlan_vid );                
            }
            else if( stricmp(popts->optarg, "remove") == 0 )
                pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.action_in_vlan_remove= 1;
            else
            {
                IFX_PPACMD_PRINT("Wrong inner vlan control. It should be insert or remove\n");
                return IFX_FAILURE;
            }
            m_opt ++;
            break;

           case 'e': //ethernet type and its mask
            ppa_get_value_mask( popts->optarg, &value, &mask);
            if( value == NULL )
            {
                IFX_PPACMD_PRINT("Wrong Ethernet type\n");
                return IFX_FAILURE;
            }
            pdata->mf_flow_info.flow.cfg0.ether_type = str_convert(STRING_TYPE_INTEGER, value, NULL);
            if( mask  == NULL )
            {
                pdata->mf_flow_info.flow.cfg0.ether_type_mask = 0;
            }
            else
            {
                pdata->mf_flow_info.flow.cfg0.ether_type_mask = str_convert(STRING_TYPE_INTEGER, mask, NULL);
            }
            e_opt ++;
            break;

        case 'p': //pppoe-session
            ppa_get_value_mask( popts->optarg, &value, &mask);
            if( value == NULL )
            {
                IFX_PPACMD_PRINT("Wrong pppoe-session flag type\n");
                return IFX_FAILURE;
            }
            pdata->mf_flow_info.flow.cfg0.pppoe_session = str_convert(STRING_TYPE_INTEGER, value, NULL);
            if( mask  == NULL )
            {
                pdata->mf_flow_info.flow.cfg0.pppoe_session_mask = 0;
            }
            else
            {
                pdata->mf_flow_info.flow.cfg0.pppoe_session_mask = str_convert(STRING_TYPE_INTEGER, mask, NULL);
            }
            p_opt ++;
            break;

        case 'd': //dscp(tos) and its mask ( 8 bits)
            ppa_get_value_mask( popts->optarg, &value, &mask);
            if( value == NULL )
            {
                IFX_PPACMD_PRINT("Wrong DSCP/TOS/mask\n");
                return IFX_FAILURE;
            }
            pdata->mf_flow_info.flow.cfg0.dscp = str_convert(STRING_TYPE_INTEGER, value, NULL);
            if( mask  == NULL )
            {
                pdata->mf_flow_info.flow.cfg0.dscp_mask = 0;
            }
            else
            {
                pdata->mf_flow_info.flow.cfg0.dscp_mask = str_convert(STRING_TYPE_INTEGER, mask, NULL);
            }
            d_opt ++;
            break;

        case 'v': //ip vversion: 4 or 6
            if( strcmp( popts->optarg, "4") == 0 )
            {
                r4_opt ++;
                pdata->mf_flow_info.flow.cfg0.ipv4 = 1;
                pdata->mf_flow_info.flow.cfg0.ipv4_mask = 0;
            }
            else if( strcmp( popts->optarg, "6") == 0 )
            {
                r6_opt ++;
                pdata->mf_flow_info.flow.cfg0.ipv6 = 1;
                pdata->mf_flow_info.flow.cfg0.ipv6_mask = 0;
            }
            v_opt ++;
            break;

        case 'f': //source ip and its mask ( 32 bits )---for ipv4 only
            ppa_get_value_mask( popts->optarg, &value, &mask);
            if( value == NULL )
            {
                IFX_PPACMD_PRINT("Wrong soruce ip/mask\n");
                return IFX_FAILURE;
            }
            if( is_digital_value( value) ) 
                pdata->mf_flow_info.flow.cfg0.s_ip = str_convert(STRING_TYPE_INTEGER, value, NULL);
            else 
            {
                PPA_IPADDR ip;
                uint32_t ip_type;

                if( (ip_type = str_convert(STRING_TYPE_IP, value, (void *)&ip)) != IP_VALID_V4)
                {
                    IFX_PPACMD_PRINT("Wrong source ip:%s\n", popts->optarg);
                    return PPA_CMD_ERR;
                }
                pdata->mf_flow_info.flow.cfg0.s_ip = ip.ip;
            }
            if( mask  == NULL )
            {
                pdata->mf_flow_info.flow.cfg0.s_ip_mask = 0;
            }
            else
            {
                pdata->mf_flow_info.flow.cfg0.s_ip_mask = str_convert(STRING_TYPE_INTEGER, mask, NULL);
            }
            f_opt ++;
            break;

        case 'u': //l3_off0 and its mask ( 32bits)
            ppa_get_value_mask( popts->optarg, &value, &mask);
            if( value == NULL )
            {
                IFX_PPACMD_PRINT("Wrong l3_off0/mask\n");
                return IFX_FAILURE;
            }
            pdata->mf_flow_info.flow.cfg0.l3_off0 = str_convert(STRING_TYPE_INTEGER, value, NULL);
            if( mask  == NULL )
            {
                pdata->mf_flow_info.flow.cfg0.l3_off0_mask = 0;
            }
            else
            {
                pdata->mf_flow_info.flow.cfg0.l3_off0_mask = str_convert(STRING_TYPE_INTEGER, mask, NULL);
            }
            u_opt ++;
            break;

        case 'w': //l3_off1 and tis mask ( 32bits)
            ppa_get_value_mask( popts->optarg, &value, &mask);
            if( value == NULL )
            {
                IFX_PPACMD_PRINT("Wrong l3_off1/mask\n");
                return IFX_FAILURE;
            }
            pdata->mf_flow_info.flow.cfg0.l3_off1 = str_convert(STRING_TYPE_INTEGER, value, NULL);
            if( mask  == NULL )
            {
                pdata->mf_flow_info.flow.cfg0.l3_off1_mask = 0;
            }
            else
            {
                pdata->mf_flow_info.flow.cfg0.l3_off1_mask = str_convert(STRING_TYPE_INTEGER, mask, NULL);
            }
            w_opt ++;
            break;

        case 'z': // whether keep original destionation or forward to CPU only
            temp = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            if( temp )
                pdata->mf_flow_info.flow.cfg0.fwd_cpu =1;
            else
                pdata->mf_flow_info.flow.cfg0.fwd_cpu =0;
            z_opt ++;
            break;

        case 'k': //packet length
            ppa_get_value_mask( popts->optarg, &value, &mask);
            if( value == NULL )
            {
                IFX_PPACMD_PRINT("Wrong packet length\n");
                return IFX_FAILURE;
            }
            pdata->mf_flow_info.flow.cfg0.pkt_length= str_convert(STRING_TYPE_INTEGER, value, NULL);
            if( mask  == NULL )
            {
                pdata->mf_flow_info.flow.cfg0.pkt_length_mask = 0;
            }
            else
            {
                pdata->mf_flow_info.flow.cfg0.pkt_length_mask = str_convert(STRING_TYPE_INTEGER, mask, NULL);
            }
            w_opt ++;
            break;

        case 'q': //destionantion queue id
            pdata->mf_flow_info.flow.cfg0.queue_id = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            q_opt ++;
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( r_opt != 1 || t_opt != 1)
    {
        IFX_PPACMD_PRINT("Wrong parameter: rx/tx name or rx/tx port ID is a must !\n");
        return PPA_CMD_ERR;
    }

    if( pdata->mf_flow_info.flow.cfg0.vlan_info.bfauto )
    {
        if( strcmp(pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_auto.rx_ifname, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_auto.tx_ifname )== 0 )
        {
            IFX_PPACMD_PRINT("tx/rx cannot be same iinterface:%s\n", pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_auto.rx_ifname );
            return PPA_CMD_ERR;
        }
    }
    else
    {
        if( (pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.rx_if_id == pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_manual.tx_if_id ) &&
            strcmp(pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_auto.rx_ifname, pdata->mf_flow_info.flow.cfg0.vlan_info.vlan_info_auto.tx_ifname ) == 0 )
        {
            IFX_PPACMD_PRINT("tx/rx cannot be same\n");
            return PPA_CMD_ERR;
        }
    }
    
    if( enable_debug) ppa_mf_flow_display( &pdata->mf_flow_info.flow, 0, pdata->mf_flow_info.index );
    return PPA_CMD_OK;
}

static void ppa_print_add_mf_flow_cmd( PPA_CMD_DATA *pdata)
{
    IFX_PPACMD_DBG("Added multiple field flow to index %d\n", pdata->mf_flow_info.index);
}
/*ppacmd addmfflow: enable multiple field  ---end*/

/*ppacmd gettmfflow: get all multiple field flow information ------------------------------------------begin*/
static void ppa_get_mf_flow_help( int summary)
{
    IFX_PPACMD_PRINT("getmf [-s start_flow_index] [-e end_flow_index]  [-o outfile]> \n");
    return;
}

static const char ppa_get_mf_flow_short_opts[] = "o:s:e:h";
static const struct option ppa_get_mf_flow_long_opts[] =
{
    {"save-to-file",   required_argument,  NULL, 'o'},
    {"start_index",   required_argument,  NULL, 's'},
    {"end_index",   required_argument,  NULL, 'e'},
    { 0,0,0,0 }
};
static int ppa_parse_get_mf_flow_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    pdata->mf_flow_info.index = -1;
    pdata->mf_flow_info.last_index = -1;
    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'o':
            g_output_to_file = 1;
            strncpy(g_output_filename, popts->optarg, PPACMD_MAX_FILENAME);
            break;
        case 's':
            pdata->mf_flow_info.index = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            break;
        case 'e':
            pdata->mf_flow_info.last_index = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            break;
            
        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    return PPA_CMD_OK;
}

static int ppa_get_mf_flow_cmd(PPA_COMMAND *pcmd, PPA_CMD_DATA *pdata)
{
    PPA_CMD_COUNT_INFO count_info;
    PPA_CMD_MULTIFIELD_FLOW_INFO flow_info;
    int i, j, start_i, end_i=0;
    int res;

    memset( &count_info, 0, sizeof(count_info) );
    if( (res = ppa_do_ioctl_cmd(PPA_CMD_GET_MULTIFIELD_ENTRY_MAX, &count_info ) != PPA_CMD_OK ) )
    {
        IFX_PPACMD_PRINT("ioctl PPA_CMD_GET_MULTIFIELD_ENTRY_MAX fail\n");
        return IFX_FAILURE;
    }
    if( count_info.count == 0 )
    {
        IFX_PPACMD_PRINT("Maximum MF flow is zero \n");
        return IFX_FAILURE;
    }
    
    if( pdata->mf_flow_info.last_index == -1 && pdata->mf_flow_info.index == -1) //both not set yet, ie, get all sessions
    {
        start_i = 0;
        end_i = count_info.count;
    }
    else if( pdata->mf_flow_info.last_index == -1) //only last_index not set, ie, get the index session only
    {
        start_i = pdata->mf_flow_info.index;
        end_i = start_i+1;        
    }
    else if( pdata->mf_flow_info.index == -1) //only last_index not set, ie, get the last_index session only
    {
        start_i = pdata->mf_flow_info.last_index;
        end_i = start_i+1;        
    }
    else if( pdata->mf_flow_info.index > pdata->mf_flow_info.last_index )
    {
        start_i = pdata->mf_flow_info.last_index;
        end_i = start_i;        
    }
    else
    {
        start_i = pdata->mf_flow_info.index;
        end_i = pdata->mf_flow_info.last_index;   
        if( end_i <= start_i ) end_i = start_i + 1;
    }
    
    IFX_PPACMD_DBG("Need to read multiple field flow information from %d to %d\n", start_i, end_i-1 );

    for(i=start_i; i<end_i; i++)
    {
        memset( &flow_info, 0, sizeof(flow_info) );
        flow_info.index = i;

        if( ( (res = ppa_do_ioctl_cmd(PPA_CMD_GET_MULTIFIELD, &flow_info ) ) == PPA_CMD_OK ) && ( flow_info.flag == IFX_SUCCESS ) )
        {
            if( !g_output_to_file  )
            {
                ppa_mf_flow_display( &flow_info.flow, 0, i);                
            }
            else
            {
                /*note, currently only last valid flow info is saved to file and all other flow informations are all overwritten */
                SaveDataToFile(g_output_filename, (char *)(&flow_info), sizeof(flow_info) );
            }
            j++;

        }
    }
    return IFX_SUCCESS;
}
/*ppacmd getmfflow: enable multiple field  ---end*/

/*ppacmd delmfflow: delete one multiple field flow information------------------------------------------begin*/
static void ppa_del_mf_flow_help( int summary)
{
    ppa_mf_flow_help("delmf", summary);
    return;
}
/*ppacmd delmfflowindex: delete all multiple field flow information via index------------------------------------------begin*/

/*ppacmd delmfflowindex: delete all multiple field flow information via index------------------------------------------begin*/
static void ppa_del_mf_flow_index_help( int summary)
{
    IFX_PPACMD_PRINT("delmf2  <-i index> \n");
    if( summary )
        IFX_PPACMD_PRINT("    Delete mf flow via index. if index is -1, it means to delete all mf flows\n");
    return;
}

static const char ppa_del_mf_flow_index_short_opts[] = "i:h";
static const struct option ppa_del_mf_flow_index_long_opts[] =
{
    {"index",   required_argument,  NULL, 'i'},
    { 0,0,0,0 }
};
static int ppa_parse_del_mf_flow_index_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int i_opt=0;

    memset( pdata, 0, sizeof(PPA_CMD_ENABLE_MULTIFIELD_INFO ) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'i':
            pdata->mf_flow_info.index= str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            i_opt ++;
            break;

        default:
            IFX_PPACMD_PRINT("delmf not support parameter -%c \n", popts->opt);
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( i_opt != 1)
    {
        IFX_PPACMD_PRINT("Wrong parameter\n");
        return PPA_CMD_ERR;
    }

    IFX_PPACMD_DBG("Try to delete multiple field flow entry %u\n", (unsigned int)pdata->mf_flow_info.index );

    return PPA_CMD_OK;
}

/*ppacmd delmfflowindex: enable multiple field  ---end*/
#endif

/*ppacmd ppa_get_portid ---begin */
static const char ppa_get_portid_short_opts[] = "i:h";
static const struct option ppa_get_portid_long_opts[] =
{
    {"ifname",   required_argument,  NULL, 'i'},
    { 0,0,0,0 }
};
static void ppa_get_portid_help( int summary)
{
    IFX_PPACMD_PRINT("getportid  <-i ifname>\n");
    return;
}

static int ppa_parse_get_portid_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int i_opt = 0;

    memset( pdata, 0, sizeof(pdata->portid_info ) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'i':
            strncpy(pdata->portid_info.ifname, popts->optarg, sizeof(pdata->portid_info.ifname) );
            i_opt ++;
            break;

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( i_opt != 1)
    {
        IFX_PPACMD_PRINT("Wrong parameter.\n");
        return PPA_CMD_ERR;
    }
    return PPA_CMD_OK;
}

static void ppa_print_get_portid_cmd( PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
    {
        IFX_PPACMD_PRINT("The %s's port id is %d\n", pdata->portid_info.ifname, (unsigned int)pdata->portid_info.portid);
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->portid_info), sizeof(pdata->portid_info) );
    }

    return;
}
/*ppacmd ppa_get_portid ---end */


/*ppacmd ppa_get_dsl_mib ---begin */
static void ppa_get_dsl_mib_help( int summary)
{
    IFX_PPACMD_PRINT("getdslmib\n");
    return;
}

static void ppa_print_get_dsl_mib_cmd( PPA_CMD_DATA *pdata)
{
    int i;

    if( !g_output_to_file )
    {
        IFX_PPACMD_PRINT("The DSL MIB info\n");
        IFX_PPACMD_PRINT("  DSL Queue DROP MIB\n");
        for(i=0; i<pdata->dsl_mib_info.mib.queue_num; i++ )
        {
            IFX_PPACMD_PRINT("    queue%02d: %10u", i, (unsigned int)pdata->dsl_mib_info.mib.drop_mib[i]);
            if( (i%4) == 3 ) IFX_PPACMD_PRINT("\n");
        }
        IFX_PPACMD_PRINT("\n");
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->dsl_mib_info), sizeof(pdata->dsl_mib_info) );
    }

    return;
}
/****ppa_get_dsl_mib ----end */

/*ppacmd ppa_clear_dsl_mib ---begin */
static void ppa_clear_dsl_mib_help( int summary)
{
    IFX_PPACMD_PRINT("cleardslmib\n");
    return;
}
/****ppa_clear_dsl_mib ----end */

/*ppacmd ppa_port_mib ---begin */
static void ppa_get_port_mib_help( int summary)
{
    IFX_PPACMD_PRINT("getportmib\n");
    return;
}

void print_port_mib(PPA_PORT_MIB *mib)
{
#define PORT_MIB_TITLE_SAPCE   32
#define PORT_MIB_NUMBER_SAPCE   10
    int i;

    if( !mib) return;

    IFX_PPACMD_DBG("print_port_mib:%d\n", (int)mib->port_num);

    //print format     space    eth0  eth1   dsl    ext1  ext2  ext3
    IFX_PPACMD_PRINT("%28s %10s %10s %10s %10s %10s %10s\n", "MIB:", "ETH0", "ETH1", "DSL", "EXT1", "EXT2","EXT3");

    IFX_PPACMD_PRINT("%28s ", "ig_fast_brg_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_PRINT("%10u ", (unsigned int) mib->mib_info[i].ig_fast_brg_pkts);
    }
    IFX_PPACMD_PRINT("\n");

    IFX_PPACMD_PRINT("%28s ", "ig_fast_brg_bytes:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_PRINT("%10u ", (unsigned int) mib->mib_info[i].ig_fast_brg_bytes);
    }
    IFX_PPACMD_PRINT("\n");

    IFX_PPACMD_PRINT("\n");
    IFX_PPACMD_PRINT("%28s ", "ig_fast_rt_ipv4_udp_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_PRINT("%10u ", (unsigned int) mib->mib_info[i].ig_fast_rt_ipv4_udp_pkts);
    }
    IFX_PPACMD_PRINT("\n");

    IFX_PPACMD_PRINT("%28s ", "ig_fast_rt_ipv4_tcp_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_PRINT("%10u ", (unsigned int) mib->mib_info[i].ig_fast_rt_ipv4_tcp_pkts);
    }
    IFX_PPACMD_PRINT("\n");

    IFX_PPACMD_PRINT("%28s ", "ig_fast_rt_ipv4_mc_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_PRINT("%10u ", (unsigned int) mib->mib_info[i].ig_fast_rt_ipv4_mc_pkts);
    }
    IFX_PPACMD_PRINT("\n");

    IFX_PPACMD_PRINT("%28s ", "ig_fast_rt_ipv4_bytes:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_PRINT("%10u ", (unsigned int) mib->mib_info[i].ig_fast_rt_ipv4_bytes);
    }
    IFX_PPACMD_PRINT("\n");
    IFX_PPACMD_PRINT("\n");

    IFX_PPACMD_PRINT("%28s ", "ig_fast_rt_ipv6_udp_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_PRINT("%10u ", (unsigned int) mib->mib_info[i].ig_fast_rt_ipv6_udp_pkts);
    }
    IFX_PPACMD_PRINT("\n");

    IFX_PPACMD_PRINT("%28s ", "ig_fast_rt_ipv6_tcp_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_PRINT("%10u ", (unsigned int) mib->mib_info[i].ig_fast_rt_ipv6_tcp_pkts);
    }
    IFX_PPACMD_PRINT("\n");
    IFX_PPACMD_PRINT("%28s ", "ig_fast_rt_ipv6_mc_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_PRINT("%10u ", (unsigned int) mib->mib_info[i].ig_fast_rt_ipv6_mc_pkts);
    }
    IFX_PPACMD_PRINT("\n");

    IFX_PPACMD_PRINT("%28s ", "ig_fast_rt_ipv6_bytes:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_PRINT("%10u ", (unsigned int) mib->mib_info[i].ig_fast_rt_ipv6_bytes);
    }
    IFX_PPACMD_PRINT("\n");
    IFX_PPACMD_PRINT("\n");

    IFX_PPACMD_PRINT("%28s ", "ig_cpu_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_PRINT("%10u ", (unsigned int) mib->mib_info[i].ig_cpu_pkts);
    }
    IFX_PPACMD_PRINT("\n");

    IFX_PPACMD_PRINT("%28s ", "ig_cpu_bytes:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_PRINT("%10u ", (unsigned int) mib->mib_info[i].ig_cpu_bytes);
    }
    IFX_PPACMD_PRINT("\n");
    IFX_PPACMD_PRINT("\n");

    IFX_PPACMD_PRINT("%28s ", "ig_drop_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_PRINT("%10u ", (unsigned int) mib->mib_info[i].ig_drop_pkts);
    }
    IFX_PPACMD_PRINT("\n");

    IFX_PPACMD_PRINT("%28s ", "ig_drop_bytes:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_PRINT("%10u ", (unsigned int) mib->mib_info[i].ig_drop_bytes);
    }
    IFX_PPACMD_PRINT("\n");
    IFX_PPACMD_PRINT("\n");

    IFX_PPACMD_PRINT("%28s ", "eg_fast_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_PRINT("%10u ", (unsigned int) mib->mib_info[i].eg_fast_pkts);
    }
    IFX_PPACMD_PRINT("\n");

}

void print_port_mib_dbg(PPA_PORT_MIB *mib)
{
    int i;

    if( !mib) return;

    IFX_PPACMD_DBG("print_port_mib:%d\n", (int)mib->port_num);

    IFX_PPACMD_DBG("%28s ", "ig_fast_brg_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_DBG("%10u ", (unsigned int) mib->mib_info[i].ig_fast_brg_pkts);
    }
    IFX_PPACMD_DBG("\n");

    IFX_PPACMD_DBG("%28s ", "ig_fast_brg_bytes:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_DBG("%10u ", (unsigned int) mib->mib_info[i].ig_fast_brg_bytes);
    }
    IFX_PPACMD_DBG("\n");

    IFX_PPACMD_DBG("\n");
    IFX_PPACMD_DBG("%28s ", "ig_fast_rt_ipv4_udp_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_DBG("%10u ", (unsigned int) mib->mib_info[i].ig_fast_rt_ipv4_udp_pkts);
    }
    IFX_PPACMD_DBG("\n");

    IFX_PPACMD_DBG("%28s ", "ig_fast_rt_ipv4_tcp_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_DBG("%10u ", (unsigned int) mib->mib_info[i].ig_fast_rt_ipv4_tcp_pkts);
    }
    IFX_PPACMD_DBG("\n");

    IFX_PPACMD_DBG("%28s ", "ig_fast_rt_ipv4_mc_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_DBG("%10u ", (unsigned int) mib->mib_info[i].ig_fast_rt_ipv4_mc_pkts);
    }
    IFX_PPACMD_DBG("\n");

    IFX_PPACMD_DBG("%28s ", "ig_fast_rt_ipv4_bytes:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_DBG("%10u ", (unsigned int) mib->mib_info[i].ig_fast_rt_ipv4_bytes);
    }
    IFX_PPACMD_DBG("\n");
    IFX_PPACMD_DBG("\n");

    IFX_PPACMD_DBG("%28s ", "ig_fast_rt_ipv6_udp_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_DBG("%10u ", (unsigned int) mib->mib_info[i].ig_fast_rt_ipv6_udp_pkts);
    }
    IFX_PPACMD_DBG("\n");

    IFX_PPACMD_DBG("%28s ", "ig_fast_rt_ipv6_tcp_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_DBG("%10u ", (unsigned int) mib->mib_info[i].ig_fast_rt_ipv6_tcp_pkts);
    }
    IFX_PPACMD_DBG("\n");

    IFX_PPACMD_DBG("%28s ", "ig_fast_rt_ipv6_bytes:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_DBG("%10u ", (unsigned int) mib->mib_info[i].ig_fast_rt_ipv6_bytes);
    }
    IFX_PPACMD_DBG("\n");
    IFX_PPACMD_DBG("\n");

    IFX_PPACMD_DBG("%28s ", "ig_cpu_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_DBG("%10u ", (unsigned int) mib->mib_info[i].ig_cpu_pkts);
    }
    IFX_PPACMD_DBG("\n");

    IFX_PPACMD_DBG("%28s ", "ig_cpu_bytes:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_DBG("%10u ", (unsigned int) mib->mib_info[i].ig_cpu_bytes);
    }
    IFX_PPACMD_DBG("\n");
    IFX_PPACMD_DBG("\n");

    IFX_PPACMD_DBG("%28s ", "ig_drop_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_DBG("%10u ", (unsigned int) mib->mib_info[i].ig_drop_pkts);
    }
    IFX_PPACMD_DBG("\n");

    IFX_PPACMD_DBG("%28s ", "ig_drop_bytes:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_DBG("%10u ", (unsigned int) mib->mib_info[i].ig_drop_bytes);
    }
    IFX_PPACMD_DBG("\n");
    IFX_PPACMD_DBG("\n");

    IFX_PPACMD_DBG("%28s ", "eg_fast_pkts:");
    for(i=0; i<mib->port_num; i++ )
    {
        IFX_PPACMD_DBG("%10u ", (unsigned int) mib->mib_info[i].eg_fast_pkts);
    }
    IFX_PPACMD_DBG("\n");

    IFX_PPACMD_DBG("%28s ", "port flag   :");
    for(i=0; i<mib->port_num; i++ )
    {
        if( mib->mib_info[i].port_flag == PPA_PORT_MODE_ETH )
        {
            IFX_PPACMD_DBG("%10s ", "ETH");
        }
        else if( mib->mib_info[i].port_flag == PPA_PORT_MODE_DSL)
        {
            IFX_PPACMD_DBG("%10s ", "DSL");
        }
        else if( mib->mib_info[i].port_flag == PPA_PORT_MODE_EXT)
        {
            IFX_PPACMD_DBG("%10s ", "EXT");
        }
        else if( mib->mib_info[i].port_flag == PPA_PORT_MODE_CPU)
        {
            IFX_PPACMD_DBG("%10s ", "CPU");
        }
        else
        {
            IFX_PPACMD_DBG("%10s ", "Unknown");
        }
    }
    IFX_PPACMD_DBG("\n");
    IFX_PPACMD_DBG("\n");

}

static void ppa_print_get_port_mib_cmd( PPA_CMD_DATA *pdata)
{
    int i, k, num;
    PPA_PORT_MIB tmp_info;

    print_port_mib_dbg(&pdata->port_mib_info.mib);

    if( !g_output_to_file )
    {
        memset( &tmp_info, 0, sizeof(tmp_info));
        IFX_PPACMD_DBG("ppa_print_get_port_mib_cmd: There are %d ports in PPE FW\n", (int)pdata->port_mib_info.mib.port_num);

        //Get first two ethernet port: for amazon_se and Danube A4, there is no eth1. But we still keep space for it
        num = 0;
        for(i=0, k=0; i<pdata->port_mib_info.mib.port_num; i++)
        {
            if( pdata->port_mib_info.mib.mib_info[i].port_flag == PPA_PORT_MODE_ETH )
            {
                tmp_info.mib_info[num] = pdata->port_mib_info.mib.mib_info[i];
                k++;
                num ++;

                if( k == 2 ) break; //only accept 2 ethernet port
            }
        }
        IFX_PPACMD_DBG("There are %d ethernet port in PPE FW\n", k);

        //Get DSL port ( note, we only print all DSL mib's sum, esp for ATM IPOA/PPPOA and ATM EOA here
        num = 2;
        for(i=0, k=0; i<pdata->port_mib_info.mib.port_num; i++)
        {
            if( pdata->port_mib_info.mib.mib_info[i].port_flag == PPA_PORT_MODE_DSL )
            {
                tmp_info.mib_info[num].ig_fast_brg_pkts += pdata->port_mib_info.mib.mib_info[i].ig_fast_brg_pkts;
                tmp_info.mib_info[num].ig_fast_brg_bytes += pdata->port_mib_info.mib.mib_info[i].ig_fast_brg_bytes;
                tmp_info.mib_info[num].ig_fast_rt_ipv4_udp_pkts += pdata->port_mib_info.mib.mib_info[i].ig_fast_rt_ipv4_udp_pkts;
                tmp_info.mib_info[num].ig_fast_rt_ipv4_tcp_pkts += pdata->port_mib_info.mib.mib_info[i].ig_fast_rt_ipv4_tcp_pkts;
                tmp_info.mib_info[num].ig_fast_rt_ipv4_mc_pkts += pdata->port_mib_info.mib.mib_info[i].ig_fast_rt_ipv4_mc_pkts;
                tmp_info.mib_info[num].ig_fast_rt_ipv4_bytes += pdata->port_mib_info.mib.mib_info[i].ig_fast_rt_ipv4_bytes;
                tmp_info.mib_info[num].ig_fast_rt_ipv6_udp_pkts += pdata->port_mib_info.mib.mib_info[i].ig_fast_rt_ipv6_udp_pkts;
                tmp_info.mib_info[num].ig_fast_rt_ipv6_mc_pkts += pdata->port_mib_info.mib.mib_info[i].ig_fast_rt_ipv6_mc_pkts;
                tmp_info.mib_info[num].ig_fast_rt_ipv6_tcp_pkts += pdata->port_mib_info.mib.mib_info[i].ig_fast_rt_ipv6_tcp_pkts;
                tmp_info.mib_info[num].ig_fast_rt_ipv6_bytes += pdata->port_mib_info.mib.mib_info[i].ig_fast_rt_ipv6_bytes;
                tmp_info.mib_info[num].ig_cpu_pkts+= pdata->port_mib_info.mib.mib_info[i].ig_cpu_pkts;
                tmp_info.mib_info[num].ig_cpu_bytes += pdata->port_mib_info.mib.mib_info[i].ig_cpu_bytes;
                tmp_info.mib_info[num].ig_drop_pkts += pdata->port_mib_info.mib.mib_info[i].ig_drop_pkts;
                tmp_info.mib_info[num].ig_drop_bytes += pdata->port_mib_info.mib.mib_info[i].ig_drop_bytes;
                tmp_info.mib_info[num].eg_fast_pkts += pdata->port_mib_info.mib.mib_info[i].eg_fast_pkts;

                tmp_info.mib_info[num].port_flag = PPA_PORT_MODE_DSL;
                k++;
                if( k == 2 ) break; //at most only two DSL port
            }
        }
        IFX_PPACMD_DBG("There are %d DSL port in PPE FW\n", k);

        //Get extension port
        num=3;
        for(i=0, k=0; i<pdata->port_mib_info.mib.port_num; i++)
        {
            if( pdata->port_mib_info.mib.mib_info[i].port_flag == PPA_PORT_MODE_EXT)
            {
                tmp_info.mib_info[num] = pdata->port_mib_info.mib.mib_info[i];
                k++;
                num ++;

                if( k == 3) break; //at most accept three extension port/directpath
            }
        }
        IFX_PPACMD_DBG("There are %d extension port ( USB/WLAN) in PPE FW\n", k);

        tmp_info.port_num = num;
        IFX_PPACMD_DBG("There are %d port to be printed\n", (int)tmp_info.port_num);

        print_port_mib( &tmp_info );

    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->dsl_mib_info), sizeof(pdata->dsl_mib_info) );
    }

    return;
}
/****ppa_get_port_mib ----end */

/*ppacmd ppa_clear_port_mib ---begin */
static void ppa_clear_port_mib_help( int summary)
{
    IFX_PPACMD_PRINT("clearmib\n");
    return;
}
/****ppa_clear_port_mib ----end */


/*ppacmd d( dbgtool ) ---begin*/
static void ppa_dbg_tool_help( int summary)
{
    if( !summary )
        IFX_PPACMD_PRINT("d [-m mem_size_in_kbytes]\n");
    else
    {
        IFX_PPACMD_PRINT("d [-m mem_size_in_kbytes for Low memory test]\n");
    }
    return;
}
static const char ppa_dbg_tool_short_opts[] = "m:h";
static int ppa_parse_dbg_tool_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{   
    memset( &pdata->dbg_tool_info, 0, sizeof(pdata->dbg_tool_info) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'm':
            pdata->dbg_tool_info.mode = PPA_CMD_DBG_TOOL_LOW_MEM_TEST;
            pdata->dbg_tool_info.value.size = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            if( ppa_do_ioctl_cmd(PPA_CMD_DBG_TOOL, &pdata->dbg_tool_info ) == PPA_CMD_OK ) 
            {
                IFX_PPACMD_PRINT("Succeed to allocate %u Kbytes buffer for Low Memory endurance test\n", pdata->dbg_tool_info.value.size );
            }
            else
                IFX_PPACMD_PRINT("Failed to allocate %u Kbytes buffer for Low Memory endurance test\n", pdata->dbg_tool_info.value.size );
            
            break;
        
        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    return PPA_CMD_OK;
}

/*ppacmd d( dbgtool ) ---end*/


/*ppacmd ppa_set_value ---begin */
struct variable_info variable_list[]={
{"early_drop_flag", PPA_VARIABLE_EARY_DROP_FLAG, 0, 1},
{"directpath_imq_flag", PPA_VARIABLE_PPA_DIRECTPATH_IMQ_FLAG, 0, 1},
{"lan_separate_flag", PPA_LAN_SEPARATE_FLAG, 0, 100},
{"wan_separate_flag", PPA_WAN_SEPARATE_FLAG, 0, 100},
};

static const char ppa_set_variable_value_short_opts[] = "n:v:h";
static const char ppa_get_variable_value_short_opts[] = "n:h";

static void ppa_set_variable_value_help( int summary)    
{
    int i;
    
    IFX_PPACMD_PRINT("setvalue  <-n variable_name > <-v value>\n");

    if( summary )
    {
        for( i=0; i<NUM_ENTITY(variable_list); i++)
        {
            if( i == 0 ) 
                IFX_PPACMD_PRINT("    %-20s           %-9s         %-9s\n", "Variable list", "min-value", "max-value");
            IFX_PPACMD_PRINT("    %-20s           %-9i         %-9i\n", variable_list[i].name, variable_list[i].min, variable_list[i].max );
        }
    }
    
    return;
}

static void ppa_get_variable_value_help( int summary)    
{
    int i;
    
    IFX_PPACMD_PRINT("getvalue  <-n variable_name >\n");

    if( summary )
    {
        for( i=0; i<NUM_ENTITY(variable_list); i++)
        {
            if( i == 0 ) 
                IFX_PPACMD_PRINT("    %-20s           %-9s         %-9s\n", "Variable list", "min-value", "max-value");
            IFX_PPACMD_PRINT("    %-20s           %-9i         %-9i\n", variable_list[i].name, variable_list[i].min, variable_list[i].max );
        }
    }
    
    return;
}

static int ppa_parse_set_variable_value_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int n_opt = 0;
    unsigned int v_opt = 0;
    int i;

    memset( pdata, 0, sizeof(pdata->var_value_info) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'n':
            for( i=0; i<NUM_ENTITY(variable_list); i++)
            {
                if( strcmp(popts->optarg, variable_list[i].name) == 0 )
                {
                    pdata->var_value_info.id = variable_list[i].id;                    
                    n_opt ++;
                }
            }
            if( !n_opt ) 
            {
                IFX_PPACMD_PRINT("Not supported variable name:%s\n", popts->optarg);
                return PPA_CMD_ERR;
            }
            break;

        case 'v':
            pdata->var_value_info.value = str_convert(STRING_TYPE_INTEGER, popts->optarg, NULL);
            v_opt ++;
            break;    

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( n_opt != 1 || v_opt != 1 )
    {
        IFX_PPACMD_PRINT("Wrong parameter.\n");
        return PPA_CMD_ERR;
    }
    if( pdata->var_value_info.value < variable_list[pdata->var_value_info.id].min ||  pdata->var_value_info.value > variable_list[pdata->var_value_info.id].max )
        IFX_PPACMD_PRINT("Wrong value(%u), it should be %u ~ %u\n", pdata->var_value_info.value , variable_list[pdata->var_value_info.id].min, variable_list[pdata->var_value_info.id].max);
    
    return PPA_CMD_OK;
}


static int ppa_parse_get_variable_value_cmd(PPA_CMD_OPTS *popts, PPA_CMD_DATA *pdata)
{
    unsigned int n_opt = 0;
    int i;

    memset( pdata, 0, sizeof(pdata->var_value_info) );

    while(popts->opt)
    {
        switch(popts->opt)
        {
        case 'n':
            for( i=0; i<NUM_ENTITY(variable_list); i++)
            {
                if( strcmp(popts->optarg, variable_list[i].name) == 0 )
                {
                    pdata->var_value_info.id = variable_list[i].id;                    
                    n_opt ++;
                }
            }
            if( !n_opt ) 
            {
                IFX_PPACMD_PRINT("Not supported variable name:%s\n", popts->optarg);
                return PPA_CMD_ERR;
            }
            break;        

        default:
            return PPA_CMD_ERR;
        }
        popts++;
    }

    if( n_opt != 1)
    {
        IFX_PPACMD_PRINT("Wrong parameter.\n");
        return PPA_CMD_ERR;
    }
  
    return PPA_CMD_OK;
}

static void ppa_print_get_variable_value_cmd( PPA_CMD_DATA *pdata)
{
    if( !g_output_to_file )
    {
        IFX_PPACMD_PRINT("The value is %i\n", pdata->var_value_info.value);
    }
    else
    {
        SaveDataToFile(g_output_filename, (char *)(&pdata->var_value_info), sizeof(pdata->var_value_info) );
    }

    return;
}
/*ppacmd ppa_set_value ---end */



/*
===============================================================================
  Command definitions
===============================================================================
*/

static PPA_COMMAND ppa_cmd[] =
{
    {
        "---PPA Initialization/Status commands",
        PPA_CMD_INIT,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },

    {
        "init",
        PPA_CMD_INIT,
        ppa_print_init_help,
        ppa_parse_init_cmd,
        ppa_do_cmd,
        ppa_print_init_fake_cmd,
        ppa_init_long_opts,
        ppa_init_short_opts
    },
    {
        "exit",
        PPA_CMD_EXIT,
        ppa_print_exit_help,
        ppa_parse_exit_cmd,
        ppa_do_cmd,
        NULL,
        ppa_no_long_opts,
        ppa_no_short_opts
    },
    {
        "control",
        PPA_CMD_ENABLE,
        ppa_print_control_help,
        ppa_parse_control_cmd,
        ppa_do_cmd,
        NULL,
        ppa_control_long_opts,
        ppa_no_short_opts
    },
    {
        "status",
        PPA_CMD_GET_STATUS,
        ppa_print_status_help,
        ppa_parse_status_cmd,
        ppa_do_cmd,
        ppa_print_status,
        ppa_no_long_opts,
        ppa_output_short_opts
    },
    {
        "getversion",
        PPA_CMD_GET_VERSION,
        ppa_get_version_help,
        ppa_parse_simple_cmd,
        ppa_do_cmd,
        ppa_print_get_version_cmd,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },

    {
        "---PPA LAN/WAN Interface control commands",
        PPA_CMD_INIT,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },
    {
        "addwan",
        PPA_CMD_ADD_WAN_IF,
        ppa_print_add_wan_help,
        ppa_parse_add_del_if_cmd,
        ppa_do_cmd,
        NULL,
        ppa_if_long_opts,
        ppa_if_short_opts
    },
    {
        "addlan",
        PPA_CMD_ADD_LAN_IF,
        ppa_print_add_lan_help,
        ppa_parse_add_del_if_cmd,
        ppa_do_cmd,
        NULL,
        ppa_if_long_opts,
        ppa_if_short_opts
    },
    {
        "delwan",
        PPA_CMD_DEL_WAN_IF,
        ppa_print_del_wan_help,
        ppa_parse_add_del_if_cmd,
        ppa_do_cmd,
        NULL,
        ppa_if_long_opts,
        ppa_if_short_opts
    },
    {
        "dellan",
        PPA_CMD_DEL_LAN_IF,
        ppa_print_del_lan_help,
        ppa_parse_add_del_if_cmd,
        ppa_do_cmd,
        NULL,
        ppa_if_long_opts,
        ppa_if_short_opts
    },
    {
        "getwan",
        PPA_CMD_GET_WAN_IF,
        ppa_print_get_wan_help,
        ppa_parse_simple_cmd,
        ppa_do_cmd,
        ppa_print_get_wan_netif_cmd,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },
    {
        "getlan",
        PPA_CMD_GET_LAN_IF,
        ppa_print_get_lan_help,
        ppa_parse_simple_cmd,
        ppa_do_cmd,
        ppa_print_get_lan_netif_cmd,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },
    {
        "setmac",
        PPA_CMD_SET_IF_MAC,
        ppa_print_set_mac_help,
        ppa_parse_set_mac_cmd,
        ppa_do_cmd,
        NULL,
        ppa_if_mac_long_opts,
        ppa_if_mac_short_opts
    },
    {
        "getmac",
        PPA_CMD_GET_IF_MAC,
        ppa_print_get_mac_help,
        ppa_parse_get_mac_cmd,
        ppa_do_cmd,
        ppa_print_get_mac_cmd,
        ppa_if_long_opts,
        ppa_if_output_short_opts
    },

    {
        "---PPA bridging related commands(For A4/D4/E4 Firmware and A5 Firmware in DSL WAN mode)",
        PPA_CMD_INIT,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },
    {
        "addbr",
        PPA_CMD_ADD_MAC_ENTRY,
        ppa_add_mac_entry_help,
        ppa_parse_add_mac_entry_cmd,
        ppa_do_cmd,
        NULL,
        ppa_if_mac_long_opts,
        ppa_if_mac_short_opts
    },
    {
        "delbr",
        PPA_CMD_DEL_MAC_ENTRY,
        ppa_del_mac_entry_help,
        ppa_parse_del_mac_entry_cmd,
        ppa_do_cmd,
        NULL,
        ppa_mac_long_opts,
        ppa_mac_short_opts
    },
    {
        "getbrnum",
        PPA_CMD_GET_COUNT_MAC,
        ppa_get_br_count_help,
        ppa_parse_get_br_count,
        ppa_do_cmd,
        ppa_print_get_count_cmd,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },
    {
        "getbrs",
        PPA_CMD_GET_ALL_MAC,
        ppa_get_all_br_help,
        ppa_parse_simple_cmd,
        ppa_get_all_br_cmd,
        NULL,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },
    {
        "setbr",  // to enable/disable bridge mac learning hook
        PPA_CMD_BRIDGE_ENABLE,
        ppa_set_br_help,
        ppa_set_br_cmd,
        ppa_do_cmd,
        NULL,
        ppa_set_br_long_opts,
        ppa_set_br_short_opts,
    },
    {
        "getbrstatus",  //get bridge mac learning hook status: enabled or disabled
        PPA_CMD_GET_BRIDGE_STATUS,
        ppa_get_br_status_help,
        ppa_get_br_status_cmd,
        ppa_do_cmd,
        ppa_print_get_br_status_cmd,
        ppa_get_br_status_long_opts,
        ppa_get_br_status_short_opts,
    },

    {
        "---PPA unicast routing acceleration related commands",
        PPA_CMD_INIT,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },
    {
        "getlansessionnum",
        PPA_CMD_GET_COUNT_LAN_SESSION,
        ppa_get_lan_session_count_help,
        ppa_parse_get_lan_session_count,
        ppa_do_cmd,
        ppa_print_get_count_cmd,
        NULL,
        ppa_get_session_count_short_opts
    },
    {
        "getlansessions",
        PPA_CMD_GET_LAN_SESSIONS,
        ppa_get_lan_sessions_help,
        ppa_parse_get_session_cmd,
        ppa_get_lan_sessions_cmd,
        NULL,
        ppa_no_long_opts,
        ppa_get_session_short_opts
    },
    {
        "getwansessionnum",
        PPA_CMD_GET_COUNT_WAN_SESSION,
        ppa_get_wan_session_count_help,
        ppa_parse_get_wan_session_count,
        ppa_do_cmd,
        ppa_print_get_count_cmd,
        NULL,
        ppa_get_session_count_short_opts
    },
    {
        "getwansessions",
        PPA_CMD_GET_WAN_SESSIONS,
        ppa_get_wan_sessions_help,
        ppa_parse_get_session_cmd,
        ppa_get_wan_sessions_cmd,
        NULL,
        ppa_no_long_opts,
        ppa_get_session_short_opts
    },
    {
        "summary",
        0,
        ppa_summary_help,
        ppa_parse_summary_cmd,
        NULL,
        NULL,
        ppa_no_long_opts,
        ppa_summary_short_opts
    },
    {
        "addsession",  // get the dsl mib
        PPA_CMD_ADD_SESSION,
        ppa_add_session_help,
        ppa_parse_add_session_cmd,
        ppa_do_cmd,
        NULL,
        ppa_add_session_long_opts,
        ppa_add_session_short_opts
    },
#ifdef NEED_DELSESSION // in fact, no need to delete session but instead we use modify sesion to add/del session from acceleration path
    {
        "delsession",  // delet the session
        PPA_CMD_DEL_SESSION,
        ppa_del_session_help,
        ppa_parse_del_session_cmd,
        ppa_do_cmd,
        NULL,
        ppa_del_session_long_opts,
        ppa_del_session_short_opts
    },
#endif
    {
        "modifysession",  // get the dsl mib
        PPA_CMD_MODIFY_SESSION,
        ppa_modify_session_help,
        ppa_parse_modify_session_cmd,
        ppa_do_cmd,
        NULL,
        ppa_modify_session_long_opts,
        ppa_modify_session_short_opts
    },
    {
        "getsessiontimer",  // get routing session polling timer
        PPA_CMD_GET_SESSION_TIMER,
        ppa_get_session_timer_help,
        ppa_parse_get_session_timer_cmd,
        ppa_do_cmd,
        ppa_print_get_session_timer,
        ppa_get_session_timer_long_opts,
        ppa_get_session_timer_short_opts
    },
    {
        "setsessiontimer",  // set routing session polling timer
        PPA_CMD_SET_SESSION_TIMER,
        ppa_set_session_timer_help,
        ppa_parse_set_session_timer_cmd,
        ppa_do_cmd,
        NULL,
        ppa_set_session_timer_long_opts,
        ppa_set_session_timer_short_opts
    },

    {
        "---PPA multicast acceleration related commands",
        PPA_CMD_INIT,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },
    {
        "addmc",
        PPA_CMD_ADD_MC,
        ppa_add_mc_help,
        ppa_parse_add_mc_cmd,
        ppa_do_cmd,
        NULL,
        ppa_mc_add_long_opts,
        ppa_mc_add_short_opts
    },

#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
    {
        "addcapwap",
        PPA_CMD_ADD_CAPWAP,
        ppa_add_capwap_help,
        ppa_parse_add_capwap_cmd,
        ppa_do_cmd,
        NULL,
        ppa_capwap_add_long_opts,
        ppa_capwap_add_short_opts
    },
#endif

#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
    {
        "delcapwap",
        PPA_CMD_DEL_CAPWAP,
        ppa_del_capwap_help,
        ppa_parse_del_capwap_cmd,
        ppa_do_cmd,
        NULL,
        ppa_capwap_del_long_opts,
        ppa_capwap_del_short_opts
    },

    {
        "getcapwap",
        PPA_CMD_GET_CAPWAP_GROUPS,
        ppa_get_capwap_help,
        ppa_parse_get_capwap_cmd,
        ppa_get_capwap_cmd,
        NULL,
        ppa_capwap_get_long_opts,
        ppa_capwap_get_short_opts
    },

#endif

    {
        "getmcnum",
        PPA_CMD_GET_COUNT_MC_GROUP,
        ppa_get_mc_count_help,
        ppa_parse_get_mc_count_cmd,
        ppa_do_cmd,
        ppa_print_get_count_cmd,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },
    {
        "getmcgroups",
        PPA_CMD_GET_MC_GROUPS,
        ppa_get_mc_groups_help,
        ppa_parse_get_mc_group_cmd,
        ppa_get_mc_groups_cmd,
        NULL,
        ppa_get_mc_group_long_opts,
        ppa_get_mc_group_short_opts
    },
    {
        "getmcextra",
        PPA_CMD_GET_MC_ENTRY,
        ppa_get_mc_extra_help,
        ppa_parse_get_mc_extra_cmd,
        ppa_do_cmd,
        ppa_print_get_mc_extra_cmd,
        ppa_get_mc_extra_long_opts,
        ppa_get_mc_extra_short_opts
    },
    {
        "setmcextra",
        PPA_CMD_MODIFY_MC_ENTRY,
        ppa_set_mc_extra_help,
        ppa_parse_set_mc_extra_cmd,
        ppa_do_cmd,
        NULL,
        ppa_set_mc_extra_long_opts,
        ppa_set_mc_extra_short_opts
    },

#if defined( CONFIG_IFX_VLAN_BR )  /*ONly supported in A4/D4 */
    {
        "---PPA VLAN bridging related commands(For PPE A4/D4/E4 only)",
        PPA_CMD_INIT,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },
    {
        "setvif",
        PPA_CMD_SET_VLAN_IF_CFG,
        ppa_set_vlan_if_cfg_help,
        ppa_parse_set_vlan_if_cfg_cmd,
        ppa_do_cmd,
        NULL,
        ppa_set_vlan_if_cfg_long_opts,
        ppa_set_vlan_if_cfg_short_opts
    },
    {
        "getvif",
        PPA_CMD_GET_VLAN_IF_CFG,
        ppa_get_vlan_if_cfg_help,
        ppa_parse_get_vlan_if_cfg_cmd,
        ppa_do_cmd,
        ppa_print_get_vif,
        ppa_if_long_opts,
        ppa_if_output_short_opts
    },
    {
        "addvfilter",
        PPA_CMD_ADD_VLAN_FILTER_CFG,
        ppa_add_vlan_filter_help,
        ppa_parse_add_vlan_filter_cmd,
        ppa_do_cmd,
        NULL,
        ppa_add_vlan_filter_long_opts,
        ppa_add_vlan_filter_short_opts
    },
    {
        "delvfilter",
        PPA_CMD_DEL_VLAN_FILTER_CFG,
        ppa_del_vlan_filter_help,
        ppa_parse_del_vlan_filter_cmd,
        ppa_do_cmd,
        NULL,
        ppa_del_vlan_filter_long_opts,
        ppa_del_vlan_filter_short_opts
    },
    {
        "getvfilternum",
        PPA_CMD_GET_COUNT_VLAN_FILTER,
        ppa_get_vfilter_count_help,
        ppa_parse_get_vfilter_count,
        ppa_do_cmd,
        ppa_print_get_count_cmd,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },
    {
        "getvfilters",
        PPA_CMD_GET_ALL_VLAN_FILTER_CFG,
        ppa_get_all_vlan_filter_help,
        ppa_parse_simple_cmd,
        ppa_get_all_vlan_filter_cmd,
        NULL,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },
    {
        "delallvfilter",
        PPA_CMD_DEL_ALL_VLAN_FILTER_CFG,
        ppa_del_all_vfilter_help,
        ppa_parse_simple_cmd,
        ppa_do_cmd,
        NULL,
        ppa_no_long_opts,
        ppa_no_short_opts
    },
#endif
    {
        "---PPA mixed LAN/WAN mode related commands",
        PPA_CMD_INIT,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },

    {
        "getvlanrangenum",
        PPA_CMD_GET_COUNT_WAN_MII0_VLAN_RANGE,
        ppa_get_wan_mii0_vlan_range_count_help,
        ppa_parse_get_mc_count_cmd,  //share function at present
        ppa_do_cmd,
        ppa_print_get_count_cmd,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },
    {
        "addvlanrange",
        PPA_CMD_WAN_MII0_VLAN_RANGE_ADD,
        ppa_add_wan_mii0_vlan_range_help,
        ppa_parse_add_wan_mii0_vlan_range_cmd,
        ppa_do_cmd,
        NULL,
        ppa_add_wan_mii0_vlan_range_long_opts,
        ppa_add_wan_mii0_vlan_range_short_opts,
    },
    {
        "getvlanranges",
        PPA_CMD_WAN_MII0_VLAN_RANGE_GET,
        ppa_get_wan_mii0_all_vlan_range_help,
        ppa_parse_simple_cmd,
        ppa_wan_mii0_all_vlan_range_cmd,
        NULL,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },

#ifdef CONFIG_IFX_PPA_QOS
    {
        "---PPA QOS related commands ( not for A4/D4/E4 Firmware)",
        PPA_CMD_INIT,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },
    {
        "getqstatus",  // get maximum eth1 queue number
        PPA_CMD_GET_QOS_STATUS,
        ppa_get_qstatus_help,
        ppa_parse_get_qstatus_cmd,
        ppa_get_qstatus_do_cmd,
        NULL,
        NULL,
        ppa_get_qstatus_short_opts,
    },

    {
        "getqnum",  // get maximum eth1 queue number
        PPA_CMD_GET_QOS_QUEUE_MAX_NUM,
        ppa_get_qnum_help,
        ppa_parse_get_qnum_cmd,
        ppa_do_cmd,
        ppa_print_get_qnum_cmd,
        ppa_get_qnum_long_opts,
        ppa_get_qnum_short_opts,
    },
    {
        "getqmib",  // get maximum eth1 queue number
        PPA_CMD_GET_QOS_MIB,
        ppa_get_qmib_help,
        ppa_parse_get_qmib_cmd,
        ppa_get_qmib_do_cmd,
        NULL,
        ppa_get_qmib_long_opts,
        ppa_get_qmib_short_opts,
    },

#ifdef CONFIG_IFX_PPA_QOS_WFQ
    {
        "setctrlwfq",  //set wfq to enable/disable
        PPA_CMD_SET_CTRL_QOS_WFQ,
        ppa_set_ctrl_wfq_help,
        ppa_parse_set_ctrl_wfq_cmd,
        ppa_do_cmd,
        NULL,
        ppa_set_ctrl_wfq_long_opts,
        ppa_set_ctrl_wfq_short_opts,
    },
    {
        "getctrlwfq",   //get  wfq control status---
        PPA_CMD_GET_CTRL_QOS_WFQ,
        ppa_get_ctrl_wfq_help,
        ppa_parse_get_ctrl_wfq_cmd,
        ppa_do_cmd,
        ppa_print_get_ctrl_wfq_cmd,
        ppa_get_ctrl_wfq_long_opts,
        ppa_get_ctrl_wfq_short_opts,
    },
    {
        "setwfq",  //set wfq weight
        PPA_CMD_SET_QOS_WFQ,
        ppa_set_wfq_help,
        ppa_parse_set_wfq_cmd,
        ppa_do_cmd,
        NULL,
        ppa_set_wfq_long_opts,
        ppa_set_wfq_short_opts,
    },
    {
        "resetwfq",  //reset WFQ weight
        PPA_CMD_RESET_QOS_WFQ,
        ppa_reset_wfq_help,
        ppa_parse_reset_wfq_cmd,
        ppa_do_cmd,
        NULL,
        ppa_reset_wfq_long_opts,
        ppa_reset_wfq_short_opts,
    },
    {
        "getwfq",   //get  WFQ weight
        PPA_CMD_GET_QOS_WFQ,
        ppa_get_wfq_help,
        ppa_parse_get_wfq_cmd,
        ppa_get_wfq_do_cmd,
        NULL,
        ppa_get_wfq_long_opts,
        ppa_get_wfq_short_opts,
    },
#endif //endof CONFIG_IFX_PPA_QOS_WFQ

#ifdef CONFIG_IFX_PPA_QOS_RATE_SHAPING
    {
        "setctrlrate",  //set rate shaping to enable/disable
        PPA_CMD_SET_CTRL_QOS_RATE,
        ppa_set_ctrl_rate_help,
        ppa_parse_set_ctrl_rate_cmd,
        ppa_do_cmd,
        NULL,
        ppa_set_ctrl_rate_long_opts,
        ppa_set_ctrl_rate_short_opts,
    },
    {
        "getctrlrate",   //get  rate shaping control status---
        PPA_CMD_GET_CTRL_QOS_RATE,
        ppa_get_ctrl_rate_help,
        ppa_parse_get_ctrl_rate_cmd,
        ppa_do_cmd,
        ppa_print_get_ctrl_rate_cmd,
        ppa_get_ctrl_rate_long_opts,
        ppa_get_ctrl_rate_short_opts,
    },
    {
        "setrate",  //set rate shaping
        PPA_CMD_SET_QOS_RATE,
        ppa_set_rate_help,
        ppa_set_rate_cmd,
        ppa_do_cmd,
        NULL,
        ppa_set_rate_long_opts,
        ppa_set_rate_short_opts,
    },
    {
        "resetrate",  //reset rate shaping
        PPA_CMD_RESET_QOS_RATE,
        ppa_reset_rate_help,
        ppa_parse_reset_rate_cmd,
        ppa_do_cmd,
        NULL,
        ppa_reset_rate_long_opts,
        ppa_reset_rate_short_opts,
    },
    {
        "getrate",   //get ate shaping
        PPA_CMD_GET_QOS_RATE,
        ppa_get_rate_help,
        ppa_parse_get_rate_cmd,
        ppa_get_rate_do_cmd,
        NULL,
        ppa_get_rate_long_opts,
        ppa_get_rate_short_opts,
    },
#endif //endof CONFIG_IFX_PPA_QOS_RATE_SHAPING
#endif //end if CONFIG_IFX_PPA_QOS

    {
        "---PPA hook manipulation related commands",
        PPA_CMD_INIT,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },
    {
        "gethooknum",  //get all registered hook number
        PPA_CMD_GET_HOOK_COUNT,
        ppa_get_hook_count_help,
        ppa_parse_simple_cmd,
        ppa_do_cmd,
        ppa_print_get_hook_count_cmd,
        ppa_get_hook_long_opts,
        ppa_get_hook_short_opts,
    },
    {
        "gethooklist",  //get all registered hook list
        PPA_CMD_GET_HOOK_LIST,
        ppa_get_hook_list_help,
        ppa_parse_simple_cmd,
        ppa_get_hook_list_cmd,
        NULL,
        ppa_get_hook_list_long_opts,
        ppa_get_hook_list_short_opts,
    },
    {
        "sethook",  //enable/disable one registered hook
        PPA_CMD_SET_HOOK,
        ppa_set_hook_help,
        ppa_parse_set_hook_cmd,
        ppa_do_cmd,
        NULL,
        ppa_set_hook_long_opts,
        ppa_set_hook_short_opts,
    },

#if defined(CONFIG_IFX_PPA_MFE) && CONFIG_IFX_PPA_MFE
    {
        "---PPA multiple field editing related commands ( not for PPE A4/D4/E4 )",
        PPA_CMD_INIT,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },
    {
        "setctrlmf",  //enable/disable multiple field feature
        PPA_CMD_ENABLE_MULTIFIELD,
        ppa_set_mf_help,
        ppa_parse_set_mf_cmd,
        ppa_do_cmd,
        NULL,
        ppa_set_mf_long_opts,
        ppa_set_mf_short_opts,
    },
    {
        "getctrlmf",  //get multiple field feature status: enabled or disabled
        PPA_CMD_GET_MULTIFIELD_STATUS,
        ppa_get_mf_status_help,
        ppa_parse_simple_cmd,
        ppa_do_cmd,
        ppa_print_get_mf_cmd,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },
    {
        "getmfnum",  //get all multiple field flows information
        PPA_CMD_GET_MULTIFIELD_ENTRY_MAX,
        ppa_get_mf_count_help,
        ppa_parse_get_mf_count_cmd,
        ppa_do_cmd,
        ppa_print_get_mf_count_cmd,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },
    {
        "addmf",  //add one multiple field flow
        PPA_CMD_ADD_MULTIFIELD,
        ppa_add_mf_flow_help,
        ppa_parse_add_mf_flow_cmd,
        ppa_do_cmd,
        ppa_print_add_mf_flow_cmd,
        ppa_add_mf_flow_long_opts,
        ppa_add_mf_flow_short_opts,
    },
    {
        "getmf",  //del one multiple field flow
        PPA_CMD_GET_MULTIFIELD,
        ppa_get_mf_flow_help,
        ppa_parse_get_mf_flow_cmd,
        ppa_get_mf_flow_cmd,  //note, here dont use ppa_do_cmd for we need to get all flow info via multiple ioctl, not just one
        NULL,
        ppa_get_mf_flow_long_opts,
        ppa_get_mf_flow_short_opts,
    },
    {
        "delmf",  // //get maximum eth1 queue number
        PPA_CMD_DEL_MULTIFIELD,
        ppa_del_mf_flow_help,
        ppa_parse_add_mf_flow_cmd, /*share same function with addmflow */
        ppa_do_cmd,
        NULL,
        ppa_add_mf_flow_long_opts,
        ppa_add_mf_flow_short_opts,
    },

    {
        "delmf2",  // //get maximum eth1 queue number
        PPA_CMD_DEL_MULTIFIELD_VIA_INDEX,
        ppa_del_mf_flow_index_help,
        ppa_parse_del_mf_flow_index_cmd,
        ppa_do_cmd,
        NULL,
        ppa_del_mf_flow_index_long_opts,
        ppa_add_mf_flow_short_opts,
    },
#endif

    {
        "---PPA MIB related commands",
        PPA_CMD_INIT,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },
    {
        "getdslmib",  // get the dsl mib
        PPA_CMD_GET_DSL_MIB,
        ppa_get_dsl_mib_help,
        ppa_parse_simple_cmd,
        ppa_do_cmd,
        ppa_print_get_dsl_mib_cmd,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },
    {
        "cleardslmib",  // get the dsl mib
        PPA_CMD_CLEAR_DSL_MIB,
        ppa_clear_dsl_mib_help,
        ppa_parse_simple_cmd,
        ppa_do_cmd,
        NULL,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },
    {
        "getportmib",  // get the port mib
        PPA_CMD_GET_PORT_MIB,
        ppa_get_port_mib_help,
        ppa_parse_simple_cmd,
        ppa_do_cmd,
        ppa_print_get_port_mib_cmd,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },
    {
        "clearmib",  // get the dsl mib
        PPA_CMD_CLEAR_PORT_MIB,
        ppa_clear_port_mib_help,
        ppa_parse_simple_cmd,
        ppa_do_cmd,
        NULL,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },

    {
        "---PPA miscellaneous commands",
        PPA_CMD_INIT,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    },
    {
        "sethaldbg",  // //set memory value  --hide the command
        PPA_CMD_SET_HAL_DBG_FLAG,
        ppa_set_hal_dbg_help,
        ppa_parse_set_hal_flag_cmd,
        ppa_do_cmd,
        NULL,
        ppa_set_hal_dbg_long_opts,
        ppa_set_hal_dbg_short_opts,
    },
    {
        "size",
        PPA_CMD_GET_SIZE,
        ppa_get_sizeof_help,
        ppa_parse_simple_cmd,
        ppa_do_cmd,
        ppa_print_get_sizeof_cmd,
        ppa_get_simple_long_opts,
        ppa_get_simple_short_opts
    },
    {
        "getportid",  // get the port id
        PPA_CMD_GET_PORTID,
        ppa_get_portid_help,
        ppa_parse_get_portid_cmd,
        ppa_do_cmd,
        ppa_print_get_portid_cmd,
        ppa_get_portid_long_opts,
        ppa_get_portid_short_opts
    },
    {
        "setvalue",  // set value
        PPA_CMD_SET_VALUE,
        ppa_set_variable_value_help,
        ppa_parse_set_variable_value_cmd,
        ppa_do_cmd,
        NULL,
        NULL,
        ppa_set_variable_value_short_opts
    },
    {
        "getvalue",  // get value
        PPA_CMD_GET_VALUE,
        ppa_get_variable_value_help,
        ppa_parse_get_variable_value_cmd,
        ppa_do_cmd,
        ppa_print_get_variable_value_cmd,
        NULL,
        ppa_get_variable_value_short_opts
    },
    {
        "r",
        PPA_CMD_READ_MEM,
        ppa_get_mem_help,
        ppa_parse_get_mem_cmd,
        ppa_get_mem_cmd,
        NULL,
        ppa_get_mem_long_opts,
        ppa_get_mem_short_opts,
    },
    {
        "w",
        PPA_CMD_SET_MEM,
        ppa_set_mem_help,
        ppa_parse_set_mem_cmd,
        ppa_do_cmd,
        ppa_print_set_mem_cmd,
        ppa_set_mem_long_opts,
        ppa_set_mem_short_opts,
    },
    {
        "d",
        PPA_CMD_DBG_TOOL,
        ppa_dbg_tool_help,
        ppa_parse_dbg_tool_cmd,
        NULL,
        NULL,
        NULL,
        ppa_dbg_tool_short_opts,
    },

#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
    {
        "setrtp",
        PPA_CMD_SET_RTP, 
        ppa_set_rtp_help,
        ppa_parse_set_rtp_cmd,
/*      ppa_set_rtp_cmd, */
        ppa_do_cmd,
        NULL,
        ppa_rtp_set_long_opts,
        ppa_rtp_set_short_opts
    },
#endif

#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE
    {
        "setmibmode",
        PPA_CMD_SET_MIB_MODE, 
        ppa_set_mib_mode_help,
        ppa_parse_set_mib_mode,
        ppa_do_cmd,
        NULL,
        NULL,
        ppa_mib_mode_short_opts
    },
#endif


#if defined(PPA_TEST_AUTOMATION_ENABLE) && PPA_TEST_AUTOMATION_ENABLE
    /*Note, put all not discolsed command at the end of the array */
    {
        "automation",  // //set memory value  --hide the command
        PPA_CMD_INIT,
        ppa_test_automation_help,
        ppa_parse_test_automation_cmd,
        ppa_test_automation_cmd,
        NULL,
        ppa_test_automation_long_opts,
        ppa_test_automation_short_opts,
    },
#endif

    { NULL, 0, NULL, NULL, NULL, NULL, NULL }
};

/*
====================================================================================
  command:   ppa_cmd_help Function
  description: prints help text
  options:   argv
====================================================================================
*/
static void ppa_print_help(void)
{
    PPA_COMMAND *pcmd;
    int i;

    IFX_PPACMD_PRINT("Usage: %s <command> {options} \n", PPA_CMD_NAME);

    IFX_PPACMD_PRINT("Commands: \n");
    for(pcmd = ppa_cmd; pcmd->name != NULL; pcmd++)
    {
        if(pcmd->print_help)
        {
            //IFX_PPACMD_PRINT(" "); //it will cause wrong alignment for hidden internal commands
            (*pcmd->print_help)(0);
        }
        else  if( pcmd->name[0] == '-' || pcmd->name[0] == ' ')
        {
#define MAX_CONSOLE_LINE_LEN 80
            int filling=strlen(pcmd->name)>=MAX_CONSOLE_LINE_LEN ? 0 : MAX_CONSOLE_LINE_LEN-strlen(pcmd->name);
            IFX_PPACMD_PRINT("\n%s", pcmd->name);
            for(i=0; i<filling; i++ ) IFX_PPACMD_PRINT("-");
            IFX_PPACMD_PRINT("\n");
        }
    }

#if PPACMD_DEBUG
    IFX_PPACMD_PRINT("\n");
    IFX_PPACMD_PRINT("* Note: Create a file %s will enable ppacmd debug mode\n", debug_enable_file );
    IFX_PPACMD_PRINT("* Note: Any number inputs will be regarded as decial value without prefix 0x\n");
    IFX_PPACMD_PRINT("* Note: Please run \"ppacmd <command name> -h\" to get its detail usage\n");
    IFX_PPACMD_PRINT("\n");
#endif

    return;
}

/*
====================================================================================
  command:   ppa_cmd_help Function
  description: prints help text
  options:   argv
====================================================================================
*/
static void ppa_print_cmd_help(PPA_COMMAND *pcmd)
{
    if(pcmd->print_help)
    {
        IFX_PPACMD_PRINT("Usage: %s ", PPA_CMD_NAME);
        (*pcmd->print_help)(1);
    }
    return;
}

/*
===============================================================================
  Command processing functions
===============================================================================
*/

/*
===========================================================================================


===========================================================================================
*/
static int get_ppa_cmd(char *cmd_str, PPA_COMMAND **pcmd)
{
    int i;

    // Locate the command where the name matches the cmd_str and return
    // the index in the command array.
    for (i = 0; ppa_cmd[i].name; i++)
    {
        if (strcmp(cmd_str, ppa_cmd[i].name) == 0)
        {
            *pcmd = &ppa_cmd[i];
            return PPA_CMD_OK;
        }
    }
    return PPA_CMD_ERR;
}

/*
===========================================================================================


===========================================================================================
*/
static int ppa_parse_cmd(int ac, char **av, PPA_COMMAND *pcmd, PPA_CMD_OPTS *popts)
{
    int opt, opt_idx, ret = PPA_CMD_OK;
    int num_opts;


    // Fill out the PPA_CMD_OPTS array with the option value and argument for
    // each option that is found. If option is help, display command help and
    // do not process command.
    for (num_opts = 0; num_opts < PPA_MAX_CMD_OPTS; num_opts++)
    {
        opt = getopt_long(ac - 1, av + 1, pcmd->short_opts, pcmd->long_opts, &opt_idx);
        if (opt != -1)
        {
            if (opt == 'h')        // help
            {
                ret = PPA_CMD_HELP;
                return ret;
            }
            else if (opt == '?')      // missing argument or invalid option
            {
                ret = PPA_CMD_ERR;
                break;
            }
            popts->opt  = opt;
            popts->optarg = optarg;
            popts++;
        }
        else
            break;
    }
    return ret;
}

/*
===========================================================================================


===========================================================================================
*/
static int ppa_parse_cmd_line(int ac, char **av, PPA_COMMAND **pcmd, PPA_CMD_DATA **data)
{
    int ret = PPA_CMD_ERR;
    PPA_CMD_DATA *pdata = NULL;
    PPA_CMD_OPTS *popts = NULL;

    if ((ac <= 1) || (av == NULL))
    {
        return PPA_CMD_HELP;
    }

    pdata = malloc(sizeof(PPA_CMD_DATA));
    if (pdata == NULL)
        return PPA_CMD_NOT_AVAIL;
    memset(pdata, 0, sizeof(PPA_CMD_DATA));

    popts = malloc(sizeof(PPA_CMD_OPTS)*PPA_MAX_CMD_OPTS);
    if (popts == NULL)
    {
        free(pdata);
        return PPA_CMD_NOT_AVAIL;
    }
    memset(popts, 0, sizeof(PPA_CMD_OPTS)*PPA_MAX_CMD_OPTS);

    ret = get_ppa_cmd(av[1], pcmd);
    if (ret == PPA_CMD_OK)
    {
        ret = ppa_parse_cmd(ac, av, *pcmd, popts);
        if ( ret == PPA_CMD_OK )
        {
            ret = (*pcmd)->parse_options(popts,pdata);
            if ( ret == PPA_CMD_OK )
                *data = pdata;
        }
        else
        {
            IFX_PPACMD_PRINT("Wrong parameter\n");
            ret = PPA_CMD_HELP;
        }
    }
    else
    {
        IFX_PPACMD_PRINT("Unknown commands:  %s\n", av[1]);
    }
    free(popts);
    return ret;
}


/*
===========================================================================================


===========================================================================================
*/
int main(int argc, char** argv)
{
    int ret;
    PPA_CMD_DATA *pdata = NULL;
    PPA_COMMAND *pcmd=NULL;

    opterr = 0; //suppress option error messages

#if PPACMD_DEBUG
    {
        FILE *fp;

        fp = fopen(debug_enable_file, "r");
        if( fp != NULL )
        {
            enable_debug = 1;
            fclose(fp);
        }
    }
#endif

    if( argc == 1)
    {
        ppa_print_help();
        return 0;
    }

    ret = ppa_parse_cmd_line (argc, argv, &pcmd, &pdata);
    if (ret == PPA_CMD_OK)
    {
        if (pcmd->do_command)
        {
            ret = pcmd->do_command(pcmd,pdata);
            if (ret == PPA_CMD_OK && pcmd->print_data)
                pcmd->print_data(pdata);
        }
    }
    else if (ret == PPA_CMD_HELP)
    {
        ppa_print_cmd_help(pcmd);
    }
    if( pdata)
    {
        free(pdata);
        pdata=NULL;
    }
    return ret;
}
