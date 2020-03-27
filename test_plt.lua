local p = require 'plt.c'
local sf = string.format


local m = collectgarbage "count"
local t = os.clock()
local pt = p.create_plt('output.pl')
local dt = os.clock() - t
local dm = collectgarbage("count") - m
print(sf('create plt cost %f, used mem:%.1fM', dt, dm / 1024))

m = collectgarbage "count"
t = os.clock()
local lt = require 'test_1'
dt = os.clock() - t
dm = collectgarbage("count") - m
print(sf('create raw cost %f, used mem:%.1fM', dt, dm / 1024))


print('---------- ipairs  ----------')
local tv = 0
t = os.clock()
for k, v in ipairs(pt) do
    -- print('k, v = ', k, v)
    tv = tv + v
end
print(sf('ipairs plt table cost %f, tv = %s', os.clock() -t, tv))

print('---------- ipairs  ----------')
tv = 0
t = os.clock()
for k, v in p.ipairs(pt) do
    -- print('k, v = ', k, v)
    tv = tv + v
end
print(sf('p.ipairs plt table cost %f, tv = %s', os.clock() -t, tv))

print('---------- ipairs  ----------')
tv = 0
t = os.clock()
for k, v in ipairs(lt) do
    -- print('k, v = ', k, v)
    tv = tv + v
end
print(sf('ipairs raw table cost %f, tv = %s', os.clock() -t, tv))


print('---------- pairs  ----------')
tv = 0
t = os.clock()
for k, v in pairs(pt) do
    -- print('k, v = ', k, v)
    tv = tv + v
end
print(sf('pairs plt table cost %f, tv = %s', os.clock() -t, tv))

-- local function pk(t, k)
--     print(k, t[k])
-- end
-- pk(pt, 11.0)
-- pk(pt, 13.0)
-- pk(pt, 15.0)
-- pk(pt, 17.0)
-- pk(pt, 19.0)

print('---------- pairs  ----------')
tv = 0
t = os.clock()
for k, v in pairs(lt) do
    -- print('k, v = ', k, v)
    tv = tv + v
end
print(sf('pairs raw table cost %f, tv = %s', os.clock() -t, tv))
