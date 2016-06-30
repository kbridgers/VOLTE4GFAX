/*************************************************************************
 *                       Teridian Semiconductor Corp.
 *                 6440 Oak Canyon, Irvine, CA 92618-5201
 *                         ph: (714) 508-8800
 *
 *        (c) Copyright 2006,2007,2008,2009,2010 Teridian Semiconductor Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Filename      : 73m1966_io.h
 * Created       : 2009/01/15
 * Description   : M1966 Device Driver
 *
 *                 This file contains defines, structures declarations for
 *                 the driver user interface
 *
 *************************************************************************
 * Change Log
 * ==========
 * $Log: $
 *
 *************************************************************************
 */

#ifndef __73M1966_IO_H
#define __73M1966_IO_H

#define FALSE      0
#define TRUE       1


/*
*************************************************************************************************
*                         73M1966 IOCTL numbers
*************************************************************************************************
*/
/* Basic Teridian IOCTLs... */
#define M1966_ATZ                   _IOWR(0xA4,0xA0,unsigned int) /* reset */
#define M1966_ATH0                  _IOWR(0xA4,0xA1,unsigned int) /* on hook */
#define M1966_ATH1                  _IOWR(0xA4,0xA2,unsigned int) /* off hook */
#define M1966_ATDP                  _IOWR(0xA4,0xA3,unsigned int) /* pulse dial */
#define M1966_ATDT                  _IOWR(0xA4,0xA4,unsigned int) /* tone dial */
#define M1966_ATLW                  _IOWR(0xA4,0xA5,unsigned int) /* compression modes */
#define M1966_ATFC                  _IOWR(0xA4,0xA6,unsigned int) /* flash timing */
#define M1966_ATFL                  _IOWR(0xA4,0xA7,unsigned int) /* set flash */
#define M1966_SAMPLE_RATE           _IOWR(0xA4,0xA8,unsigned int) /* set PCM sample rate */ 
#define M1966_ATBG                  _IOWR(0xA4,0xA9,unsigned int) /* Battery get */
#define M1966_ATPG                  _IOWR(0xA4,0xAA,unsigned int) /* Polarity get */
#define M1966_ATRG                  _IOWR(0xA4,0xAB,unsigned int) /* Ring get */
#define M1966_ATOH                  _IOWR(0xA4,0xAC,unsigned int) /* Off Hook State */
#define M1966_ATDD                  _IOWR(0xA4,0xAD,unsigned int) /* dial timings */
#define M1966_ATSR                  _IOWR(0xA4,0xAF,unsigned int) /* register access */
#define M1966_EVENT_GET             _IOWR(0xA4,0xB1,unsigned int) /* get 1966 events */
#define M1966_ERROR_CODE_GET        _IOWR(0xA4,0xB2,unsigned int) /* Rertreive last error code */
#define M1966_THRESHOLD_OVERRIDE    _IOWR(0xA4,0xB3,unsigned int) /* threshold override */
#define M1966_HW_REVISION_GET       _IOWR(0xA4,0xB4,unsigned int) /* get HW reversion */
#define M1966_CNTRY_NMBR_GET        _IOWR(0xA4,0xB5,unsigned int) /* nmbr from cntry code */
#define M1966_BAT_GET               _IOWR(0xA4,0xB6,unsigned int) /* get 1966 events */
#define M1966_POL_GET               _IOWR(0xA4,0xB7,unsigned int) /* get 1966 events */
#define M1966_RNG_GET               _IOWR(0xA4,0xB8,unsigned int) /* get 1966 events */
#define M1966_POH_GET               _IOWR(0xA4,0xB9,unsigned int) /* get 1966 events */
#define M1966_FLSH_CFG              _IOWR(0xA4,0xBA,unsigned int) /* get 1966 events */
#define M1966_FLSH_GET              _IOWR(0xA4,0xBB,unsigned int) /* get 1966 events */
#define M1966_FLSH_SET              _IOWR(0xA4,0xBC,unsigned int) /* get 1966 events */
#define M1966_LOOPBACK              _IOWR(0xA4,0xBD,unsigned int) /* loopback command */
#define M1966_ATEE                  _IOWR(0xA4,0xBE,unsigned int) /* events enable */
#define M1966_ATED                  _IOWR(0xA4,0xBF,unsigned int) /* events disable */
#define M1966_GPIO_CONFIG           _IOWR(0xA4,0xC0,unsigned int) /* GPIO config */
#define M1966_GPIO_CONTROL          _IOWR(0xA4,0xC1,unsigned int) /* GPIO control */
#define M1966_GPIO_DATA             _IOWR(0xA4,0xC2,unsigned int) /* GPIO data */
#define M1966_BTONE_FILTER          _IOWR(0xA4,0xC3,unsigned int) /* Billing tone filter */
#define M1966_LINE_DISABLE          _IOWR(0xA4,0xC4,unsigned int) /* Disable line */
#define M1966_LINE_ENABLE           _IOWR(0xA4,0xC5,unsigned int) /* Enable line  */

