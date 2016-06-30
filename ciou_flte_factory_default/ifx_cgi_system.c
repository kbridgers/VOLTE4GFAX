//#include <sys/syslog.h>
#include <ifx_emf.h>
#include "ifx_httpd_method.h"
#include "ifx_amazon_cgi_time.h"
#include <time.h>
//#include <signal.h>
//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/wait.h>
#include <ifx_common.h>
#include "./ifx_amazon_cgi.h"
#include "./ifx_amazon_cgi_getFunctions.h"
#include "./ifx_cgi_common.h"
#define trace(...)
#define PART_MAX 100
//#include "ifx_snmp_api.h"
//#include "ifx_api_ipt_common.h"
#include <string.h>
//#include <sys/ioctl.h>

extern int nIdleTime;
extern char_t *status_str;
int log_disp_level;
int global_log_mode;
#ifdef CONFIG_FEATURE_IFX_USB_HOST
uint16 vun_UsbMountStatus = 0;
#endif
extern int reboot_status;
extern int ifx_get_version_info(struct ifx_version_info *);
extern int ifx_httpd_parse_args(int argc, char_t ** argv, char_t * fmt, ...);
extern void websNextPage(httpd_t wp);
extern struct connection_profil_list *connlist;

#ifdef CONFIG_FEATURE_SYSTEM_LOG
void ifx_set_system_log(httpd_t wp, char_t * path, char_t * query);	// system_log.asp
int ifx_get_Securitylog(int eid, httpd_t wp, int argc, char_t ** argv);
#endif
int ifx_set_lan_addr_radvd_opt(httpd_t wp, char_t * path, char_t * query);
void ifx_set_wizard_host(httpd_t wp, char_t * path, char_t * query);	// wizard_host.asp; system_hostname.asp
void ifx_set_system_password(httpd_t wp, char_t * path, char_t * query);	// system_password.asp
void ifx_set_system_upgrade(httpd_t wp, char_t * path, char_t * query);	// system_upgrade.asp
void ifx_set_system_reset(httpd_t wp, char_t * path, char_t * query);	// system_reset.asp
void ltq_cgi_web_config_set(httpd_t wp, char_t * path, char_t * query);	//autologout.asp
int ifx_get_SystemHostName(int eid, httpd_t wp, int argc, char_t ** argv);
int ifx_get_SystemDomainName(int eid, httpd_t wp, int argc, char_t ** argv);
int ifx_get_SignalStrength(int eid, httpd_t wp, int argc, char_t ** argv);
int ifx_get_IMEInumber(int eid, httpd_t wp, int argc, char_t ** argv);
int ifx_get_IMSInumber(int eid, httpd_t wp, int argc, char_t ** argv);
int ifx_get_FAXnumber(int eid, httpd_t wp, int argc, char_t ** argv);

int ifx_get_FirmwareVer(int eid, httpd_t wp, int argc, char_t ** argv);
int ifx_get_Autologouttime(int eid, httpd_t wp, int argc, char_t ** argv);
#ifdef CONFIG_PACKAGE_NTPCLIENT
void ifx_set_wizard_tz(httpd_t wp, char_t * path, char_t * query);	// wizard_tz.asp
int ifx_get_TimeSetting(int eid, httpd_t wp, int argc, char_t ** argv);
int ifx_get_TimeSetting1(int eid, httpd_t wp, int argc, char_t ** argv);
int ifx_get_cfdaysave(int eid, httpd_t wp, int argc, char_t ** argv);
int ifx_get_Daylight_StartMon(int eid, httpd_t wp, int argc, char_t ** argv);
int ifx_get_Daylight_StartDay(int eid, httpd_t wp, int argc, char_t ** argv);
int ifx_get_Daylight_EndMon(int eid, httpd_t wp, int argc, char_t ** argv);
int ifx_get_Daylight_EndDay(int eid, httpd_t wp, int argc, char_t ** argv);
#endif
#ifdef CONFIG_FEATURE_IFX_USB_HOST
extern void ifx_httpdNextPage(httpd_t wp);
uint16 usb_ret_status = 0;
#endif



int user_status = 0;
static int sig_read=0;
static CGI_ENUMSEL_S web_Enum_TimeZoneList[] = {
	{-720, "(GMT-12:00) Eniwetok, Kwajalein, International Date Line West"},
	{-660, "(GMT-11:00) Midway Island, Samoa"},
	{-600, "(GMT-10:00) Hawaii"},
	{-540, "(GMT-09:00) Alaska"},
	{-480, "(GMT-08:00) Pacific Time (US and Canada); Tijuana"},
	{-420, "(GMT-07:00) Arizona, Mountain Time (US and Canada), Chihuahua"},
	{-360,
	 "(GMT-06:00) Central America, Central Time (US and Canada), Mexico City, Saskatchewan"},
	{-300,
	 "(GMT-05:00) Bogota, Lima, Quito, Eastern Time (US and Canada), Indiana (East)"},
	{-270, "(GMT-04:30) Caracas"},
	{-240, "(GMT-04:00) Atlantic Time (Canada), Caracas, La Paz, Santiago"},
	{-210, "(GMT-03:30) Newfoundland"},
	{-180,
	 "(GMT-03:00) Brasilia, Buenos Aires, Georgetown, Greenland, Montevideo"},
	{-120, "(GMT-02:00) Mid-Atlantic"},
	{-60, "(GMT-01:00) Azores, Cape Verde Is."},
	{0,
	 "(GMT) Casablanca, Monrovia, Greenwich Mean Time: Dublin, Edinburgh, Lisbon, London"},
	{+60,
	 "(GMT+01:00) Amsterdam, Berlin, Rome, Stockholm, Vienna, Prague, Brussels, Madrid, Paris"},
	{+120,
	 "(GMT+02:00) Athens, Istanbul, Bucharest, Cairo, Harare, Pretoria, Helsinki, Jerusalem"},
	{+180,
	 "(GMT+03:00) Baghdad, Kuwait, Riyadh, Moscow, St. Petersburg, Volgograd, Nairobi"},
	{+210, "(GMT+03:30) Tehran"},
	{+240,
	 "(GMT+04:00) Abu Dhabi, Muscat, Baku, Tbilisi, Yerevan, Port Louis"},
	{+270, "(GMT+04:30) Kabul"},
	{+300, "(GMT+05:00) Ekaterinburg, Islamabad, Karachi, Tashkent"},
	{+330,
	 "(GMT+05:30) Calcutta, Chennai, Mumbai, New Delhi, Sri Jayawardenepura"},
	{+345, "(GMT+05:45) Kathmandu"},
	{+360, "(GMT+06:00) Almaty, Novosibirsk, Astana, Dhaka"},
	{+390, "(GMT+06:30) Yangon (Rangoon)"},
	{+420, "(GMT+07:00) Bangkok, Hanoi, Jakarta, Krasnoyarsk"},
	{+480,
	 "(GMT+08:00) Beijing, Hong Kong, Ulaan Bataar, Kuala Lumpur, Singapore, Perth, Taipei"},
	{+540, "(GMT+09:00) Osaka, Sapporo, Tokyo, Seoul, Yakutsk"},
	{+570, "(GMT+09:30) Adelaide, Darwin"},
	{+600,
	 "(GMT+10:00) Brisbane, Canberra, Melbourne, Sydney, Guam, Hobart, Vladivostok"},
	{+660, "(GMT+11:00) Magadan, Solomon Is., New Caledonia"},
	{+720,
	 "(GMT+12:00) Auckland, Wellington, Fiji, Kamchatka, Marshall Is."},
	{+780, "(GMT+13:00) Nuku'alofa"}
};
#if 0
int ifx_get_SignalStrength(int eid, httpd_t wp, int argc, char_t ** argv)
{
	if(0==sig_read)
	{
		ifx_httpdWrite(wp, "%s", "13%");
		sig_read=1;
	}
	else
	{
		ifx_httpdWrite(wp, "%s", "58%");
		sig_read=0;
	}
	return 0;
}

int ifx_get_IMEInumber(int eid, httpd_t wp, int argc, char_t ** argv)
{
	ifx_httpdWrite(wp, "%s", "123456789012345");
	trace(8,"In IMEI Number\n");
	return 0;
}

int ifx_get_IMSInumber(int eid, httpd_t wp, int argc, char_t ** argv)
{
	ifx_httpdWrite(wp, "%s", "123451234512345");
	return 0;
}

int ifx_get_FAXnumber(int eid, httpd_t wp, int argc, char_t ** argv)
{
	ifx_httpdWrite(wp, "%s", "+915");
	return 0;
}
#else
#define CIOU_FLTE  1
#if CIOU_FLTE
#include<stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#define MAX_DEVICE_NUM_LEN  20
#define MAX_IMSI_LEN  20
#define MAX_IMEI_LEN  20

typedef struct
{
        unsigned char FLTEDeviceNo[MAX_DEVICE_NUM_LEN];                         //data asociated with cmd
        unsigned char FLTEimsiNo[MAX_IMSI_LEN];                                         //Address to be written
        unsigned char FLTEimeiNo[MAX_IMEI_LEN];                                         //Data to be read
}LTEDeviceInfo_t;

typedef struct
{
        unsigned char SignalStrength;
        unsigned char SystemStatus;
}LTEDevicesignalstrength_t;

#define READ_DEVICE_SIGNAL_STRENGTH _IOWR('w', 1, LTEDevicesignalstrength_t *)
#define READ_DEVICE_IMEI_NUMBER _IOWR('w', 2, LTEDeviceInfo_t *)
#define READ_DEVICE_IMSI_NUMBER _IOWR('w', 3, LTEDeviceInfo_t *)			/*Setting Device Address to the driver*/
#define READ_DEVICE_NUMBER _IOWR('w',4,LTEDeviceInfo_t *)
#define READ_DEVICE_QUERY_ALL _IOWR('w',5,LTEDeviceInfo_t *)

/*IOCTL Commands*/
#define WRITE_DEVICE_SIGNAL_STRENGTH _IOWR('w',6, LTEDevicesignalstrength_t *)
#define WRITE_DEVICE_IMEI_NUMBER _IOWR('w', 7, LTEDeviceInfo_t *)
#define WRITE_DEVICE_IMSI_NUMBER _IOWR('w', 8, LTEDeviceInfo_t *)			/*Setting Device Address to the driver*/
#define WRITE_DEVICE_NUMBER _IOWR('w',9,LTEDeviceInfo_t *)
#define WRITE_DEVICE_QUERY_ALL _IOWR('w',10,LTEDeviceInfo_t *)

LTEDeviceInfo_t DeviceInfo;
LTEDevicesignalstrength_t LTEDeviceSignalStrength;
#endif



#if 1				//Added By sanju
int ifx_get_SignalStrength(int eid, httpd_t wp, int argc, char_t ** argv)
{
	sig_read=1;
#if 0
	if(0==sig_read)
	{
		ifx_httpdWrite(wp, "%s", "13%");
		sig_read=1;
	}
	else
	{
		ifx_httpdWrite(wp, "%s", "58%");
		sig_read=0;
	}
#endif
	int fd;
	fd=open("/dev/LTEDeviceInfo", O_RDWR |O_NONBLOCK);
	if(fd<0)
	{
		printf("No Device Found.\n");
	}
	ioctl(fd,READ_DEVICE_SIGNAL_STRENGTH,&LTEDeviceSignalStrength);
	ifx_httpdWrite(wp, "%d", LTEDeviceSignalStrength.SignalStrength);		/*Writing Signal Strength*/
	//return 0;
	close(fd);
	return 0;
}

int ifx_get_IMEInumber(int eid, httpd_t wp, int argc, char_t ** argv)
{
        int fd;
        fd=open("/dev/LTEDeviceInfo", O_RDWR |O_NONBLOCK);
        if(fd<0)
        {
                printf("No Device Found.\n");
        }
        ioctl(fd,READ_DEVICE_IMEI_NUMBER,&DeviceInfo);
        ifx_httpdWrite(wp, "%s",DeviceInfo.FLTEimeiNo);             /*Writing Signal Strength*/
        //return 0;
        close(fd);
	//ifx_httpdWrite(wp, "%s", "123456789012345");
	return 0;
}

int ifx_get_IMSInumber(int eid, httpd_t wp, int argc, char_t ** argv)
{
	//ifx_httpdWrite(wp, "%s", "123451234512345");
	        int fd;
        fd=open("/dev/LTEDeviceInfo", O_RDWR |O_NONBLOCK);
        if(fd<0)
        {
                printf("No Device Found.\n");
        }
        ioctl(fd,READ_DEVICE_IMSI_NUMBER,&DeviceInfo);
        ifx_httpdWrite(wp, "%s",DeviceInfo.FLTEimsiNo);             /*Writing Signal Strength*/
        //return 0;
        close(fd);

	return 0;
}

