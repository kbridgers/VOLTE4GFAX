/etc/rc.d/node_led.sh start
echo "led node opened";
/etc/rc.d/led_updater &
/etc/rc.d/node_gpio.sh start
/etc/rc.d/node_codec.sh start
echo "codec node opened"
echo "configuring the codec"
#/etc/rc.d/working_configs
echo "codec configuration finished"
