--zimg server config

--server config
--是否后台运行
is_daemon           = 1
--绑定IP
ip                  = '0.0.0.0'
--端口
port                = 4869
--运行线程数，默认值为服务器CPU数
--thread_num        = 4
backlog_num         = 1024
max_keepalives      = 1
retry               = 3
system              = io.popen('uname -sn'):read('*l')
pwd                 = io.popen('pwd'):read('*l')

--header config
--返回时所带的HTTP header
headers             = 'Cache-Control:max-age=7776000'
--是否启用etag缓存
etag                = 1

--access config
--support mask rules like 'allow 10.1.121.138/24'
--NOTE: remove rule can improve performance
--上传接口的IP控制权限，将权限规则注释掉可以提升服务器处理能力，下同
--upload_rule       = 'allow all'
--下载接口的IP控制权限
--download_rule     = 'allow all'
--管理接口的IP控制权限
admin_rule          = 'allow 127.0.0.1'

--cache config
--是否启用memcached缓存
cache               = 1
--缓存服务器IP
mc_ip               = '127.0.0.1'
--缓存服务器端口
mc_port             = 11211

--log config
--log_level output specified level of log to logfile
--[[
LOG_FATAL 0     System is unusable
LOG_ALERT 1     Action must be taken immediately
LOG_CRIT 2      Critical conditions
LOG_ERROR 3     Error conditions
LOG_WARNING 4   Warning conditions
LOG_NOTICE 5    Normal, but significant
LOG_INFO 6      Information
LOG_DEBUG 7     DEBUG message
]]
--输出log级别
log_level           = 6
--输出log路径
log_name            = pwd .. '/log/zimg.log'

--htdoc config
--默认主页html文件路径
root_path           = pwd .. '/www/index.html'
--admin页面html文件路径
admin_path          = pwd .. '/www/admin.html'

--image process config
--禁用URL图片处理
disable_args        = 0
--禁用lua脚本图片处理
disable_type        = 0
--禁用图片放大
disable_zoom_up     = 0
--禁用自动调整照片方向
disable_auto_orient = 0
--禁用progressive jpeg
disable_progressive = 0
--max pixel for w and h in args
--允许返回的最大尺寸，超过将返回错误
max_pixel           = 2000
--lua process script
--lua脚本文件路径
script_name         = pwd .. '/script/process.lua'
--format value: 'none' for original or other format names
--默认保存新图的格式，字符串'none'表示以原有格式保存，或者是期望使用的格式名
format              = 'jpeg'
--quality value: 1~100(default: 75)
--默认保存新图的质量
quality             = 75

--storage config
--zimg support 3 ways for storage images
--value 1 is for local disk storage;
--value 2 is for memcached protocol storage like beansdb;
--value 3 is for redis protocol storage like SSDB.
--存储后端类型，1为本地存储，2为memcached协议后端如beansdb，3为redis协议后端如SSDB
mode                = 1
--save_new value: 0.don't save any 1.save all 2.only save types in lua script
--新文件是否存储，0为不存储，1为全都存储，2为只存储lua脚本产生的新图
save_new            = 1
--上传图片大小限制，默认100MB
max_size            = 100*1024*1024
--允许上传图片类型列表
allowed_type        = {'jpeg', 'jpg', 'png', 'gif', 'webp'}

--mode[1]: local disk mode
--本地存储时的存储路径
img_path            = pwd .. '/img'

--mode[2]: beansdb mode
--beansdb服务器IP
beansdb_ip          = '127.0.0.1'
--beansdb服务器端口
beansdb_port        = 7900

--mode[3]: ssdb mode
--SSDB服务器IP
ssdb_ip             = '127.0.0.1'
--SSDB服务器端口
ssdb_port           = 8888

--SSL config
--HTTPS配置文件路径
--pemfile           = "conf/server.crt"
--privfile          = "conf/server.key"
--cafile            = "conf/ca.crt"

--lua conf functions
--部分与配置有关的函数在lua中实现，对性能影响不大
function is_img(type_name)
    local found = -1
    for _, allowed in pairs(allowed_type) do
        if string.lower(type_name) == allowed then
            found = 1
            break
        end
    end
    return found
end

