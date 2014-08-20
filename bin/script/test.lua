local type_list = {
	test100 = {
		cols                = 100,
		rows				= 100,
		quality			    = 75,
		gray			    = 1,
        format              = 'WEBP',
	},
}

function f()
    local code = -1
    local rtype = zimg.type() --Get the request type from url argument

    local arg = type_list[rtype] --Find the type details
    if not arg then
        zimg.ret(code)
    end

    local ret = zimg.scale(arg.cols, arg.rows) --Scale image
    if ret ~= 0 then
        zimg.ret(code)
    end

    if arg.quality and zimg.quality() > arg.quality then
        zimg.set_quality(arg.quality) --Set quality of image
    end

    if arg.format then
        ret = zimg.set_format(arg.format) --Set format
        if ret ~= 0 then
            zimg.ret(code)
        end
    end

    if arg.gray and arg.gray == 1 then
        ret = zimg.gray() --Grayscale image
        if ret ~= 0 then
            zimg.ret(code)
        end
    end

    code = 0
    zimg.ret(code) --Return the result to zimg
end
