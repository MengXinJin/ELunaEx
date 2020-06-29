
//
// test.cpp
//

#include <ELunaHelp.h>

struct BaseClass
{
	double a{ 10 };
	double b{ 20 };
}; 

BaseClass fun_test(){
	BaseClass ab;
	ab.a = 1531.0;
	ab.b = 78945.456;
	return ab;
}

class BaseClassA{
public:
	virtual ~BaseClassA(){}
	virtual void fun(){
		std::cout << "BaseClassA :: fun" << std::endl;
	}
};

class DeriveClassA : public BaseClassA{
public:
	void fun(){
		std::cout << "DeriveClassA :: fun" << std::endl;
	}
};

int main(int argc, char* argv[])
{

	lua_State* L = ELuna::openLua();
	ELuna::registerClass<BaseClass>(L, "BaseClass", ELuna::constructor<BaseClass>);
	ELuna::registerProperty<BaseClass, double>(L, "a", &BaseClass::a);
	ELuna::registerProperty<BaseClass, double>(L, "b", &BaseClass::b);
	ELuna::registerFunctionEx(L, "fun_test", fun_test);


	ELuna::registerClass<BaseClassA>(L, "BaseClassA", ELuna::constructor<BaseClassA>);
	ELuna::registerMethod<BaseClassA>(L, "fun", &BaseClassA::fun);

	//
	// ◊¢≤·≈……˙¿‡
	//
	ELuna::registerDeriveClass<DeriveClassA, BaseClassA>(L, "DeriveClassA", ELuna::constructor<DeriveClassA>);

	ELuna::doFile(L, "test.lua");
	ELuna::LuaObject mObj(L, "obj");
	ELuna::LuaObject mObj2(L, "ab");
	ELuna::LuaObject mObj3(L, "mp");
	BaseClass* obj2 = mObj2.get<BaseClass*>();
	std::cout << mObj3.get_value<std::string>("hello") << std::endl;
	std::map<std::string, std::string> m3 = mObj3.get<std::map<std::string, std::string>>();
	if (mObj3.is_table()){
		ELuna::LuaObject tempobj = mObj3["hello"];
		if (!tempobj.is_nil()){
			tempobj.set("Hello World");
		}
		mObj3.set_value("Hawa", 123456);
	}
	std::vector<std::string> vec = { "1", "2", "3", "4", "5", "6, 7, 8, 9" };
	mObj.set(vec);
	ELuna::MLuaFunEx<void> mFun(L, "show");
	mFun();

	obj2->a = 100.2;
	obj2->b = 78956.1547;

	mFun();
	BaseClass abc;
	abc.a = 78953.12;
	abc.b = 4578862.65;
	mObj2.set(abc);

	std::map<std::string, std::string> m;
	m["hello"] = "hehehee";
	m["nihao"] = "3213131";
	mObj3.set(m);
	mObj3.erase_key("Hawa");
	std::vector<int> vec2 = { 12,16,15,85,34 };
	mObj.set(vec2);
	mFun();

	system("pause");
	return 0;
}