--This script is used for scaling images.
--Scale methods supported:
--
-- square()
-- max_width()
-- max_size()
-- proportion()
-- crop()
--
--Request type in "resize-type:":
--
-- woriginal   mw2048  thumb30     thumb50     thumb150    thumb180    thumb300    square  cover   sq612
-- thumbnail   ms160   small       bmiddle     mw1024      mw720       mw600       mw690   mw220   mw240
-- mw205       large   wap720      ms080       wap800      wap690      wap35       wap50   wap120  wap360
-- wap180      wap128  wap176      wap240      wap320      webp720     webp440     webp360 webp180 crop
-- (more in future..)
--
--There are two libs used in this script:
--
-- request 
-- webimg
--
--More usages are in codes.



TS_OK = 0
TS_FAILED = -1

WI_OK = 0
WI_E_UNKNOW_FORMAT = 1
WI_E_LOADER_INIT = 2
WI_E_LOADER_LOAD = 3
WI_E_READ_FILE = 4
WI_E_WRITE_FILE = 5
WI_E_ENCODE = 6

CT_SQUARE = 0
CT_MAX_WIDTH = 1
CT_MAX_SIZE = 2
CT_PROPORTION = 3
CT_CROP = 4
CT_NONE = 5

CF_JPEG = 0
CF_PNG = 1
CF_GIF = 2
CF_WEBP = 3

WAP_QUALITY = 70
THUMB_QUALITY = 85
QUALITY_90 = 90
QUALITY_95 = 95