int ifx_get_FAXnumber(int eid, httpd_t wp, int argc, char_t ** argv)
{
	//ifx_httpdWrite(wp, "%s", "+91456233456699998");
	int fd;
        fd=open("/dev/LTEDeviceInfo", O_RDWR |O_NONBLOCK);
        if(fd<0)
        {
                printf("No Device Found.\n");
        }
        ioctl(fd,READ_DEVICE_NUMBER,&DeviceInfo);
        ifx_httpdWrite(wp, "%s",DeviceInfo.FLTEDeviceNo);             /*Writing Signal Strength*/
        //return 0;
        close(fd);
	return 0;
}
#endif
#endif
#ifdef CONFIG_FEATURE_IPv6
int ifx_set_lan_addr_radvd_opt(httpd_t wp, char_t * path, char_t * query)
{

	char_t *lanip6, *lanrip6, *lanrgw6, *lanrpdns6, *lanrsdns6,*land6sname;
	char_t *lan6_dhcp_saddr, *lan6_dhcp_eaddr;
	uint32 lan6_dhcp_prefix;
	uint32 prefix_len = 0, rprefix_len = 0;
	int32 ret = IFX_SUCCESS, flags = IFX_F_MODIFY;
	char8 buf[MAX_FILELINE_LEN];
	IP6_ADDR_STRING v6addr;
	LAN_IPv6_SL_Config v6radvd;
	LAN_IPv6_DHCPv6_Config dhcpv6;
	int32 lan_ipv6_mode = 0;
	/* lan_ipv6_mode  
	 *      0 = Stateless Address Autoconf 
	 *      1 = Stateless Address Autoconf + stateless DHCP
	 *      2 = Statefull DHCP
	 */

	lanip6 = NULL;
	lanrip6 = NULL;
	lanrgw6 = NULL;
	lanrpdns6 = NULL;
	lanrsdns6 = NULL;
	land6sname = NULL;

	lan6_dhcp_saddr = NULL;
	lan6_dhcp_eaddr = NULL;
	lan6_dhcp_prefix = 0;

	NULL_TERMINATE(buf, 0, sizeof(buf));
	memset(buf, 0x00, sizeof(buf));

	memset(&v6addr, 0x00, sizeof(v6addr));
	memset(&v6radvd, 0x00, sizeof(v6radvd));
	memset(&dhcpv6, 0x00, sizeof(dhcpv6));

	/* Getting br0 Address */
	lanip6 = ifx_httpdGetVar(wp, T("address"), T(""));
	prefix_len = atoi(ifx_httpdGetVar(wp, T("prefix_len"), T("")));

	inet_pton(AF_INET6, lanip6, (struct in6_addr *)&v6addr.ip.s6_addr32);
	v6addr.prefix_len = prefix_len = 64;

	/* Getting LAN mode */
	lan_ipv6_mode = atoi(ifx_httpdGetVar(wp, T("MODE"), T("")));

	switch (lan_ipv6_mode) {

	case 0:		/* SLAAC */
		lanrip6 = ifx_httpdGetVar(wp, T("prefix"), T(""));
		rprefix_len = atoi(ifx_httpdGetVar(wp, T("rpre_len"), T("")));
		lanrgw6 = ifx_httpdGetVar(wp, T("route"), T(""));
		lanrpdns6 = ifx_httpdGetVar(wp, T("rdns6_1"), T(""));
		lanrsdns6 = ifx_httpdGetVar(wp, T("rdns6_2"), T(""));

		inet_pton(AF_INET6, lanrip6,
			  (struct in6_addr *)&v6radvd.ip6Addr.ip.s6_addr32);
		v6radvd.ip6Addr.prefix_len = rprefix_len;
		inet_pton(AF_INET6, lanrgw6,
			  (struct in6_addr *)&v6radvd.gw6addr.ip.s6_addr32);
		inet_pton(AF_INET6, lanrpdns6,
			  (struct in6_addr *)&v6radvd.dnsv6Addr.ip.s6_addr32);
		inet_pton(AF_INET6, lanrsdns6,
			  (struct in6_addr *)&v6radvd.dnsv6SecAddr.ip.
			  s6_addr32);
		v6radvd.f_enable = 1;
		break;
	case 1:		/* SLAAC + Stateless DHCP */

		lanrip6 = ifx_httpdGetVar(wp, T("prefix"), T(""));
		rprefix_len = atoi(ifx_httpdGetVar(wp, T("rpre_len"), T("")));
		lanrpdns6 = ifx_httpdGetVar(wp, T("d6saddr_1"), T(""));
		lanrsdns6 = ifx_httpdGetVar(wp, T("d6saddr_2"), T(""));
		land6sname = ifx_httpdGetVar(wp, T("d6sname"), T(""));

		inet_pton(AF_INET6, lanrip6,
			  (struct in6_addr *)&v6radvd.ip6Addr.ip.s6_addr32);
		v6radvd.ip6Addr.prefix_len = rprefix_len;

		inet_pton(AF_INET6, lanrpdns6,
			  (struct in6_addr *)&dhcpv6.dnsv6Addr.ip.s6_addr32);
		inet_pton(AF_INET6, lanrsdns6,
			  (struct in6_addr *)&dhcpv6.dnsv6SecAddr.ip.s6_addr32);
		dhcpv6.domainName = land6sname;
		dhcpv6.f_enable = 1;
		break;
	case 2:		/* Stateful DHCP */
		lan6_dhcp_saddr = ifx_httpdGetVar(wp, T("d6start_addr"), T(""));
		lan6_dhcp_eaddr = ifx_httpdGetVar(wp, T("d6end_addr"), T(""));
		lan6_dhcp_prefix =
		    atoi(ifx_httpdGetVar(wp, T("d6pre_len"), T("")));
		lanrpdns6 = ifx_httpdGetVar(wp, T("d6saddr_1"), T(""));
		lanrsdns6 = ifx_httpdGetVar(wp, T("d6saddr_2"), T(""));
		land6sname = ifx_httpdGetVar(wp, T("d6sname"), T(""));

		inet_pton(AF_INET6, lan6_dhcp_saddr,
			  (struct in6_addr *)&dhcpv6.saddr.ip.s6_addr32);
		inet_pton(AF_INET6, lan6_dhcp_eaddr,
			  (struct in6_addr *)&dhcpv6.eaddr.ip.s6_addr32);
		inet_pton(AF_INET6, lanrpdns6,
			  (struct in6_addr *)&dhcpv6.dnsv6Addr.ip.s6_addr32);
		inet_pton(AF_INET6, lanrsdns6,
			  (struct in6_addr *)&dhcpv6.dnsv6SecAddr.ip.s6_addr32);
		dhcpv6.domainName = land6sname;
		dhcpv6.eaddr.prefix_len = lan6_dhcp_prefix;
		dhcpv6.f_enable = 1;
		break;
	default:
		IFX_DBG("Unknown lan_ipv6_mode = %d\n", lan_ipv6_mode);
		ifx_httpdError(wp, 500, "Unknown  lan mode %d  !!",
			       lan_ipv6_mode);
		return IFX_FAILURE;
	}

	system(". /etc/rc.d/rc.bringup_lan v6stop");

	IFX_DBG("addr = %s, %d , %s , %d , %s , %s , %s \n", lanip6, prefix_len,
		lanrip6, rprefix_len, lanrgw6, lanrpdns6, lanrsdns6);

	printf("addr = %s, %d , %s , %d , %s , %s , %s \n", lanip6, prefix_len,
	       lanrip6, rprefix_len, lanrgw6, lanrpdns6, lanrsdns6);

	ret =
	    ifx_set_lan_ula6(IFX_OP_MOD, lan_ipv6_mode, "br0", &v6addr,
			     &v6radvd, &dhcpv6, IFX_F_MODIFY);
	IFX_DBG("\n\n SET ULA6 SUCCESS !! \n\n");
	if (ret != IFX_SUCCESS) {
#ifdef IFX_LOG_DEBUG
		IFX_DBG("\n\n Error : set api returned IFX_FAILURE !! \n\n");
#endif
		ifx_httpdError(wp, 500,
			       "Failed to save lan device configuration !!");
		goto IFX_Handler;
	}

	NULL_TERMINATE(buf, 0, sizeof(buf));
	memset(buf, 0x00, sizeof(buf));

	sprintf(buf, "lan_ipv6_mode=\"%d\"\n", lan_ipv6_mode);
	if ((ret =
	     ifx_SetObjData(FILE_RC_CONF, TAG_LAN_IPV6, flags, 1,
			    buf)) != IFX_SUCCESS) {
		IFX_DBG("[%s:%d], ERROR IN SETTING 1 [%s]\n", __FUNCTION__,
			__LINE__, FILE_RC_CONF);
		ifx_httpdError(wp, 500, "fail to save setting");
		goto IFX_Handler;
	}
	// save setting
	if (ifx_flash_write() <= 0) {
		ifx_httpdError(wp, 500, "fail to save setting");
		ret = IFX_FAILURE;
		goto IFX_Handler;
	}

	system(". /etc/rc.d/rc.bringup_lan v6start");

//        return IFX_SUCCESS ;                                                                                                 
      IFX_Handler:
	websNextPage(wp);
	return ret;
}

#endif

// system_hostname.asp
void ifx_set_wizard_host(httpd_t wp, char_t * path, char_t * query)
{
	char_t *pHostname, *pDomainname;
	char_t sHostname[MAX_DATA_LEN];
#ifndef CONFIG_FEATURE_LTQ_HNX_CONFIG
	char_t sValue[MAX_DATA_LEN];
	char_t sdhcps_IP_Start[MAX_DATA_LEN];
	char_t sdhcps_IP_End[MAX_DATA_LEN];
	char_t sdhcps_Max_Leases[MAX_DATA_LEN];
	char_t sdhcps_Domain[MAX_DATA_LEN];
	char_t sdhcps_Lease[MAX_DATA_LEN];
#endif				/* !CONFIG_FEATURE_LTQ_HNX_CONFIG */
	sHostname[0] = '\0';
	// Get value for ASP file
	pHostname = ifx_httpdGetVar(wp, T("HostName"), T(""));
	pDomainname = ifx_httpdGetVar(wp, T("DomainName"), T(""));

	if (pHostname == 0 || pDomainname == 0) {
		ifx_httpdError(wp, 400, T("Setup Error"));
		return;
	}
	gsprintf(sHostname, "hostname=\"%s.%s\"\n", pHostname, pDomainname);
	// Update rc.conf in ramdisk
	if (ifx_SetCfgData(FILE_RC_CONF, TAG_HOSTNAME, 1, sHostname) == 0) {
		ifx_httpdError(wp, 500, "Fail to set hostname");
		return;
	}
#ifndef CONFIG_FEATURE_LTQ_HNX_CONFIG
	if (ifx_GetCfgData(FILE_RC_CONF, TAG_LAN_DHCPS, "dhcps_Domain", sValue)
	    == 1) {
		if (gstrcmp(sValue, T("0")) != 0) {
			if (ifx_GetCfgData
			    (FILE_RC_CONF, TAG_LAN_DHCPS, "dhcps_IP_Start",
			     sValue) == 1)
				snprintf(sdhcps_IP_Start,
					 sizeof(sdhcps_IP_Start),
					 "dhcps_IP_Start=\"%s\"\n", sValue);
			if (ifx_GetCfgData
			    (FILE_RC_CONF, TAG_LAN_DHCPS, "dhcps_IP_End",
			     sValue) == 1)
				snprintf(sdhcps_IP_End, sizeof(sdhcps_IP_End),
					 "dhcps_IP_End=\"%s\"\n", sValue);
			if (ifx_GetCfgData
			    (FILE_RC_CONF, TAG_LAN_DHCPS, "dhcps_Max_Leases",
			     sValue) == 1)
				snprintf(sdhcps_Max_Leases,
					 sizeof(sdhcps_Max_Leases),
					 "dhcps_Max_Leases=\"%s\"\n", sValue);
			gsprintf(sdhcps_Domain, "dhcps_Domain=\"%s\"\n",
				 pDomainname);
			if (ifx_GetCfgData
			    (FILE_RC_CONF, TAG_LAN_DHCPS, "dhcps_Lease",
			     sValue) == 1)
				snprintf(sdhcps_Lease, sizeof(sdhcps_Lease),
					 "dhcps_Lease=\"%s\"\n", sValue);
			if (ifx_SetCfgData
			    (FILE_RC_CONF, TAG_LAN_DHCPS, 5, sdhcps_IP_Start,
			     sdhcps_IP_End, sdhcps_Max_Leases, sdhcps_Domain,
			     sdhcps_Lease) == 0) {
				ifx_httpdError(wp, 500, "Fail to set hostname");
				return;
			}
		}
	}
#endif				/* !CONFIG_FEATURE_LTQ_HNX_CONFIG */
	// save setting
	if (ifx_flash_write() <= 0) {
		ifx_httpdError(wp, 500, "Fail to save Setting");
		return;
	}
	//6090601: hsur update /etc/hosts in run-time
	//update /etc/hosts
	{
		FILE *fd;
		fd = fopen("/etc/hosts", "w");
		if (fd) {
			fprintf(fd, "127.0.0.1\t localhost.localdomain \n");
			char_t sLanIP[MAX_DATA_LEN];
			memset(sLanIP, 0x00, MAX_DATA_LEN);
			websGetIFInfo(LAN_IF_TYPE, IP_INFO, 1, 4, FALSE, NULL,
				      sLanIP);
			fprintf(fd, "%s\t %s.%s  %s\n", sLanIP, pHostname,
				pDomainname, pHostname);
			fclose(fd);
		}
	}
#ifndef CONFIG_FEATURE_LTQ_HNX_CONFIG
	system(SERVICE_UDHCPD_STOP);
	system(SERVICE_UDHCPD_START);
	// apply the new dns server ip to the dns relay server
	system(SERVICE_DNS_RELAY_RESTART);
#else				/* !CONFIG_FEATURE_LTQ_HNX_CONFIG */
	system("/etc/rc.d/init.d/avahi-daemon restart");
#endif				/* CONFIG_FEATURE_LTQ_HNX_CONFIG */
	websNextPage(wp);
}

// wizard_tz.asp
int ifx_get_SystemDomainName(int eid, httpd_t wp, int argc, char_t ** argv)
{
	char_t sHostname[MAX_DATA_LEN];
	int pos = 0;
	if (ifx_GetCfgData(FILE_RC_CONF, TAG_HOSTNAME, T("hostname"), sHostname)
	    == 0) {
		ifx_httpdError(wp, 500, T("Hostname not found"));
		return -1;
	}
	pos = strcspn(sHostname, ".");
	ifx_httpdWrite(wp, T("%s"), sHostname + pos + 1);
	return 0;
}
int ifx_get_SystemHostName(int eid, httpd_t wp, int argc, char_t ** argv)
{
	char_t sHostname[MAX_DATA_LEN];
	if (ifx_GetCfgData(FILE_RC_CONF, TAG_HOSTNAME, T("hostname"), sHostname)
	    == 0) {
		ifx_httpdError(wp, 500, T("Hostname not found"));
		return -1;
	}

	strtok(sHostname, ".");
	ifx_httpdWrite(wp, T("%s"), sHostname);
	return 0;
}

//end system_hostname.asp

/*manohar : set and get CGI for web configuration page*/
void ltq_cgi_web_config_set(httpd_t wp, char_t * path, char_t * query)
{
				char_t *autologout_op_selected = ifx_httpdGetVar(wp, T("userAutologouttime"), T(""));
				char_t *https_op_selected = ifx_httpdGetVar(wp, T("HTTPs_Enable"), T(""));
				char_t autologout_buff[MAX_FILELINE_LEN];
				char_t https_buff[MAX_FILELINE_LEN];

				char_t pre_https_op[40];
				if(ifx_GetObjData(FILE_RC_CONF, TAG_SYSTEM_WEB_CONFIG,"HTTPsEnable",IFX_F_GET_ANY,NULL,pre_https_op) != IFX_SUCCESS) {
								ifx_httpdError(wp, 500, T("https enable not found"));
								return;
				}

				gsprintf(autologout_buff, "AutoLogoutTime=\"%s\"\n", autologout_op_selected);

				if (atoi(https_op_selected)) 
								gsprintf(https_buff, "HTTPsEnable=\"%s\"\n",https_op_selected);   
				else
								gsprintf(https_buff, "HTTPsEnable=\"%s\"\n","0");

				if(ifx_SetCfgData(FILE_RC_CONF, TAG_SYSTEM_WEB_CONFIG, 2, autologout_buff,https_buff) == 0) {
								ifx_httpdError(wp, 500, "Fail to set Autologout time");
								return;
				}
				else {
								nIdleTime = 0;
				}


				if(ifx_flash_write() <= 0) {
								ifx_httpdError(wp, 500, "Fail to save Setting");
								return;
				}

				if(atoi(pre_https_op) != atoi(https_op_selected))
								system("/etc/init.d/mini_httpd restart");

				websNextPage(wp);
				return;
}