#define M1966_PCM_CLK_INIT          _IOWR(0xA4,0xC7,unsigned int)
#define M1966_CH_INIT               _IOWR(0xA4,0xC8,unsigned int)
#define M1966_LINE_TYPE_SET         _IOWR(0xA4,0xC9,unsigned int)
#define M1966_PHONE_VOLUME_SET      _IOWR(0xA4,0xCA,unsigned int)
#define M1966_DEBUG_REPORT_SET      _IOWR(0xA4,0xCB,unsigned int)
#define M1966_LASTERR               _IOWR(0xA4,0xCC,unsigned int)
#define M1966_VERSION_CHECK         _IOWR(0xA4,0xCD,unsigned int)
#define M1966_VERSION_GET           _IOWR(0xA4,0xCE,unsigned int)
#define M1966_CNTRY_CODE_SET        _IOWR(0xA4,0xCF,unsigned int) /* use cntry code */
#define M1966_PCM_IF_CSET_OPMODE_SET        _IOWR(0xA4,0xD0,unsigned int)
#define M1966_PCM_IF_CSET_FREQUENCY_SET     _IOWR(0xA4,0xD1,unsigned int)
#define M1966_PCM_IF_CSET_RPOL_SET          _IOWR(0xA4,0xD2,unsigned int) 
#define M1966_PCM_IF_CSET_TPOL_SET          _IOWR(0xA4,0xD3,unsigned int) 
#define M1966_PCM_IF_CSET_RCS_SET           _IOWR(0xA4,0xD4,unsigned int)
#define M1966_PCM_IF_CSET_TCS_SET           _IOWR(0xA4,0xD5,unsigned int) 
#define M1966_PCM_CSET_TSLOT_RX_SET         _IOWR(0xA4,0xD6,unsigned int)
#define M1966_PCM_CSET_TSLOT_TX_SET         _IOWR(0xA4,0xD7,unsigned int) 
#define M1966_PCM_CSET_COMPRESSION_SET      _IOWR(0xA4,0xD8,unsigned int)
#define M1966_PCM_ACTIVATION_SET            _IOWR(0xA4,0xD9,unsigned int)
#define M1966_PCM_ACTIVATION_CLEAR          _IOWR(0xA4,0xDA,unsigned int) 
#define M1966_PCM_ACTIVATION_GET            _IOWR(0xA4,0xDB,unsigned int) 
#define M1966_PCM_CFG_GET                   _IOWR(0xA4,0xDC,unsigned int) 
#define M1966_ATDP_CANCEL                   _IOWR(0xA4,0xDD,unsigned int)  /* cancel and on-going pulse dial */
#define M1966_ATDP_PARAM                    M1966_ATDD                     /* Get/Set pulse dial parameters */

#define M1966_DEBUG_LEVEL_GET               _IOWR(0xA4,0xE0,unsigned int)
#define M1966_DEBUG_LEVEL_SET               _IOWR(0xA4,0xE1,unsigned int)

#define	M1966_SET_MIN_INTER_RING_GAP        _IOWR(0xA4,0xE2,unsigned int)
#define	M1966_SET_RING_MIN_FREQ             _IOWR(0xA4,0xE3,unsigned int)
#define	M1966_SET_RING_MAX_FREQ             _IOWR(0xA4,0xE4,unsigned int)

#define	M1966_MEASURE_START                 _IOWR(0xA4,0xE5,unsigned int)
#define	M1966_MEASURE_STOP                  _IOWR(0xA4,0xE6,unsigned int)
#define	M1966_MEASURE_UPDATE                _IOWR(0xA4,0xE7,unsigned int)

#define	M1966_TEST_CODE                     _IOWR(0xA4,0xE8,unsigned int)

