local p = require 'plt.c'
local a = p.create_plt('output.pl')

print(a[1])
print(a[1][1])
print(a['hah'])
print(a[1243])
print(a.haha.oh)
print(a.ha)
print(a.haha.ohh)

print('---------- ipairs ----------')
for k, v in ipairs(a) do
    print(k, v)
end

print('---------- pairs ----------')
for k, v in pairs(a) do
    print(k, v)
end

print('---------- pairs a.haha ----------')
for k, v in pairs(a.haha) do
    print(k, v)
end

