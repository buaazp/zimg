#zimg#


zimg ([Homepage](http://zimg.buaa.us/)) is a light image storage and processing system. It's written by C and it has high performance in image field. There is a benchmark test report with more infomation: [Benchmark](http://zimg.buaa.us/benchmark.html). zimg is designed for high concurrency image server. It supports many features for uploading and downloading images.  

Author: [@招牌疯子](http://weibo.com/819880808)  
Contact me: zp@buaa.us  

[![Bitdeli Badge](https://d2weczhvl823v0.cloudfront.net/buaazp/zimg/trend.png)](https://bitdeli.com/free "Bitdeli Badge")  

### Versions:
- 04/16/2014 - zimg 2.0.0 Release. *Important milestone for zimg.*
- 03/10/2014 - zimg 1.1.0 Release.
- 08/01/2013 - zimg 1.0.0 Release.

### Dependence:
> We stand on the shoulders of giants.  

[libevent](https://github.com/libevent/libevent): Provides a sophisticated framework for buffered network IO.  
[libevhtp](https://github.com/ellzey/libevhtp): A more flexible replacement for libevent's httpd API. *[Note]: zimg contains libevhtp.*  
[imagemagick](http://www.imagemagick.org/script/magick-wand.php): A software suite to create, edit, compose, or convert bitmap images.  
[memcached](https://github.com/memcached/memcached): A distributed memory object caching system.  
[lua](http://www.lua.org/): Lua is a lightweight multi-paradigm programming language designed as a scripting language with extensible semantics as a primary goal.  
#### [Optional] For beansdb backend:  
[beansdb](https://github.com/douban/beansdb): Beansdb is a distributed key-value storage system designed for large scale online system, aiming for high avaliablility and easy management.  
[beanseye](https://github.com/douban/beanseye): Beanseye is proxy and monitor for beansdb, written in Go.  
#### [Optional] For ssdb backend:  
[hiredis](https://github.com/redis/hiredis): Hiredis is a minimalistic C client library for the Redis database.  
[ssdb](https://github.com/ideawu/ssdb): SSDB is a high performace key-value(key-string, key-zset, key-hashmap) NoSQL database, an alternative to Redis.  


### Supplying:
Receive and storage users' upload images.  
Transfer image through HTTP protocol.  
Process resized and grayed image by request parameter.  
Use memcached to improve performance.  
Multi-thread support for multi-core processor server.  
Use lua for conf and other functions.  
Support SSDB mode to save images and backups.  
**Support beansdb mode to save images into distributed storage backend.**

### In Planning:
Performance optimization.  
Security measures.  

### Documentation:
There is an architecture design document of zimg v1.0.  
It is written in Chinese.  
[Architecture Design of zimg](http://zimg.buaa.us/arch_design.html)  
And this document is to introduce zimg v2.0.  
[Distributed Image Storage Server: zimg](http://zimg.buaa.us/arch_design_distributed.html)

### Download:
The source code is licensed under a BSD-like License.  
All versions on [Github](https://github.com/buaazp/zimg).  

### Build:

You should build dependences above and make sure the beansdb, beanseye or ssdb backend is working well.   
 
````
git clone https://github.com/buaazp/zimg
cd zimg   
make  
cd bin  
./zimg conf/zimg.lua
````
### Config:

````
--zimg server config

--server config
daemon=1
port=4869
thread_num=4
backlog_num=1024
max_keepalives=1

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
beansdb_port='7905'

--mode[3]: ssdb mode
ssdb_ip='127.0.0.1'
ssdb_port='8888'



````

### Feedback:
If you have any question, please submit comment here or mention me on [Weibo](http://weibo.com/819880808).  
Technical issues are also welcomed to be submitted on [GitHub](https://github.com/buaazp/zimg/issues).