#define M1966_CPROG_MONITOR                 _IOWR(0xA4,0xF0,unsigned int)
#define M1966_SEND_WETTING_PULSE            _IOWR(0xA4,0xF1,unsigned int)
#define M1966_CLEAR_RX_OFFSET_REG           _IOWR(0xA4,0xF2,unsigned int)
#define M1966_GET_COUNTRY_CONFIG            _IOWR(0xA4,0xF3,unsigned int)
#define M1966_SET_COUNTRY_CONFIG            _IOWR(0xA4,0xF4,unsigned int)
#define M1966_GET_LOOP_CURRENT              _IOWR(0xA4,0xF5,unsigned int)
#define M1966_PERF_RX_OFFSET_CALIB          _IOWR(0xA4,0xF6,unsigned int)
#define M1966_ENNOM_DELAY_TIMER             _IOWR(0xA4,0xF7,unsigned int)
#define M1966_ENTER_CID_MODE                _IOWR(0xA4,0xF8,unsigned int)
#define M1966_EXIT_CID_MODE                 _IOWR(0xA4,0xF9,unsigned int)
#define M1966_ENABLE_CALLER_ID              _IOWR(0xA4,0xFA,unsigned int)
#define M1966_DISABLE_CALLER_ID             _IOWR(0xA4,0xFB,unsigned int)

#define M1966_API_TERMINATE                 _IOWR(0xA4,0xFF,unsigned int)

/*********************************************************************************************
**  73M1966 FXO Events
**********************************************************************************************/
#define M1966_OFH_PHU_DETECT            0x00200000   /* parallel phone hung   up       */
#define M1966_OFH_PPU_DETECT            0x00100000   /* parallel phone picked up       */
#define M1966_OFH_POLARITY_CHG          0x00080000   /* Polarity change while off hook */
#define M1966_ONH_DETECT                0x00040000   /* Device went on hook            */
#define M1966_ONH_NOPOH_DETECT          0x00020000   /* NO Parallel phone is Off Hook  */
#define M1966_ONH_APOH_DETECT           0x00010000   /* A  Parallel phone is Off Hook  */
#define M1966_ONH_POLARITY_CHG          0x00008000   /* Polarity change while on hook  */
#define M1966_GPIO_INTERRUPT            0x00004000   /* GPIO interrupt event           */
#define M1966_BATTERY_DROPPED           0x00002000   /* Battery voltage dropped        */
#define M1966_BATTERY_FEEDED            0x00001000   /* Battery voltage restored       */
#define M1966_RING_DETECT_END           0x00000400   /* Ring end detected */
#define M1966_RING_DETECT_START         0x00000200   /* Incoming ring detected */
#define M1966_DEVICE_RECOVERED          0x00000100   /* Barrier sync recovered */
#define M1966_DEVICE_FAILURE_DETECT     0x00000080   /* Barrier sync lost detected */
#define M1966_SPI_ERROR_DETECT          0x00000040   /* SPI erorr detected */
#define M1966_OV_DETECT                 0x00000020   /* Over voltage detected */
#define M1966_OI_DETECT                 0x00000010   /* Over current detected */
#define M1966_OL_DETECT                 0x00000008   /* Over load detecetd */
#define M1966_LINE_STATE                0x00000004   /* Line state event - voltage/current */
#define M1966_DIAL_COMPLETE             0x00000002   /* Pulse dial completed */
#define M1966_DIAL_ABORTED              0x00000001   /* Pulse dial aborted */
#define M1966_API_TERMINATE_EVENT       0x80000000   /* Terminate FXO API, used internally */

/*******************************************************************
**  73M1966 Driver Last Error Code
*******************************************************************/
#define M1966_ERR_OK                    0x00000000   /* NO Error */
#define M1966_ERR_INVALID_GPIO_NUM      0x00000001   /* Invalid GPIO number */
#define M1966_ERR_INVALID_CNTRY_CODE    0x00000002   /* Invalid country code */
#define M1966_ERR_INVALID_PARAM         0x00000003   /* Invalid parameter */
#define M1966_ERR_INVALID_STATE         0x00000004   /* Invalid state for the command */
#define M1966_ERR_INVALID_IOCTL         0x00000005   /* Invalid ioctl */
#define M1966_ERR_INVALID_FD            0x00000005   /* Invalid File Descriptor */
#define M1966_ERR_INVALID_HOST_REV      0x00000006   /* Invalid Host Revision number */
#define M1966_ERR_COPY_TO_USER          0x00000020   /* memcopy to user failed */
#define M1966_ERR_COPY_FROM_USER        0x00000021   /* memcopy from user failed */
#define M1966_ERR_PLL_NOT_LOCKED        0x00000022   /* PLL not locked */
#define M1966_ERR_BARRIER_NOT_SYNC      0x00000023   /* Barrier not synced */
#define M1966_ERR_NO_EVENT_DATA         0x00000024   /* No event data available */

