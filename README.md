#zimg#


zimg ([Homepage](http://zimg.buaa.us/)) is a light image storage and processing system. It's written by C and it has high performance in image field. There is a benchmark test report with more infomation: [Benchmark](http://zimg.buaa.us/benchmark.html). zimg is designed for high concurrency image server. It supports many features for uploading and getting images.  

Author: [@招牌疯子](http://weibo.com/819880808)  
Contact me: zp@buaa.us  

### Versions:
- 03/10/2014 - zimg 1.1.0 Release.
- 08/01/2013 - zimg 1.0.0 Release.

### Dependence:
> We stand on the shoulders of giants.  

[libevent](https://github.com/libevent/libevent): Provides a sophisticated framework for buffered network IO.  
[libevhtp](https://github.com/ellzey/libevhtp): A more flexible replacement for libevent's httpd API.  
[imagemagick](http://www.imagemagick.org/script/magick-wand.php): A software suite to create, edit, compose, or convert bitmap images.  
[memcached](https://github.com/memcached/memcached): A distributed memory object caching system.  
[lua](http://www.lua.org/): Lua is a lightweight multi-paradigm programming language designed as a scripting language with extensible semantics as a primary goal.  
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

### In Planning:
Distributed architecture.  
Performance optimization.  
Security measures.  

### Documentation:
There is an architecture design document of zimg. It is written in Chinese.  
[Architecture Design of Image Server](http://zimg.buaa.us/arch_design.html)

### Download:
The source code is licensed under a BSD-like License.  
All versions on [Github](https://github.com/buaazp/zimg).  

### Build:

	cd zimg  
	cmake .  
	make  
	./zimg  
	
### Example:
A zimg server in my VPS to test functions.  
[http://zimg.buaa.us:4869/](http://zimg.buaa.us:4869/)

### Feedback:
If you have any question, please submit comment here or mention me on [Weibo](http://weibo.com/819880808).  
Technical issues are also welcomed to be submitted on [GitHub](https://github.com/buaazp/zimg/issues).

