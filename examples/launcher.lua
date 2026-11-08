local inifile = require('inifile')
local posix = require('posix')
local newt = require('newt')

local function getdirs()
	local t = {}
	local e = os.getenv("XDG_DATA_DIRS") or "/usr/share"
	for d in string.gmatch(e, "([^:]+)") do
		table.insert(t, d)
	end
	return t
end

local function getdesktopfiles(dirs)
	local allfiles = {}
	for k,v in ipairs(dirs) do
		local pat = v .. "/applications/*.desktop"
		--print("glob " .. pat)
		local desktops = posix.glob.glob(pat, 0) or {}
		for _, d in ipairs(desktops) do
			table.insert(allfiles, d)
		end
	end

	return allfiles
end

local function parsedesktopfiles(files)
	local parsed = {}
	for k, v in ipairs(files) do
		parsed[v] = inifile.parse(v)
	end
	return parsed
end

local function parsecategories(desktop)
	local c = desktop["Desktop Entry"]["Categories"]
	local catkey = {}
	local catvalue = {}
	if not c then return {}, {} end

	for cat in string.gmatch(c, "([^;]+)") do
		catkey[cat] = true
		table.insert(catvalue, cat)
	end
	return catkey, catvalue
end

local xdgdatadirs = getdirs()

local alldesktopfiles = getdesktopfiles(xdgdatadirs)

local parsedfiles = parsedesktopfiles(alldesktopfiles)

local maincats = {
	"AudioVideo", "Audio", "Video",
	"Development", "Education", "Game",
	"Graphics", "Network", "Office",
	"Science", "Settings", "System",
	"Utility",
}

local hier = {}

for k,v in pairs(parsedfiles) do
	if not v["Desktop Entry"].OnlyShowIn then
		local catkey, catvalue = parsecategories(v)
		--print("cats for " .. k .. ": " .. table.concat(catvalue, " "))
		local match = false
		for _, c in ipairs(maincats) do
			if catkey[c] then
				if not hier[c] then hier[c] = {} end
				table.insert(hier[c], v)
				match = true
			end
		end

	--[[
		if not match then
			if not hier["Other"] then hier["Other"] = {} end
			table.insert(hier["Other"], v)
		end
	--]]
	end
end

--[[
for k,v in pairs(hier) do
	print(k)
	for _, e in ipairs(v) do
		print("  " .. e["Desktop Entry"]["Name"])
	end
end
--]]

newt.init()
newt.cls()

local w,h = newt.getscreensize()
local dw, dh = math.floor(w/2),math.floor(h/2)

local selected

repeat
	newt.popwindow()
	newt.openwindow(2, 2, w-5, h-5, "XDG Desktop Menu")
	local lb = newt.listbox(dw-20, 1, h-7, { returnexit = true, border = true })
	lb:setwidth(40)

	for _, v in ipairs(maincats) do
		lb:append(v, v)
	end

	local form = newt.form()
	form:addcomponents(lb)
	form:addhotkey(newt.KEY_ESCAPE)

	newt.refresh()
	local reason, what = form:run()
	if reason == "hotkey" then
		newt.finished()
		return
	end

	local selectedcat = lb:current()

	local lb = newt.listbox(dw-50, 1, h-7, { returnexit = true, border = true })
	lb:setwidth(100)

	for _, v in pairs(hier[selectedcat]) do
		local de = v["Desktop Entry"]
		lb:append(string.format("%s (%s)", de.Name, de.Comment or ""), de)
	end

	local form = newt.form()
	form:addcomponents(lb)
	form:addhotkey(newt.KEY_ESCAPE)

	local reason, what = form:run()
	if reason == "hotkey" then
		goto continue
	end

	if reason == "component" then
		selected = lb:current()
		break
	end

	::continue::
until false

newt.finished()

print(selected.Name, selected.Exec)

local execl = string.gsub(selected.Exec, "%%%a", "")
local exectab = {}
for w in string.gmatch(execl, "([^ ]+)") do
	table.insert(exectab, w)
end
local cmd = table.remove(exectab, 1)

print("Executing '" .. cmd .. "'" .. " '" .. table.concat(exectab, " ") .. "'")
local _, err, errno = posix.unistd.execp(cmd, exectab)
error(err)