int ltq_cgi_web_config_get(int eid, httpd_t wp, int argc, char_t ** argv)
{
				char *name = NULL;
				char_t sValue[MAX_FILELINE_LEN]={0};
				if (ifx_httpd_parse_args(argc, argv, T("%s"), &name) < 1) {
								ifx_httpdError(wp, 400, T("Insufficient args\n"));
								return -1;
				}

				if (!gstrcmp(name, T("autologout")))
				{
								if (ifx_GetObjData(FILE_RC_CONF, TAG_SYSTEM_WEB_CONFIG,"AutoLogoutTime",IFX_F_GET_ANY,NULL,sValue) != IFX_SUCCESS) {
												ifx_httpdError(wp, 500, T("Logout time not found"));
												return -1;
								}
								ifx_httpdWrite(wp, T("%s"),sValue);
				}
				if (!gstrcmp(name, T("https")))
				{
								if(ifx_GetObjData(FILE_RC_CONF, TAG_SYSTEM_WEB_CONFIG,"HTTPsEnable",IFX_F_GET_ANY,NULL,sValue) != IFX_SUCCESS) {
												ifx_httpdError(wp, 500, T("https enable not found"));
												return -1;
								}
								if(atoi(sValue) == 1)
												ifx_httpdWrite(wp, T("%s"),"checked");
								else
												ifx_httpdWrite(wp, T("%s"),"");
				}
				if (!gstrcmp(name, T("https_support")))
				{
#ifdef CONFIG_FEATURE_HTTPS
					ifx_httpdWrite(wp, T("%s"),"1");
#else
					ifx_httpdWrite(wp, T("%s"),"0");
#endif
				}

				return 0;
}

int ltq_cgi_sys_admin_get(int eid, httpd_t wp, int argc, char_t ** argv)
{
	char *name = NULL;
	char_t sValue[MAX_FILELINE_LEN]={0};
	if (ifx_httpd_parse_args(argc, argv, T("%s"), &name) < 1) {
		ifx_httpdError(wp, 400, T("Insufficient args\n"));
		return -1;
	}

	if (!gstrcmp(name, T("passwdEnable")))
	{
		if (ifx_GetObjData(FILE_RC_CONF,TAG_SYSTEM_PASSWORD, T("PasswdProtect"),IFX_F_GET_ANY,NULL,sValue) != IFX_SUCCESS) {
			ifx_httpdError(wp, 500, T("passwd protect not found"));
			return -1;
		}
		ifx_httpdWrite(wp, T("%s"),sValue);
	}
	return 0;
}
/*manohar end*/

//system_password.htm
void ifx_set_system_password(httpd_t wp, char_t * path, char_t * query)
{
	char_t *passwd_protect = ifx_httpdGetVar(wp, T("passwd_enable"), T(""));
	char_t passwdProtect[MAX_DATA_LEN]; 
	user_obj_t user_obj;
	int		ret = IFX_SUCCESS;
	enum access_levels {
		ACCESS_NONE = 0,
		ACCESS_OPT1, ACCESS_ADD = ACCESS_OPT1, ACCESS_ENA = ACCESS_OPT1,
		ACCESS_OPT2, ACCESS_DEL = ACCESS_OPT2,
		ACCESS_BOTH
	};

	memset(&user_obj, 0, sizeof(user_obj));
	if(atoi(passwd_protect) == 0) {
		char_t *pUserName = ifx_httpdGetVar(wp, T("users"), T(""));
		char_t *pUserEditEnable = ifx_httpdGetVar(wp, T("user_edit_enable"), T(""));
		char_t *pUserEnable = ifx_httpdGetVar(wp, T("userEnable"), T(""));
		char_t *pUserNameEdit = ifx_httpdGetVar(wp, T("userNew"), T(""));
		char_t *pUserOldPswd = ifx_httpdGetVar(wp, T("userOldPswd"), T(""));
		char_t *pUserPswdEdit = ifx_httpdGetVar(wp, T("user_passwd_edit_enable"), T(""));
		char_t *pUserNewPswd = ifx_httpdGetVar(wp, T("userNewPswd"), T(""));
		char_t *pUserWebLocalAccess = ifx_httpdGetVar(wp, T("userLaccess"), T(""));
		char_t *pUserWebRemoteAccess = ifx_httpdGetVar(wp, T("userRaccess"), T(""));
		char_t *pUserFTPAccess = ifx_httpdGetVar(wp, T("userFTPaccess"), T(""));
		char_t *pUserSambaAccess = ifx_httpdGetVar(wp, T("userSMBaccess"), T(""));
		char_t *pUserTelnetAccess = ifx_httpdGetVar(wp, T("userTelaccess"), T(""));
		char_t *pUserId = ifx_httpdGetVar(wp, T("curr_user_id"), T(""));
		char_t *pUserSubmitFlag = ifx_httpdGetVar(wp, T("submitflag"), T(""));

		/* 1. make global password protect flag as '0'
		 * 2. update details of the user requested
		 * 3. update current http session password if applicable
		 */
		gsprintf(passwdProtect, "PasswdProtect=\"%s\"\n","0");
		ifx_SetCfgData(FILE_RC_CONF, TAG_SYSTEM_PASSWORD, 1, passwdProtect);

		user_obj.iid.cpeId.Id = atoi(pUserId);

		if (atoi(pUserSubmitFlag) == ACCESS_ADD) { //Add new user operation
			strcpy(user_obj.username, pUserNameEdit);
			strcpy(user_obj.password, pUserNewPswd);
		} else {
			if(mapi_user_obj_details_get(&user_obj, IFX_F_MODIFY) != IFX_SUCCESS) {
#ifdef IFX_LOG_DEBUG
				IFX_DBG("[%s:%d] reading current configuration of user [%s] failed", __FUNCTION__, __LINE__, pUserName);
#endif
				return;
			}

			/* If Delete flag is Enabled (pUserSubmitFlag == ACCESS_DEL), then delete the user */
			if (atoi(pUserSubmitFlag) == ACCESS_DEL) {
				if(strcmp(pUserName, "admin") == 0) {
					ifx_httpdError(wp, 500,
							T
							("<body><h2>Cannot delete the user admin!!</h2>\n"));
					return;
				} else {
					if(strcmp(user_obj.password, pUserOldPswd)) {
#ifdef IFX_LOG_DEBUG
						IFX_DBG("[%s:%d] Delete opertation - current password validation failed for user [%s]", __FUNCTION__, __LINE__, pUserName);
#endif
						COPY_TO_STATUS
						    ("<span class=\"textTitle\">Current password validation failed.</span><br><p>Delete operation failed for the user %s. Please make sure password provided for 'Current Password' matches password already configured for specified user.</p><br>\n", pUserName);
						ifx_httpdRedirect(wp, T("err_page.html"));
						ret = IFX_FAILURE;
						return;
					}
					if(mapi_user_obj_details_set(IFX_OP_DEL, &user_obj, IFX_F_DELETE) != IFX_SUCCESS) {
						ifx_httpdError(wp, 500,
								T
								("<body><h2>Unable to delete the user: %s!!</h2>\n"),pUserName);
						return;
					}
					user_status = ACCESS_DEL;
				}
			} else {

				/* If Modify flag is Enabled, then modify the user details */
				if(atoi(pUserEditEnable) == ACCESS_ENA) {
					/* compare current user password from the config file and the one provided in web page */
					if(strcmp(user_obj.password, pUserOldPswd)) {
#ifdef IFX_LOG_DEBUG
						IFX_DBG("[%s:%d] current password validation failed for user [%s]", __FUNCTION__, __LINE__, pUserName);
#endif
						COPY_TO_STATUS
						    ("<span class=\"textTitle\">Current password validation failed.</span><br><p>Please make sure password provided for 'Current Password' matches password already configured for specified user.</p><br>\n");
						ifx_httpdRedirect(wp, T("err_page.html"));
						ret = IFX_FAILURE;
						if(ret != IFX_SUCCESS)
							return;
					}

					if(strcmp(pUserName, "admin") == 0) {
						strcpy(user_obj.username, pUserName);
					} else {
						strcpy(user_obj.username, pUserNameEdit);
					}

					if (atoi(pUserPswdEdit) == ACCESS_ENA) {
						strcpy(user_obj.password, pUserNewPswd);
					}
				}
			}
		}

		if (atoi(pUserSubmitFlag) != ACCESS_DEL) { //Delete not set
			if((atoi(pUserWebLocalAccess) == ACCESS_ENA) && (atoi(pUserWebRemoteAccess) == ACCESS_ENA)) {
				user_obj.webAccess = ACCESS_BOTH;
			} else if (atoi(pUserWebLocalAccess) == ACCESS_ENA) {
				user_obj.webAccess = ACCESS_OPT1;
			} else if (atoi(pUserWebRemoteAccess) == ACCESS_ENA) {
				user_obj.webAccess = ACCESS_OPT2;
			} else {
				user_obj.webAccess = ACCESS_NONE;
			}
	
			if((atoi(pUserFTPAccess) == ACCESS_ENA) && (atoi(pUserSambaAccess) == ACCESS_ENA)) {
				user_obj.fileShareAccess = ACCESS_BOTH;
			} else if (atoi(pUserFTPAccess) == ACCESS_ENA) {
				user_obj.fileShareAccess = ACCESS_OPT1;
			} else if (atoi(pUserSambaAccess) == ACCESS_ENA) {
				user_obj.fileShareAccess = ACCESS_OPT2;
			} else {
				user_obj.fileShareAccess = ACCESS_NONE;
			}

			user_obj.telnetAccess = atoi(pUserTelnetAccess);

			/* read enable flag from web page for users other than root and admin
			 * as those user accounts can't be enabled/disabled from web page and
			 * local web access must be granted for root and admin.
			 */
			if(strcmp(pUserName, "admin") && strcmp(pUserName, "root")) {
				user_obj.f_enable = atoi(pUserEnable);
			} else {
				user_obj.f_enable = ACCESS_ENA;
				if((user_obj.webAccess == ACCESS_NONE) || (user_obj.webAccess == ACCESS_OPT2)) {
					user_obj.webAccess++;
				}
			}

			if (atoi(pUserSubmitFlag) == ACCESS_ENA) {
				if(mapi_user_obj_details_set(IFX_OP_ADD, &user_obj, IFX_F_DEFAULT) != IFX_SUCCESS) {
					ifx_httpdError(wp, 500,
						T
						("<body><h2>Unable to add user details!!</h2>\n"));
					return;
				}
			} else {
				if(mapi_user_obj_details_set(IFX_OP_MOD, &user_obj, IFX_F_MODIFY) != IFX_SUCCESS) {
					ifx_httpdError(wp, 500,
							T
							("<body><h2>Unable to update user details!!</h2>\n"));
					return;
				}
			}
			user_status = ACCESS_ADD;
		}

		/* Change existing session password */
//		ifx_httpdSetPassword(pUserNewPswd);

		// save setting
		if (ifx_flash_write() <= 0) {
			user_status = ACCESS_NONE;
			ifx_httpdError(wp, 500, "Fail to save Setting");
			return;
		}
#ifdef CONFIG_FEATURE_SAMBA
		{
			const char *fname = "/etc/samba/smbpasswd";
			FILE *fp = NULL;
			char sLine[MAX_FILELINE_LEN];
			char sTemp[MAX_FILELINE_LEN];
			char buf[BUF_SIZE_1K];
			char main_buf[BUF_SIZE_1K];
			int i;

			if ((fp = fopen(fname, "r")) == NULL) {
#ifdef IFX_LOG_DEBUG
				IFX_DBG("\ncan't open file %s\n", fname);
#endif
			} else {
				sLine[0] = '\0';
				sTemp[0] = '\0';
				buf[0] = '\0';
				main_buf[0] = '\0';
				i = 0;
				while (fgets(sLine, sizeof(sLine), fp)) {
					sLine[strlen(sLine)-1]='\0';
					snprintf(sTemp, sizeof(sTemp),
						 "%s%d='%s'\n",
						 TAG_SMB_PASSWORD_FILE_LINE, i,
						 sLine);
					LTQ_STRNCAT(buf, sTemp, sizeof(buf));
					i++;
					sLine[0] = '\0';
					sTemp[0] = '\0';
				}
				snprintf(main_buf,BUF_SIZE_1K, "%s='%d'\n%s",
					TAG_SMB_PASSWORD_FILE_LINE, i, buf);
				fclose(fp);
				ifx_SetCfgData(FILE_RC_CONF,
					       PREFIX_SMB_PASSWORD_FILE, 1,
					       main_buf);
			}
		}
		// save setting
		if (ifx_flash_write() <= 0) {
			user_status = ACCESS_NONE;
			ifx_httpdError(wp, 500, "Fail to save Setting");
			return;
		}
#endif				/* CONFIG_FEATURE_SAMBA */
		websNextPage(wp);
	}
	else {
		gsprintf(passwdProtect, "PasswdProtect=\"%s\"\n","1");
		ifx_SetCfgData(FILE_RC_CONF, TAG_SYSTEM_PASSWORD, 1 ,passwdProtect);
		if (ifx_flash_write() <= 0) {
			user_status = ACCESS_NONE;
			ifx_httpdError(wp, 500, "Fail to save Setting");
			return;
		}
		ifx_httpdRedirect(wp, T("system_password.htm"));
	}
IFX_Handler:
	return;
}

//end system_password.htm

//sysconfig_update.asp
int32 ifx_sysconfig_change(httpd_t wp, char_t * path, char_t * query)	//(int32 status, uint32 flags)
{
	char_t *pCheck;
	int status = 2;
	int flags = IFX_F_DEFAULT;
	// Get value from ASP file
	pCheck = ifx_httpdGetVar(wp, T("config_mode"), T(""));
	status = atoi(pCheck);
#ifdef CONFIG_FEATURE_IFX_USB_HOST
	switch (status) {
	case 0:
	case 1:
#if 0
		system("/usr/sbin/read_img sysconfig /tmp/rc.conf.gz");
#endif
		ifx_mapi_backup_config(flags);
		if (0 == status) {
			ifx_httpdRedirect(wp, T("sysconfig_download.asp"));
		} else {
			ifx_httpdRedirect(wp, T("sysconfig_backup_to_usb.asp"));
		}
		break;
	case 2:
		ifx_httpdRedirect(wp, T("sysconfig_upgrade.asp"));
		break;
	case 3:
		ifx_httpdRedirect(wp, T("sysconfig_restore_from_usb.asp"));
		break;
	default:
		break;
	}

#else
	if (status == 0) {
#if 0
		system("/usr/sbin/read_img sysconfig /tmp/rc.conf.gz");
#endif
		ifx_mapi_backup_config(flags);
		ifx_httpdRedirect(wp, T("sysconfig_download.asp"));
	} else {
		ifx_httpdRedirect(wp, T("sysconfig_upgrade.asp"));
	}
#endif
	return 0;
}