type_list = {
    woriginal = {
		type				= CT_NONE,
		quality			    = WAP_QUALITY,
    },
	mw2048 = {
		type				= CT_NONE,
		quality			    = WAP_QUALITY,
		interlace			= 1,
	},
	thumb30 = {
		type				= CT_SQUARE,
		size				= 30,
		quality			    = THUMB_QUALITY,
	},
	thumb50 = {
		type				= CT_SQUARE,
		size				= 50,
		quality			    = THUMB_QUALITY,
	},
	thumb150 = {
		type				= CT_SQUARE,
		size				= 150,
		quality			    = THUMB_QUALITY,
	},
	thumb180 = {
		type				= CT_SQUARE,
		size				= 180,
		quality		    	= THUMB_QUALITY,
	},
	thumb300 = {
		type				= CT_SQUARE,
		size				= 300,
		quality			    = THUMB_QUALITY,
	},
	square = {
		type				= CT_SQUARE,
		size				= 80,
		quality			    = THUMB_QUALITY,
	},
	cover = {
		type				= CT_SQUARE,
		size				= 200,
		quality			    = THUMB_QUALITY,
	},
	sq612 = {
		type				= CT_SQUARE,
		size				= 612,
		quality			    = QUALITY_90,
	},
	thumbnail = {
		type				= CT_MAX_SIZE,
		size				= 120,
		quality			    = THUMB_QUALITY,
	},
	ms160 = {
		type				= CT_MAX_SIZE,
		size				= 160,
		quality			    = THUMB_QUALITY,
	},
	small = {
		type				= CT_MAX_SIZE,
		size				= 200,
		quality			    = THUMB_QUALITY,
		ratio				= 3.0/10,
	},
	bmiddle = {
		type				= CT_MAX_WIDTH,
		size				= 440,
		quality			    = QUALITY_90,
	},
	mw1024 = {
		type				= CT_MAX_WIDTH,
		size				= 1024,
		quality			    = QUALITY_95,
		interlace			= 1,
	},
	mw720 = {
		type				= CT_MAX_WIDTH,
		size				= 720,
		quality			    = THUMB_QUALITY,
		interlace			= 1,
	},
	mw600 = {
		type				= CT_MAX_WIDTH,
		size				= 600,
		quality			    = QUALITY_95,
	},
	mw690 = {
		type				= CT_MAX_WIDTH,
		size				= 690,
		quality			    = QUALITY_95,
		interlace			= 1,
	},
	mw220 = {
		type				= CT_MAX_WIDTH,
		size				= 220,
		quality			    = THUMB_QUALITY,
	},
	mw240 = {
		type				= CT_MAX_WIDTH,
		size				= 240,
		quality			    = THUMB_QUALITY,
	},
	mw205 = {
		type				= CT_MAX_WIDTH,
		size				= 205,
		quality			    = THUMB_QUALITY,
	},
	large = {
		type				= CT_MAX_WIDTH,
		size				= 2048,
		quality			    = QUALITY_95,
	},
	wap720 = {
		type				= CT_MAX_WIDTH,
		size				= 720,
		quality			    = WAP_QUALITY,
	},
	ms080 = {
		type				= CT_MAX_SIZE,
		size				= 80,
		quality			    = THUMB_QUALITY,
	},
	wap800 = {
		type				= CT_MAX_SIZE,
		size				= 800,
		quality			    = WAP_QUALITY,
	},
	wap690 = {
		type				= CT_MAX_SIZE,
		size				= 690,
		quality			    = WAP_QUALITY,
	},
	wap35 = {
		type				= CT_MAX_SIZE,
		size				= 35,
		quality			    = WAP_QUALITY,
	},
	wap50 = {
		type				= CT_MAX_SIZE,
		size				= 50,
		quality			    = WAP_QUALITY,
	},
	wap120 = {
		type				= CT_MAX_SIZE,
		size				= 120,
		quality			    = WAP_QUALITY,
	},
	wap360 = {
		type				= CT_MAX_SIZE,
		size				= 360,
		ratio				= 3.0/10,
		quality			    = WAP_QUALITY,
	},
	wap180 = {
		type				= CT_MAX_SIZE,
		size				= 180,
		ratio				= 3.0/10,
		quality			    = WAP_QUALITY,
	},
	wap128 = {
		type				= CT_PROPORTION,
		cols				= 128,
		rows				= 160,
		quality			    = WAP_QUALITY,
	},
	wap176 = {
		type				= CT_PROPORTION,
		cols				= 176,
		rows				= 220,
		quality			    = WAP_QUALITY,
	},
	wap240 = {
		type				= CT_PROPORTION,
		cols				= 240,
		rows				= 320,
		quality			    = WAP_QUALITY,
	},
	wap320 = {
		type				= CT_PROPORTION,
		cols				= 320,
		rows				= 480,
		quality			    = WAP_QUALITY,
	},
	webp720 = {
		type				= CT_MAX_WIDTH,
		size				= 720,
		quality			    = WAP_QUALITY,
		format				= CF_WEBP,
	},
	webp440 = {
		type				= CT_MAX_WIDTH,
		size				= 440,
		quality			    = QUALITY_90,
		format				= CF_WEBP,
	},
	webp360 = {
		type				= CT_MAX_SIZE,
		size				= 360,
		ratio				= 3.0/10,
		quality			    = WAP_QUALITY,
		format				= CF_WEBP,
	},
	webp180 = {
		type				= CT_MAX_SIZE,
		size				= 180,
		ratio				= 3.0/10,
		quality			    = WAP_QUALITY,
		format				= CF_WEBP,
	},
	crop = {
		type				= CT_CROP,
		quality			    = QUALITY_90,
	},
}

function parse_crop_arg(str)
    --TODO
    --buaazp: need to process wrong arg.
    --print("parse_crop_arg")
    local start
    start, _ = string.find(str, "%.")
    if(start == nil) then
        return TS_FAILED
    else
        str = string.sub(str, start+1, -1)
    end
    --print("str(after): " .. str)
    local back
    start, back = string.find(str, "%d+")
    local x = tonumber(string.sub(str, start, back))
    str = string.sub(str, back+1, -1)

    start, back = string.find(str, "%d+")
    local y = tonumber(string.sub(str, start, back))
    str = string.sub(str, back+1, -1)

    start, back = string.find(str, "%d+")
    local cols = tonumber(string.sub(str, start, back))
    str = string.sub(str, back+1, -1)

    start, back = string.find(str, "%d+")
    local rows = tonumber(string.sub(str, start, back))
    str = string.sub(str, back+1, -1)

    start, back = string.find(str, "%d+")
    local cl = tonumber(string.sub(str, start, back))

    --print("x=" .. x .." y=".. y .. " cols=" .. cols .. " rows=" .. rows .. " cl=" .. cl)
    return x, y, cols, rows, cl
end


