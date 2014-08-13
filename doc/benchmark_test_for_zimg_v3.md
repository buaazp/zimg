### Benchmark Tnfomation

We used webimg to replace imagemagick as a new image processing library in zimg since v3.0. This benchmark test will show you how fast webimg is.

### Test targets

- zimg v2.2.0
- zimg v3.0
- zimg v3.0 with lua

### Environment

OS：openSUSE 12.3  
CPU：Intel Xeon E3-1230 V2  
Memory：8GB DDR3 1333MHz  
Disk：WD 1TB 7200  

### Test Method

Both v2.2 and v3.0 disabled save new image option. Each request will spend mass of CPU to process and compress image. All requests will send by ab from apache httpd util. Both zimg use 4 threads to handle the requests. Different concurrency levels will be tested independently.

```
ab -c 10 -n 10000 "http://127.0.0.1:4868/xxx?w=100&h=100&g=1"
ab -c 10 -n 10000 "http://127.0.0.1:4869/xxx?t=test"
```

In the zimg-lua mode, type test is designed to do the same thing of request above.

### Test Result

| Concurrency | 10 | 50 | 100 | 300 | 500 |
| :------:| :----: | :----: | :----:  | :----:  | :----:  |
| zimg v2.2 | 75.80 | 75.72 | 75.65 | 76.04 | 76.47 |
| zimg v3.0 | 1046.22 | 1040.68 | 1039.42 | 1059.20 | 1059.52 |
| zimg-lua | 988.85 | 987.23 | 986.14 | 996.33 | 986.20 |


