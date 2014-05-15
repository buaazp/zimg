--zimg server config

--server config
is_daemon=1
port=4869
thread_num=4
backlog_num=1024
max_keepalives=1
retry=3
system=io.popen("uname -s"):read("*l")

--access config
download_rule="allow 127.0.0.1;allow 10.209.67.182/32;deny all"
upload_rule="allow 127.0.0.1;allow 10.209.67.182/32;deny all"

--cache config
cache=1
mc_ip='127.0.0.1'
mc_port=11211

--log config
log=1
log_name='./log/zimg.log'

--htdoc config
root_path='./www/index.html'

--storage config
--zimg support 3 ways for storage images
mode=1

--mode[1]: local disk mode
img_path='./img'

--mode[2]: beansdb mode
beansdb_ip='127.0.0.1'
beansdb_port='7900'

--mode[3]: ssdb mode
ssdb_ip='127.0.0.1'
ssdb_port='8888'