/*******************************************************************
**  73M1966 Country code List - Internet Country Codes 
*******************************************************************/
#define	M1966_CNTRY_CODE_AR             0       /* "Argentina"    */
#define	M1966_CNTRY_CODE_AU             1       /* "Australia"    */
#define	M1966_CNTRY_CODE_AT             2       /* "Austria"      */
#define	M1966_CNTRY_CODE_BH             3       /* "Bahrain"      */
#define	M1966_CNTRY_CODE_BE             4       /* "Belgium"      */
#define	M1966_CNTRY_CODE_BR             5       /* "Brazil"       */
#define	M1966_CNTRY_CODE_BG             6       /* "Bulgaria"     */
#define	M1966_CNTRY_CODE_CA             7       /* "Canada"       */
#define	M1966_CNTRY_CODE_CL             8       /* "Chile"        */
#define	M1966_CNTRY_CODE_C1             9       /* "ChineData"    */
#define	M1966_CNTRY_CODE_C2             10      /* "ChinaVoice"   */
#define	M1966_CNTRY_CODE_CO             11      /* "Columbia"     */
#define	M1966_CNTRY_CODE_HR             12      /* "Croatia"      */
#define	M1966_CNTRY_CODE_TB             13      /* "TBR 21"       */
#define	M1966_CNTRY_CODE_CY             14      /* "Cyprus"       */
#define	M1966_CNTRY_CODE_CZ             15      /* "Czech Rep"    */
#define M1966_CNTRY_CODE_DK             16      /* "Denmark"      */
#define M1966_CNTRY_CODE_EC             17      /* "Equador"      */
#define M1966_CNTRY_CODE_EG             18      /* "Egypt"        */
#define M1966_CNTRY_CODE_SV             19      /* "El Salvador"  */
#define M1966_CNTRY_CODE_FI             20      /* "Finland"      */
#define M1966_CNTRY_CODE_FR             21      /* "France"       */
#define M1966_CNTRY_CODE_DE             22      /* "Germany"      */
#define M1966_CNTRY_CODE_GR             23      /* "Greece"       */
#define M1966_CNTRY_CODE_GU             24      /* "Guam"         */
#define M1966_CNTRY_CODE_HK             25      /* "Hong Kong"    */
#define M1966_CNTRY_CODE_HU             26      /* "Hungary"      */
#define M1966_CNTRY_CODE_IS             27      /* "Iceland"      */
#define M1966_CNTRY_CODE_IN             28      /* "India"        */
#define M1966_CNTRY_CODE_ID             29      /* "Indonesia"    */
#define M1966_CNTRY_CODE_IE             30      /* "Ireland"      */
#define M1966_CNTRY_CODE_IL             31      /* "Israel"       */
#define M1966_CNTRY_CODE_IT             32      /* "Italy"        */
#define M1966_CNTRY_CODE_JP             33      /* "Japan"        */
#define M1966_CNTRY_CODE_JO             34      /* "Jordan"       */
#define M1966_CNTRY_CODE_KZ             35      /* "Kazakhstan"   */
#define M1966_CNTRY_CODE_KW             36      /* "Kuwait"       */
#define M1966_CNTRY_CODE_LV             37      /* "Latvia"       */
#define M1966_CNTRY_CODE_LB             38      /* "Lebanon"      */
#define M1966_CNTRY_CODE_LU             39      /* "Luxembourg"   */
#define M1966_CNTRY_CODE_MO             40      /* "Macao"        */
#define M1966_CNTRY_CODE_MY             41      /* "Malaysia"     */
#define M1966_CNTRY_CODE_MT             42      /* "Malta"        */
#define M1966_CNTRY_CODE_MX             43      /* "Mexico"       */
#define M1966_CNTRY_CODE_MA             44      /* "Morocco"      */
#define M1966_CNTRY_CODE_NL             45      /* "Netherlands"  */
#define M1966_CNTRY_CODE_NZ             46      /* "New Zealand"  */
#define M1966_CNTRY_CODE_NG             47      /* "Nigeria"      */
#define M1966_CNTRY_CODE_NO             48      /* "Norway"       */
#define M1966_CNTRY_CODE_OM             49      /* "Oman"         */
#define M1966_CNTRY_CODE_PK             50      /* "Pakistan"     */
#define M1966_CNTRY_CODE_PR             51      /* "Peru"         */
#define M1966_CNTRY_CODE_PH             52      /* "Philippines"  */
#define M1966_CNTRY_CODE_PL             53      /* "Poland"       */
#define M1966_CNTRY_CODE_PT             54      /* "Portugal"     */
#define M1966_CNTRY_CODE_RO             55      /* "Romainia"     */
#define M1966_CNTRY_CODE_RU             56      /* "Russia"       */
#define M1966_CNTRY_CODE_SA             57      /* "Saudi Arabia" */
#define M1966_CNTRY_CODE_SG             58      /* "Singapore"    */
#define M1966_CNTRY_CODE_SK             59      /* "Slovakia"     */
#define M1966_CNTRY_CODE_SI             60      /* "Slovenia"     */
#define M1966_CNTRY_CODE_ZA             61      /* "S. Africa"    */
#define M1966_CNTRY_CODE_KR             62      /* "S. Korea"     */
#define M1966_CNTRY_CODE_ES             63      /* "Spain"        */
#define M1966_CNTRY_CODE_SE             64      /* "Sweden"       */
#define M1966_CNTRY_CODE_CH             65      /* "Switzerland"  */
#define M1966_CNTRY_CODE_SY             66      /* "Syria"        */
#define M1966_CNTRY_CODE_TW             67      /* "Taiwan"       */
#define M1966_CNTRY_CODE_TH             68      /* "Thailand"     */
#define M1966_CNTRY_CODE_AE             69      /* "UAE"          */
#define M1966_CNTRY_CODE_UK             70      /* "UK"           */
#define M1966_CNTRY_CODE_US             71      /* "USA"          */
#define M1966_CNTRY_CODE_YE             72      /* "Yemen"        */
#define M1966_CNTRY_CODE_MAX            M1966_CNTRY_CODE_YE

