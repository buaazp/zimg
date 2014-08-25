#zimg#


Project zimg is a light image storage and processing system. It's written by C and it has high performance in image field. The zimg is designed for high concurrency image server. It supports many features for storing and processing images.  

Homepage: [zimg.buaa.us](http://zimg.buaa.us/)  
Author: [@招牌疯子](http://weibo.com/819880808)  
Contact me: zp@buaa.us  

[![Build Status](https://travis-ci.org/buaazp/zimg.svg?branch=master)](https://travis-ci.org/buaazp/zimg)
[![Build Status](https://drone.io/github.com/buaazp/zimg/status.png)](https://drone.io/github.com/buaazp/zimg/latest)  

### Versions:
- 08/28/2014 - zimg 3.0.0 Release. New generation.
- 04/26/2014 - zimg 2.0.0 Release. Supported distributed storage backends.
- 08/01/2013 - zimg 1.0.0 Release.

### Dependence:
> We stand on the shoulders of giants.  

webimg: A software suite to create, edit, compose, or convert bitmap images. Developed by SINA imgbed group.  
[libevhtp](https://github.com/ellzey/libevhtp): A more flexible replacement for libevent's httpd API.  
[LuaJIT](http://luajit.org/): LuaJIT is JIT compiler for the Lua language.  
[hiredis](https://github.com/redis/hiredis): Hiredis is a minimalistic C client library for the Redis database.  
[libevent](https://github.com/libevent/libevent): Provides a sophisticated framework for buffered network IO.  
[libmemcached](https://github.com/trondn/libmemcached): LibMemcached is designed to provide the greatest number of options to use Memcached.  

#### [Optional] For Storage:  
[memcached](https://github.com/memcached/memcached): A distributed memory object caching system.  
[beansdb](https://github.com/douban/beansdb): Beansdb is a distributed key-value storage system designed for large scale online system, aiming for high avaliablility and easy management.  
[beanseye](https://github.com/douban/beanseye): Beanseye is proxy and monitor for beansdb, written in Go.  
[SSDB](https://github.com/ideawu/ssdb): SSDB is a high performace key-value(key-string, key-zset, key-hashmap) NoSQL database, an alternative to Redis.  
[twemproxy](https://github.com/twitter/twemproxy): Twemproxy is a fast and lightweight proxy for memcached and redis protocol.  

### Supplying:
Uploading, downloading and processing images through HTTP protocol.  
High performance in concurrency I/O and compressing image.  
Support memcached and redis protocols to save images into distributed storage backends.  
Varied config options for operation and maintenance.  
Support lua scripts to deal with customize compressing strategy.

### Documents:
There are several documents to introduce the design and architecture of zimg:  
[Documents of zimg](http://zimg.buaa.us/documents/)  
And this picture below shows the architecture of zimg's storage:  

![architecture_of_zimg_v3](http://ww2.sinaimg.cn/large/4c422e03jw1ejjdk4vdccj20kf0momzd.jpg)

### Download:
The source code is licensed under a BSD-like License.  
All versions on [Github](https://github.com/buaazp/zimg).  

### Build:
You should build dependences above and make sure the beansdb, beanseye or ssdb backend is working well.   
 
```
git clone https://github.com/buaazp/zimg -b master --depth=1
cd zimg   
make  
cd bin  
./zimg conf/zimg.lua
```

More infomation for building zimg:
[Guide Book of zimg](http://zimg.buaa.us/documents/install/)  

### Feedback:
If you have any question, please submit comment here or mention me on [Weibo](http://weibo.com/819880808).  
Technical issues are also welcomed to be submitted on [GitHub](https://github.com/buaazp/zimg/issues).


