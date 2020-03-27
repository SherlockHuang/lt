local num = 1000000
local sf = string.format
local f = io.open('test_1.lua', 'w')
f:write('return {\n')

-- for i = 1, num // 2 do
--     f:write(sf('    [%s] = %s,\n', i, i))
-- end

local max_len = 16
local min_len = 1

local fmt_arr = {}
for i = min_len, max_len do
    local fmt = ''
    for j = 1, i do
        fmt = fmt ..'%c'
    end
    fmt_arr[i] = fmt
end

for i = 1, num do
    local len = math.random(min_len, max_len)
    local args = {}
    for j = 1, len do
        -- printable charaters
        local c = math.random(32, 126)
        if c == 92 or c == 34 then
            c = c + 1
        end
        table.insert(args, c)
    end
    local s = string.format(fmt_arr[len], table.unpack(args))

    -- f:write(sf('    [%s] = %s,\n', (i - 1) * 2 + 1, i))
    f:write(sf('    ["%s"] = %s,\n', s, i))
end

-- for i = 1, num do
--     f:write(sf('    [%s] = %s,\n', i, i))
-- end

f:write('}')
f:close()
