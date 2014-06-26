--zimg server config

--server config
is_daemon=1
port=4869
thread_num=4
backlog_num=1024
max_keepalives=1
retry=3
system=io.popen("uname -s"):read("*l")


--header config
--headers="Cache-Control:max-age=7776000"
etag=0

--access config
--support mask rules like "allow 10.1.121.138/24"
--NOTE: remove rule can improve performance
--upload_rule="allow 127.0.0.1;deny all"
--download_rule="allow all"

--cache config
cache=0
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
save_new=1

--mode[1]: local disk mode
img_path='./img'

--mode[2]: beansdb mode
beansdb_ip='127.0.0.1'
beansdb_port='7900'

--mode[3]: ssdb mode
ssdb_ip='127.0.0.1'
ssdb_port='8888'


