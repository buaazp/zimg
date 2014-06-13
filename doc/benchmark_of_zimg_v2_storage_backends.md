## benchmark of zimg v2 storage backends

[@招牌疯子](http://weibo.com/819880808)

这个测试是为了检验zimg采用不同存储后端时读取数据时的性能差异，以便根据测试数据选择最佳的存储方案。

#### 测试方案

提前向三种存储后端（本地磁盘、beansdb和SSDB）中存入测试图片，zimg分别以不同的模式启动并进行测试。测试工具为ab（httpd v2.2.24自带）。为了避免测试工具对测试对象的影响，ab位于测试机A中，zimg及其存储后端位于测试机B中，AB位于同一机房中。  
测试分别在并发10~1000水平下进行，每种模式在每个并发水平下测试11次，每次发送20000个请求，记录每次测试的QPS和单请求平均完成时间。  
为了避免误差带来的影响，手工摘除掉11此测试中结果最差的一次，然后取剩余10次的平均值，以代表该模式在对应并发水平下的处理能力。  

#### 测试机

测试机为中等配置的服务器：  

```
CPU: Intel(R) Xeon(R) CPU E5-2620 @ 2.00GHz, 12 Cores  
Memory: 32GB  
OS: CentOS release 5.8  
Disk: 
  buffered disk reads: 156.39 MB/sec
  cached reads: 11024.11 MB/sec
zimg: version 2.0.0 Beta
```

#### 测试结果

在所有并发水平下local disk模式都优于其他两种存储后端，beansdb和SSDB相差不大，这主要是因为zimg的处理能力还远远达不到其二者的理论极限（beansdb可达到24428，SSDB更是到了夸张的55541），所以成了水桶效应中的短板。  
以下所有测试结果都是限定zimg请求响应时间在300ms以内级别，更高的并发水平下请求响应时间变长，已经脱离了现实意义。  
测试结果如下图所示，原始数据链接在最后。  

![qps](http://ww1.sinaimg.cn/large/4c422e03tw1efvtggyr4pj216k0vitec.jpg)

![time](http://ww2.sinaimg.cn/large/4c422e03tw1efvtgg1xsfj216k0vm77f.jpg)

[测试结果详细数据](http://ww4.sinaimg.cn/large/4c422e03tw1efvtzen5soj216v32gkjl.jpg)