//end sysconfig_update.asp

#ifdef CONFIG_FEATURE_IFX_USB_HOST
/*****************************************************************************
*  Name: IFX_HTTP_USBMount
*  Function: This function mounts USB devices
*  Input:
*  Output:
*  Return Value:
******************************************************************************/
int32 IFX_HTTP_USBMount(httpd_t wp, char_t * path, char_t * query)
{
	vun_UsbMountStatus = 3;

	system("if [ ! -d /tmp/usb ];then mkdir -p -m 777 /tmp/usb;fi");
	/*Chekc if already mounted */
	if (system("grep /dev/sda1 /etc/mtab") == 0) {
		vun_UsbMountStatus = 1;	/*Already Mounted */
	} else if (system("mount -t vfat -o rw,umask=000 /dev/sda1 /tmp/usb") ==
		   0) {
		system("/usr/sbin/led_control -l 2 on");
		vun_UsbMountStatus = 2;	/*Mounted Successfully */
	}

	websNextPage(wp);
	return 0;
}

/*****************************************************************************
*  Name: IFX_HTTP_USBUmount
*  Function: This functions un-mounts the USB device
*  Input: u
*  Output:
*  Return Value:
******************************************************************************/
int32 IFX_HTTP_USBUmount(httpd_t wp, char_t * path, char_t * query)
{
	char usb_status[300] = "";
	int ret = 0;
	char_t *pUsb_SerialNum = ifx_httpdGetVar(wp, T("usb_serialnum"), T(""));
	char_t buff[300] = "", tmpbuf[100] = "";
	FILE *fp;
	if (pUsb_SerialNum == NULL) {
		ifx_httpdError(wp, 400, T("Not specified USB Index"));
		ret = -1;
	}
	snprintf(buff, 74, "/etc/init.d/usb_automount_status.sh umount %s",
		 pUsb_SerialNum);
	fp = popen(buff, "r");
	if (fp == NULL) {
		ifx_httpdError(wp, 400, T("popen fail"));
		ret = -1;
                goto IFX_Handler;
	}
	memset(&usb_status, 0, sizeof(usb_status));
	if (fp != NULL) {
		while (fgets(tmpbuf, 100, fp) != 0) {
			tmpbuf[strlen(tmpbuf) - 1] = '\0';
			LTQ_STRNCAT(usb_status, tmpbuf, sizeof(usb_status));
			strcat(usb_status, "\\n");
		}
	}
	pclose(fp);

	COPY_TO_STATUS("%s%s%s", "<script type=\"text/javascript\">alert(\"",
		       usb_status, "\");</script>");
	ifx_httpdRedirect(wp, "usb.asp");
	ret = 0;
      IFX_Handler:
	return ret;
}

/*****************************************************************************
*  Name: ltq_get_usb_connections
*  Function: This function display connected USB device(s) info
*  Input: NIL
*  Output: Show connected device(s) details if available
*  Return Value: Always 0.
******************************************************************************/
int32 ltq_get_usb_connections(int eid, httpd_t wp, int argc, char_t ** argv)
{
        FILE *fp;
        char *buf = NULL;
	char *p = NULL, *t = NULL;
	char type[512];
	size_t fsize = 0, res;
	int flg = 0;

	typedef struct t_usbdev_info
	{
		char *v, *m, *r, *s, *d;
	} usbdev_info;

	usbdev_info udv;

	if ((fp = fopen ("/proc/bus/usb/devices", "rb")) == NULL) {
		return (0);
	}

	while (fgetc(fp) != EOF) fsize++;
	fclose (fp);
	
	if (!fsize) return (0);

	if ((fp = fopen ("/proc/bus/usb/devices", "rb")) == NULL) {
		return (0);
	}

	buf = (char *) malloc (sizeof(char) *fsize);
	if (buf == NULL) {
		fclose (fp);
		return (0); /* We donot need an error report for this function */
	}

	res = fread (buf, 1, fsize, fp);
	if (res != fsize) {
		fclose (fp);
		free (buf);
		return (0); /* We donot need an error report for this function */
	}
	buf [res - 1] = '\0';

	fclose(fp);

	p = buf;

	void write_content (int end)
	{
		if ((end) && (flg)) {
			ifx_httpdWrite(wp, T("</table>"));
		} else if (!end) {
			if(flg == 0) {
				ifx_httpdWrite(wp, T("<table class=\"tableInfo\" summary=\"Table\">"));
				ifx_httpdWrite(wp, T("<tr><th class=\"curveLeft\" colspan=\"6\">\
					<span class='cellWrapper'><img alt=\"\" src='images/RightCurve.png'>\
					</span>Connected USB Devices</th></tr>"));
				ifx_httpdWrite(wp, T("<tr><th class=\"curveLeft\">USB Type</th><th>USB Version</th>\
					<th>Product Name</th><th>Manufacturer</th><th>Serial Number</th>\
					<th class=\"colInput curveRight\">Description</th></tr>"));
				flg++;
			}
			
			ifx_httpdWrite(wp, T("<tr>\n"));
			if (type[0] != '\0') ifx_httpdWrite(wp, T("<td>%s</td>"), type);
			else ifx_httpdWrite(wp, T("<td>NA</td>"));
			if (udv.v != NULL) ifx_httpdWrite(wp, T("<td>USB %s</td>"), udv.v);
			else ifx_httpdWrite(wp, T("<td>NA</td>"));
			if (udv.r != NULL) ifx_httpdWrite(wp, T("<td>%s</td>"), udv.r);
			else ifx_httpdWrite(wp, T("<td>NA</td>"));
			if (udv.m != NULL) ifx_httpdWrite(wp, T("<td>%s</td>"), udv.m);
			else ifx_httpdWrite(wp, T("<td>NA</td>"));
			if (udv.s != NULL) ifx_httpdWrite(wp, T("<td>%s</td>"), udv.s);
			else ifx_httpdWrite(wp, T("<td>NA</td>"));

			if (strcmp(type, "usb-storage") == 0) {
				ifx_httpdWrite(wp, T("<td>Mass Storage Device<br>(Drives will be automounted based<br>on supported filesystems.)</td>"));
			} else if (strcmp(type, "usblp") == 0) {
				ifx_httpdWrite(wp, T("<td>USB Printer<br>(Use TCP/IP port 9100 to connect.)</td>"));
			} else {
				ifx_httpdWrite(wp, T("<td>NA</td>"));
			}
			ifx_httpdWrite(wp, T("</tr>\n"));
		}
	}

	memset (&type, 0, sizeof(type));
	memset (&udv, 0, sizeof(usbdev_info));
	while (*p != '\0') {
		if((t = strstr (p, "\n\n")) != NULL) {
			*t = '\0';
			t = p;
			p = strchr(p, '\0') + 1;
		} else {
			t = p;
			p = strchr (p, '\0');
		}
		//printf("%s<<\n", t);
		if (strlen(t) > 1) {
			if((udv.v = strstr (t, "Ver=")) != NULL) udv.v += 4;
			if((udv.m = strstr (t, "Manufacturer=")) != NULL) udv.m += 13;
			if((udv.r = strstr (t, "Product=")) != NULL) udv.r += 8;
			if((udv.s = strstr (t, "SerialNumber=")) != NULL) udv.s += 13;
			udv.d = strstr (t, "Driver=");

			if((udv.v != NULL) && ((t = strstr (udv.v, "Cls=")) != NULL)) *t = '\0';
			if((udv.m != NULL) && ((t = strchr (udv.m, '\n')) != NULL)) *t = '\0';
			if((udv.r != NULL) && ((t = strchr (udv.r, '\n')) != NULL)) *t = '\0';
			if((udv.s != NULL) && ((t = strchr (udv.s, '\n')) != NULL)) *t = '\0';
			if (udv.d != NULL) {
				while (*udv.d != '\0') {
					if ((t = strstr (udv.d, "Driver=")) != NULL) {
						t += 7;
						if((udv.d = strchr (t, '\n')) != NULL) {
							*udv.d = '\0';
							udv.d += 1;
						} else udv.d = strchr (t, '\0');
						if (strcmp(t, "(none)") != 0) {
							if (type[0] != '\0') strcat (type, "/");
							strcat (type, t);
						}
					} else {
						udv.d = strchr (udv.d, '\0');
					}
				}
			}
			if (strcmp (type, "hub") != 0) write_content(0);
		}
		memset (&type, 0, sizeof(type));
		memset (&udv, 0, sizeof(usbdev_info));
	}

	write_content(1);

	free(buf);
	return (0);
}

int32 ifx_get_usb_info(int eid, httpd_t wp, int argc, char_t ** argv)
{
	FILE *fp;
	char diskcount_str[20] = "";
	char diskname_str[20] = "";
	char diskname[100] = "";
	char diskserialnum[100] = "";
	char diskpartcount_str[20] = "";
	char diskinfo_str[20] = "";
	char diskinfopath[100] = "";
	char diskinfofs[20] = "";
	char diskinfosize[20] = "";
	char diskinfofresp[20] = "";
	char diskinfousedsp[20] = "";
	char diskinfouse[20] = "";
	unsigned int i = 0, j = 0;
	unsigned int diskcount = 0;
	unsigned int diskpartcount = 0;
	int ret = -1;
	system("/etc/init.d/usb_automount_status.sh status");
	fp = fopen("/tmp/usb_info.txt", "r");
	if (fp == NULL) {
#ifdef IFX_LOG_DEBUG
		IFX_DBG("[%s:%d] failed to read /tmp/usb_info.txt",
			__FUNCTION__, __LINE__);
#endif
		ret = -1;
		goto IFX_Handler;
	}
	if (fscanf(fp, "%20s %u", diskcount_str, &diskcount) != 2) {
#ifdef IFX_LOG_DEBUG
		IFX_DBG
		    ("[%s:%d] failed to read diskcount from /tmp/usb_info.txt",
		     __FUNCTION__, __LINE__);
#endif
		ret = -1;
		goto IFX_Handler;
	}
#ifdef IFX_LOG_DEBUG
	IFX_DBG("[%s:%d] Disk count -> %u\n", __FUNCTION__, __LINE__,
		diskcount);
#endif
	if (diskcount == 0) {
#ifdef IFX_LOG_DEBUG
		IFX_DBG("[%s:%d] Diskcount is 0\n", __FUNCTION__, __LINE__);
#endif
		ifx_httpdWrite(wp, T("No USB/SATA Storage Devices mounted."));
		ret = -1;
		goto IFX_Handler;
	}
	for (i = 0; i < diskcount; i++) {
		if (fscanf
		    (fp, "%20s %100s %[^=\n]", diskname_str, diskserialnum,
		     diskname) == 0) {
#ifdef IFX_LOG_DEBUG
			IFX_DBG
			    ("[%s:%d] failed to read diskname from /tmp/usb_info.txt",
			     __FUNCTION__, __LINE__);
#endif
			ret = -1;
			goto IFX_Handler;
		}
#ifdef IFX_LOG_DEBUG
		IFX_DBG("[%s:%d] Disk %u : \n diskname -> %s\n", __FUNCTION__,
			__LINE__, i, diskname);
#endif
		if (fscanf(fp, "%20s %u", diskpartcount_str, &diskpartcount) != 2) {
#ifdef IFX_LOG_DEBUG
			IFX_DBG
			    ("[%s:%d] failed to read diskpartcount from /tmp/usb_info.txt",
			     __FUNCTION__, __LINE__);
#endif
			ret = -1;
			goto IFX_Handler;
		}
#ifdef IFX_LOG_DEBUG
		IFX_DBG("[%s:%d] Disk %u : \n diskpartcount -> %u\n",
			__FUNCTION__, __LINE__, i, diskpartcount);
#endif
		//Table field names
		ifx_httpdWrite(wp,
			       T
			       ("<table class=\"tableInfo\" summary=\"Table\">"));
		ifx_httpdWrite(wp, T("<tr>"));
		ifx_httpdWrite(wp,
			       T
			       ("<th class=\"curveLeft\" colspan=\"6\"><span class='cellWrapper'><img alt=\"\" src='images/RightCurve.png'></span>%s - %s</th>"),
			       diskname, diskserialnum);
		ifx_httpdWrite(wp, T("</tr>"));
		ifx_httpdWrite(wp, T("<tr>"));
		ifx_httpdWrite(wp,
			       T("<th class=\"curveLeft\">Mount Path</th>"));
		ifx_httpdWrite(wp, T("<th>File System</th>"));
		ifx_httpdWrite(wp, T("<th>Total Size</th>"));
		ifx_httpdWrite(wp, T("<th>Used Space</th>"));
		ifx_httpdWrite(wp, T("<th>Free Space</th>"));
		ifx_httpdWrite(wp,
			       T
			       ("<th class=\"colInput curveRight\">Percentage of Use</th>"));
		ifx_httpdWrite(wp, T("</tr>"));
		for (j = 0; j < diskpartcount; j++) {
			if (fscanf
			    (fp, "%20s %100s %20s %20s %20s %20s %20s", diskinfo_str,
			     diskinfopath, diskinfofs, diskinfosize,
			     diskinfofresp, diskinfousedsp, diskinfouse) != 7) {
#ifdef IFX_LOG_DEBUG
				IFX_DBG
				    ("[%s:%d] failed to read diskpartvalue from /tmp/usb_info.txt",
				     __FUNCTION__, __LINE__);
#endif
				ret = -1;
				goto IFX_Handler;
			}
#ifdef IFX_LOG_DEBUG
			IFX_DBG
			    ("[%s:%d] diskpath -> %s\n diskinfofs -> %s\n diskinfosize -> %s\n diskinfofresp -> %s\n diskinfousee -> %s \n",
			     __FUNCTION__, __LINE__, diskinfopath, diskinfofs,
			     diskinfosize, diskinfofresp, diskinfouse);
#endif
			ifx_httpdWrite(wp, T("<tr align='left'>"));
			ifx_httpdWrite(wp, T("<td class=\"curveLeft\">%s</td>"),
				       diskinfopath);
			ifx_httpdWrite(wp, T("<td>%s</td>"), diskinfofs);
			ifx_httpdWrite(wp, T("<td>%s</td>"), diskinfosize);
			ifx_httpdWrite(wp, T("<td>%s</td>"), diskinfofresp);
			ifx_httpdWrite(wp, T("<td>%s</td>"), diskinfousedsp);
			ifx_httpdWrite(wp,
				       T
				       ("<td class=\"colInput curveRight\">%s</td>"),
				       diskinfouse);
			ifx_httpdWrite(wp, T("</tr>"));
		}
#ifdef IFX_LOG_DEBUG
		IFX_DBG("[%s:%d] diskcount -> %u\n", __FUNCTION__, __LINE__,
			diskcount);
#endif
		ifx_httpdWrite(wp, T("<tr><td colspan=6>"));
		if (diskpartcount > 0) {
			ifx_httpdWrite(wp, T("<div align=\"right\">"));
			ifx_httpdWrite(wp,T("<a href=\"#\" class=\"button\" value=\"Safe Remove\" name=\"Safe Remove\" onClick=\"return evaltFUMount('%s');\">Safe Remove</a></div>"),
				diskserialnum);
		} else {
			ifx_httpdWrite(wp, T("<div align=\"center\">Partitions not mounted or unmounted.</div>"));
		}
		ifx_httpdWrite(wp, T("</td></tr>"));
		ifx_httpdWrite(wp, T("</table>"));
		ifx_httpdWrite(wp, T("<br>"));
		ifx_httpdWrite(wp, T("<br>"));
	}
	ret = 0;
      IFX_Handler:
	if (fp)
		fclose(fp);
	else
		ifx_httpdWrite(wp, T("No USB/SATA Storage Devices mounted."));
//      websNextPage(wp);
	return ret;

}