/******************************************************************
** DEBUG TRACE MASK 
******************************************************************/
#define M1966_DEBUG_EVENT         0x00000001
#define M1966_DEBUG_INIT          0x00000002
#define M1966_DEBUG_RING_PATH     0x00000004
#define M1966_DEBUG_TRACE         0x00000008
#define M1966_DEBUG_COUNTRY_CODE  0x00000010
#define M1966_DEBUG_UNUSED_A      0x00000020
#define M1966_DEBUG_LINE_STATE    0x00000040
#define M1966_DEBUG_IOCTL         0x00000080
#define M1966_DEBUG_PCM           0x00000100
#define M1966_DEBUG_BARRIER       0x00000200
#define M1966_DEBUG_INT           0x00000400
#define M1966_DEBUG_PHU           0x00000800
#define M1966_DEBUG_TAPI          0x00001000
#define M1966_DEBUG_KPROC         0x00002000
#define M1966_DEBUG_SPI           0x00004000
#define M1966_DEBUG_LOOPBACK      0x00008000
#define M1966_DEBUG_GPIO          0x00010000
#define M1966_DEBUG_ERROR         0x80000000

/*******************************************************************
**  73M1966 Register Access code 
*******************************************************************/
#define M1966_REG_READ                 0       /* Register READ   */ 
#define M1966_REG_READALL              1       /* Register READALL*/
#define M1966_REG_WRITE                2       /* Register WRITE  */

/*******************************************************************
**  Pulse Dial Parameters
*******************************************************************/
#define MIN_PULSE_DIAL_HOOK_DURATION   20      /* min On/Off-hook duration */
#define MAX_PULSE_DIAL_HOOK_DURATION   200     /* max On/Off-hook duration */
#define MIN_PULSE_DIAL_INTER_DIGIT_DURATION 20 /* min inter-digit duration */
#define MAX_PULSE_DIAL_INTER_DIGIT_DURATION 2500 /* max inter-digit duration */
#define MAX_PHONE_NMBR_DIGIT_CNT       64      /* max number of digits for pulse dial */ 

#define MAX_MEASURE_SAMPLE_TIME        100     /* 100 milliseconds */
#define MAX_MEASURE_AVG_SAMPLE_COUNT   20      /* max sample count */
#define MAX_IET_ROWS                   10      /* max number of IET rows */

/*******************************************************************
*
*  Teridian 1966 Channel Init Structure
*  This structure is instantiated by the user, with
*  user selected values and thresholds needed for
*  local operation of the Teridian 1966 DAA chipset.
*
********************************************************************/
struct M1966_init_struct {

    /* country code selection */
    unsigned char country_code;

    /* The following 4 fields allow you to suppliment or override */
    /*   the country code setting. */

    /* Country Code setting involve acv, dciv and ring_thresh, */
    /* below. Set country_override to 1, to use the values below */
    /* instead of the Country Code setting. */

    unsigned char country_override;     /* Override Country Setting */
    #define CCODE_OVERRIDE     1        /* Implement Country Overrides */

