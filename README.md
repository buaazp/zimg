#zimg#


Project zimg is a light image storage and processing system. It's written by C and it has high performance in image field. The zimg is designed for high concurrency image server. It supports many features for storing and processing images.  

Homepage: [zimg.buaa.us](http://zimg.buaa.us/)  
Author: [@招牌疯子](http://weibo.com/819880808)  
Contact me: zp@buaa.us  

[![Build Status](https://travis-ci.org/buaazp/zimg.svg?branch=master)](https://travis-ci.org/buaazp/zimg)
[![Build Status](https://drone.io/github.com/buaazp/zimg/status.png)](https://drone.io/github.com/buaazp/zimg/latest)  

### Versions:
- 09/09/2014 - zimg 3.1.0 Release. New generation.
- 04/26/2014 - zimg 2.0.0 Release. Supported distributed storage backends.
- 08/01/2013 - zimg 1.0.0 Release.

### Synopsis
- The zimg is an image storage and processing server. You can get a compressed and scaled image from zimg with the parameters of URL.  
http://demo.buaa.us/5f189d8ec57f5a5a0d3dcba47fa797e2?w=500&h=500&g=1&x=0&y=0&r=45&q=75&f=jpeg

- The parameters contain width, height, resize type, gray, crop postion (x, y), rotate, quality and format. And you can control the default type of images by configuration file.  
And you can get the information of image in zimg server like this:  
http://demo.buaa.us/5f189d8ec57f5a5a0d3dcba47fa797e2?info=1

- If you want to customize the transform rule of image you can write a zimg-lua script. Use `t=type` parameter in your URL to get the special image:  
[http://demo.buaa.us/5f189d8ec57f5a5a0d3dcba47fa797e2?t=webp500](http://demo.buaa.us/5f189d8ec57f5a5a0d3dcba47fa797e2?t=webp500)

- The concurrent I/O, distributed storage and in time processing ability of zimg is excellent. You needn't nginx in your image server any more. In the benchmark test, zimg can deal with 3000+ image downloading task per second and 90000+ HTTP echo request per second on a high concurrency level. The performance is higher than PHP or other image processing server. More infomation of zimg is in the documents below.

### Supplying:
Uploading, downloading and processing images through HTTP protocol.  
High performance in concurrency I/O and compressing image.  
Support memcached and redis protocols to save images into distributed storage backends.  
Varied config options for operation and maintenance.  
Support lua scripts to deal with customize compressing strategy.

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
[Install Guide](http://zimg.buaa.us/documents/install/) 

### Thanks to:
> We stand on the shoulders of giants.  

[libevhtp](https://github.com/ellzey/libevhtp): A more flexible replacement for libevent's httpd API.  
[LuaJIT](http://luajit.org/): LuaJIT is JIT compiler for the Lua language.  
[imagemagick](http://www.imagemagick.org/): A software suite to create, edit, compose, or convert bitmap images.  
[hiredis](https://github.com/redis/hiredis): Hiredis is a minimalistic C client library for the Redis database.  
[libmemcached](https://github.com/trondn/libmemcached): LibMemcached is designed to provide the greatest number of options to use Memcached.  

#### [Optional] For Storage:  
[memcached](https://github.com/memcached/memcached): A distributed memory object caching system.  
[beansdb](https://github.com/douban/beansdb): Beansdb is a distributed key-value storage system designed for large scale online system, aiming for high avaliablility and easy management.  
[beanseye](https://github.com/douban/beanseye): Beanseye is proxy and monitor for beansdb, written in Go.  
[SSDB](https://github.com/ideawu/ssdb): SSDB is a high performace key-value(key-string, key-zset, key-hashmap) NoSQL database, an alternative to Redis.  
[twemproxy](https://github.com/twitter/twemproxy): Twemproxy is a fast and lightweight proxy for memcached and redis protocol.  


### Documents:
There are several documents to introduce the design and architecture of zimg:  
[Documents of zimg](http://zimg.buaa.us/documents/)  
And this picture below shows the architecture of zimg's storage:  

![architecture_of_zimg_v3](http://ww2.sinaimg.cn/large/4c422e03jw1ejjdk4vdccj20kf0momzd.jpg)

### Download:
The source code is licensed under a BSD-like License.  
All versions on [Github](https://github.com/buaazp/zimg/releases).  

### Feedback:
If you have any question, please submit comment here or mention me on [Weibo](http://weibo.com/819880808).  
Technical issues are also welcomed to be submitted on [GitHub](https://github.com/buaazp/zimg/issues).