/*int32 ifx_get_usbdir() function check if any usb device is mounted  and returns the directories present in the usb device 
 * connected to the CPE */

int32 ifx_get_usbdir(int eid, httpd_t wp, int argc, char_t ** argv);
int32 ifx_get_usbdir(int eid, httpd_t wp, int argc, char_t ** argv)
{
	FILE *fp;
	int /*value=0, */ ret = 0;

	char usbpath[PART_MAX], *name = NULL;
	if (ifx_httpd_parse_args(argc, argv, T("%s"), &name) < 1) {
		ifx_httpdError(wp, 400, T("Insufficient args\n"));
		ret = -1;
	}

	if ((!gstrcmp(name, T("check"))) || (!gstrcmp(name, T("list")))) {
		fp = popen
		    ("for i in `ls -d /mnt/usb/*/Disc* 2>/dev/null`; do echo \"$i\" | sed 's/\\/mnt\\/usb//g'; done",
		     "r");
		if (fp == NULL) {
#ifdef IFX_LOG_DEBUG
			IFX_DBG
			    ("[%s:%d] failed to list directories of a mounted USB path",
			     __FUNCTION__, __LINE__);
#endif
			ret = -1;
			ifx_httpdWrite(wp, T("%d"), ret);

		} else {
			if (!gstrcmp(name, T("check"))) {
				if (fgets(usbpath, PART_MAX, fp) != NULL) {
					ret = 1;
					ifx_httpdWrite(wp, T("%d"), ret);
				}
			} else {
				while (fgets(usbpath, PART_MAX, fp) != NULL) {
					usbpath[strlen(usbpath) - 1] = '\0';
					ifx_httpdWrite(wp,
						       T
						       ("<option value=\"%s\">%s</option>"),
						       usbpath, usbpath);
					//value++;
				}
			}
			pclose(fp);
			ret = 1;
		}
	} else if ((!gstrcmp(name, T("check_sysconfig_file")))
		   || (!gstrcmp(name, T("list_sysconfig_file")))) {
		fp = popen
		    ("for i in `ls /mnt/usb/*/Disc*/rc.conf.gz 2>/dev/null`; do echo \"$i\" | sed 's/\\/mnt\\/usb//g'; done",
		     "r");
		if (fp == NULL) {
#ifdef IFX_LOG_DEBUG
			IFX_DBG
			    ("[%s:%d] failed to list directories which contain rc.conf.gz file under mounted USB paths",
			     __FUNCTION__, __LINE__);
#endif
			ret = -1;
			ifx_httpdWrite(wp, T("%d"), ret);
		} else {
			if (!gstrcmp(name, T("check_sysconfig_file"))) {
				if (fgets(usbpath, PART_MAX, fp) != NULL) {
					ret = 1;
					ifx_httpdWrite(wp, T("%d"), ret);
				}
			} else {
				while (fgets(usbpath, PART_MAX, fp) != NULL) {
					usbpath[strlen(usbpath) - 1] = '\0';
					ifx_httpdWrite(wp,
						       T
						       ("<option value=\"%s\">%s</option>"),
						       usbpath, usbpath);
					//value++;
				}
			}
			pclose(fp);
			ret = 1;
		}
	}
	return ret;
}

//ifx_httpdWrite(wp, T("\t\t\t\t\t<option name=\"sroute_%s\" value=\"%s\">%s</option>"),GET_WAN_CONN_NAME(wan_conn_names, i), GET_WAN_CONN_NAME(wan_conn_names, i), GET_WAN_CONN_NAME(wan_conn_names, i));

/*****************************************************************************
*  Name: ifx_set_backupto_usb
*  Function: This functions un-mounts the USB device
*  Input: u
*  Output:
*  Return Value:
******************************************************************************/
int32 ifx_set_backupto_usb(httpd_t wp, char_t * path, char_t * query)
{
	int flags = IFX_F_DEFAULT;
	char_t mnt_buf[PART_MAX];
	char_t *pmnt_path = ifx_httpdGetVar(wp, T("sel_mnt_path"), T(""));

	/* Check if USB mount path exists */
	sprintf(mnt_buf, "ls /mnt/usb%s/ >/dev/null", pmnt_path);
	if (system(mnt_buf) != 0) {
		usb_ret_status = 2;
		websNextPage(wp);
		return -1;
	}

	/* Backup current rc.conf */
	ifx_mapi_backup_config(flags);

	/* Copy rc.conf.gz to USB */
	sprintf(mnt_buf, "cp -f /tmp/rc.conf.gz /mnt/usb%s/", pmnt_path);
	if (system(mnt_buf) != 0) {
		usb_ret_status = 2;
		websNextPage(wp);
		return -1;
	}

	usb_ret_status = 1;
	websNextPage(wp);
	return 0;
}

int32 ifx_set_restore_from_usb(httpd_t wp, char_t * path, char_t * query)
{
	int flags = IFX_F_DEFAULT;
	struct stat file_status;
	int iTotalSize = 0;
	FILE *fp;
	int ret = 0;
	int iMaxFileSizeAllowed = 0;
	int iMinFileSizeAllowed = 0;
	char_t mnt_buf[PART_MAX];

	char_t *psyscfg_file =
	    ifx_httpdGetVar(wp, T("sel_mnt_sysconf_file"), T(""));

	iMaxFileSizeAllowed = CONFIG_UBOOT_CONFIG_IFX_MEMORY_SIZE * 1024;
#ifdef IFX_LOG_DEBUG
	IFX_DBG("CONFIG_UBOOT_CONFIG_IFX_MEMORY_SIZE %d\n",
		CONFIG_UBOOT_CONFIG_IFX_MEMORY_SIZE);
#endif
/*	if(CONFIG_IFX_CONFIG_FLASH_SIZE == 2)
		iMaxFileSizeAllowed = 16*1024; // max size allowed for download need to move to header file
	if(CONFIG_IFX_CONFIG_FLASH_SIZE == 4)
		iMaxFileSizeAllowed = 32*1024; // max size allowed for download need to move to header file 
	if(CONFIG_IFX_CONFIG_FLASH_SIZE == 8)
		iMaxFileSizeAllowed = 32*1024; // max size allowed for download need to move to header file 
*/
#ifndef CONFIG_FEATURE_SELECTIVE_BACKUP_RESTORE
	iMinFileSizeAllowed = 3 * 1024;	// max size allowed for download need to move to header file
#endif

	/* Check if rc.conf.gz exists */
	sprintf(mnt_buf, "ls /mnt/usb%s 2>/dev/null | grep -w rc.conf.gz",
		psyscfg_file);
	if (system(mnt_buf) != 0) {
		usb_ret_status = 2;
		websNextPage(wp);
		return -1;
	}

	/* copy rc.conf.gz /tmp/ */
	sprintf(mnt_buf, "cp -f /mnt/usb%s /tmp/", psyscfg_file);
	if (system(mnt_buf) != 0) {
		usb_ret_status = 2;
		websNextPage(wp);
		return -1;
	}

	/* check for length */
	fp = popen("du -b /tmp/rc.conf.gz", "r");
	if (fp == NULL) {
		ifx_httpdError(wp, 500, T("Cannot open %s file"),
			       "/tmp/rc.conf.gz");
		usb_ret_status = 2;
		websNextPage(wp);
	//	pclose(fp);
		return -1;
	} else {
		if (stat("/tmp/rc.conf.gz", &file_status) != 0) {
			IFX_DBG("error......\n");
		}
		iTotalSize = file_status.st_size;
		if ((iTotalSize > iMaxFileSizeAllowed)
		    || (iTotalSize < iMinFileSizeAllowed)) {
			//ifx_httpdError(wp, 500, T("<span class="textTitle">Incorrect configuration file.</span><br>File size greater or less than expected<br>\n"));
			COPY_TO_STATUS
			    ("<span class=\"textTitle\">Incorrect configuration file.</span><br><p>File size greater or less than expected</p><br>\n");
			ifx_httpdRedirect(wp, T("err_page.html"));
			IFX_DBG
			    ("Incorrect configuration file. File size greater than expected<br>\n");
			pclose(fp);
			if (system("rm -rf /tmp/rc.conf.gz") != 0) {
				usb_ret_status = 2;
				websNextPage(wp);
				return -1;
			}
			usb_ret_status = 2;
			return ret;
		}

		pclose(fp);
	}
	/*length check end */

        system("mv /tmp/rc.conf.gz /tmp/sysconf.gz");

	/* TBD - Pramod : Need to align the return code handling as done in ifx_set_sysconfig_upgrade */
	/* Factory config update is always 0 ? */
	if (ifx_mapi_restore_config(0, flags) != 0) {
		usb_ret_status = 2;
		websNextPage(wp);
		return -1;
	}
	/*
	   if (system("/etc/rc.d/backup") != 0){
	   usb_ret_status= 3;
	   ifx_httpdNextPage (wp);
	   return -1;
	   }
	 */
	system("/etc/rc.d/backup");
	if (system("/etc/rc.d/rebootcpe.sh 5 &") != 0) {
		usb_ret_status = 3;
		websNextPage(wp);
		return -1;
	}

IFX_Handler:
	IFX_DBG("Configuration file restore aborted \n");

	websNextPage(wp);
	return 0;
}

int32 ifx_get_backupto_usb(int eid, httpd_t wp, int argc, char_t ** argv)
{
	char_t *name, *mode;
	if (ifx_httpd_parse_args(argc, argv, T("%s%s"), &name, &mode) < 1) {
		ifx_httpdError(wp, 400, T("Insufficient args\n"));
		return -1;
	}
	if (!gstrcmp(name, T("status"))) {
		ifx_httpdWrite(wp, T("%d"), usb_ret_status);
		usb_ret_status = 0;

	}
	if (!gstrcmp(name, T("MountStatus"))) {
		ifx_httpdWrite(wp, T("%d"), vun_UsbMountStatus);
		vun_UsbMountStatus = 0;
	}

	return 0;
}
#endif

//system_ssl.asp
void ifx_set_ssl_cert_upgrade(httpd_t wp)
{
	int iret = IFX_SUCCESS;
	char buf_cmd[128] = { 0 };

	iret = ifx_httpdGetVarMbuff(wp, T("certificate_key"), T(""));
	if (iret != IFX_SUCCESS) {
		ifx_httpdError(wp, 500, "Fail to save Certificate");
		system("rm -f /tmp/certificate.txt");
		return;

	} else {

		//sprintf(buf_cmd, "mv /tmp/certificate.txt %s",
		//	SSL_CERT_FILE_NAME);
		sprintf(buf_cmd, "tr -d '\\r' < /tmp/certificate.txt > %s",
			SSL_CERT_FILE_NAME);

		system(buf_cmd);

	}
	system("/etc/rc.d/backup");

	websNextPage(wp);
}

void ifx_get_ssl_cert(int eid, httpd_t wp, int argc, char_t ** argv)
{

	// Read the Cert from File into a Buffer
	FILE *fp;
	char ptr[256] = { 0 };
	int line_size = 256;
	fp = fopen(SSL_CERT_FILE_NAME, "r");
	if (fp == NULL) {
		IFX_DBG("[%s:%d]", __FUNCTION__, __LINE__);
		return;
	}
	while (fgets(ptr, line_size, fp)) {
		ifx_httpdWrite(wp, T("%s"), ptr);
	}
	fclose(fp);
}

//end system_ssl.asp

//system_reset.htm
void ifx_set_system_reset(httpd_t wp, char_t * path, char_t * query)
{
	//  char_t  sCommand[MAX_DATA_LEN] = "(sleep 3;reboot)&";
	char_t *pFactory = ifx_httpdGetVar(wp, T("factory"), T(""));
	reboot_status = 3;
#if defined PLATFORM_VBX
	ifx_httpdRedirect(wp, "reboot_vb300.html");
	if (pFactory && pFactory[0] == '1') {	//165001:henryhsu:20050822:Modify for reset to default fail.
		system("/usr/sbin/factorycfg.sh &");
	} else {
		system("/etc/rc.d/rebootcpe.sh 10 &");
	}
#else
	ifx_httpdRedirect(wp, "reboot_cpe.html");
	if (pFactory && pFactory[0] == '1') {	//165001:henryhsu:20050822:Modify for reset to default fail.
		system("/usr/sbin/upgrade /etc/voip.conf.gz voip 0 0");	//000046:jelly
		//system("/usr/sbin/upgrade /etc/rc.conf.gz sysconfig 0 0");//509071:tc.chen
		system("/etc/rc.d/restore_tr69_defaults");	//509071:tc.chen
	}
	//   ifx_httpdWrite(wp,T("<center><h3> It may take some time to complete this process and restart the system.\n Please don't turn off the CPE before the process is complete.<br></h3></center>"));
	system("/etc/rc.d/rebootcpe.sh 10 &");
#endif
	return;
	//    system(sCommand);
}

