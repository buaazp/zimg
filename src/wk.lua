print("hello world!")

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

function square(size)
    print("square()...")
    print("size: " .. size)
    local ret, x, y, cols

    local im_cols = webimg.cols()
    local im_rows = webimg.rows()
    print("im_cols: " .. im_cols)
    print("im_rows: " .. im_rows)
    if(im_cols > im_rows) then
        cols = im_rows
        y = 0
        x = math.ceil((im_cols - im_rows)/2)
    else
        cols = im_cols
        x = 0
        y = math.ceil((im_rows - im_cols)/2)
    end
    print("x: " .. x)
    print("y: " .. y)

    ret = webimg.wi_crop(x, y, cols, cols)
    if (ret ~= WI_OK) then
        return TS_FAILED
    end

    ret = webimg.scale(size, size)
    print("webimg.scale() ret = " .. ret)
    if (ret ~= WI_OK) then
        print("webimg.scale() failed")
        return TS_FAILED
    end
    print("webimg.scale() succ")

    return TS_OK
end

function proportion(arg)
    --print("proportion()...")
	local ret, ratio, cols, rows
    local im_cols = webimg.cols()
    local im_rows = webimg.rows()

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

	ret = webimg.scale(cols, rows)
	if (ret == WI_OK) then
        return TS_OK
    else
        return TS_FAILED
    end
end

local code = TS_FAILED
local rtype = request.pull()
print("rtype:" .. rtype)

local arg = type_list[rtype]
if (arg ~= nil) then
    local ret
    print("arg.type = " .. arg.type)
    print("arg.size = " .. arg.size)
    local switch = {
        [CT_SQUARE]         = function()    ret = square(arg.size)      end,
        [CT_PROPORTION]     = function()    ret = proportion(arg)       end,
        [CT_NONE]           = function()    ret = TS_OK                 end,
    }
    print("start scale image...")
    local action = switch[arg.type]
    if (action ~= nil) then
        action()
        if (ret ~= TS_OK) then
            print("scale image failed.")
        else
            if (webimg.quality() > arg.quality) then
                webimg.wi_set_quality(arg.quality)
                print("webimg.wi_set_quality(im, " .. arg.quality .. ")")
            end

            print("webimg.format(im) = " .. webimg.format(im))
            if (string.find(webimg.format(), "GIF") ~= nil) then
                if (arg.format ~= nil and arg.format == CF_WEBP) then
                    print("arg.format = " .. arg.format)
                    ret = webimg.wi_set_format("WEBP")
                else
                    print("arg.format = " .. arg.format)
                    ret = webimg.wi_set_format("JPEG")
                end
            end

            code = ret
            print("code = " .. code)
        end
    end
end

request.push(code)
return

