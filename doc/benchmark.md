## Benchmark Report ##
I wrote a PHP server which has the same features as zimg for testing. The test result below proved zimg is more efficient than PHP.  

Author: buaazp  
Date: 08/01/2013  

### Test Method ###
This suite of testing is using ab which is a tool of Apache. Every test send 100,000 requests in the same concurrency level. And the processing ability is judged by number of processing request per second. Nginx and PHP is built as a constrast.  

Test commonds:  
>ab2 -c 100 -n 100000 http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2  
>ab2 -c 100 -n 100000 http://127.0.0.1:80/zimg.php?md5=5f189d8ec57f5a5a0d3dcba47fa797e2  
>ab2 -c 100 -n 100000 http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1  
>ab2 -c 100 -n 100000 http://127.0.0.1:80/zimg.php?md5=5f189d8ec57f5a5a0d3dcba47fa797e2&w=100&h=100&g=1  

	The unit of results below is rps - request per second.

### Environment ###
OS：openSUSE 12.3  
CPU：Intel Xeon E3-1230 V2  
Memory：8GB DDR3 1333MHz  
Disk：WD 1TB 7200  
### Software Version ###
zimg：1.0.0  
Nginx：1.2.9  
PHP：5.3.17  
### Testing Result ###

| Object     |zimg| zimg+memcached |  Nginx+PHP  |
| :------:| :----: | :----: | :----:  |
| Static Image   | 2857.80 | 4995.95  |   426.56     |
| Processing  | 2799.34 | 4658.35 |  58.61  |
 
Because of special design, zimg is 6 to 79 times faster than PHP. 
### High Pressure Testing ###
There is a echo test between http module in zimg and Nginx. They receive 200,000 requests and return a sigle string "It works!". Nginx performing well without php-fpm. While zimg is more stable. I record the processing number in every different concurrency level.  

Test commonds:  
>ab2 -c 5000 -n 200000 http://127.0.0.1:4869/  
>ab2 -c 5000 -n 200000 http://127.0.0.1:80/  

Test result:  

| Concurrency | zimg | Nginx |
| :--: | :--: | :--: |
|	100	|32765.35	|33412.12|
|	300	|32991.86	|32063.05|
|	500	|31364.29	|30599.07||	1000	|28936.67|	28163.63||	2000	|27939.02|	25124.51||	3000	|28168.56|	22053.22||	4000	|28463.45|	21464.88||	5000	|27947.37|	13536.93||	6000	|27533.83|	14430.21||	7000	|27502.03|	14623.62||	8000	|26505.07|	13389.28||	9000	|27124.89|	13650.01||	10000	|27446.23|	10901.13||	11000	|26335.22|	10585.73||	12000	|27068.68|	10461.54||	13000	|26798.55|	8530.11||	14000	|26741.93|	7628.09||	15000	|26556.54|	9832.16||	16000	|26815.70|	8018.44||	17000	|27811.33|	7951.21||	18000	|25722.97|	6246.00||	19000	|26730.02|	8134.93||	20000	|27678.67|	6106.95|
Notes: After 1,000 concurrency level, nginx begins to return failed. After 9,000 level, nginx cann't complete 200,000 requests any more. So I reduce request number of nginx. zimg return all successful responses in all concurrency level.   This chart is the result of high pressure testing.
![测试结果](http://single1024.qiniudn.com/wp-content/uploads/2013/08/zimg_vs_nginx.png)