function square(im, size)
    --print("square()...")
    --print("size: " .. size)
    local ret, x, y, cols

    local im_cols = webimg.cols(im)
    local im_rows = webimg.rows(im)
    --print("im_cols: " .. im_cols)
    --print("im_rows: " .. im_rows)
    if(im_cols > im_rows) then
        cols = im_rows
        y = 0
        x = math.ceil((im_cols - im_rows)/2)
    else
        cols = im_cols
        x = 0
        y = math.ceil((im_rows - im_cols)/2)
    end
    --print("x: " .. x)
    --print("y: " .. y)

    ret = webimg.wi_crop(im, x, y, cols, cols)
    if (ret ~= WI_OK) then
        return TS_FAILED
    end

    ret = webimg.scale(im, size, 0)
    if (ret ~= WI_OK) then
        return TS_FAILED
    end

    return TS_OK
end

function max_width(im, arg)
    --print("max_width()...")
    local ret, ratio, cols, rows 
    local im_cols = webimg.cols(im)
    local im_rows = webimg.rows(im)
    --print("im_cols: " .. im_cols)
    --print("arg.size: " .. arg.size)
    if(im_cols <= arg.size) then
        --print("im_cols <= arg.size return.")
        return TS_OK
    end

    cols = arg.size
    --print("cols = " .. cols)
    if (arg.ratio ~= nil) then
        ratio = im_cols / im_rows
        if (ratio < arg.ratio) then
            cols = im_cols
            rows = math.ceil(cols / arg.ratio)
            if (rows > im_rows) then
                rows = im_rows
            end
            ret = webimg.crop(im, 0, 0, cols, rows)
            if (ret ~= WI_OK) then
                return TS_FAILED
            end
        else
            cols = math.min(arg.size, im_cols)
            rows = im_rows

            ret = webimg.crop(im, 0, 0, cols, rows)
            if(ret == WI_OK) then
                return TS_OK
            else
                return TS_FAILED
            end
        end
    else
        ret = webimg.scale(im, cols, 0)
        --print("webimg.scale() done.")
        if(ret == WI_OK) then
            return TS_OK
        else
            return TS_FAILED
        end
    end
end

function max_size(im, arg)
    --print("max_size()...")
    local ret, ratio, rows, cols
    local im_cols = webimg.cols(im)
    local im_rows = webimg.rows(im)

    if (im_cols <= arg.size and im_rows <= arg.size) then
        return TS_OK
    end

    if (arg.ratio ~= nil) then
        ratio = im_cols / im_rows
        --print("ratio = " .. ratio)
        if (im_cols < (arg.size * arg.ratio) and ratio < arg.ratio) then
            cols = im_cols
            rows = math.min(im_rows, arg.size)

            ret = webimg.crop(im, 0, 0, cols, rows)
            if (ret == WI_OK) then
                return TS_OK 
            else
                return TS_FAILED
            end
        end
        if (im_rows < (arg.size * arg.ratio) and ratio > (1.0 / arg.ratio)) then
            rows = im_rows
            cols = math.min(im_cols, arg.size)

            ret = webimg.crop(im, 0, 0, cols, rows)
            if (ret == WI_OK) then
                return TS_OK
            else
                return TS_FAILED
            end
        end

        if (im_cols > im_rows and ratio > (1.0 / arg.ratio)) then
            rows = im_rows
            cols = rows / arg.ratio
            if (cols > im_cols) then
                cols = im_cols
            end

            ret = webimg.crop(im, 0, 0, cols, rows)
            if (ret ~= WI_OK) then
                return TS_FAILED
            end
        elseif (ratio < arg.ratio) then
            cols = im_cols
            rows = cols / arg.ratio
            if (rows > im_rows) then
                rows = im_rows
            end
            ret = webimg.crop(im, 0, 0, cols, rows)
            if (ret ~= WI_OK) then
                return TS_FAILED
            end
        end
    end

    if (im_cols > im_rows) then
        cols = arg.size
        rows = 0
    else
        rows = arg.size
        cols = 0
    end

    ret = webimg.scale(im, cols, rows)
    if (ret == WI_OK) then
        return TS_OK
    else
        return TS_FAILED
    end
end

