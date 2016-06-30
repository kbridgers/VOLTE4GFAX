#!/bin/sh
# Auto-startup script for minidlna - used by mountd

ms_BIN=/usr/bin/minidlna
ms_PID=/var/run/minidlna.pid
ms_CFG=/tmp/minidlna.conf
ms_PORT=8200
ms_IFACE=br0

if [ ! "$ENVLOADED" ]; then
	if [ -r /etc/rc.conf ]; then
		. /etc/rc.conf 2> /dev/null
		ENVLOADED="1"
	fi
fi

write_cfg ()
{
	echo -en "port=$ms_PORT\nnetwork_interface=$ms_IFACE\n" > $ms_CFG
	echo -en "$1\n"|grep media_dir >> $ms_CFG
	echo "friendly_name=$ms_0_name" >> $ms_CFG
	if [ "$ms_0_extDb" = "1" ]; then
		echo "db_dir=/mnt/usb$ms_0_extDbPath" >> $ms_CFG
	else
		echo "db_dir=/mnt/usb/mediacache" >> $ms_CFG
	fi
	echo "album_art_names=Cover.jpg/cover.jpg/AlbumArtSmall.jpg/albumartsmall.jpg/AlbumArt.jpg/albumart.jpg/Album.jpg/album.jpg/Folder.jpg/folder.jpg/Thumb.jpg/thumb.jpg" >> $ms_CFG
	echo -en "inotify=no\nenable_tivo=no\nstrict_dlna=no\nnotify_interval=900\n" >> $ms_CFG
	echo -en "serial=12345678\nmodel_number=1\n" >> $ms_CFG
}

start ()
{
	if [ "$ms_0_ena" = "1" ]; then
		if [ "$ms_0_scanStart" = "1" ]; then
			ms_OPTS="-R"
		fi
		$ms_BIN -P $ms_PID -f $ms_CFG $ms_OPTS
	fi
}

stop ()
{
	killall minidlna
	if [ -f "$ms_PID" ]; then
		killall -9 minidlna
		rm -f $ms_PID
	fi
	ps -ef | grep minidlna >/dev/null 2>/dev/null
	[ $? -eq 0 ] && killall -9 minidlna
	rm -f $ms_CFG
}

check ()
{
	local ml_path;
	local md_dir;
	if [ "$ms_0_ena" = "1" ]; then
		if [ -d "/mnt/usb$ms_0_extDbPath" ]; then
			if [ -n "$media_locations_Count" -a "$media_locations_Count" -gt "0" ]; then
				local i=0; local pfound=0;
				while [ $i -lt $media_locations_Count ]; do
					eval ml_path='$ml_'$i'_path';
					eval ml_type='$ml_'$i'_type';
					[ -n "$ml_path" ] && ml_path="/mnt/usb$ml_path";
					if [ -d "$ml_path" ]; then
						pfound=$((pfound+1));
						case "$ml_type" in
							0) md_dir="$md_dir\nmedia_dir=$ml_path";;
							1) md_dir="$md_dir\nmedia_dir=P,$ml_path";;
							2) md_dir="$md_dir\nmedia_dir=A,$ml_path";;
							3) md_dir="$md_dir\nmedia_dir=V,$ml_path";;
						esac
					fi;
					i=$((i+1));
				done
				if [ "$pfound" = "0" ]; then
					echo "Stopping Media server.."
					stop
				elif [ -f $ms_CFG ] && [ "`grep -w media_dir $ms_CFG`" = "`echo -en $md_dir|grep -w media_dir`" ]; then
					echo "Media server: No configuration update. Do nothing.."
				else
					echo "Restarting Media server.."
					stop
					write_cfg "$md_dir"
					start
				fi
			else
				echo "No Media directories configured. Stopping Media server.."
				stop
			fi
		else
			echo "Media db path missing. Stopping media server.."
			stop
		fi
	else
		echo "Media server not enabled. Stopping Media server.."
		stop
	fi
}

media_check ()
{
	local ival;
	if [ -f "/tmp/ltq_dlna_check" ]; then
		ival=`cat /tmp/ltq_dlna_check`;
		[ -n "$ival" ] && ival=$((ival+1)) || ival=0;
	else
		ival=0;
	fi
	
	echo $ival > /tmp/ltq_dlna_check;
	
	local icnt=0
	while :; do
		[ "$ival" = "`cat /tmp/ltq_dlna_check`" ] && sleep 1 || break;
		icnt=$((icnt+1));
		if [ $icnt -ge 6 ]; then
			rm -f /tmp/ltq_dlna_check
			check;
			break;
		fi
	done
}

([ -n "$1" ] && [ "$1" = "restart" ]) && {
	stop
}

media_check &

