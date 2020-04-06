local p = require 'lt.c'
local sf = string.format

local m = collectgarbage "count"
local t = os.clock()
local pt = p.create_lt('output.pl')
local dt = os.clock() - t
local dm = collectgarbage("count") - m
print(sf('create lt cost %f, used mem:%.1fM', dt, dm / 1024))

collectgarbage "collect"
dm = collectgarbage("count") - m
print(sf('create lt used mem:%.1fM', dm / 1024))

m = collectgarbage "count"
t = os.clock()
local lt = loadfile('test_1.lua')()
dt = os.clock() - t
dm = collectgarbage("count") - m
print(sf('create raw cost %f, used mem:%.1fM', dt, dm / 1024))

collectgarbage "collect"
dm = collectgarbage("count") - m
print(sf('create raw used mem:%.1fM', dm / 1024))

m = collectgarbage "count"
t = os.clock()
local lc = loadfile('test_1.luac')()
dt = os.clock() - t
dm = collectgarbage("count") - m
print(sf('create compiled raw cost %f, used mem:%.1fM', dt, dm / 1024))

collectgarbage "collect"
dm = collectgarbage("count") - m
print(sf('create compiled raw used mem:%.1fM', dm / 1024))

print('---------- ipairs  ----------')
local tv = 0
t = os.clock()
for k, v in ipairs(pt) do
    tv = tv + v
end
print(sf('ipairs lt table cost %f, tv = %s', os.clock() -t, tv))

print('---------- ipairs  ----------')
tv = 0
t = os.clock()
for k, v in p.ipairs(pt) do
    tv = tv + v
end
print(sf('p.ipairs lt table cost %f, tv = %s', os.clock() -t, tv))

print('---------- ipairs  ----------')
tv = 0
t = os.clock()
for k, v in ipairs(lt) do
    tv = tv + v
end
print(sf('ipairs raw table cost %f, tv = %s', os.clock() -t, tv))

print('---------- ipairs  ----------')
tv = 0
t = os.clock()
for k, v in ipairs(lc) do
    tv = tv + v
end
print(sf('ipairs compiled raw table cost %f, tv = %s', os.clock() -t, tv))

print('---------- pairs  ----------')
tv = 0
t = os.clock()
for k, v in pairs(pt) do
    tv = tv + v
end
print(sf('pairs lt table cost %f, tv = %s', os.clock() -t, tv))

print('---------- pairs  ----------')
tv = 0
t = os.clock()
for _, k, v in p.pairs(pt) do
    tv = tv + v
end
print(sf('pairs lt.pairs table cost %f, tv = %s', os.clock() -t, tv))

print('---------- pairs  ----------')
tv = 0
t = os.clock()
for k, v in pairs(lt) do
    tv = tv + v
end
print(sf('pairs raw table cost %f, tv = %s', os.clock() -t, tv))

print('---------- pairs  ----------')
tv = 0
t = os.clock()
for k, v in pairs(lc) do
    tv = tv + v
end
print(sf('pairs compiled raw table cost %f, tv = %s', os.clock() -t, tv))
