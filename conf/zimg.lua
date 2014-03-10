--zimg server config

--server config
daemon=0
port=4869
thread_num=4
backlog_num=1024
max_keepalives=1
system=io.popen("uname -s"):read("*l")

--cache config
cache=0
mc_ip='127.0.0.1'
mc_port=11211

--log config
log=0
log_name='./log/zimg.log'

--htdoc config
root_path='./www/index.html'

--storage config
--zimg support 2 ways for storage images
mode=2

--local disk mode
img_path='./img'

--ssdb mode
ssdb_ip='127.0.0.1'
ssdb_port='8888'


