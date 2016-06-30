/* ===========================================================================
 * @file sys_env_type.h
 *
 * @path $sys_app/interface/inc
 *
 * @desc This file contains the source for watch dog manager.
 *
 * AUTHOR: sarin.ms@vvdntech.com
 *
 * The code contained herein is licensed under the GNU General Public  License.
 * You may obtain a copy of the GNU General Public License Version 2 or later
 * at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 * ========================================================================== */
/**
 * @file sys_env_type.h
 * @brief System evironment structure and Global definition.
 */
#ifndef __SYS_ENV_TYPE_H__
#define __SYS_ENV_TYPE_H__


#include <asm/types.h>
#include <netinet/in.h>
#include <time.h>
/*
   MAGIC_NUM log
   0x9A131904
   0x56313930
   0x56323030
   0x56313600
 */

/* vesion info */
#define PRODUCT_VER_HSC1		0	/* Product Version			*/
#define MAIN_BOARD_RevA			0	/* Main board Revision		*/
#define INTERFACE_BOARD_RevA	0	/* Interface Board Revision	*/

#define MAIN_REV		1   /* Main revision*/
#define MAIN_REL		5   /* Main Release*/
#define SUB_REL			4   /* Main Release*/
#define HSC1_FIRMWARE_VERSION ( (MAIN_REV<< 16) |(MAIN_REL << 8 )| SUB_REL)
#define MAIN_REV_MASK		0xff0000   /* Main revision Mask*/
#define MAIN_REL_MASK		0xff00     /* Main Release Mask*/
#define SUB_REL_MASK		0xff       /* Main Release Mask*/

#define MAGIC_NUM		0x9A131904
#define FAIL			-1
#define SYS_ENV_SIZE		sizeof(SysInfo)
#define MAX_LOG_PAGE_NUM	20
#define NUM_LOG_PER_PAGE	20
#define LOG_ENTRY_SIZE		sizeof(LogEntry_t)
#define MAC_LENGTH		6		/* Length of MAC address.*/
#define MAX_FILE_NAME		128		/* Maximum length of file name*/
#define MAC_LENGTH		6		/* Length of MAC address.*/
#define MAX_USER_ACCOUNTS	10		/* How many acounts which are stored in system */
#define USER_LEN		32		/* Maximum account username length.*/
#define MIN_USER_LEN		4		/* Minimum account username length.*/
#define PASSWORD_LEN		16		/* Maximum account password length.*/
#define MIN_PASSWORD_LEN	4		/* Minimum account password length.*/
#define MAX_CHAR_16		16
#define MAX_CHAR_30		30
#define MAX_PASSWORD_CHAR	8
#define MAX_UDP_CLIENTS		15
#define NUM_UART_PORTS		1
#define NO_OF_TMP_SENSORS   3
#define	MAX_NETWORK_PORTS	10
#define IP_STR_LEN		20
#define IP_DEVNO		0
#define LOG_STR_SIZE		100
#define MIN_TEMP_EVENT		2
#define MAX_TEMP_EVENT		3

#define CONFIG_HSC1_POST	1	/* Enables the POST result logging */
/* Types for global return status */
#define STD_FAIL_RETURN		-1	//Use this only for standard library functions like msgsnd which returns -1 for error;

#define SUCCESS		0x00000000
#define FAILURE		0x00010000
#define SYSTEM_ERROR	0x00020000
#define STATUS_MASK	0xffff0000
char sh_cmd[100];
#define shell_cmd(x, y...) ((sprintf(sh_cmd, x, y) >= 0) && \
		(system(sh_cmd) == SUCCESS))

#define LVDS_SRC 0
#define COMPONENT_SRC 1
#define COMPOSITE_SRC 2
#define PATTERN_SRC 4


#if 0
enum {
	FAILURE_SUBTYPE_START = 0,

	INVALID_REQUEST,
	ARGUMENT_UNSUPPORTED,
	SYSTEM_BUSY,

	FAILURE_SUBTYPE_END,
};

enum {
	SYSTEM_ERROR_SUBTYPE_START = FAILURE_SUBTYPE_END,

