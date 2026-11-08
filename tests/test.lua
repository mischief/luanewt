-- test.lua: luanewt test suite.  Requires a tty; run under script(1).
local newt = require('newt')

local failed = 0
local function check(cond, msg)
	if not cond then
		failed = failed + 1
		print('FAIL: ' .. msg)
	else
		print('ok: ' .. msg)
	end
end

local function expect_error(fn, pat, msg)
	local ok, err = pcall(fn)
	check(not ok and (pat == nil or tostring(err):find(pat, 1, true)),
	    msg .. ' [' .. tostring(err) .. ']')
end

-- module surface
check(type(newt.init) == 'function', 'init is a function')
check(type(newt.form) == 'function', 'form is a function')
check(type(newt.button) == 'function', 'button is a function')
check(type(newt.listbox) == 'function', 'listbox is a function')
check(type(newt.KEY_ESCAPE) == 'number', 'KEY_ESCAPE')
check(type(newt.FD_READ) == 'number', 'FD_READ')
check(type(newt.KEY_F12) == 'number', 'KEY_F12')

-- init gating (these must fail before any newt call)
expect_error(newt.finished, 'not initialized', 'finished before init')
expect_error(function() newt.openwindow(0, 0, 1, 1, 't') end,
    'not initialized', 'openwindow before init')

newt.init()

-- double init
expect_error(newt.init, 'already initialized', 'double init')

-- argument validation
expect_error(function() newt.openwindow(0, 0, 0, 10, 't') end,
    'width must be positive', 'openwindow width 0')
expect_error(function() newt.centeredwindow(0, 5, 't') end,
    'width must be positive', 'centeredwindow width 0')
expect_error(function() newt.delay(-1) end,
    'delay must be non-negative', 'negative delay')
expect_error(function() newt.button(1, 2) end,
    'string expected', 'button missing text')
expect_error(function() newt.button('a', 'b', 'c') end,
    'number expected', 'button wrong type')
expect_error(function() newt.delay('x') end,
    'number expected', 'delay wrong type')

-- userdata type checks
local b = newt.button(1, 1, 'x')
check(getmetatable(b) ~= nil, 'button has metatable')
expect_error(function() b:missingmethod() end, nil, 'missing method errors')

-- a component may belong to only one form
local f1, f2 = newt.form(), newt.form()
f1:addcomponents(b)
expect_error(function() f2:addcomponents(b) end,
    'component already belongs to a form', 'reuse component across forms')
f1:destroy()

-- unattached component collection
local function make_many()
	for i = 1, 20 do
		check(getmetatable(newt.button(i % 20, 1, tostring(i))) ~= nil,
		    'created button ' .. i)
	end
end
make_many()
collectgarbage('collect')
check(true, 'gc of unattached components survived')

-- listbox round trip
local lb = newt.listbox(1, 1, 5, { border = true })
lb:append('one', 1)
lb:append('two', 'two')
lb:append('three', { x = 3 })
check(lb:itemcount() == 3, 'listbox itemcount 3')
check(lb:current() == 1, 'listbox current first')
lb:setcurrent(3)
check(type(lb:current()) == 'table' and lb:current().x == 3, 'listbox setcurrent')
check(not pcall(lb.setcurrent, lb, 4), 'listbox setcurrent out of range')
lb:setcurrent(1)
lb:delete(1)
check(lb:itemcount() == 2, 'listbox delete')
check(lb:current() == 'two', 'listbox current after delete')
lb:clear()
check(lb:itemcount() == 0, 'listbox clear')
check(lb:selected() == nil, 'listbox selected nil')

-- checkbox boolean
local cb = newt.checkbox(1, 1, 'c', true)
check(cb:get() == true, 'checkbox on -> true')
cb:set(' ')
check(cb:get() == false, 'checkbox off -> false')

-- form ownership
local f = newt.form()
local child = newt.button(1, 1, 'c')
f:addcomponents(child)
f:destroy()
expect_error(function() child:callback(function() end) end,
    'component destroyed', 'child use after form destroy')

-- double callback registration must not crash
local b2 = newt.button(1, 1, 'z')
b2:callback(function() end)
b2:callback(function() end)
check(true, 'double callback registration')

-- newly wrapped components
local e = newt.entry(1, 1, 'hello', 20, { password = false })
check(getmetatable(e) ~= nil, 'entry has metatable')
check(e:get() == 'hello', 'entry get initial')
e:set('world', true)
check(e:get() == 'world', 'entry set/get')

local tb = newt.textbox(1, 1, 20, 5, { wrap = true })
check(getmetatable(tb) ~= nil, 'textbox has metatable')
tb:settext('line1\nline2')
check(tb:getnumlines() >= 1, 'textbox numlines')
tb:setheight(8)

local sc = newt.scale(1, 1, 20, 100)
check(getmetatable(sc) ~= nil, 'scale has metatable')
sc:set(50)

local sb = newt.verticalscrollbar(1, 1, 10, 1, 2)
check(getmetatable(sb) ~= nil, 'scrollbar has metatable')
sb:set(4, 20)

check(type(newt.winmessage) == 'function', 'winmessage is a function')
check(type(newt.winchoice) == 'function', 'winchoice is a function')

newt.finished()

print(failed == 0 and 'ALL PASS' or (failed .. ' FAILURES'))
os.exit(failed == 0 and 0 or 1)
