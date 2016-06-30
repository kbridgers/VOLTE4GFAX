#!/bin/sh /etc/rc.common

#START=41
start() {
	# Start System log
	if [ -f /sbin/syslogd ]; then
		killall syslogd 2>/dev/null
#		echo "Bringing up syslog"
		if [ "$system_log_mode" = "1" -o "$system_log_mode" = "2" ]; then
			if [ -n "$system_log_IP" -a "$system_log_IP" != "0.0.0.0" ]; then
				if [ -n "$system_log_port" -a "$system_log_port" != "0" ]; then
					if [ "$system_log_mode" = "2" ]; then
						/sbin/syslogd -L -s	$CONFIG_FEATURE_SYSTEM_LOG_BUFFER_SIZE -b $CONFIG_FEATURE_SYSTEM_LOG_BUFFER_COUNT -R $system_log_IP:$system_log_port -l $system_log_log_level
					else
						/sbin/syslogd -s $CONFIG_FEATURE_SYSTEM_LOG_BUFFER_SIZE -b $CONFIG_FEATURE_SYSTEM_LOG_BUFFER_COUNT -R $system_log_IP:$system_log_port -l $system_log_log_level
					fi
				else
					if [ "$system_log_mode" = "2" ]; then
						/sbin/syslogd -L -s $CONFIG_FEATURE_SYSTEM_LOG_BUFFER_SIZE -b $CONFIG_FEATURE_SYSTEM_LOG_BUFFER_COUNT 0 -R $system_log_IP -l $system_log_log_level
					else
						/sbin/syslogd -s $CONFIG_FEATURE_SYSTEM_LOG_BUFFER_SIZE -b $CONFIG_FEATURE_SYSTEM_LOG_BUFFER_COUNT 0 -R $system_log_IP -l $system_log_log_level
					fi
				fi
			fi
		else
			/sbin/syslogd -s $CONFIG_FEATURE_SYSTEM_LOG_BUFFER_SIZE -b $CONFIG_FEATURE_SYSTEM_LOG_BUFFER_COUNT -l $system_log_log_level
		fi
	fi
}