	IPC_COMMN_FAILURE,
	FILE_ACCESS_FAILURE,
	DEVICE_ACCESS_FAILURE,
	DATA_FETCH_FAILURE,
	MEMORY_ALLOC_FAILURE,
	THREAD_OPERATION_FAILURE,
	SOCKET_OPERATION_FAILURE,

	SYSTEM_ERROR_SUBTYPE_END
};

enum {
	COMMON_SUBTYPE_START = SYSTEM_ERROR_SUBTYPE_END,
	/*Add API specific success/failure returns here*/
	NO_CONTENT,
	USER_EXIST,
	USER_DO_NOT_EXIST,
	MAX_USER_LIMIT,
	ATTEMPT_ADMIN_DELETE,

	/* NETWORK RELATED. total 16 [0x11 to 0x20] */
	INVALID_IP,
	IP_OUT_OF_UNICAST_RANGE,
	LOOPBACK_IP,
	INVALID_HOST_ADDRESS,
	BROADCAST_IP,
	INVALID_MASK,
	MASK_VALUE_ERROR,
	ALL_ZEROS,
	INVALID_GATEWAY,
	GATEWAY_OUT_OF_UNICAST_RANGE,
	LOOPBACK_GATEWAY,
	BROADCAST_GATEWAY,
	IP_GATEWAY_MISSMATCH,
	INVALID_DNS,
	DNS_NOT_IN_UNICAST_RANGE,
	LOOPBACK_DNS,

	/* CAMERA RELATED total 2 [0x21 to 0x22] */
	CAMERA_COMM_FAILURE,
	CAMERA_MODULE_UNKNOWN,

	/*CAN RELATED total 4 [0x23 to 0x26] */
	CAN_FILTER_APPLY_FAILURE,
	MAX_CAN_FILTER_LIMIT,
	CAN_FILTER_EXIST,
	CAN_BITRATE_SET_FAILURE,

	/*UDP Event push related total 4 [0x27 to 0x2A] */
	MAX_UDP_CLIENT_LIMIT,
	UDP_CLIENT_UNREGISTER,
	UDP_CLIENT_DO_NOT_EXIST,
	UDP_CLIENT_EXISTS,

	/* UART RELATED total 9 [0x2B to 0x33] */
	INVALID_PORT,
	INVALID_INTERFACE,
	INVALID_BAUDRATE,
	INVALID_PARITY,
	INVALID_HW_FLOW_CONTROL_INFO,
	INVALID_NODE,
	INVALID_RESPONSE_PARAMETER,
	UART_COMM_FAILURE,
	WRITE_UNSUCCESSFUL,
	UART_TIMEOUT,

/* EEPROM RELATED total 11 [0x34 to 0x3E ] */
	EEPROM_INVALID_LOCATION,
	EEPROM_READ_FAILED,
	EEPROM_WRITE_FAILED,
	EEPROM_AUTH_FAILED,
	EEPROM_PASSWD_SET,
	EEPROM_CHECK_ARG_LEN,
	EEPROM_BAD_PASSWD,
	EEPROM_SN_NOT_SET,
	EEPROM_MAC_NOT_SET,
	EEPROM_DATA_CORRUPTED,
	EEPROM_NO_LOG_FOUND,

	/* NETWORK PORT BLOCKING RELATED total 11 [0x3F to 0x49] */
	INVALID_NETWORK_PORT,
	INVALID_NETWORK_PORT_TYPE,
	INVALID_NETWORK_PORT_DIRECTION,
	INVALID_NETWORK_PORT_FLAG,
	MAX_BLOCKED,
	PORT_ALREADY_BLOCKED,
	HTTP_PORT_BLOCKING,
	PORT_NOT_BLOCKED,
	HIGHER_RANGE_EXIST,
	CONTAINS_EXISTING_RANGE,
	RANGE_OVERLAPS,

	/* multicast ip related */
	IP_OUT_OF_MULTICAST_RANGE,
	/*Server ip and rtp destination Ip are on differnet domain*/
	IP_SERVER_RTP_DEST_MISMATCH,