    unsigned char   acz;                /* AC termination loop impedance */
    unsigned char   dciv;               /* DC IV mask */
    unsigned int    ring_thresh;        /* Ring threshold in 3 voltage incrments */

    unsigned char threshold_override;   /* Override Country Setting */
    #define THRESH_OVERRIDE     1       /* Implement threshold Overrides */

    /* The following 6 fields control the voltage and timing values */
    /*   governing line and ring related events. */
    /* Change these to override the defaults. */

    unsigned int    bl_parm;    /* Battery feed high threshold */
    unsigned int    bf_parm;    /* Battery feed low threshold */
    unsigned int    liu_parm;   /* Line-in-use (when on hook) */
    unsigned int    lnu_parm;   /* Line-not-in-use (when on hook) */
    unsigned int    lv_max;     /* Line in use Voltage threshold */
    unsigned int    pol_rev_time;    /* Polarity Reversal Detection */
};
typedef struct M1966_init_struct  M1966_CH_INIT_STRUCT_t;


/*
 *  Event control block -
 *
 */
typedef struct {
    unsigned int         event_id;    /* Event ID          */
    unsigned int         channel_id;  /* Channel ID        */
    unsigned int         event_cnt;   /* number of remaining queued events */
    unsigned int         event_data1; /* additional data 1 */
    unsigned int         event_data2; /* additional data 2 */
    unsigned int         event_data3; /* additional data 3 */
    unsigned int         event_data4; /* additional data 4 */
}
M1966_FXO_EVENT_t;

/* transmit gain table entry definition */
typedef struct {
        unsigned char tx_burst;
        unsigned char daa;
        unsigned char digital_gain;
}
TX_GAIN_ENTRY_t;

/* receive gain table entry definition */
typedef struct {
        unsigned char rxg;
        unsigned char digital_gain;
}
RX_GAIN_ENTRY_t;


/* Call progress monitor - voltage reference */
typedef enum
{
        M1966_CPROG_MON_VOLT_REF_1_5       = 0, /* 1.5 Vdc   */
        M1966_CPROG_MON_VOLT_REF_VCC_DIV_2 = 1  /* VCC/2 Vdc */
}
M1966_CPROG_MON_VOLT_REF;

/* call progress monitor - gain setting */
typedef enum
{
        M1966_CPROG_MON_GAIN_0DB           = 0, /* Gain setting of 0dB  */
        M1966_CPROG_MON_GAIN_MINUS_6DB     = 1, /* Gain setting of -6dB  */
        M1966_CPROG_MON_GAIN_MINUS_12DB    = 2, /* Gain setting of -12dB */
        M1966_CPROG_MON_GAIN_MUTE          = 3  /* Mute */
}
M1966_CPROG_MON_GAIN;

/* call progress monitor */
typedef struct m1966_cprog_monitor
{
        M1966_CPROG_MON_VOLT_REF voltage_ref;   /* Voltage reference */
        M1966_CPROG_MON_GAIN     tx_gain;       /* Tx path gain setting */
        M1966_CPROG_MON_GAIN     rx_gain;       /* Rx path gain setting */
}
M1966_CPROG_MONITOR_t;

/* country default parameters */
typedef struct m1966_cntry_struct
{
        unsigned int       cnum;                /* Country code                     */
        unsigned char      ccode[4];            /* Two letter internet country code */
        unsigned char      country[16];         /* Country Name                     */
        unsigned int       ac_impedance;
        unsigned int       dc_vi_mask;
        unsigned int       rgth_value;
        int                auto_cid_enable;     /* automatically enable CID */
        int                use_seize_state;     /* ring tone, silent duration */
} 
M1966_CNTRY_STRUCT_t;

/* pulse dial command data */
typedef struct m1966_pulse_dial_struct 
{
        unsigned int       length;              /* digit length */
        unsigned char      digits[MAX_PHONE_NMBR_DIGIT_CNT]; /* pulse dial digits */
}
M1966_PULSE_DIAL_t;


/* pulse dial timing parameters */

#define M1966_DIAL_PARAM_GET    0               /* read currrent timing parameters */
#define M1966_DIAL_PARAM_SET    1               /* write timing parameters         */

typedef struct m1966_pulse_dial_param_struct 
{
        unsigned int       command;             /* pulse dial param command - GET/SET */
        unsigned int       onhook_duration;     /* on-hook duration */
        unsigned int       offhook_duration;    /* off-hook duration */
        unsigned int       inter_digit_duration;/* intra-digit duration */
}
M1966_PULSE_DIAL_PARAM_t;

