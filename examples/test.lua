local newt = require('newt')

newt.init()
newt.cls()

--[[
newt.drawroottext(0, 0, "some root text")
newt.refresh()
newt.delay(1000000)

newt.pushhelpline("")
newt.refresh()
newt.delay(1000000)

newt.pushhelpline("a help line")
newt.refresh()
newt.delay(1000000)

newt.pophelpline()
newt.refresh()
newt.delay(1000000)

newt.cls()
newt.delay(1000000)
--]]
--
local w,h = newt.getscreensize()

newt.openwindow(2, 2, w-5, h-5, "Button Sample")

local l1 = newt.label(10, 6, string.format("%d x %d", w, h))
local ctr = 0
local b1 = newt.compactbutton(10, 1, "Ok")
b1:callback(function(b)
end)

local b2 = newt.compactbutton(22, 1, "Cancel")
local cb1 = newt.checkbox(10, 5, "Check", true)
cb1:callback(function(b)
end)

local b3 = newt.button(13, 17, "Beep")

local cb2 = newt.checkbox(20, 5, "Error?", true)
cb2:callback(function(b)
	error("wut")
end)

local lb = newt.listbox(10, 10, 5, { returnexit = true, border = true })
lb:append("test", 1)
lb:append("test again", 2)

local form = newt.form()
form:addcomponents(l1, b1, b2, b3, cb1, cb2, lb)
form:addhotkey(newt.KEY_ESCAPE)

repeat
	local reason, what = form:run()
	print(reason, what)
	if reason == "component" then
		if what == b1 then
			l1:settext("Foo " .. ctr)
			ctr = ctr + 1
		end

		if what == lb then
			l1:settext("LB " .. tostring(lb:current()))
		end

		if what == b3 then
			newt.bell()
		end
	end

	if reason == "hotkey" and what == newt.KEY_ESCAPE then break end
until (reason == "component" and what == b2)

--newt.delay(500000)
newt.popwindow()
newt.cls()
--newt.delay(500000)

print(cb1:get())


newt.finished()

