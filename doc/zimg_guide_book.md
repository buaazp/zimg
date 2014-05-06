## zimg使用手册

[@招牌疯子](http://weibo.com/819880808)

### 综述
zimg新版本发出来后吸引了很多新同学的关注，大部分没有看过[第一版的设计文档](http://zimg.buaa.us/arch_design.html)，很可能连zimg是干嘛用的都不太清楚，所以迫切需要一篇简单介绍用途和用法的文档，本文是匆忙之下完成的，难免会有疏漏，后续会继续改进。

### 用途
zimg是一个可以存储和简单处理图片的服务器程序，最初我开发它的原因是这样的，有天我写了一款GUI小程序，做了个主页，其中有一个截图，这个截图是1280x800分辨率的，图非常大，导致每次打开主页加载都很慢。而实际上在这个网页里我只需要展示一个500x300大小的截图即可，我当然可以使用CSS来控制图片显示大小，但浏览器依然需要每次都去拉取原图。当时我就想，有没有这么一个图床，我可以在图片地址后面加上我期望的分辨率，这样它就给我返回被裁剪过的图片，降低流量，提高加载速度。

基于这样的想法，我开始写了zimg。所以说，zimg其实就是一个图床，你可以把图片上传到起了zimg的服务器上，当你需要获取这张图片的时候，你可以在图片地址后面加上自己所需要的参数，以获取被裁剪和压缩过的图片，地址大约是这样的：

````
http://127.0.0.1:4869/1f08c55a7ca155565f638b5a61e99a3e?w=500&h=300
````

由于zimg是采用C写的，跑起来非常快，再加上一些关于图片的优化，它可以用于图片量比较大的网站、公司内部的私有图片存储服务、或者各种APP的图片服务器等用途。后来zimg不断改进，到2.0版本的时候已经可以支持存储量非常大的分布式后端，这使得zimg的应用场景变得更加广泛。如果你有类似的需求，还在被PHP裁图的低效率而困扰，那么可以尝试使用zimg。

### 编译
zimg目前支持在Linux和Mac OS下运行，你需要安装一些依赖来保证它的编译和运行。  
如果你使用Mac，以下所有依赖都可以通过brew来安装。  
如果你使用ubuntu，可以使用apt-get来安装所需的依赖：

````
sudo apt-get install openssl libevent-dev cmake imagemagick lua5.1 libtolua-dev
wget https://launchpad.net/libmemcached/1.0/1.0.18/+download/libmemcached-1.0.18.tar.gz
tar zxvf libmemcached-1.0.18.tar.gz
cd libmemcached-1.0.18
./configure
make
sudo make install 
cd ..
git clone https://github.com/buaazp/zimg
cd zimg
make
````
如果你使用其他的Linux发行版，请依次安装所需的依赖。

#### openssl（可选）
openssl并非zimg必须的，但是安装它之后可以使libevent开启很多特性，建议安装。

````
wget http://www.openssl.org/source/openssl-1.0.1g.tar.gz 
tar zxvf  openssl-1.0.1g.tar.gz 
./config shared --prefix=/usr/local --openssldir=/usr/ssl 
make && make install 
````

#### libevent
````
wget http://cloud.github.com/downloads/libevent/libevent/libevent-2.0.21-stable.tar.gz 
tar zxvf libevent-2.0.17-stable.tar.gz 
./configure --prefix=/usr/local 
make && make install 
````

#### cmake
zimg的编译使用cmake工具2.8以上版本，因此你需要安装它。

````
wget "http://www.cmake.org/files/v2.8/cmake-2.8.10.2.tar.gz"tar xzvf cmake-2.8.10.2.tar.gz 
cd cmake-2.8.10.2 
./bootstrap --prefix=/usr/local 
make && make install 
````

#### imagemagick
在安装imagemagick之前需要安装几个基础图片库，不同平台的名字可能略有不同，请依据自己的系统进行安装：

````
libjpeg libjpeg-devel libpng libpng-devel libgif libgif-devel
````

装完之后安装imagemagick

````
wget http://www.imagemagick.org/download/ImageMagick-6.8.9-0.tar.gz
tar xzvf ImageMagick-6.8.9-0.tar.gz 
cd ImageMagick-6.8.9-0 
./configure  --prefix=/usr/local 
make && make install 
````

#### libmemcached
libmemcached的编译比较复杂，需要特殊版本的编译器，所以没有集成到zimg的源码中：

````
wget https://launchpad.net/libmemcached/1.0/1.0.18/+download/libmemcached-1.0.18.tar.gz
tar zxvf libmemcached-1.0.18.tar.gz
cd libmemcached-1.0.18
./configure -prefix=/usr/local --with-memcached 
make &&　make install 
````

#### lua & lua-dev
在安装lua之前你可能需要安装```readline ```和```readline-devel```

````
wget http://www.lua.org/ftp/lua-5.1.5.tar.gz  
tar zxvf lua-5.1.5.tar.gz  
cd lua-5.1.5  
make linux  
make install
````

#### memcached（可选）
zimg可以支持配置memcached作为cache，可以大幅提高热点图片的读取能力，推荐安装。

````
wget http://www.memcached.org/files/memcached-1.4.19.tar.gz
tar zxvf memcached-1.4.19.tar.gz
cd memcached-1.4.19
./configure --prefix=/usr/local
make
make install
````

#### beansdb（可选）
zimg可以选择不同的存储后端，包括beansdb和SSDB，你可以依据自己的需要进行选择，安装其中的一个即可。

````
git clone https://github.com/douban/beansdb
cd beansdb
./configure --prefix=/usr/local
make
````

如果后端选择beansdb的话，建议同时安装beansdb的代理程序beanseye，它可以使beansdb具有主从同步功能。beanseye是采用GO语言编写的，因此你要配置好系统中的GO开发环境。

````
git clone git@github.com:douban/beanseye.git
cd beanseye
make
````

#### SSDB（可选）
````
wget --no-check-certificate https://github.com/ideawu/ssdb/archive/master.zip
unzip master
cd ssdb-master
make
````

#### twemproxy（可选）
如果数据量较大，建议安装twemproxy来进行数据分片，这样可以实现分布式存储功能。

````
git clone git@github.com:twitter/twemproxy.git
cd twemproxy
autoreconf -fvi
./configure --enable-debug=log
make
src/nutcracker -h
````

#### zimg
至此所有依赖都安装完毕，可以开始编译zimg主程序。默认编译选项log中不含debug信息，如果需要详细的log，可以使用```make debug```来进行编译。

````
git clone https://github.com/buaazp/zimg
cd zimg   
make  
````

### 运行
如果需要启用缓存，你需要运行memcached；如果后端选择beansdb或SSDB，你需要按自己需要启动这些后端的一个或多个实例；如果需要使用twemproxy进行数据分片，可以使用以下配置文件启动：

````
beansdb:
  listen: 127.0.0.1:22121
  hash: fnv1a_64
  distribution: ketama
  timeout: 400
  backlog: 1024
  preconnect: true
  auto_eject_hosts: true
  server_retry_timeout: 2000
  server_failure_limit: 3
  servers:
   - 127.0.0.1:7900:1 beansdb1
   - 127.0.0.1:7901:1 beansdb2

ssdb:
  listen: 127.0.0.1:22122
  hash: fnv1a_64
  distribution: ketama
  redis: true
  timeout: 400
  backlog: 1024
  preconnect: true
  auto_eject_hosts: true
  server_retry_timeout: 2000
  server_failure_limit: 3
  servers:
   - 127.0.0.1:6380:1 ssdb1
   - 127.0.0.1:6381:1 ssdb2
````

zimg本身的所有选项都在配置文件中进行配置，你可以根据自己的需要修改配置文件：

````
--zimg server config

--server config
is_daemon=1
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
beansdb_port='22121'

--mode[3]: ssdb mode
ssdb_ip='127.0.0.1'
ssdb_port='22122'
````

然后启动zimg：

````
cd bin  
./zimg conf/zimg.lua
````

### 使用
zimg启动之后就可以开始上传和下载图片了，上传方式有两种：

第一种是通过浏览器上传，启动zimg后的默认地址就是一个简单的图片上传页：

````
http://127.0.0.1:4869/
````
大约是这个样子的：

![index.html](http://ww3.sinaimg.cn/large/4c422e03gw1eg3c74v7qbj20e704vjro.jpg)

上传成功之后会以HTML的格式返回该图片的MD5：

![upload_succ](http://ww4.sinaimg.cn/large/4c422e03gw1eg3c73pkkaj20mc04lt9o.jpg)

第二种是通过其他工具来发送POST请求上传图片，注意此上传请求是form表单类型，比如使用curl工具来上传时命令如下：

````
curl -F "blob=@testup.jpeg;type=image/jpeg" "http://127.0.0.1:4869/upload"
````

上传成功之后就可以通过不同的参数来获取图片了：

````
http://127.0.0.1:4869/1f08c55a7ca155565f638b5a61e99a3e
http://127.0.0.1:4869/1f08c55a7ca155565f638b5a61e99a3e?w=500
http://127.0.0.1:4869/1f08c55a7ca155565f638b5a61e99a3e?w=500&h=300
http://127.0.0.1:4869/1f08c55a7ca155565f638b5a61e99a3e?w=500&h=300&p=0
http://127.0.0.1:4869/1f08c55a7ca155565f638b5a61e99a3e?w=500&h=300&p=1&g=1
````

其组成格式为：
zimg服务器IP + 端口 / 图片MD5 （? + 长 + 宽 + 等比例 + 灰化）

你可以在自己的APP或网页里嵌入自己需要的URL以获取不同的图片，不同分辨率的图片第一次拉取时会实时生成，之后就会从缓存或后端中获取，无需再次压缩。

### 尾声
需要提醒的是，zimg并没有经过大型线上应用的检验，更不是微博图床所采用的方案，它只适用于小型的图床服务，难免会有bug存在。不过源码并不复杂，如果你需要的功能zimg不支持，可以很轻易地进行修改使用。