//end syistem_reset.htm
//FILE *TestFile;
//FILE *NewFile;
//char datac[512];
//system_upgrade.asp : sanju
void ifx_set_system_upgrade(httpd_t wp, char_t * path, char_t * query)
{
	//char_t img_type[16];
	//int iExapndDir = 0;
	char errorMsg[128];
        //int len=0;
	memset(errorMsg, 0x00, sizeof(errorMsg));
	//ifx_httpdError(wp, 400, " printf from sanju\n");
	//system("echo printf from sanju");
	//if(path!=NULL)
	//{
	//	ifx_httpdError(wp, 400, "path of the file\n");
	//	puts("Name of File ----\n");
	//	puts(FILE_IMAGE_KERNEL);
	//	puts("--------");
		//system("echo printf from sanju");
//	}
//	if (ifx_chkImage(FILE_IMAGE_KERNEL, errorMsg, img_type, &iExapndDir)) {
//#define BOOTMODE 0              //Added by sanju 0- Pirimary,1-secondary
#if 0
		system("/etc/rc.d/bootmodeset.sh");	//setting the bootmode
		reboot_status = 2;
		ifx_httpdRedirect(wp, "reboot_cpe.html");	//sleep(15);
//		 ifx_httpdRedirect(wp, T("reboot_cpe.html"));
                sleep(5);
		system("/etc/rc.d/rebootcpe.sh 10 &");
//           ifx_httpdError(wp, 500,"%s",errorMsg);
           	system("killall FAXManager LTEManager");
		system("reboot");
#else
		/*Write the upgrade section and boot reset section here*/
		reboot_status=2;
                system("echo 8 > /dev/fifo_led");
		ifx_httpdRedirect(wp, "reboot_cpe.html");       //sleep(15);
                sleep(5);
		system("/etc/rc.d/upgradeimage.sh");     //setting the bootmode
		system("echo 9 > /dev/fifo_led");
		system("/etc/rc.d/rebootcpe.sh 10 &");
		ifx_httpdRedirect(wp, "upgradesuccess.html");
		sleep(5);
		system("reboot");

#endif
//		return;
//	}
//	NewFile=fopen(FILE_IMAGE_KERNEL,"r");
//	puts(FILE_IMAGE_KERNEL);
//	TestFile=fopen("/tmp/newfile","ab");
//	if(NewFile>0)
//	{
//		while(1)
//		{
//			len=fread(datac,512,1,NewFile);
//			if(len>0)
//			{
////				fwrite(datac,len,1,TestFile);
//				puts("Written data-----\n");
//				//puts(datac);
//				puts("---------------");
//			}
//			else
//			{
//				puts("Copying Successful: sanju\n");
//				break;
//			}
//		}

//	}
	
	//fprintf(TestFile,"This is the test printing Now\n");
//	fclose(TestFile);
//	fclose(NewFile);
	// Display update result
	//websNextPage(wp);
	//reboot_status = 1;
//	ifx_httpdRedirect(wp, T("reboot_cpe.html"));
//	ifx_invokeUpgd(FILE_IMAGE_KERNEL, img_type, iExapndDir);
//	ifx_httpdError(wp, 400, "exiting file\n");
	//system("echo printf from sanju");
	return;
}

int ifx_get_FirmwareVer(int eid, httpd_t wp, int argc, char_t ** argv)
{
	char_t *name;
	struct ifx_version_info ver;

	if (ifx_httpd_parse_args(argc, argv, T("%s"), &name) < 1) {
		ifx_httpdError(wp, 400, T("Insufficient args\n"));
		return -1;
	}

	memset(&ver, 0x00, sizeof(ver));

	if (ifx_get_version_info(&ver) == IFX_SUCCESS) {
		if (!gstrcmp(name, "-b"))
			ifx_httpdWrite(wp, "%s", ver.BOOTLoader);
		if (!gstrcmp(name, "-l"))
			ifx_httpdWrite(wp, "%s", ver.BSP);
		if (!gstrcmp(name, "-r"))
			ifx_httpdWrite(wp, "%s", ver.Software);
		if (!gstrcmp(name, "-t"))
			ifx_httpdWrite(wp, "%s", ver.Tool_Chain);
		if (!gstrcmp(name, "-f")) {
#ifdef IFX_LOG_DEBUG
			IFX_DBG("[%s:%d] fw version = %s length=%d",
				__FUNCTION__, __LINE__, ver.Firmware,
				strlen(ver.Firmware));
#endif
			ver.Firmware[strlen(ver.Firmware) - 1] = '\0';
			ifx_httpdWrite(wp, "%s", ver.Firmware);
		}
		if (!gstrcmp(name, "-c"))
			ifx_httpdWrite(wp, "%s", ver.CPU);

		if (!gstrcmp(name, "-w"))
			ifx_httpdWrite(wp, "%s", ver.WAVE300_CV);
	}
	return 0;
}

//end system_upgrade.asp

//system_log.asp
#ifdef CONFIG_FEATURE_SYSTEM_LOG
int ifx_get_Securitylog(int eid, httpd_t wp, int argc, char_t ** argv)
{
	char_t sCommand[MAX_FILELINE_LEN];
	char_t sValue[MAX_FILELINE_LEN];
	char_t *name = NULL;
	int nLine = 0;		// nReturn;
	FILE *fp;
	SYSLOG_INFO sys_log;

	memset(sValue, 0x00, MAX_FILELINE_LEN);
	memset(&sys_log, 0x00, sizeof(sys_log));
	sys_log.buf = NULL;

	if (ifx_httpd_parse_args(argc, argv, T("%s"), &name) < 1) {
		ifx_httpdError(wp, 400, T("Insufficient args\n"));
		return -1;
	}

	if (ifx_get_syslog_info(&sys_log, IFX_F_DEFAULT) != IFX_SUCCESS) {
		ifx_httpdError(wp, 400,
			       T
			       ("Failed to syslog info for remote log support\n"));
		return -1;
	}

	if (!strcmp(name, "rlog_ip")) {
		ifx_httpdWrite(wp, T("%s"), inet_ntoa(sys_log.remote_ip));
	} else if (!strcmp(name, "log_mode")) {
		ifx_httpdWrite(wp, T("%d"), sys_log.mode);
	} else if (!strcmp(name, "rlog_port")) {
		ifx_httpdWrite(wp, T("%d"), sys_log.port);
	} else {
		if (ifx_get_syslog_info_filter
		    (log_disp_level, &sys_log, IFX_F_DEFAULT) != IFX_SUCCESS) {
			ifx_httpdError(wp, 400,
				       T
				       ("Failed to get syslog filter info \n"));
			return -1;
		}

		if (sys_log.buf != NULL) {	/* no log display for only remote logging */
			/* write the buffer to a temporary file, get the last APPS_SECURITYLOG_NUM of lines from the
			   temporary file and display on the web page */
			fp = fopen("/tmp/filter_syslog", "w");
			if (fp == NULL) {
				ifx_httpdError(wp, 400,
					       T
					       ("Failed to get log messages for log level specified\n"));
				return -1;
			}
			fwrite(sys_log.buf, strlen(sys_log.buf), 1, fp);
			fclose(fp);

			gsprintf(sCommand, T("cat  %d /tmp/filter_syslog"),
				 APPS_SECURITYLOG_NUM * 4);
			fp = popen(sCommand, "r");
			ifx_httpdWrite(wp, "<div align=\"center\">");
			ifx_httpdWrite(wp,
				       "<table class=\"tableInfo\" cellspacing=\"1\" cellpadding=\"6\" summary=\"\">");
			do {
				nLine++;
				if ((fp)
				    && (fgets(sValue, sizeof(sValue), fp) ==
					NULL))
					break;
				ifx_httpdWrite(wp, "<tr>");
				ifx_httpdWrite(wp, "<td>");
				if (strstr(sValue, ".info"))
					ifx_httpdWrite(wp,
						       "<font size=\"2\" color=\"#000000\">");
				else if (strstr(sValue, ".debug"))
					ifx_httpdWrite(wp,
						       "<font size=\"2\" color=\"#00aa00\">");
				else if (strstr(sValue, ".emerg"))
					ifx_httpdWrite(wp,
						       "<font size=\"2\" color=\"#ff0000\">");
				else if (strstr(sValue, ".alert"))
					ifx_httpdWrite(wp,
						       "<font size=\"2\" color=\"#0000ff\">");
				else if (strstr(sValue, ".crit"))
					ifx_httpdWrite(wp,
						       "<font size=\"2\" color=\"#aa0000\">");
				else if (strstr(sValue, ".err"))
					ifx_httpdWrite(wp,
						       "<font size=\"2\" color=\"#ff0060\">");
				else if (strstr(sValue, ".warn"))
					ifx_httpdWrite(wp,
						       "<font size=\"2\" color=\"#600a00\">");
				else if (strstr(sValue, ".notice"))
					ifx_httpdWrite(wp,
						       "<font size=\"2\" color=\"#00aa70\">");
				else
					ifx_httpdWrite(wp,
						       "<font size=\"2\" color=\"#000000\">");

				ifx_httpdWrite(wp, T("%s"), sValue);
				ifx_httpdWrite(wp, "</font>");
				ifx_httpdWrite(wp, "</td>");
				ifx_httpdWrite(wp, "</tr>");

			} while (1);
			ifx_httpdWrite(wp, "</font>");
			ifx_httpdWrite(wp, "</table>");
			ifx_httpdWrite(wp, "</font></div>");
			fclose(fp);

			IFX_MEM_FREE(sys_log.buf)
		}
	}

	return 0;
}

int ifx_get_Securitylog_modified(int eid, httpd_t wp, int argc, char_t ** argv)
{
	char_t sCommand[MAX_FILELINE_LEN];
	char_t sValue[MAX_FILELINE_LEN];
	char_t *name = NULL;
	int nLine = 0;		// nReturn;
	FILE *fp;
	SYSLOG_INFO sys_log;

	memset(&sys_log, 0x00, sizeof(sys_log));
	sys_log.buf = NULL;

	if (ifx_httpd_parse_args(argc, argv, T("%s"), &name) < 1) {
		ifx_httpdError(wp, 400, T("Insufficient args\n"));
		return -1;
	}
	if (ifx_get_syslog_info_filter(log_disp_level, &sys_log, IFX_F_DEFAULT)
	    != IFX_SUCCESS) {
		ifx_httpdError(wp, 400,
			       T("Failed to get syslog filter info\n"));
		return -1;
	}

	if (!strcmp(name, "log_info")) {
		ifx_httpdWrite(wp, T("%s"), "<td width='50%'>");
		ifx_httpdWrite(wp,
			       "<input type='radio' name='log_mode' value='0' %s>Local</td>",
			       sys_log.mode == SYSLOG_LOCAL ? "checked" : "");
		ifx_httpdWrite(wp, T("%s"), "<td width='50%'>");
		ifx_httpdWrite(wp,
			       "<input type='radio' name='log_mode' value='1' %s>Remote</td>",
			       sys_log.mode == SYSLOG_REMOTE ? "checked" : "");
		ifx_httpdWrite(wp, T("%s"), "<td width='50%'>");
		ifx_httpdWrite(wp, T("%s"),
			       "<input type='radio' name='log_mode' value='2' %s>Local and Remote</td>",
			       sys_log.mode ==
			       SYSLOG_BOTH_LOCAL_REMOTE ? "checked" : "");
	} else if (!strcmp(name, "remote_info")) {
		if (sys_log.mode == SYSLOG_REMOTE
		    || sys_log.mode == SYSLOG_BOTH_LOCAL_REMOTE) {
			ifx_httpdWrite(wp,
				       "<td width='50%'>IP Address</td><td width='50%'>");
			ifx_httpdWrite(wp,
				       "<input type='text' name='rlog_ip' size='15' maxlength='15' value='%s'>",
				       inet_ntoa(sys_log.remote_ip));
			ifx_httpdWrite(wp, "</td></tr><tr align='left'>");
			ifx_httpdWrite(wp,
				       "<td width='50%'>UDP Port</td><td width='50%'>");
			ifx_httpdWrite(wp,
				       "<input type='text' name='rlog_port' size='5' maxlength='5' value='%d'></td>",
				       sys_log.port);
		}
	} else if (!strcmp(name, "rlog_ip")) {
		if (sys_log.mode == SYSLOG_REMOTE
		    || sys_log.mode == SYSLOG_BOTH_LOCAL_REMOTE)
			ifx_httpdWrite(wp, T("%s"),
				       inet_ntoa(sys_log.remote_ip));
	} else if (!strcmp(name, "log_mode")) {
		ifx_httpdWrite(wp, T("%d"), sys_log.mode);
	} else if (!strcmp(name, "rlog_port")) {
		if (sys_log.mode == SYSLOG_REMOTE
		    || sys_log.mode == SYSLOG_BOTH_LOCAL_REMOTE)
			ifx_httpdWrite(wp, T("%d"), sys_log.port);
	} else {
		if (sys_log.mode != SYSLOG_REMOTE && sys_log.buf != NULL) {	/* no log display for only remote logging */
			/* write the buffer to a temporary file, get the last APPS_SECURITYLOG_NUM of lines from the
			   temporary file and display on the web page */
			fp = fopen("/tmp/filter_syslog", "w");
			if (fp == NULL) {
				ifx_httpdError(wp, 400,
					       T
					       ("Failed to get log messages for log level specified\n"));
				return -1;
			}
			fwrite(sys_log.buf, strlen(sys_log.buf), 1, fp);
			fclose(fp);

			gsprintf(sCommand, T("tail -n %d /tmp/filter_syslog"),
				 APPS_SECURITYLOG_NUM * 4);
			fp = popen(sCommand, "r");
			do {
				nLine++;
				if ((fp)
				    && (fgets(sValue, sizeof(sValue), fp) ==
					NULL))
					break;
				ifx_httpdWrite(wp, T("%s<BR>"), sValue);
			} while (1);
			fclose(fp);
			IFX_MEM_FREE(sys_log.buf)
		}
	}

	return 0;
}

/* This function will either print all the supported display levels for syslog or
        will filter the syslog output on the web page based on the display level specified */
