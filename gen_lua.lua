local num = 1000000
local sf = string.format
local f = io.open('test_1.lua', 'w')
f:write('return {\n')
for i = 1, num do
    f:write(sf('    {[%s] = %s},\n', (i - 1) * 2 + 1, i))
end
f:write('}')
f:close()
