#!/bin/zsh
 
 
param=$1
 
start()
{
    fpms=`ps aux | grep -i "php-fpm" | grep -v grep | awk '{print $2}'`
    if [ ! -n "$fpms" ]; then
        sudo php-fpm -y /usr/local/etc/php/5.5/php-fpm.conf
        echo "PHP-FPM Start"
    else
        echo "PHP-FPM Already Start"
    fi
}
 
stop()
{
    fpms=`ps aux | grep -i "php-fpm" | grep -v grep | awk '{print $2}'`
    echo $fpms | xargs kill -9
 
    for pid in $fpms; do
        if echo $pid | egrep -q '^[0-9]+$'; then
            echo "PHP-FPM Pid $pid Kill"
        else
            echo "$pid IS Not A PHP-FPM Pid"
        fi
    done
}
 
case $param in
    'start')
        start;;
    'stop') 
        stop;;
    'restart')
        stop
        start;;
    *)
        echo "Usage: ./phpfpm.sh start|stop|restart";;
esac
