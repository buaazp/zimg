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
	crop200 = {
		type				= CT_CROP,
        size                = 200,
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
    if im_cols > im_rows then
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
    if ret ~= WI_OK then
        return TS_FAILED
    end

    ret = webimg.scale(size, size)
    print("webimg.scale() ret = " .. ret)
    if ret ~= WI_OK then
        print("webimg.scale() failed")
        return TS_FAILED
    end

    print("webimg.scale() succ")
    return TS_OK
end

function max_width(arg)
    print("max_width()...")
    local ret, ratio, cols, rows 
    local im_cols = webimg.cols()
    local im_rows = webimg.rows()
    print("im_cols: " .. im_cols)
    print("arg.size: " .. arg.size)
    if im_cols <= arg.size then
        print("im_cols <= arg.size return.")
        return TS_OK
    end

    cols = arg.size
    print("cols = " .. cols)
    if arg.ratio then
        ratio = im_cols / im_rows
        if ratio < arg.ratio then
            cols = im_cols
            rows = math.ceil(cols / arg.ratio)
            if rows > im_rows then
                rows = im_rows
            end
            ret = webimg.crop(0, 0, cols, rows)
            if ret ~= WI_OK then
                return TS_FAILED
            end
        else
            cols = math.min(arg.size, im_cols)
            rows = im_rows

            ret = webimg.crop(0, 0, cols, rows)
            if ret == WI_OK then
                return TS_OK
            else
                return TS_FAILED
            end
        end
    else
        rows = math.ceil((cols / im_cols) * im_rows);
        ret = webimg.scale(cols, rows)
        print("webimg.scale(" .. cols .. ", " .. rows .. ") done.")
        if ret == WI_OK then
            return TS_OK
        else
            return TS_FAILED
        end
    end
end

function max_size(arg)
    print("max_size()...")
    local ret, ratio, rows, cols
    local im_cols = webimg.cols(im)
    local im_rows = webimg.rows(im)

    if im_cols <= arg.size and im_rows <= arg.size then
        return TS_OK
    end

    if arg.ratio then
        ratio = im_cols / im_rows
        print("ratio = " .. ratio)
        if im_cols < (arg.size * arg.ratio) and ratio < arg.ratio then
            cols = im_cols
            rows = math.min(im_rows, arg.size)

            ret = webimg.crop(0, 0, cols, rows)
            if (ret == WI_OK) then
                return TS_OK 
            else
                return TS_FAILED
            end
        end
        if im_rows < (arg.size * arg.ratio) and ratio > (1.0 / arg.ratio) then
            rows = im_rows
            cols = math.min(im_cols, arg.size)

            ret = webimg.crop(0, 0, cols, rows)
            if (ret == WI_OK) then
                return TS_OK
            else
                return TS_FAILED
            end
        end

        if im_cols > im_rows and ratio > (1.0 / arg.ratio) then
            rows = im_rows
            cols = rows / arg.ratio
            if (cols > im_cols) then
                cols = im_cols
            end

            ret = webimg.crop(0, 0, cols, rows)
            if (ret ~= WI_OK) then
                return TS_FAILED
            end
        elseif ratio < arg.ratio then
            cols = im_cols
            rows = cols / arg.ratio
            if (rows > im_rows) then
                rows = im_rows
            end
            ret = webimg.crop(0, 0, cols, rows)
            if (ret ~= WI_OK) then
                return TS_FAILED
            end
        end
    end

    if im_cols > im_rows then
        cols = arg.size
        rows = math.ceil((cols / im_cols) * im_rows);
    else
        rows = arg.size
        cols = math.ceil((rows / im_rows) * im_cols);
    end
    print("cols = " .. cols .. " rows = ".. rows)

    ret = webimg.scale(cols, rows)
    if ret == WI_OK then
        return TS_OK
    else
        return TS_FAILED
    end
end

function proportion(arg)
    print("proportion()...")
	local ret, ratio, cols, rows
    local im_cols = webimg.cols()
    local im_rows = webimg.rows()

	ratio = im_cols / im_rows
	if arg.ratio and ratio > arg.ratio then
		cols = arg.cols
		if im_cols < cols then
            return TS_OK
        end
        rows = math.ceil((cols / im_cols) * im_rows);
	else
		rows = arg.rows
		if im_rows < rows then
            return TS_OK
        end
        cols = math.ceil((rows / im_rows) * im_cols);
    end

    print("cols = " .. cols .. " rows = ".. rows)
	ret = webimg.scale(cols, rows)
	if ret == WI_OK then
        return TS_OK
    else
        return TS_FAILED
    end
end

function crop(arg)
    print("crop()...")
    local ret, x, y, cols, rows
    local im_cols = webimg.cols()
    local im_rows = webimg.rows()

    if arg.cols then
        cols = arg.cols
    else
        cols = arg.size
    end
    if arg.rows then
        rows = arg.rows
    else
        rows = arg.size
    end

    if cols > im_cols then
        cols = im_cols
    end
    if rows > im_rows then
        rows = im_rows
    end

    x = math.ceil((im_cols - cols) / 2.0);
    y = math.ceil((im_rows - rows) / 2.0);

    ret = webimg.crop(x, y, cols, rows)
    print("webimg.crop(" .. x .. y .. cols .. rows .. ")")
    if ret ~= WI_OK then
        return TS_FAILED
    end
    return TS_OK
end

local code = TS_FAILED
local rtype = request.pull()
print("rtype:" .. rtype)

local arg = type_list[rtype]
if arg then
    local ret = -1
    print("arg.type = " .. arg.type)
    local switch = {
        [CT_SQUARE]         = function()    ret = square(arg.size)     end,
        [CT_MAX_WIDTH]      = function()    ret = max_width(arg)       end,
        [CT_MAX_SIZE]       = function()    ret = max_size(arg)        end,
        [CT_PROPORTION]     = function()    ret = proportion(arg)      end,
        [CT_CROP]           = function()    ret = crop(arg)            end,
        [CT_NONE]           = function()    ret = TS_OK                end,
    }
    print("start scale image...")
    local action = switch[arg.type]
    if action then
        action()
        if ret == TS_OK then
            if arg.quality and webimg.quality() > arg.quality then
                webimg.wi_set_quality(arg.quality)
                print("webimg.wi_set_quality(" .. arg.quality .. ")")
            end

            local format = webimg.format()
            print("webimg.format() = " .. format)
            if not string.find(format, "GIF") then
                print("not GIF, change")
                if arg.format and arg.format == CF_WEBP then
                    print("arg.format = WEBP")
                    ret = webimg.wi_set_format("WEBP")
                else
                    print("arg.format = JPEG")
                    ret = webimg.wi_set_format("JPEG")
                end
            else
                print("GIF, donot change")
            end

            code = ret
            print("code = " .. code)
        else
            print("scale image failed.")
        end
    else
        print("action = nil")
    end
else
    print("arg = nil")
end

request.push(code)
return

