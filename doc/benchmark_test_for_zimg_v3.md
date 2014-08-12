### Benchmark Tnfomation

I used webimg to replace imagemagick as a new image processing library for zimg since v3.0. This benchmark test will show you how fast webimg is.

### Test targets

- zimg v2.2.0
- zimg v3.0

### Environment

OS: CentOS release 5.8 (Final)

CPU: Intel(R) Xeon(R) CPU E5-2620 @ 2.00GHz, 12 Cores

Memory: 32GB

### Test Method

Both v2.2 and v3.0 disabled save new image. Each request will spend mass of CPU to process and compress image. All requests will send by ab from apache http util. Zimg use 12 threads to handle the requests. Different concurrency level will be tested.

```
ab -c 10 -n 10000 "http://x.x.x.x:4868/xxx?w=100&h=100&g=1"
```
### Test Result

| Concurrency     |10| 50 |  500  |
| :------:| :----: | :----: | :----:  |
| zimg v2.2  | 24.11 | 24.10 | 23.93 |
| zimg v3.0  | 683.13 | 843.03 | 839.34 |