	/* GPIO LIBRARY RELATED  [total 4]							*/
	/* Only Library function exists, no HTTP APIs are provided	*/
	/* hence all the HTTP API returns are to be added above this*/
	GPIO_INIT_FAILED,
	GPIO_READ_FAILED,
	GPIO_WRITE_FAILED,
	GPIO_TRIG_CH_FAILED,

	COMMON_SUBTYPE_END,

	/* EVENT RELATED */
	EVENT_ACK,
	EVENT_NACK,

	/* TEMPERATURE SENSOR RELATED */
	INVALID_TEMPERATURE_RANGE,
	INVALID_SENSOR_ID,
	INVALID_STATUS,

	/*FPGA IMAGE LOAD RELATED*/
	IMAGE_LOAD_FAILURE,


	/* GPIO EXPANDER RELATED*/
	INVALID_GPIO_EXP_PIN,
	INVALID_GPIO_EXP_PIN_DIRECTION,
	INVALID_GPIO_EXP_PIN_VALUE,
	GPIO_EXP_PIN_DIR_INPUT,
	GPIO_EXP_PIN_DIR_OUTPUT,
	GPIO_EXP_PIN_CONFIG_ERROR,

	INVALID_9AXIS_ENABLE_PARAMETER,

	UNSUPPORTED_STREAM_SAMPLE_RATE,
};

typedef enum {
	OFF = 0,
	ON
} on_off_t;

/**
 * @brief Infomation of network settings.
 */
typedef struct {
	struct in_addr	ip;				/* IP address in static IP mode*/
	struct in_addr	netmask;		/* Netmask in static IP mode*/
	struct in_addr	gateway;		/* Gateway in static IP mode*/
	struct in_addr	dns;			/* DNS IP in static IP mode*/
	__u8	MAC[MAC_LENGTH];		/* hardware MAC address */
	__u16	dhcp_enable;			/* DHCP Enable*/
}NetworkConfigData;

typedef struct {
	int type;
	int max;
	int current_index;
	SET set[2];
}INPUTVIDEOSIGNAL;

typedef struct {
	int type;
	int max;
	int current_index;
	SET set[5];
} FRAMEINTERVAL;

typedef struct {
	int type;
	int max;
	int current_index;
	SET set[4];
} FRAMERATE;

typedef struct {
	int type;/*0 CBR 1 VBR*/
	int max; /* Maximum bitrate incase of VBR OR bitrate in case of CBR*/
	int min; /* Minimum in case of */
	/* current bitrate in case of VBR. in case of CBR it will be same as max.*/
	int cur;
} BITRATETYPE;

/* IPNC */
typedef struct {
	int type;
	int limit;
	char value[MAX_CHAR_16];
} IPADDRESS;

typedef struct {
	int type;
	int limit;
	char value[MAX_CHAR_30];
} ID;

typedef struct {
	int type;
	int limit;
	char value[MAX_PASSWORD_CHAR];
} PASSWORD;

typedef struct {
	IPADDRESS IpAddress;
	INT_VAR port;
	ID  id;
	PASSWORD password;
} SYS_IPNCINFO;

typedef struct {
	int hdmi_en;/* 0:hdmi live disable, 1: hdmi enable*/
	int fpga_resolution;
	int fpga_framerate;
	int sd_aspect_ratio;	/*Aspect ratio for LVDS video (SD resolution)*/
	int adv_resolution;
	int adv_framerate;
	BITRATETYPE enc_bitrate;
	int dynamic_bitrate_enable;			/*Dynamic bitrate enable/disable*/
	int CameraMode;
	int rtsp_running_st;
	int streaming_mode;/* Streaming mode */
	int udp_trans_type; /*Rtsp udp tranport type*/
	char udp_multicast_ip[IP_STR_LEN];		/* RTSP multicast group ip address*/
	char udp_dest_ip[IP_STR_LEN];		/* RTP destination IP address*/
	int udp_dest_port;					/* RTP destination port*/
	int vid_src_lvds_st;				/* Video Source selection status */
	int max_client_allowed;				/* max client allowed in rtsp */
	int rtp_enable;						/*Rtp stream needed or not*/
} SYS_VIDEOINFO;     /*video INFO*/