function proportion(im, arg)
    --print("proportion()...")
	local ret, ratio, cols, rows
    local im_cols = webimg.cols(im)
    local im_rows = webimg.rows(im)

	ratio = im_cols / im_rows
	if (arg.ratio ~= nil and ratio > arg.ratio) then
		cols = arg.cols
		if (im_cols < cols) then
            return TS_OK
        end
		rows = 0
	else
		rows = arg.rows
		if (im_rows < rows) then
            return TS_OK
        end
		cols = 0
    end

	ret = webimg.scale(im, cols, rows)
	if (ret == WI_OK) then
        return TS_OK
    else
        return TS_FAILED
    end
end

function crop(im, rtype)
    local ret
    local im_cols = webimg.cols(im)

    x, y, cols, rows, cl = parse_crop_arg(rtype)
    --print("arg.cl: " .. cl .. " im_cols: " .. im_cols)
    if (x == TS_FAILED) then
        --print(req.code, req.rtype, req.size)
        return TS_FAILED
    end
    --print(x, y, cols, rows)
    ret = webimg.crop(im, x, y, cols, rows)
    if (ret ~= WI_OK) then
        --print("failed! webimg.crop()")
        return TS_FAILED
    end

    if (cl > 0 and cl < im_cols) then
        ret = webimg.scale(im, cl, 0)
        --print("webimg.scale(im, " .. cl .. ", 0)")
        if (ret ~= WI_OK) then
            return TS_FAILED
        end
    end
    return TS_OK
end

local req = {}
req.rtype, req.size, req.data = request.init()
--print("req.rtype:" .. req.rtype)
req.code = TS_FAILED

local start, rtype
rtype = req.rtype
--print("rtype(before): " .. rtype)
start, _ = string.find(req.rtype, "%.")
if(start ~= nil) then
    --print("start = " .. start)
    rtype = string.sub(req.rtype, 1, start-1)
end
--print("rtype(after): " .. rtype)

local arg = type_list[rtype]
if (arg == nil) then
    --print("rtype not supported!")
else
    --print("arg.type: " .. arg.type)

    local im = webimg.new()
    if (im == nil) then
        --print("wi_new_image failed!")
    else
        --print("wi_new_image succ.")

        local ret = webimg.wi_read_blob(im, req.data, req.size)
        if (ret ~= WI_OK) then
            print("wi_read_blob failed!")
        else
            --print("wi_read_blob succ.")
            local switch = {
                [CT_SQUARE]         = function()    ret = square(im, arg.size)     end,
                [CT_MAX_WIDTH]      = function()    ret = max_width(im, arg)       end,
                [CT_MAX_SIZE]       = function()    ret = max_size(im, arg)        end,
                [CT_PROPORTION]     = function()    ret = proportion(im, arg)      end,
                [CT_CROP]           = function()    ret = crop(im, req.rtype)      end,
                [CT_NONE]           = function()    ret = TS_OK                    end,
            }
            --print("start scale image...")
            local action = switch[arg.type]
            if (action == nil) then
                print("scale code not matched!")
            else
                action()
                if (ret ~= TS_OK) then
                    print("scale image failed.")
                else
                    if (webimg.quality(im) > arg.quality) then
                        webimg.wi_set_quality(im, arg.quality)
                        --print("webimg.wi_set_quality(im, " .. arg.quality .. ")")
                    end

                    --print("webimg.format(im) = " .. webimg.format(im))
                    if (string.find(webimg.format(im), "GIF") ~= nil) then
                        if (arg.format ~= nil and arg.format == CF_WEBP) then
                            --print("arg.format = " .. arg.format)
                            ret = webimg.wi_set_format(im, "WEBP")
                        else
                            ret = webimg.wi_set_format(im, "JPEG")
                        end
                        if (ret == TS_FAILED) then
                            local result = request.push(req.code, req.size, req.data)
                            --print(result, req.code, req.rtype, req.size)
                            return
                        end
                    end

                    --print("req.size(before scale): " .. req.size)
                    req.data, req.size = webimg.wi_get_blob(im)
                    --print("req.size(after scale): " .. req.size)
                    req.code = TS_OK 
                end
            end
        end
        webimg.destroy(im)
    end
end

local result = request.push(req.code, req.size, req.data)
--print(result, req.code, req.rtype, req.size)
return


