enum led_indications
{
 LED_SIG_NO =48,//0
 LED_SIG_LOW=49,//1
 LED_SIG_MEDIUM=50,//2
 LED_SIG_HIGH=51,//3
 LED_STAT_FAILURE,//4
 LED_DATA_START,//5
 LED_DATA_END,//6
 LED_PWR_ON,//7
 LED_OTA_UPDATE//8
};

typedef enum
{
ON,
OFF
}led_state;
