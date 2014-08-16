### Benchmark Tnfomation

We used webimg to replace imagemagick as a new image processing library in zimg since v3.0. This benchmark test will show you how fast webimg is. 

### Test targets

- zimg v2.2.0
- zimg v3.0
- zimg v3.0 with lua

### Environment

OS：openSUSE 12.3  
CPU：Intel Xeon E3-1230 V2 4 cores 8 threads  
Memory：8GB DDR3 1333MHz  
Disk：WD 1TB 7200  

### Test Method

Both v2.2 and v3.0 disabled save new image option. Each request will spend mass of CPU to process and compress image. All requests will send by [wrk](https://github.com/wg/wrk). Both zimg use 8 threads to handle the requests. Different concurrency levels will be tested independently.

```
wrk -v -c10 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1"
wrk -v -c10 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?t=test"
```

NOTE: In the zimg-lua mode, type test is designed to do the same thing of request above.

### Result Summary

| Concurrency | 10 | 50 | 100 | 300 | 500 |
| :------:| :----: | :----: | :----:  | :----:  | :----:  |
| zimg v2.2 | 75.80 | 75.72 | 75.65 | 76.04 | 76.47 |
| zimg v3.0 | 1046.22 | 1040.68 | 1039.42 | 1059.20 | 1059.52 |
| zimg-lua | 988.85 | 987.23 | 986.14 | 996.33 | 986.20 |

### Result Details

#### zimg v2.2.0

```
➜ wrk git:(master) ✗ wrk -v -c10 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1
  8 threads and 10 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   109.04ms   21.52ms 177.02ms   68.52%
    Req/Sec     8.92      1.89    16.00     52.43%
  2216 requests in 30.01s, 5.44MB read
  Socket errors: connect 0, read 2216, write 0, timeout 0
Requests/sec:     73.85
Transfer/sec:    185.49KB

➜ wrk git:(master) ✗ wrk -v -c50 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1
  8 threads and 50 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   648.86ms   72.95ms 845.11ms   74.93%
    Req/Sec     8.95      3.04    22.00     82.82%
  2247 requests in 30.01s, 5.51MB read
  Socket errors: connect 0, read 2247, write 0, timeout 0
Requests/sec:     74.88
Transfer/sec:    188.08KB

➜ wrk git:(master) ✗ wrk -v -c100 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1
  8 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     1.29s   123.14ms   1.51s    77.72%
    Req/Sec     8.99      4.68    22.00     72.86%
  2250 requests in 30.01s, 5.52MB read
  Socket errors: connect 0, read 2250, write 0, timeout 0
Requests/sec:     74.98
Transfer/sec:    188.33KB

➜ wrk git:(master) ✗ wrk -v -c300 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1
  8 threads and 300 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     3.91s   625.45ms   4.42s    93.66%
    Req/Sec     9.40      7.74    28.00     58.21%
  2252 requests in 30.01s, 5.52MB read
  Socket errors: connect 0, read 2252, write 0, timeout 2160
Requests/sec:     75.04
Transfer/sec:    188.47KB

➜ wrk git:(master) ✗ wrk -v -c500 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1
  8 threads and 500 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     6.24s     1.33s    7.08s    88.53%
    Req/Sec     9.33      8.34    35.00     64.55%
  2267 requests in 30.01s, 5.56MB read
  Socket errors: connect 0, read 2266, write 0, timeout 4942
Requests/sec:     75.53
Transfer/sec:    189.71KB
```

#### zimg v3.0.0

```
➜ wrk git:(master) ✗ wrk -v -c10 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1
  8 threads and 10 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     5.98ms    1.50ms  17.70ms   76.18%
    Req/Sec   173.85     37.98   285.00     56.68%
  40407 requests in 30.00s, 89.83MB read
  Socket errors: connect 0, read 40404, write 0, timeout 0
Requests/sec:   1346.82
Transfer/sec:      2.99MB
```

```
➜ wrk git:(master) ✗ wrk -v -c50 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1
  8 threads and 50 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    35.69ms    3.68ms  56.03ms   70.28%
    Req/Sec   168.49     14.37   230.00     64.76%
  40313 requests in 30.00s, 89.62MB read
  Socket errors: connect 0, read 40310, write 0, timeout 0
Requests/sec:   1343.75
Transfer/sec:      2.99MB
```

```
➜ wrk git:(master) ✗ wrk -v -c100 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1
  8 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    72.65ms    6.23ms 123.09ms   72.62%
    Req/Sec   165.37     10.65   203.00     72.79%
  39682 requests in 30.00s, 88.21MB read
  Socket errors: connect 0, read 39681, write 0, timeout 0
Requests/sec:   1322.64
Transfer/sec:      2.94MB
```

```
➜ wrk git:(master) ✗ wrk -v -c300 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1
  8 threads and 300 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   223.00ms   10.46ms 273.89ms   71.03%
    Req/Sec   165.84      5.16   184.00     74.00%
  39868 requests in 30.00s, 88.63MB read
  Socket errors: connect 0, read 39862, write 0, timeout 0
Requests/sec:   1328.88
Transfer/sec:      2.95MB
```

```
➜ wrk git:(master) ✗ wrk -v -c500 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?w=100&h=100&g=1
  8 threads and 500 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   367.65ms   15.11ms 513.09ms   77.53%
    Req/Sec   167.44     17.27   205.00     68.59%
  40261 requests in 30.00s, 89.50MB read
  Socket errors: connect 0, read 40256, write 0, timeout 0
Requests/sec:   1342.05
Transfer/sec:      2.98MB
```

#### zimg v3.0.0 using lua

```
➜ wrk git:(master) ✗ wrk -v -c10 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?t=test"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?t=test
  8 threads and 10 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     6.15ms    1.75ms  33.46ms   79.81%
    Req/Sec   169.92     39.44   307.00     71.39%
  39493 requests in 30.00s, 76.42MB read
  Socket errors: connect 0, read 39490, write 0, timeout 0
Requests/sec:   1316.37
Transfer/sec:      2.55MB

➜ wrk git:(master) ✗ wrk -v -c50 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?t=test"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?t=test
  8 threads and 50 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    36.74ms    4.67ms  94.37ms   77.17%
    Req/Sec   163.68     15.19   216.00     69.51%
  39176 requests in 30.00s, 75.81MB read
  Socket errors: connect 0, read 39174, write 0, timeout 0
Requests/sec:   1305.69
Transfer/sec:      2.53MB

➜ wrk git:(master) ✗ wrk -v -c100 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?t=test"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?t=test
  8 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    72.70ms    5.48ms 102.96ms   69.67%
    Req/Sec   165.01      9.06   206.00     69.74%
  39602 requests in 30.00s, 76.63MB read
  Socket errors: connect 0, read 39601, write 0, timeout 0
Requests/sec:   1319.99
Transfer/sec:      2.55MB

➜ wrk git:(master) ✗ wrk -v -c300 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?t=test"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?t=test
  8 threads and 300 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   221.35ms   14.53ms 410.69ms   85.99%
    Req/Sec   165.96     18.67   265.00     79.23%
  39750 requests in 30.00s, 76.92MB read
  Socket errors: connect 0, read 39745, write 0, timeout 0
Requests/sec:   1324.93
Transfer/sec:      2.56MB

➜ wrk git:(master) ✗ wrk -v -c500 -t8 -d30s "http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?t=test"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/5f189d8ec57f5a5a0d3dcba47fa797e2?t=test
  8 threads and 500 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   369.83ms   34.16ms 747.57ms   95.59%
    Req/Sec   165.38     21.41   339.00     96.30%
  39647 requests in 30.00s, 76.72MB read
  Socket errors: connect 0, read 39644, write 0, timeout 0
Requests/sec:   1321.39
Transfer/sec:      2.56MB
```

### HTTP echo Test

```
➜ wrk git:(master) ✗ wrk -v -c300 -t8 -d30s "http://127.0.0.1:4869/echo"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/echo
  8 threads and 300 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     2.60ms   29.02ms 587.80ms   99.69%
    Req/Sec    12.05k     3.96k   27.89k    69.18%
  2734945 requests in 30.00s, 354.72MB read
  Socket errors: connect 0, read 2734910, write 0, timeout 10
Requests/sec:  91177.88
Transfer/sec:     11.83MB
```

```
➜ wrk git:(master) ✗ wrk -v -c300 -t8 -d30s "http://127.0.0.1:4869/echo"
wrk 3.1.1 [epoll] Copyright (C) 2012 Will Glozer
Running 30s test @ http://127.0.0.1:4869/echo
  8 threads and 300 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     3.51ms   37.83ms 750.66ms   99.53%
    Req/Sec    11.94k     4.05k   28.22k    69.24%
  2710556 requests in 29.99s, 374.82MB read
  Socket errors: connect 0, read 2710497, write 0, timeout 0
Requests/sec:  90369.03
Transfer/sec:     12.50MB
```