typedef struct {
	unsigned int hdmi_en;
	unsigned int lineout_en;
	unsigned int stream_en;
	unsigned int vol_level;
	unsigned int sample_rate;
} SYS_AUDIOINFO;

typedef struct {
	int valid;
	int audio_video;
	char ip[15];
	int rtp_port;
	int rtcp_port;
}SYS_RTSP_SESSION_INFO;

typedef enum {
	AUTHORITY_ADMIN = 0,
	AUTHORITY_OPERATOR,
	AUTHORITY_VIEWER,
	AUTHORITY_NONE = 9
} AUTHORITY;

typedef enum {
	TMP_SENSOR_PROCESSOR = 1,
	TMP_SENSOR_POWER_BOARD,
	TMP_SENSOR_9_AXIS
} TMP_SENSOR_ID;

typedef struct {
	int enable;
	in_port_t udp_port;
	struct in_addr ip_addr;
} UDP_CLIENT;

typedef struct {
	char user_name[USER_LEN];
	AUTHORITY authority;
	char password[PASSWORD_LEN];
} USER_ACCOUNT;

/* Network configurations */
typedef struct {
	int req_type;				/* 0: STATIC,1: DHCP. Req_type=1 => DHCP_enabled*/
	int cur_type;				/* 0: STATIC,1: DHCP, 2: ZERO_Config*/
	int state;					/* 0: link down, 1: link up*/
	int zero_conf_en;			/* 0: disabled,  1: enable*/
	char ip[IP_STR_LEN];		/* ip address*/
	char mask[IP_STR_LEN];		/* netmask*/
	char gate[IP_STR_LEN];		/* gateway*/
	char dns[IP_STR_LEN];		/* dns*/
	char mac[IP_STR_LEN];		/* mac address of the adapter*/
} NET_INFO;

/* Network configurations */
typedef struct {
	char ip[IP_STR_LEN];		/* ip address*/
	char server_mask[IP_STR_LEN];		/* netmask*/
	char server_ip[IP_STR_LEN];		/* gateway*/
} RTP_DEST_IP_INFO;
/*UART configuration structure*/
typedef struct {
	short int uart_port;				/*0: uart0*/
	short int uart_port_interface;		/*0: RS-232, 1: RS-485*/
	short int baud_rate;				/*0: 2400, 1:4800, 2:9600, 3:19200, 4:38400, 5:116200*/
	short int parity;					/*0: even, 1:odd, 2: no parity*/
	short int uart_hw_flow_control;     /*0: off, 1:on*/
} UART_CONFIG;

/*NETWORK port blocking configuration structure*/
typedef struct {
	int network_port_start;						/*	values 0 to 65534 possible	*/
	int network_port_stop;						/*			start ≤ stop		*/
	unsigned short int network_port_type;		/*0:UDP, 1:TCP*/
	unsigned short int network_port_direction;	/*0:INPUT, 1:OUTPUT*/
	unsigned short int network_port_flag;		/*0:UNBLOCK, 1:BLOCK*/
} NETWORK_PORT_CONFIG;

/*UART data storage structure*/
typedef struct {
	unsigned int uart_port;                   /*0: uart0*/
	char data[30];
	unsigned char flag;
	int len;
} UART_DATA;


/* ACTION STRUCTURE */
typedef struct {
	int	test_action1;
	int	test_action2;
	int	test_action3;
	int	test_action4;
} ACTION_T;

/*	EVENT CONFIGURATION */
typedef struct {
	int	enable;
	ACTION_T	actions;
	int retrigger_interval;
} EVENT_CONFIG_T;

/* VIDEO LOSS EVENT CONFIGURATION */
typedef struct {
	EVENT_CONFIG_T      videoloss_t;
} VIDEOLOSS_CONFIG_T;
/* MOTION DETECTION EVENT CONFIGURATION */
typedef struct {
	EVENT_CONFIG_T      motionEvent_t;
} MOTION_CONFIG_T;

