/*---------------------- COMMON ------------------------- 
[COMMON]
Fileformat=1.0
Chipset=VINETIC-2CPE V2.x
Version of Chipset=1.0
Slic type=SLIC-DC [PEF4268] for CPE
-------------------- Program Info ----------------------- 
Programname=VINETICOS
Programversion=V1.1.1.6
Programdate=06.07.2005
Calculation Date: 2005-7-7-10:34:56
CRAM coefficients for all filters
-------------------------------------------------------*/ 

static const unsigned char bbd_buf_cpe[]=
{
    0x00, 0x0A,                 /* master_tag_16 */
    0x00, 0x01,                 /* version_16 */
    0x00, 0x00, 0x00, 0x0C,     /* length_32 */
    0x21, 0x62, 0x62, 0x64,     /* magic_32 */
    0x32, 0x34, 0x31, 0x30,     /* family_type_32 */
    0x05,                       /* year_8 */
    0x07,                       /* month_8 */
    0x07,                       /* date_8 */
    0xFF,                       /* dummy_8 */
    0x10, 0x01,                 /* cram_tag_16 */
    0x00, 0x01,                 /* cram_version_16 */
    0x00, 0x00, 0x00, 0xD6,     /* cram_length_32 */
    0x00, 0x05,                 /* cram_offset_16 */
    0x00, 0x20,                 /* ac_analog, addr: 1 */ 
    0x00, 0x00,                 /* ac_rx_dc_add, addr: 2 */ 
    0x00, 0x00,                 /* ac_rx_dither_gain, addr: 3 */ 
    0xC6, 0x81,                 /* ac_im_fir_0, addr: 4 */ 
    0xE7, 0x25,                 /* ac_im_fir_1, addr: 5 */ 
    0xFE, 0x13,                 /* ac_im_fir_2, addr: 6 */ 
    0x0B, 0x19,                 /* ac_im_fir_3, addr: 7 */ 
    0x0F, 0x46,                 /* ac_im_fir_4, addr: 8 */ 
    0x0C, 0xB4,                 /* ac_im_fir_5, addr: 9 */ 
    0x06, 0x33,                 /* ac_im_fir_6, addr: 10 */ 
    0xFE, 0xDE,                 /* ac_im_fir_7, addr: 11 */ 
    0xF9, 0xB4,                 /* ac_im_fir_8, addr: 12 */ 
    0xF9, 0x2C,                 /* ac_im_fir_9, addr: 13 */ 
    0xFE, 0xE5,                 /* ac_im_fir_10, addr: 14 */ 
    0x7F, 0xE3,                 /* ac_im_lp1_0, addr: 15 */ 
    0x63, 0xD5,                 /* ac_im_lp2_0, addr: 16 */ 
    0xFA, 0x5B,                 /* ac_im_gain, addr: 17 */ 
    0x7F, 0xFC,                 /* ac_tx_im_gain, addr: 18 */ 
    0x7F, 0xFF,                 /* ac_tx_fir_0, addr: 19 */ 
    0x00, 0x00,                 /* ac_tx_fir_1, addr: 20 */ 
    0xF7, 0x68,                 /* ac_tx_fir_2, addr: 21 */ 
    0x00, 0x00,                 /* ac_tx_fir_3, addr: 22 */ 
    0x03, 0xA3,                 /* ac_tx_fir_4, addr: 23 */ 
    0x00, 0x00,                 /* ac_tx_fir_5, addr: 24 */ 
    0xFE, 0x0C,                 /* ac_tx_fir_6, addr: 25 */ 
    0x00, 0x00,                 /* ac_tx_fir_7, addr: 26 */ 
    0x00, 0xED,                 /* ac_tx_fir_8, addr: 27 */ 
    0x00, 0x00,                 /* ac_tx_fir_9, addr: 28 */ 
    0x00, 0x00,                 /* ac_tx_fir_10, addr: 29 */ 
    0x23, 0xB7,                 /* ac_tx_lp1_0, addr: 30 */ 
    0xE9, 0x2A,                 /* ac_tx_lp1_1, addr: 31 */ 
    0x35, 0x4D,                 /* ac_tx_lp1_2, addr: 32 */ 
    0xC8, 0x98,                 /* ac_tx_lp1_3, addr: 33 */ 
    0x1F, 0x44,                 /* ac_tx_lp1_4, addr: 34 */ 
    0x9C, 0x0F,                 /* ac_tx_lp1_5, addr: 35 */ 
    0x12, 0x57,                 /* ac_tx_lp1_6, addr: 36 */ 
    0x03, 0xDC,                 /* ac_tx_lp2_0, addr: 37 */ 
    0xD1, 0xE8,                 /* ac_tx_lp2_1, addr: 38 */ 
    0xDF, 0x6A,                 /* ac_tx_lp2_2, addr: 39 */ 
    0x97, 0xA8,                 /* ac_tx_lp2_3, addr: 40 */ 
    0xC7, 0x47,                 /* ac_tx_lp2_4, addr: 41 */ 
    0x6F, 0x5B,                 /* ac_tx_lp3_0, addr: 42 */ 
    0x9C, 0x16,                 /* ac_tx_lp3_1, addr: 43 */ 
    0x7D, 0x2D,                 /* ac_tx_lp3_2, addr: 44 */ 
    0x94, 0xB6,                 /* ac_tx_lp3_3, addr: 45 */ 
    0x79, 0xA7,                 /* ac_tx_lp3_4, addr: 46 */ 
    0x8C, 0x22,                 /* ac_tx_lp3_5, addr: 47 */ 
    0x76, 0x60,                 /* ac_tx_lp3_6, addr: 48 */ 
    0x83, 0xFA,                 /* ac_tx_lp3_7, addr: 49 */ 
    0x74, 0x91,                 /* ac_tx_lp3_8, addr: 50 */ 
    0x6E, 0x42,                 /* ac_tx_hp1_0, addr: 51 */ 
    0x8D, 0xD8,                 /* ac_tx_hp1_1, addr: 52 */ 
    0x7E, 0xE9,                 /* ac_tx_hp1_2, addr: 53 */ 
    0x7F, 0xEE,                 /* ac_tx_hp2_0, addr: 54 */ 
    0x44, 0x9A,                 /* ac_tx_gain1, addr: 55 */ 
    0x4A, 0x12,                 /* ac_tx_gain2, addr: 56 */ 
    0x79, 0x06,                 /* ac_tx_gain3, addr: 57 */ 
    0x7F, 0xFF,                 /* ac_rx_fir_0, addr: 58 */ 
    0x00, 0x00,                 /* ac_rx_fir_1, addr: 59 */ 
    0x00, 0x4E,                 /* ac_rx_fir_2, addr: 60 */ 
    0x00, 0x00,                 /* ac_rx_fir_3, addr: 61 */ 
    0xFD, 0x25,                 /* ac_rx_fir_4, addr: 62 */ 
    0x00, 0x00,                 /* ac_rx_fir_5, addr: 63 */ 
    0x00, 0x2E,                 /* ac_rx_fir_6, addr: 64 */ 
    0x00, 0x00,                 /* ac_rx_fir_7, addr: 65 */ 
    0x00, 0x07,                 /* ac_rx_fir_8, addr: 66 */ 
    0x00, 0x00,                 /* ac_rx_fir_9, addr: 67 */ 
    0x00, 0x00,                 /* ac_rx_fir_10, addr: 68 */ 
    0x23, 0xB7,                 /* ac_rx_lp1_0, addr: 69 */ 
    0xE9, 0x2A,                 /* ac_rx_lp1_1, addr: 70 */ 
    0x35, 0x4D,                 /* ac_rx_lp1_2, addr: 71 */ 
    0xC8, 0x98,                 /* ac_rx_lp1_3, addr: 72 */ 
    0x1F, 0x44,                 /* ac_rx_lp1_4, addr: 73 */ 
    0x9C, 0x0F,                 /* ac_rx_lp1_5, addr: 74 */ 
    0x12, 0x57,                 /* ac_rx_lp1_6, addr: 75 */ 
    0x04, 0xE6,                 /* ac_rx_lp2_0, addr: 76 */ 
    0xD2, 0xC2,                 /* ac_rx_lp2_1, addr: 77 */ 
    0xE0, 0x65,                 /* ac_rx_lp2_2, addr: 78 */ 
    0x98, 0x57,                 /* ac_rx_lp2_3, addr: 79 */ 
    0xC7, 0x1C,                 /* ac_rx_lp2_4, addr: 80 */ 
    0x74, 0xBC,                 /* ac_rx_lp3_0, addr: 81 */ 
    0x93, 0xD6,                 /* ac_rx_lp3_1, addr: 82 */ 
    0x7E, 0xBC,                 /* ac_rx_lp3_2, addr: 83 */ 
    0x8F, 0x61,                 /* ac_rx_lp3_3, addr: 84 */ 
    0x7D, 0x06,                 /* ac_rx_lp3_4, addr: 85 */ 
    0x89, 0x86,                 /* ac_rx_lp3_5, addr: 86 */ 
    0x7B, 0x46,                 /* ac_rx_lp3_6, addr: 87 */ 
    0x83, 0x3B,                 /* ac_rx_lp3_7, addr: 88 */ 
    0x7A, 0x3B,                 /* ac_rx_lp3_8, addr: 89 */ 
    0x7C, 0x00,                 /* ac_rx_hp1_0, addr: 90 */ 
    0x66, 0x67,                 /* ac_rx_gain1, addr: 91 */ 
    0x57, 0x03,                 /* ac_rx_gain2, addr: 92 */ 
    0x0D, 0x27,                 /* ac_th_fir_0, addr: 93 */ 
    0xFC, 0x3E,                 /* ac_th_fir_1, addr: 94 */ 
    0x50, 0x9E,                 /* ac_th_fir_2, addr: 95 */ 
    0xFB, 0x1F,                 /* ac_th_fir_3, addr: 96 */ 
    0xFD, 0x9C,                 /* ac_th_fir_4, addr: 97 */ 
    0x04, 0x29,                 /* ac_th_fir_5, addr: 98 */ 
    0xF4, 0x00,                 /* ac_th_fir_6, addr: 99 */ 
    0x09, 0x59,                 /* ac_th_fir_7, addr: 100 */ 
    0xFC, 0x99,                 /* ac_th_fir_8, addr: 101 */ 
    0x00, 0x7B,                 /* ac_th_fir_9, addr: 102 */ 
    0xFF, 0xBC,                 /* ac_th_fir_10, addr: 103 */ 
    0x7E, 0x00,                 /* ac_th_hp_0, addr: 104 */ 
    0x20, 0x00,                 /* ac_th_ap_0, addr: 105 */ 
    0xB4, 0x65,                 /* cram_crc_16 */
    0x10, 0x02,                 /* slictype_tag_16 */
    0x00, 0x01,                 /* slictype_version_16 */
    0x00, 0x00, 0x00, 0x02,     /* slictype_length_32 */
    0x01, 0x01,                 /* slictype_16 DC */
    0x10, 0x03,                 /* ringcfg_tag_16 */
    0x00, 0x01,                 /* ringcfg_version_16 */
    0x00, 0x00, 0x00, 0x06,     /* ringcfg_length_32 */
    0x00, 0xCC,                 /* ringcfg_ring_f_16 */
    0x39, 0x93,                 /* ringcfg_ring_a_16  */
    0x2D, 0x09,                 /* ringcfg_hook_level_16  */
    0x10, 0x04,                 /* dc_threshold_tag_16 */
    0x00, 0x01,                 /* dc_threshold_version_16 */
    0x00, 0x00, 0x00, 0x04,     /* dc_threshold_length_32 */
    0x00, 0x0C,                 /* dc_threshold_hook_duptime_16 */
    0x00, 0x40,                 /* dc_threshold_settling_on_hook_time_16   */
    0x00, 0x00,                 /* endblock_tag_16 */
    0x00, 0x00,                 /* endbock_version_16 */
    0x00, 0x00, 0x00, 0x00,     /* endblock_lenght_16 */
};

unsigned int bbd_size_cpe = sizeof(bbd_buf_cpe);