int ifx_get_filter_log(int eid, httpd_t wp, int argc, char_t ** argv)
{
	char_t *name = NULL;
	SYSLOG_INFO sys_log;

	memset(&sys_log, 0x00, sizeof(sys_log));
	sys_log.buf = NULL;

	if (ifx_httpd_parse_args(argc, argv, T("%s"), &name) < 1) {
		ifx_httpdError(wp, 400, T("Insufficient args\n"));
		return -1;
	}

	if (!gstrcmp(name, "conf_log_levels")) {
		if (ifx_get_syslog_info(&sys_log, IFX_F_DEFAULT) != IFX_SUCCESS) {
			ifx_httpdError(wp, 400,
				       T("Failed to get syslog info \n"));
			return -1;
		}

		/* just write the display levels supported */
		ifx_httpdWrite(wp, T("<option value='%d' %s>Default</option>"),
			       SYSLOG_DISP_LEVEL_DEFAULT,
			       ((sys_log.log_level) ==
				SYSLOG_DISP_LEVEL_DEFAULT) ? "selected" : " ");
		ifx_httpdWrite(wp,
			       T("<option value='%d' %s>Emergency</option>"),
			       SYSLOG_DISP_LEVEL_EMERG,
			       ((sys_log.log_level) ==
				SYSLOG_DISP_LEVEL_EMERG) ? "selected" : " ");
		ifx_httpdWrite(wp, T("<option value='%d' %s>Alert</option>"),
			       SYSLOG_DISP_LEVEL_ALERT,
			       ((sys_log.log_level) ==
				SYSLOG_DISP_LEVEL_ALERT) ? "selected" : " ");
		ifx_httpdWrite(wp, T("<option value='%d' %s>Critical</option>"),
			       SYSLOG_DISP_LEVEL_CRIT,
			       ((sys_log.log_level) ==
				SYSLOG_DISP_LEVEL_CRIT) ? "selected" : " ");
		ifx_httpdWrite(wp, T("<option value='%d' %s>Error</option>"),
			       SYSLOG_DISP_LEVEL_ERR,
			       ((sys_log.log_level) ==
				SYSLOG_DISP_LEVEL_ERR) ? "selected" : " ");
		ifx_httpdWrite(wp, T("<option value='%d' %s>Warning</option>"),
			       SYSLOG_DISP_LEVEL_WARN,
			       ((sys_log.log_level) ==
				SYSLOG_DISP_LEVEL_WARN) ? "selected" : " ");
		ifx_httpdWrite(wp, T("<option value='%d' %s>Notice</option>"),
			       SYSLOG_DISP_LEVEL_NOTICE,
			       ((sys_log.log_level) ==
				SYSLOG_DISP_LEVEL_NOTICE) ? "selected" : " ");
		ifx_httpdWrite(wp, T("<option value='%d' %s>Info</option>"),
			       SYSLOG_DISP_LEVEL_INFO,
			       ((sys_log.log_level) ==
				SYSLOG_DISP_LEVEL_INFO) ? "selected" : " ");
		ifx_httpdWrite(wp, T("<option value='%d' %s>Debug</option>"),
			       SYSLOG_DISP_LEVEL_DEBUG,
			       ((sys_log.log_level) ==
				SYSLOG_DISP_LEVEL_DEBUG) ? "selected" : " ");
	} else if (!gstrcmp(name, "disp_log_levels")) {
		/* just write the display levels supported */
		ifx_httpdWrite(wp,
			       T
			       ("\t\t\t\t<option value=\"%d\" selected>%s</option>"),
			       SYSLOG_DISP_LEVEL_DEFAULT, "Default");
		ifx_httpdWrite(wp,
			       T("\t\t\t\t<option value=\"%d\">%s</option>"),
			       SYSLOG_DISP_LEVEL_EMERG, "Emergency");
		ifx_httpdWrite(wp,
			       T("\t\t\t\t<option value=\"%d\">%s</option>"),
			       SYSLOG_DISP_LEVEL_ALERT, "Alert");
		ifx_httpdWrite(wp,
			       T("\t\t\t\t<option value=\"%d\">%s</option>"),
			       SYSLOG_DISP_LEVEL_CRIT, "Critical");
		ifx_httpdWrite(wp,
			       T("\t\t\t\t<option value=\"%d\">%s</option>"),
			       SYSLOG_DISP_LEVEL_ERR, "Error");
		ifx_httpdWrite(wp,
			       T("\t\t\t\t<option value=\"%d\">%s</option>"),
			       SYSLOG_DISP_LEVEL_WARN, "Warning");
		ifx_httpdWrite(wp,
			       T("\t\t\t\t<option value=\"%d\">%s</option>"),
			       SYSLOG_DISP_LEVEL_NOTICE, "Notice");
		ifx_httpdWrite(wp,
			       T("\t\t\t\t<option value=\"%d\">%s</option>"),
			       SYSLOG_DISP_LEVEL_INFO, "Info");
		ifx_httpdWrite(wp,
			       T("\t\t\t\t<option value=\"%d\">%s</option>"),
			       SYSLOG_DISP_LEVEL_DEBUG, "Debug");
	}
	IFX_MEM_FREE(sys_log.buf)
	    return 0;
}
#endif

#ifdef CONFIG_FEATURE_SYSTEM_LOG
void ifx_set_system_log(httpd_t wp, char_t * path, char_t * query)
{
	int log_conf_level = 7;
	char_t sCommand[MAX_FILELINE_LEN];
	char_t *pClear = ifx_httpdGetVar(wp, T("securityclear"), T(""));
	char_t *pAction = ifx_httpdGetVar(wp, T("logAction"), T(""));
	char_t *pShowLog = ifx_httpdGetVar(wp, T("show_syslog"), T(""));
	SYSLOG_INFO sys_log;

	memset(&sys_log, 0x00, sizeof(sys_log));
	if (!gstrcmp(pAction, "Please wait..")) {
		char_t *prlogMode = ifx_httpdGetVar(wp, T("log_mode"), T(""));
		char_t *prlogIP = ifx_httpdGetVar(wp, T("rlog_ip"), T(""));
		char_t *prlogPort = ifx_httpdGetVar(wp, T("rlog_port"), T(""));
		log_conf_level =
		    atoi(ifx_httpdGetVar(wp, T("syslog_conf_level"), T("")));

		/* copy data read from asp to sys_log structure */
		sys_log.mode = atoi(prlogMode);
		global_log_mode = sys_log.mode;
		sys_log.port = atoi(prlogPort);
		sys_log.remote_ip.s_addr = inet_addr(prlogIP);
		sys_log.log_level = log_conf_level;
		sys_log.buf = NULL;

		if (ifx_set_syslog_info(IFX_OP_MOD, &sys_log, IFX_F_MODIFY) !=
		    IFX_SUCCESS) {
			ifx_httpdError(wp, 500, T("Failed to save setting"));
			return;
		}
	}
	if (!gstrcmp(pShowLog, "Loading..")) {
		log_disp_level =
		    atoi(ifx_httpdGetVar(wp, T("syslog_disp_level"), T("")));
		ifx_httpdRedirect(wp, T("system_log_view.asp"));
	}
	if (!gstrcmp(pClear, T("Clear"))) {
		gsprintf(sCommand,
			 "rm -f /var/log/messages /tmp/filter_syslog");
		system(sCommand);
		gsprintf(sCommand, "echo " ">/tmp/filter_syslog");
		system(sCommand);
		ifx_httpdRedirect(wp, T("system_log_view.asp"));
	}
	websNextPage(wp);
	return;
}
#endif
//end system_log.asp

void ifx_get_systemtime(int eid, httpd_t wp, int argc, char_t ** argv)
{
	struct tm *ptr;
	time_t lt;
	lt = time(NULL);
	ptr = localtime(&lt);
	if (ptr)
		ifx_httpdWrite(wp, T("%s"), asctime(ptr));
}

#ifdef CONFIG_PACKAGE_NTPCLIENT
//////////////////////////////////////////////////////////////////////////////////
// wizard_tz.asp
//

CGI_ENUMSEL_S web_Enum_NTP_Server_List[] = {
	{0, "time.windows.com"}
	,
	{1, "time.nist.gov"}
	,
	{2, "time-a.nist.gov"}
	,
	{3, "time-b.nist.gov"}
	,
	{4, "time-a.timefreq.bldrdoc.gov"}
	,
	{5, "time-b.timefreq.bldrdoc.gov"}
	,
	{6, "time-c.timefreq.bldrdoc.gov"}
	,
	{7, "time-nw.nist.gov"}
};

static char_t ntpcSValue[MAX_FILELINE_LEN];

char_t *ifx_web_GetNTPServerByIndex(int index)
{
	char_t *ret = NULL;// sValue[MAX_FILELINE_LEN];;
	NTP_CLIENT_CFG ntpC;
	memset(&ntpC, 0x00, sizeof(ntpC));

	if (ifx_get_ntp_client_cfg(&ntpC, IFX_F_DEFAULT) != IFX_SUCCESS) {
		return ret;
	}

	if (index >= 0
	    && index < sizeof(web_Enum_NTP_Server_List) / sizeof(CGI_ENUMSEL_S))
		ret = web_Enum_NTP_Server_List[index].str;
	else {
		gsprintf(ntpcSValue, "%s", ntpC.ntpServer1);
		ret = ntpcSValue;
	}
	return ret;
}

//vipul start
char_t *ifx_web_GetNTPServerByIndex1(int index)
{
	char_t *ret = NULL;// sValue[MAX_FILELINE_LEN];;
	NTP_CLIENT_CFG ntpC;
	memset(&ntpC, 0x00, sizeof(ntpC));

	if (ifx_get_ntp_client_cfg(&ntpC, IFX_F_DEFAULT) != IFX_SUCCESS) {
		return ret;
	}

	if (index >= 0
	    && index < sizeof(web_Enum_NTP_Server_List) / sizeof(CGI_ENUMSEL_S))
		ret = web_Enum_NTP_Server_List[index].str;
	else {
		gsprintf(ntpcSValue, "%s", ntpC.ntpServer2);
		ret = ntpcSValue;
	}
	return ret;
}

//vipul end

int ifx_get_TimeSetting(int eid, httpd_t wp, int argc, char_t ** argv)
{
	char_t *name, sValue[MAX_FILELINE_LEN];
	int nIndex, nSelIndex, flag = 0;
	NTP_CLIENT_CFG ntpC;

	if (ifx_httpd_parse_args(argc, argv, T("%s"), &name) < 1) {
		ifx_httpdError(wp, 400, T("Insufficient args\n"));
		return -1;
	}

	memset(&ntpC, 0x00, sizeof(ntpC));

	if (ifx_get_ntp_client_cfg(&ntpC, IFX_F_DEFAULT) != IFX_SUCCESS) {
		ifx_httpdError(wp, 500, T("Failed to get NTP Configurations"));
		return -1;
	}

	if (!gstrcmp(name, T("TimeZone"))) {
		nSelIndex = ntpC.timeMinutesOffset;
		for (nIndex = 0;
		     nIndex <
		     sizeof(web_Enum_TimeZoneList) / sizeof(CGI_ENUMSEL_S);
		     nIndex++) {
			// Get Selected option index form admweb.conf
			ifx_httpdWrite(wp,
				       T
				       ("\t<option  name=\"GMT%d\" value=\"%d\" %s>%s</option>\n"),
				       nIndex, nIndex,
				       (web_Enum_TimeZoneList[nIndex].value ==
					nSelIndex) ? "selected" : "",
				       web_Enum_TimeZoneList[nIndex].str);
		}
	}			// endif TimeZone
	else if (!gstrcmp(name, T("NTP_Client"))) {
		ifx_httpdWrite(wp, T("%s"),
			       (ntpC.f_enable == 1) ? "checked" : "");
	}			// NTP_Client
	else if (!gstrcmp(name, T("NTP_Server_List"))) {
		for (nIndex = 0;
		     nIndex <
		     sizeof(web_Enum_NTP_Server_List) / sizeof(CGI_ENUMSEL_S);
		     nIndex++) {
			gsprintf(sValue, "%s", ntpC.ntpServer1);
			// Get Selected option index form admweb.conf
			if (!strcmp
			    (sValue, web_Enum_NTP_Server_List[nIndex].str)) {
				gsprintf(sValue, T("%s"), "selected");
				flag = 1;
			} else
				gsprintf(sValue, T("%s"), "");
			ifx_httpdWrite(wp,
				       T
				       ("\t<option name=\"psntp_serv%d\" value=\"%d\" %s>%s</option>\n"),
				       nIndex, nIndex, sValue,
				       web_Enum_NTP_Server_List[nIndex].str);
		}

		if (flag == 0) {
			ifx_httpdWrite(wp,
				       T
				       ("\t<option name=\"ssntp_serv%d\" value=\"%d\" %s>%s</option>\n"),
				       nIndex, nIndex, "selected",
				       ntpC.ntpServer1);
		}

	}			// NTP_Server
	else
		ifx_httpdError(wp, 400,
			       T("Invalidate argument to get Time Setting"));
	return 0;
}

//vipul start

int ifx_get_TimeSetting1(int eid, httpd_t wp, int argc, char_t ** argv)
{
	char_t *name, sValue[MAX_FILELINE_LEN];
	int nIndex, nSelIndex, flag = 0;
	NTP_CLIENT_CFG ntpC;

	if (ifx_httpd_parse_args(argc, argv, T("%s"), &name) < 1) {
		ifx_httpdError(wp, 400, T("Insufficient args\n"));
		return -1;
	}

	memset(&ntpC, 0x00, sizeof(ntpC));

	if (ifx_get_ntp_client_cfg(&ntpC, IFX_F_DEFAULT) != IFX_SUCCESS) {
		ifx_httpdError(wp, 500, T("Failed to get NTP Configurations"));
		return -1;
	}

	if (!gstrcmp(name, T("TimeZone"))) {
		nSelIndex = ntpC.timeZoneIdx;
		for (nIndex = 0;
		     nIndex <
		     sizeof(web_Enum_TimeZoneList) / sizeof(CGI_ENUMSEL_S);
		     nIndex++) {
			ifx_httpdWrite(wp,
				       T
				       ("\t<option value=\"%d\" %s>%s</option>\n"),
				       nIndex,
				       (nIndex == nSelIndex) ? "selected" : "",
				       web_Enum_TimeZoneList[nIndex].str);
		}
	} else if (!gstrcmp(name, T("NTP_Client"))) {
		ifx_httpdWrite(wp, T("%s"),
			       (ntpC.f_enable == 1) ? "checked" : "");
	}			// NTP_Client
	else if (!gstrcmp(name, T("NTP_Server_List"))) {
		for (nIndex = 0;
		     nIndex <
		     sizeof(web_Enum_NTP_Server_List) / sizeof(CGI_ENUMSEL_S);
		     nIndex++) {
			gsprintf(sValue, "%s", ntpC.ntpServer2);
			// Get Selected option index form admweb.conf
			if (!strcmp
			    (sValue, web_Enum_NTP_Server_List[nIndex].str)) {
				gsprintf(sValue, T("%s"), "selected");
				flag = 1;
			} else
				gsprintf(sValue, T("%s"), "");
			ifx_httpdWrite(wp,
				       T
				       ("\t<option value=\"%d\" %s>%s</option>\n"),
				       nIndex, sValue,
				       web_Enum_NTP_Server_List[nIndex].str);

		}
		if (flag == 0) {
			ifx_httpdWrite(wp,
				       T
				       ("\t<option value=\"%d\" %s>%s</option>\n"),
				       nIndex, "selected", ntpC.ntpServer2);
		}
	}			// NTP_Server

	else
		ifx_httpdError(wp, 400,
			       T("Invalidate argument to get Time Setting"));
	return 0;

}

