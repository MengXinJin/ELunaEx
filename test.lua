--
-- test.lua
--

obj = {}
mp = {}
mp.hello = "world"
mp.nihao = "nihao"

ab = BaseClass()
function show()
	local abc = fun_test()
	print("obj len = ",#obj,ab:get_a(),ab:get_b(),abc:get_a(),abc:get_b())
	
	for i=1,#obj do
		print(obj[i])
	end
	
	for i,v in pairs(mp) do
		print(i,v)
	end
	
	local obj2 = BaseClassA()
	obj2:fun()
	
	local obj3 = DeriveClassA()
	obj3:fun()
	
end