typedef enum
{
        M1966_RATE_SEL_8KHZ  = 0,               /* PCM sample rate at 8Khz */
        M1966_RATE_SEL_16KHZ = 1                /* PCM sample rate at 16Khz */
}
M1966_SAMPLE_RATE_SELECTION;


/* GPIO related definitions */

typedef enum 
{
        M1966_GPIO_NUM_5 = 0x20,                /* GPIO-5             */
        M1966_GPIO_NUM_6 = 0x40,                /* GPIO-6             */
        M1966_GPIO_NUM_7 = 0x80                 /* GPIO-7             */
}
M1966_GPIO_NUMBER;

typedef enum 
{
        M1966_GPIO_CONFIG_GET = 0,              /* GPIO config GET    */
        M1966_GPIO_CONFIG_SET = 1               /* GPIO config SET    */
}
M1966_GPIO_CONFIG_COMMAND;

typedef enum 
{
        M1966_GPIO_CONTROL_DISABLE = 0,         /* disable GPIO       */
        M1966_GPIO_CONTROL_ENABLE  = 1          /* enable GPIO        */
}
M1966_GPIO_CONTROL_TYPE;

typedef enum 
{
        M1966_GPIO_DATA_GET   = 0,              /* Read GPIO data     */
        M1966_GPIO_DATA_SET   = 1               /* Write GPIO data    */
}
M1966_GPIO_DATA_COMMAND;

typedef enum 
{
        M1966_GPIO_DATA_LOW   = 0,              /* GPIO data - low    */
        M1966_GPIO_DATA_HIGH  = 1               /* GPIO data - high   */
}
M1966_GPIO_DATA_TYPE;

typedef enum 
{
        M1966_GPIO_DIR_OUTPUT = 0,             /* GPIO pin signal direction - OUTPUT */
        M1966_GPIO_DIR_INPUT  = 1              /* GPIO pin signal direction - INPUT  */
}
M1966_GPIO_SIGNAL_DIR;

typedef enum 
{
        M1966_GPIO_POL_RISING  = 0,            /* Sig transition edge polarity - RISING  */
        M1966_GPIO_POL_FALLING = 1             /* Sig transition edge polarity - FALLING */
}
M1966_GPIO_INTR_POLARITY;

typedef struct gpio_config 
{
        M1966_GPIO_CONFIG_COMMAND command;     /* command */
        M1966_GPIO_NUMBER         gpio;        /* GPIO number     */
        M1966_GPIO_SIGNAL_DIR     direction;   /* signal direction */
        M1966_GPIO_INTR_POLARITY  polarity;    /* intr edge selection */
}
M1966_GPIO_CONFIG_t;

typedef struct gpio_data 
{
        M1966_GPIO_DATA_COMMAND   command;     /* command */
        M1966_GPIO_NUMBER         gpio;        /* GPIO number */
        M1966_GPIO_DATA_TYPE      data;        /* data */
}
M1966_GPIO_DATA_t;

typedef struct gpio_control 
{
        M1966_GPIO_CONTROL_TYPE   control;     /* control - enable/disable */
        M1966_GPIO_NUMBER         gpio;        /* gpio */
}
M1966_GPIO_CONTROL_t;

/*******************************************************************
**  Defines measuring entities and actions
*******************************************************************/

typedef enum
{
        M1966_HOOK_STATE_ON   = 0,
        M1966_HOOK_STATE_OFF  = 1,
}
M1966_HOOK_STATE;

typedef enum
{
        M1966_MEASURE_ENTITY_CURRENT = 0,
        M1966_MEASURE_ENTITY_VOLTAGE = 1,
}
M1966_MEASURE_ENTITIES;

typedef enum
{
        M1966_MEASURE_ACTION_GET     = 0,
        M1966_MEASURE_ACTION_SET     = 1,
        M1966_MEASURE_ACTION_CLEAR   = 2,
}
M1966_MEASURE_ACTION;

typedef struct m1966_measure_start_stop_struct
{
        M1966_MEASURE_ENTITIES entity;          /* sample entity - volage or current     */
        unsigned int sample_time;               /* interval between two sampling (in ms) */
        unsigned int average_sample_count;      /* sample count for average calculation  */
}
M1966_MEASURE_START_STOP_t;