CGI_ENUMSEL_S web_Enum_Daylight_StartMon[] = {
	{0, "   "}
	,
	{1, "JAN"}
	,
	{2, "FEB"}
	,
	{3, "MAR"}
	,
	{4, "APR"}
	,
	{5, "MAY"}
	,
	{6, "JUN"}
	,
	{7, "JUL"}
	,
	{8, "AUG"}
	,
	{9, "SEP"}
	,
	{10, "OCT"}
	,
	{11, "NOV"}
	,
	{12, "DEC"}
};

int ifx_get_Daylight_StartMon(int eid, httpd_t wp, int argc, char_t ** argv)
{
	char_t sValue[MAX_DATA_LEN];
	int nIndex, nSelIndex;
	if (ifx_GetCfgData(FILE_RC_CONF, TAG_TIME_SECTION, "StartMon", sValue)
	    == 0) {
		nSelIndex = 0;
		trace(8, "StartMon value not found.\n");
	} else {
		trace(8, "StartMon value is %s\n", sValue);
		nSelIndex = gatoi(sValue);
	}

	for (nIndex = 0;
	     nIndex <
	     sizeof(web_Enum_Daylight_StartMon) / sizeof(CGI_ENUMSEL_S);
	     nIndex++) {
		// Get Selected option index form admweb.conf
		if (nIndex == nSelIndex)
			gstrcpy(sValue, "selected");
		else
			gstrcpy(sValue, "");

		ifx_httpdWrite(wp, T("\t<option value=\"%d\" %s>%s</option>\n"),
			       nIndex, sValue,
			       web_Enum_Daylight_StartMon[nIndex].str);
	}
	return 0;
}
int ifx_get_Daylight_EndMon(int eid, httpd_t wp, int argc, char_t ** argv)
{
	char_t sValue[MAX_DATA_LEN];
	int nIndex, nSelIndex;
	if (ifx_GetCfgData(FILE_RC_CONF, TAG_TIME_SECTION, "EndMon", sValue) ==
	    0) {
		nSelIndex = 0;
		trace(8, "EndMon value not found.\n");
	} else {
		trace(8, "EndMon value is %s\n", sValue);
		nSelIndex = gatoi(sValue);
	}

	for (nIndex = 0;
	     nIndex <
	     sizeof(web_Enum_Daylight_StartMon) / sizeof(CGI_ENUMSEL_S);
	     nIndex++) {
		// Get Selected option index form admweb.conf
		if (nIndex == nSelIndex)
			gstrcpy(sValue, "selected");
		else
			gstrcpy(sValue, "");

		ifx_httpdWrite(wp, T("\t<option value=\"%d\" %s>%s</option>\n"),
			       nIndex, sValue,
			       web_Enum_Daylight_StartMon[nIndex].str);
	}
	return 0;
}

CGI_ENUMSEL_S web_Enum_Daylight_StartDay[] = {
	{0, "  "}
	,
	{1, " 1"}
	,
	{2, " 2"}
	,
	{3, " 3"}
	,
	{4, " 4"}
	,
	{5, " 5"}
	,
	{6, " 6"}
	,
	{7, " 7"}
	,
	{8, " 8"}
	,
	{9, " 9"}
	,
	{10, "10"}
	,
	{11, "11"}
	,
	{12, "12"}
	,
	{13, "13"}
	,
	{14, "14"}
	,
	{15, "15"}
	,
	{16, "16"}
	,
	{17, "17"}
	,
	{18, "18"}
	,
	{19, "19"}
	,
	{20, "20"}
	,
	{21, "21"}
	,
	{22, "22"}
	,
	{23, "23"}
	,
	{24, "24"}
	,
	{25, "25"}
	,
	{26, "26"}
	,
	{27, "27"}
	,
	{28, "28"}
	,
	{29, "29"}
	,
	{30, "30"}
	,
	{31, "31"}
};
int ifx_get_Daylight_StartDay(int eid, httpd_t wp, int argc, char_t ** argv)
{
	char_t sValue[MAX_DATA_LEN];
	int nIndex, nSelIndex;
	if (ifx_GetCfgData(FILE_RC_CONF, TAG_TIME_SECTION, "StartDay", sValue)
	    == 0) {
		nSelIndex = 0;
		trace(8, "StartDay value not found.\n");
	} else {
		trace(8, "StartDay value is %s\n", sValue);
		nSelIndex = gatoi(sValue);
	}

	for (nIndex = 0;
	     nIndex <
	     sizeof(web_Enum_Daylight_StartDay) / sizeof(CGI_ENUMSEL_S);
	     nIndex++) {
		// Get Selected option index form admweb.conf
		if (nIndex == nSelIndex)
			gstrcpy(sValue, "selected");
		else
			gstrcpy(sValue, "");

		ifx_httpdWrite(wp, T("\t<option value=\"%d\" %s>%s</option>\n"),
			       nIndex, sValue,
			       web_Enum_Daylight_StartDay[nIndex].str);
	}
	return 0;
}
int ifx_get_cfdaysave(int eid, httpd_t wp, int argc, char_t ** argv)
{
	char_t sValue[MAX_DATA_LEN];
	if (ifx_GetCfgData(FILE_RC_CONF, TAG_TIME_SECTION, "cfdaysave", sValue)
	    == 0) {
		gstrcpy(sValue, "0");
		trace(8, "cfdaysave value not found.\n");
	} else {
		trace(8, "cfdaysave value is %s\n", sValue);
	}
	ifx_httpdWrite(wp, T("0"));
	return 0;
}

int ifx_get_Daylight_EndDay(int eid, httpd_t wp, int argc, char_t ** argv)
{
	char_t sValue[MAX_DATA_LEN];
	int nIndex, nSelIndex;
	if (ifx_GetCfgData(FILE_RC_CONF, TAG_TIME_SECTION, "EndDay", sValue) ==
	    0) {
		nSelIndex = 0;
		trace(8, "EndDay value not found.\n");
	} else {
		trace(8, "EndDay value is %s\n", sValue);
		nSelIndex = gatoi(sValue);
	}

	for (nIndex = 0;
	     nIndex <
	     sizeof(web_Enum_Daylight_StartDay) / sizeof(CGI_ENUMSEL_S);
	     nIndex++) {
		// Get Selected option index form admweb.conf
		if (nIndex == nSelIndex)
			gstrcpy(sValue, "selected");
		else
			gstrcpy(sValue, "");

		ifx_httpdWrite(wp, T("\t<option value=\"%d\" %s>%s</option>\n"),
			       nIndex, sValue,
			       web_Enum_Daylight_StartDay[nIndex].str);
	}
	return 0;
}
#endif				//CONFIG_PACKAGE_NTPCLIENT

// wizard_tz.asp
#ifdef CONFIG_PACKAGE_NTPCLIENT
int32 TimeZone_Map[] = {
	-720,			// TZ_ENEWETAK_KWAJALEIN,      // TZ=CST-12
	-660,			// TZ_MIDWAY_ISLAND,
	-600,			// TZ_HAWAII,
	-540,			// TZ_ALASKA,
	-480,			// TZ_PACIFIC,
	-420,			// TZ_ARIZONA,
	-360,			// TZ_CENTRAL_AMERICA,
	-300,			// TZ_BOGOTA,
	-270,			// TZ_CARACAS
	-240,			// TZ_ATLANTIC,
	-210,			// TZ_NEWFOUNDLAND,
	-180,			// TZ_BRASILIA,
	-120,			// TZ_MID_ATLANTIC,
	-60,			// TZ_AZORES,
	0,			// TZ_CASABLANCA,
	60,			// TZ_AMSTERDAM,
	120,			// TZ_ATHENS,
	180,			// TZ_BAGHDAD,
	210,			// TZ_TEHRAN,
	240,			// TZ_ABU_DHABI,
	270,			// TZ_KABUL,
	300,			// TZ_EKATERINBURG,
	330,			// TZ_CALCUTTA,
	345,			// TZ_KATHMANDU,
	360,			// TZ_ALMATY,
	390,			// TZ_RANGOON,
	420,			// TZ_BANGKOK,
	480,			// TZ_BEIJING,
	540,			// TZ_TOKYO,
	570,			// TZ_ADELAIDE,
	600,			// TZ_BRISBANE,
	660,			// TZ_MAGADAN,
	720,			// TZ_AUCKLAND,
	780			// TZ_NUKUALOFA,
};

void ifx_set_wizard_tz(httpd_t wp, char_t * path, char_t * query)
{
	char_t *ptz = NULL;
	int ifx_webs_NTP_Server_Index = 0, nIndex = 0;
	NTP_CLIENT_CFG ntpC;
	memset(&ntpC, 0x00, sizeof(ntpC));
	// get TimeZone Setting	
	
	if (ifx_get_ntp_client_cfg(&ntpC, IFX_F_DEFAULT) != IFX_SUCCESS) 
	{
		return ;
	}
	
	ptz = ifx_httpdGetVar(wp, T("tz"), T(""));
	if (ptz) {
		ntpC.timeZoneIdx = gatoi(ptz);
		if (gatoi(ptz) >= 0)
			ntpC.timeMinutesOffset = TimeZone_Map[gatoi(ptz)];
	} else {
		ifx_httpdError(wp,500,T("Failed to get NTP Variables"));
		return ;
        }
	for (nIndex = 0;
	     nIndex < sizeof(web_Enum_TimeZoneList) / sizeof(CGI_ENUMSEL_S);
	     nIndex++) {
		if (nIndex == gatoi(ptz)) {
			sprintf(ntpC.tzName, "%s",
				web_Enum_TimeZoneList[nIndex].str);
			break;
		}
	}
	// get NTP_Client Setting
	ptz = ifx_httpdGetVar(wp, T("NTP_Client"), T(""));
	if (gatoi(ptz) == 1) {
		ntpC.f_enable = IFX_ENABLED;
	} else {
		ntpC.f_enable = IFX_DISABLED;
	}
	// get NTP_Server Setting
	ptz = ifx_httpdGetVar(wp, T("NTP_Server_List"), T(""));
	ifx_webs_NTP_Server_Index = gatoi(ptz);
	ptz = ifx_web_GetNTPServerByIndex(ifx_webs_NTP_Server_Index);
	sprintf(ntpC.ntpServer1, "%s", ptz ? ptz : "");

// vipul start

	// get NTP_Server Setting
	ptz = ifx_httpdGetVar(wp, T("NTP_Secondary_Server_List"), T(""));
	//    gsprintf(s_ifx_webs_NTP_Server_List, "NTP_Server_List=\"%s\"\n", ptz);
	ifx_webs_NTP_Server_Index = gatoi(ptz);
	ptz = ifx_web_GetNTPServerByIndex1(ifx_webs_NTP_Server_Index);
	sprintf(ntpC.ntpServer2, "%s", ptz ? ptz : "");

// vipul end

	/* set cpeid and pcpeid to 1 */
	ntpC.iid.cpeId.Id = 1;
	ntpC.iid.pcpeId.Id = 1;
	/* set owner as web */
	ntpC.iid.config_owner = IFX_WEB;
	if (ifx_set_ntp_client_cfg(IFX_OP_MOD, &ntpC, IFX_F_MODIFY) !=
	    IFX_SUCCESS) {
		ifx_httpdError(wp, 500, T("Failed to save NTP Configurations"));
		return;
	}
	websNextPage(wp);
}
#endif				// CONFIG_PACKAGE_NTPCLIENT

int cgi_get_all_user_details(int eid, httpd_t wp, int argc, char_t ** argv)
{
	struct connection_profil_list *seekPtr = NULL;
	char8	buf[MAX_FILELINE_LEN * 4], writebuf[MAX_DATA_LEN], curr_user[MAX_UNAME_LEN];
	int32	i = 0, count = 0, users_count = 0;
	user_obj_t *users = NULL;

	memset(buf, 0, sizeof(buf));
	memset(writebuf, 0, sizeof(writebuf));

	if(mapi_get_all_user_obj_details(&count, &users, IFX_F_DEFAULT) != IFX_SUCCESS) {
#ifdef IFX_LOG_DEBUG
		IFX_DBG("[%s:%d] failed to get user accounts details", __FUNCTION__, __LINE__);
#endif
	}
	else {
		seekPtr = connlist;

		/* get current session user name */
		while(seekPtr != NULL) {
			if(strncmp(seekPtr->connection.ipaddr, wp->ipaddr, sizeof(seekPtr->connection.ipaddr)) == 0) {
				sprintf(curr_user, "%s", seekPtr->connection.user);
				break;
			}
			seekPtr = seekPtr->next;
		}

		for(i=0; i<count ;i++) {
			memset(buf, 0, sizeof(buf));
			if(strlen(curr_user) == 0 || strcmp(curr_user, "root") == 0 || strcmp(curr_user, "admin") == 0) {
				/* if current user is root or admin or NULL (in case of passwordless login) then show all users in the list */
				sprintf(buf, "username[%d]=\"%s\";\nenable[%d]=\"%d\";\nwebAccess[%d]=\"%d\";\nfileShareAccess[%d]=\"%d\";\ntelnetAccess[%d]=\"%d\";\nuserid[%d]=\"%d\";\n", users_count, (users + i)->username, users_count, (users + i)->f_enable, users_count, (users + i)->webAccess, users_count,(users + i)->fileShareAccess, users_count, (users + i)->telnetAccess, users_count, (users + i)->iid.cpeId.Id);
				users_count++;
			}
			else if(strcmp(curr_user, (users + i)->username) == 0) {
				/* if current user not root or admin then show only that user in the list */
				sprintf(buf, "username[%d]=\"%s\";\nenable[%d]=\"%d\";\nwebAccess[%d]=\"%d\";\nfileShareAccess[%d]=\"%d\";\ntelnetAccess[%d]=\"%d\";\nuserid[%d]=\"%d\";\n", users_count, (users + i)->username, users_count, (users + i)->f_enable, users_count, (users + i)->webAccess, users_count,(users + i)->fileShareAccess, users_count, (users + i)->telnetAccess, users_count, (users + i)->iid.cpeId.Id);
				users_count++;
			}
			LTQ_STRNCAT(writebuf, buf,MAX_DATA_LEN-1);
		}

		ifx_httpdWrite(wp, "curr_sess_user=\"%s\";\n", curr_user);
		ifx_httpdWrite(wp, "users_count=%d;\n", users_count);
		ifx_httpdWrite(wp, "%s", writebuf);
		IFX_MEM_FREE(users)
	}
	return 0;
}

int cgi_get_user_update_last_status(int eid, httpd_t wp, int argc, char_t ** argv)
{
	switch (user_status) {
		case 1: ifx_httpdWrite(wp, "User details updated successfully."); break;
		case 2: ifx_httpdWrite(wp, "Delete operation completed successfully."); break;
		default: ifx_httpdWrite(wp, ""); break;
	}
	user_status = 0;
	return 0;
}