/* CAM BASED EVENT CONFIGURATION */
typedef struct {
	VIDEOLOSS_CONFIG_T      videoloss_config_t;
	MOTION_CONFIG_T		motionEvent_config_t;
} CAM_EVENTS_T;

/* TEMPERATURE CHANGE EVENT CONFIGURATION */
typedef struct {
	EVENT_CONFIG_T      temperature_change_t;
	int min;
	int max;
	int sensor_id;
} TEMPERATURE_CONFIG_T;

/* NETWORK BANDWIDTH CHANGE EVENT CONFIGURATION */
typedef struct {
	EVENT_CONFIG_T      network_bw_change_t;
} NETWORK_BW_CONFIG_T;

/* ETHERNETLOSS  EVENT CONFIGURATION */
typedef struct {
	EVENT_CONFIG_T      ethernet_loss_t;
} ETHERNETLOSS_CONFIG_T;

/* GPIOLEVEL CHANGE EVENT CONFIGURATION */
typedef struct {
	EVENT_CONFIG_T      gpio_level_t;
} GPIO_LEVEL_CONFIG_T;

/*      NON  CAM BASED EVENTS */
typedef struct {
	TEMPERATURE_CONFIG_T temperature_config_t[NO_OF_TMP_SENSORS];
	NETWORK_BW_CONFIG_T network_bw_config_t;
	ETHERNETLOSS_CONFIG_T ethernet_loss_config_t;
	GPIO_LEVEL_CONFIG_T gpio_level_config_t;
} NONCAM_EVENTS_T;

/* GPIO EXPANDER CONFIGURATION */
typedef struct {
	short int gpio_exp_pin;           /* 0=192, 1=193, 2=194, 3=195 */
	short int gpio_exp_pin_direction; /* 0=out, 1=in */
	short int gpio_exp_pin_value;
} GPIO_EXP_CONFIG;

/* 9axis values */
typedef struct {
	int en[3];
	float mag_x;
	float mag_y;
	float mag_z;
	float acc_x;
	float acc_y;
	float acc_z;
	float gyro_x;
	float gyro_y;
	float gyro_z;
} STRUCT_9AXIS;

typedef struct {
	SYS_VIDEOINFO   video;
	SYS_AUDIOINFO   audio;
} MEDIA_INFO;

typedef struct SysInfo {
	unsigned long	relUniqueId;	      /* Release ID*/
	char		hostname[MAX_FILE_NAME];  /* Host name */
	int		configure_status;
	int		hdmi_console_en;
	NET_INFO	net;		/* Network configurations */
	USER_ACCOUNT	account[MAX_USER_ACCOUNTS];/* User Accounts */
	SYS_VIDEOINFO	video;
	SYS_AUDIOINFO	audio;
	CAN_INFO	can;
	/* CAM BASED EVENTS */
	CAM_EVENTS_T	cam_event;
	/* NON CAM BASED EVENT */
	NONCAM_EVENTS_T	noncam_event;
	int		camera_baudrate;
	NETWORK_PORT_CONFIG network_port_config[MAX_NETWORK_PORTS]; /* NETWORK configurations*/
	int dscp;			/* QoS DSCP flag. Value 0-64 Possible */
	int diagtool_status; /* 0 IDLE; 1 RUNNING */
	int video_src_prio; /* 0 FPGA; 1 DECODER */
	char video_latency_quality; /* 0 Low latency; 1 High latency */
	BOARD_INFO	board_info;
	UDP_CLIENT	client[MAX_UDP_CLIENTS];
	UART_CONFIG uart_config;/* UART configurations*/
	int post_camera;	/* 0 WORKING; 1 ERROR */
	int post_video;		/* 0 WORKING; 1 ERROR */
	int post_can;		/* 0 WORKING; 1 ERROR */
	int default_config_flag;	/* 0 default configuration; 1 changed configuration */
} SysInfo;

#endif
#endif /* __SYS_ENV_TYPE_H__ */