typedef struct m1966_measure_update_struct
{
        unsigned int           row;             /* row number to be updated              */
        M1966_MEASURE_ENTITIES entity;          /* entity - voltage or current           */
        M1966_MEASURE_ACTION   action;          /* action - GET or SET                   */
        unsigned int           interval_min;    /* lower bound (mA or mV)                */
        unsigned int           interval_max;    /* upper bound (mA or mV)                */
        unsigned int           event;           /* Event to emit                         */
}
M1966_MEASURE_UPDATE_t;

typedef enum
{
        M1966_LOOPBACK_CMD_GET       = 0,       /* Get the current loopback session */
        M1966_LOOPBACK_CMD_SET       = 1,       /* Set (initiate) a loopback session */
        M1966_LOOPBACK_CMD_CLEAR     = 2        /* Clear (terminate) a loopback session */
}
M1966_LOOPBACK_COMMAND;

typedef enum
{
        M1966_LOOPBACK_MODE_NONE     = 0,       /* No loopback         */	
        M1966_LOOPBACK_MODE_PCMLB    = 1,       /* PCM Loopback        */
        M1966_LOOPBACK_MODE_DIGLB1   = 2,       /* Digital Loopback-1  */
        M1966_LOOPBACK_MODE_INTLB1   = 3,       /* Internal Loopback-1 */
        M1966_LOOPBACK_MODE_DIGLB2   = 4,       /* Digital Loopback-2  */
        M1966_LOOPBACK_MODE_INTLB2   = 5,       /* Internal Loopback-2 */
        M1966_LOOPBACK_MODE_ALB      = 6        /* Analog Loopback     */
}
M1966_LOOPBACK_MODE;

typedef struct m1966_loopback_struct
{
        M1966_LOOPBACK_COMMAND command;
        M1966_LOOPBACK_MODE    mode;
}
M1966_LOOPBACK_t;

typedef struct m1966_thresh_override
{
        unsigned char acz;                      /* Active Termination Loop */
        unsigned char dciv;                     /* DC current voltage charac. control */
        unsigned char rgth;                     /* Ring threshold */ 
}
M1966_THRESH_OVERRIDE_t;

typedef enum
{
        M1966_BTONE_FILTER_DISABLE   = 0,      /* Disable billing tone filter */
        M1966_BTONE_FILTER_ENABLE    = 1       /* Enable billing tone filter */
}
M1966_BTONE_FILTER_COMMAND;

typedef enum
{
        M1966_BTONE_FREQ_12KHZ       = 0,      /* 12KHz (F1) */
        M1966_BTONE_FREQ_16KHZ       = 1       /* 16KHz (F2) */
}
M1966_BTONE_FREQUENCY;

typedef struct m1966_btone_filter
{
        M1966_BTONE_FILTER_COMMAND command;    /* command */
        M1966_BTONE_FREQUENCY      frequency;  /* billing tone frequency */
}
M1966_BTONE_FILTER_t;

/*
 *
 *  Driver/Application Interface structure for IOCTL calls
 *  ======================================================
 */

struct m1966_user_cmd {         /*   Usage... */
  unsigned int   data_xchg[3];  /* IOCTL data exchange area... */
                                /*   serves as both input and output */
                                /*   depending on IOCTL request type */
                                /* */
                                /* 1. Register Request input: 0 to 36 & 99 */
                                /* 2. CID:  input:      in data_xchg[0] */
                                /*     0x00 - 0x0F: SPI device addresses */
                                /* 3. CID:  return:     in data_xchg[0] */
                                /* 4. Pulse dialing timing values */
                                /*      input:  a, b, c in data_xchg[0:2] */
                                /*      return: a, b, c in resp_data[0:2] */
                                /* 5. Compression value */
                                /*          return: in resp_data[0] */
                                /*                     0:  A-law */
                                /*                     1:  Mu-law */
                                /*                     2:  Linear */
                                /* 6. Pulse dialing digit count */
#define MODIFY_MASK (1 << 31)
#define REG_COUNT   (0x25 + 3)  /* Registers 0x00 thru 0x24 + padding */

  unsigned char  reg_resp[REG_COUNT]; /* 1. Reg Reqst return: in [0] or [0:36] */
                                      /* 2. Telephone number input [0:21]... */
                                      /*    Count of digits is in data_xchg[0] */
  char       digit_string[ MAX_PHONE_NMBR_DIGIT_CNT ];
};



typedef struct LINE_VOLUME
{
   /** Volume setting for the receiving path, speakerphone
       The value is given in dB with the range (-24dB ... 24dB) */
   int rx_gain;
   /** Volume setting for the transmitting path, microphone
       The value is given in dB with the range (-24dB ... 24dB) */
   int tx_gain;
} LINE_VOLUME_t;


#endif /* __73M1966_IO_H */
