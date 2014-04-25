zimg use localdisk:
ab -c 300 -n 20000 "http://10.77.121.138:4869/0c35cd4015c17bb2e2675ba39175fafa?w=300"

[#1]
Server Software:        zimg/2.0.0
Server Hostname:        10.77.121.138
Server Port:            4869

Document Path:          /0c35cd4015c17bb2e2675ba39175fafa?w=300
Document Length:        26915 bytes

Concurrency Level:      300
Time taken for tests:   10.166 seconds
Complete requests:      20000
Failed requests:        0
Write errors:           0
Total transferred:      540180000 bytes
HTML transferred:       538300000 bytes
Requests per second:    1967.44 [#/sec] (mean)
Time per request:       152.483 [ms] (mean)
Time per request:       0.508 [ms] (mean, across all concurrent requests)
Transfer rate:          51893.02 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0   13 194.0      0    3001
Processing:    10  138  22.8    148     173
Waiting:        7  134  23.4    144     169
Total:         11  151 194.8    148    3157

Percentage of the requests served within a certain time (ms)
  50%    148
  66%    153
  75%    155
  80%    156
  90%    162
  95%    165
  98%    167
  99%    170
 100%   3157 (longest request)


[#2]
Server Software:        zimg/2.0.0
Server Hostname:        10.77.121.138
Server Port:            4869

Document Path:          /0c35cd4015c17bb2e2675ba39175fafa?w=300
Document Length:        26915 bytes

Concurrency Level:      300
Time taken for tests:   10.234 seconds
Complete requests:      20000
Failed requests:        0
Write errors:           0
Total transferred:      540188760 bytes
HTML transferred:       538308666 bytes
Requests per second:    1954.25 [#/sec] (mean)
Time per request:       153.511 [ms] (mean)
Time per request:       0.512 [ms] (mean, across all concurrent requests)
Transfer rate:          51546.14 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   2.0      0      29
Processing:    11  152  12.9    152     178
Waiting:        7  147  13.2    147     165
Total:         36  152  11.6    152     185

Percentage of the requests served within a certain time (ms)
  50%    152
  66%    159
  75%    161
  80%    161
  90%    163
  95%    165
  98%    166
  99%    167
 100%    185 (longest request)

[#3]
Server Software:        zimg/2.0.0
Server Hostname:        10.77.121.138
Server Port:            4869

Document Path:          /0c35cd4015c17bb2e2675ba39175fafa?w=300
Document Length:        26915 bytes

Concurrency Level:      300
Time taken for tests:   10.335 seconds
Complete requests:      20000
Failed requests:        0
Write errors:           0
Total transferred:      540180000 bytes
HTML transferred:       538300000 bytes
Requests per second:    1935.25 [#/sec] (mean)
Time per request:       155.018 [ms] (mean)
Time per request:       0.517 [ms] (mean, across all concurrent requests)
Transfer rate:          51044.22 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   1.5      0      20
Processing:    25  153  48.4    153    3013
Waiting:       12  148  47.5    149    3008
Total:         40  154  48.3    153    3028

Percentage of the requests served within a certain time (ms)
  50%    153
  66%    156
  75%    158
  80%    160
  90%    163
  95%    165
  98%    167
  99%    171
 100%   3028 (longest request)



zimg use ssdb:
ab -c 300 -n 20000 "http://10.77.121.138:4869/0c35cd4015c17bb2e2675ba39175fafa?w=300"

[#1]
Server Software:        zimg/2.0.0
Server Hostname:        10.77.121.138
Server Port:            4869

Document Path:          /0c35cd4015c17bb2e2675ba39175fafa?w=300
Document Length:        26915 bytes

Concurrency Level:      300
Time taken for tests:   6.685 seconds
Complete requests:      20000
Failed requests:        0
Write errors:           0
Total transferred:      540180000 bytes
HTML transferred:       538300000 bytes
Requests per second:    2991.80 [#/sec] (mean)
Time per request:       100.274 [ms] (mean)
Time per request:       0.334 [ms] (mean, across all concurrent requests)
Transfer rate:          78911.54 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    2  56.1      0    3001
Processing:    21   98  66.9     95    3009
Waiting:       10   94  66.0     92    3005
Total:         37  100  87.5     95    3110

Percentage of the requests served within a certain time (ms)
  50%     95
  66%    101
  75%    104
  80%    106
  90%    108
  95%    110
  98%    112
  99%    129
 100%   3110 (longest request)

[#2]
Server Software:        zimg/2.0.0
Server Hostname:        10.77.121.138
Server Port:            4869

Document Path:          /0c35cd4015c17bb2e2675ba39175fafa?w=300
Document Length:        26915 bytes

Concurrency Level:      300
Time taken for tests:   6.697 seconds
Complete requests:      20000
Failed requests:        0
Write errors:           0
Total transferred:      540190220 bytes
HTML transferred:       538310032 bytes
Requests per second:    2986.23 [#/sec] (mean)
Time per request:       100.461 [ms] (mean)
Time per request:       0.335 [ms] (mean, across all concurrent requests)
Transfer rate:          78766.14 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   1.4      0      21
Processing:    22   99  81.9     94    3018
Waiting:        6   95  80.9     91    3013
Total:         31  100  82.1     94    3029

Percentage of the requests served within a certain time (ms)
  50%     94
  66%    106
  75%    109
  80%    110
  90%    112
  95%    113
  98%    114
  99%    140
 100%   3029 (longest request)

[#3]
Server Software:        zimg/2.0.0
Server Hostname:        10.77.121.138
Server Port:            4869

Document Path:          /0c35cd4015c17bb2e2675ba39175fafa?w=300
Document Length:        26915 bytes

Concurrency Level:      300
Time taken for tests:   6.627 seconds
Complete requests:      20000
Failed requests:        0
Write errors:           0
Total transferred:      540180000 bytes
HTML transferred:       538300000 bytes
Requests per second:    3017.92 [#/sec] (mean)
Time per request:       99.406 [ms] (mean)
Time per request:       0.331 [ms] (mean, across all concurrent requests)
Transfer rate:          79600.63 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1  36.8      0    3001
Processing:    26   98  53.2     93    3006
Waiting:        7   94  52.1     90    3003
Total:         41   99  64.7     93    3107

Percentage of the requests served within a certain time (ms)
  50%     93
  66%    107
  75%    108
  80%    108
  90%    110
  95%    111
  98%    115
  99%    118
 100%   3107 (longest request)

zimg use beansdb double:
ab -c 300 -n 20000 "http://10.77.121.138:4869/0c35cd4015c17bb2e2675ba39175fafa?w=300"

[#1]
Server Software:        zimg/2.0.0
Server Hostname:        10.77.121.138
Server Port:            4869

Document Path:          /0c35cd4015c17bb2e2675ba39175fafa?w=300
Document Length:        49 bytes

Concurrency Level:      300
Time taken for tests:   8.938 seconds
Complete requests:      20000
Failed requests:        0
Write errors:           0
Non-2xx responses:      20000
Total transferred:      2920000 bytes
HTML transferred:       980000 bytes
Requests per second:    2237.55 [#/sec] (mean)
Time per request:       134.075 [ms] (mean)
Time per request:       0.447 [ms] (mean, across all concurrent requests)
Transfer rate:          319.03 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.6      0       8
Processing:    51  133  10.3    134     163
Waiting:       51  133  10.3    133     163
Total:         52  133  10.1    134     163

Percentage of the requests served within a certain time (ms)
  50%    134
  66%    136
  75%    139
  80%    140
  90%    143
  95%    146
  98%    149
  99%    152
 100%    163 (longest request)

[#2]
Server Software:        zimg/2.0.0
Server Hostname:        10.77.121.138
Server Port:            4869

Document Path:          /0c35cd4015c17bb2e2675ba39175fafa?w=300
Document Length:        49 bytes

Concurrency Level:      300
Time taken for tests:   8.797 seconds
Complete requests:      20000
Failed requests:        0
Write errors:           0
Non-2xx responses:      20000
Total transferred:      2920000 bytes
HTML transferred:       980000 bytes
Requests per second:    2273.63 [#/sec] (mean)
Time per request:       131.948 [ms] (mean)
Time per request:       0.440 [ms] (mean, across all concurrent requests)
Transfer rate:          324.17 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.5      0       8
Processing:     5  131  10.2    131     163
Waiting:        5  131  10.2    131     163
Total:          5  131  10.3    131     171

Percentage of the requests served within a certain time (ms)
  50%    131
  66%    133
  75%    135
  80%    136
  90%    139
  95%    142
  98%    145
  99%    148
 100%    171 (longest request)

[#3]
Server Software:        zimg/2.0.0
Server Hostname:        10.77.121.138
Server Port:            4869

Document Path:          /0c35cd4015c17bb2e2675ba39175fafa?w=300
Document Length:        49 bytes

Concurrency Level:      300
Time taken for tests:   8.816 seconds
Complete requests:      20000
Failed requests:        0
Write errors:           0
Non-2xx responses:      20001
Total transferred:      2920146 bytes
HTML transferred:       980049 bytes
Requests per second:    2268.58 [#/sec] (mean)
Time per request:       132.241 [ms] (mean)
Time per request:       0.441 [ms] (mean, across all concurrent requests)
Transfer rate:          323.47 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.5      0       7
Processing:     5  131  11.6    131     172
Waiting:        5  131  11.6    131     172
Total:          5  131  11.6    131     172

Percentage of the requests served within a certain time (ms)
  50%    131
  66%    133
  75%    135
  80%    137
  90%    141
  95%    146
  98%    152
  99%    156
 100%    172 (longest request)



zimg use beansdb single:
ab -c 300 -n 20000 "http://10.77.121.138:4869/0c35cd4015c17bb2e2675ba39175fafa?w=300"


[#1]
Server Software:        zimg/2.0.0
Server Hostname:        10.77.121.138
Server Port:            4869

Document Path:          /0c35cd4015c17bb2e2675ba39175fafa?w=300
Document Length:        26915 bytes

Concurrency Level:      300
Time taken for tests:   6.075 seconds
Complete requests:      20000
Failed requests:        0
Write errors:           0
Total transferred:      540180000 bytes
HTML transferred:       538300000 bytes
Requests per second:    3292.30 [#/sec] (mean)
Time per request:       91.122 [ms] (mean)
Time per request:       0.304 [ms] (mean, across all concurrent requests)
Transfer rate:          86837.61 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   1.7      0      26
Processing:    20   90  11.7     91     315
Waiting:       12   86   9.6     88     121
Total:         43   90  11.3     92     321

Percentage of the requests served within a certain time (ms)
  50%     92
  66%     96
  75%     97
  80%     98
  90%    100
  95%    101
  98%    103
  99%    118
 100%    321 (longest request)



[#2]
Server Software:        zimg/2.0.0
Server Hostname:        10.77.121.138
Server Port:            4869

Document Path:          /0c35cd4015c17bb2e2675ba39175fafa?w=300
Document Length:        26915 bytes

Concurrency Level:      300
Time taken for tests:   6.330 seconds
Complete requests:      20000
Failed requests:        0
Write errors:           0
Total transferred:      540204820 bytes
HTML transferred:       538324726 bytes
Requests per second:    3159.55 [#/sec] (mean)
Time per request:       94.950 [ms] (mean)
Time per request:       0.317 [ms] (mean, across all concurrent requests)
Transfer rate:          83339.97 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1  21.3      0    3000
Processing:    29   93  70.0     95    3010
Waiting:       13   90  69.8     91    3007
Total:         46   94  73.3     95    3105

Percentage of the requests served within a certain time (ms)
  50%     95
  66%    101
  75%    103
  80%    104
  90%    106
  95%    108
  98%    110
  99%    115
 100%   3105 (longest request)

[#3]
Server Software:        zimg/2.0.0
Server Hostname:        10.77.121.138
Server Port:            4869

Document Path:          /0c35cd4015c17bb2e2675ba39175fafa?w=300
Document Length:        26915 bytes

Concurrency Level:      300
Time taken for tests:   6.374 seconds
Complete requests:      20000
Failed requests:        0
Write errors:           0
Total transferred:      540196060 bytes
HTML transferred:       538315966 bytes
Requests per second:    3137.72 [#/sec] (mean)
Time per request:       95.611 [ms] (mean)
Time per request:       0.319 [ms] (mean, across all concurrent requests)
Transfer rate:          82762.94 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   1.6      0      19
Processing:    25   94  44.6     96    3012
Waiting:       11   90  43.2     93    3008
Total:         43   95  44.7     96    3027

Percentage of the requests served within a certain time (ms)
  50%     96
  66%    101
  75%    102
  80%    104
  90%    106
  95%    109
  98%    112
  99%    137
 100%   3027 (longest request)

