
/*////////////////////////////////////////////////////////////////////////////////////
ELunaEx - ELuna Extend

ELunaEx is an extension and supplement to ELuna, 
making binding between Lua and C++ more flexible. 
It supports binding of derived classes and data members, which just
depends on Lua library and ELuna. It provides some simple API to bind cpp class,
method, function or to bind lua function, table. You can include ELunaEx and Eluna
,Lua in your project to use.

Mail: sh06155@hotmail.com
 
The MIT License

Copyright (C) 2020 rick

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ELuna.h"
#include <iostream>
#include <string>

namespace ELuna{
	//
	// 实现Lua函数的回调操作
	//
#pragma region 实现Lua函数的回调操作

	template<typename T, typename HelpType>
	struct ExternalMethodClass : GenericMethod {
		std::vector<HelpType*> mPtrs;
		ExternalMethodClass(const char* name) : GenericMethod(name){};

		~ExternalMethodClass() {
			for (auto& __Ptr__ : mPtrs){
				delete __Ptr__;
				__Ptr__ = nullptr;
			}
			mPtrs.clear();
		};

		inline virtual int call(lua_State *L) {
			T* obj = read2cpp<T*>(L, 1);
			const char* fun = lua_tostring(L, 2);
			if (fun){
				mPtrs.push_back(new HelpType(L, obj, fun));
			}
			else{
				std::cout << "arg is must function name" << std::endl;
			}
			return 0;
		};
	};

	inline int proxyExternalMethodCall(lua_State *L) {
		GenericMethod* pMethod = static_cast<GenericMethod*>(lua_touserdata(L, lua_upvalueindex(1)));
		return pMethod->call(L); // execute method
	}

	template<typename T, typename __HelpType__>
	inline void registerExternalMethod(lua_State* L, const char* name) {
		luaL_getmetatable(L, ClassName<T>::getName());
		if (lua_istable(L, -1)) {
			lua_pushstring(L, name);
			new (lua_newuserdata(L, sizeof(ExternalMethodClass<T, __HelpType__>))) ExternalMethodClass<T, __HelpType__>(name);
			lua_pushcclosure(L, &proxyExternalMethodCall, 1);
			lua_rawset(L, -3);
		}
		else {
			printf("please register class %s\n", ClassName<T>::getName());
		}
		lua_pop(L, 1);
	}

#pragma endregion 实现Lua函数的回调操作


	//
	// 任意参数
	//
#pragma region 注册外部函数

	template<int N,class RL,class T>
	struct LuaFunHelpRead;

	template<class RL, class...ArgsType>
	struct LuaFunHelpRead<0, RL, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static RL Apply(lua_State* L, RL(*Fun)(ArgsType...), Args...args){
			return ApplyRun(Fun, args...);
		}
		template<class...Args>
		static RL ApplyRun(RL(*Fun)(ArgsType...), Args...args){
			return (*Fun)(args...);
		}
	};

	template<class RL, class...ArgsType>
	struct LuaFunHelpRead<1, RL,mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static RL Apply(lua_State* L, RL(*Fun)(ArgsType...), Args...args){
			typedef typename mjTL::index_type<mjTL::MTypeList<ArgsType...>, 0>::type __type;
			return ApplyRun(Fun, read2cpp<__type>(L, 2), args...);
		}
		template<class...Args>
		static RL ApplyRun(RL(*Fun)(ArgsType...), Args...args){
			 return (*Fun)(args...);
		}
	};

	template<int N, class RL, class...ArgsType>
	struct LuaFunHelpRead<N, RL,mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static RL Apply(lua_State* L, RL(*Fun)(ArgsType...), Args...args){
			typedef typename mjTL::index_type<mjTL::MTypeList<ArgsType...>, N-1>::type __type;
			return LuaFunHelpRead<N - 1, RL,mjTL::MTypeList<ArgsType...>>::Apply(L, Fun, read2cpp<__type>(L, N + 1), args...);
		}
	};


	template<class...ArgsType>
	struct LuaFunHelpRead<1, void, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static void Apply(lua_State* L, void(*Fun)(ArgsType...), Args...args){
			typedef typename mjTL::index_type<mjTL::MTypeList<ArgsType...>, 0>::type __type;
			ApplyRun(Fun, read2cpp<__type>(L, 2), args...);
		}
		template<class...Args>
		static void ApplyRun(void(*Fun)(ArgsType...), Args...args){
			(*Fun)(args...);
		}
	};

	template<int N, class...ArgsType>
	struct LuaFunHelpRead<N, void, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static void Apply(lua_State* L, void(*Fun)(ArgsType...), Args...args){
			typedef typename mjTL::index_type<mjTL::MTypeList<ArgsType...>, N - 1>::type __type;
			LuaFunHelpRead<N - 1, void,mjTL::MTypeList<ArgsType...>>::Apply(L, Fun, read2cpp<__type>(L, N + 1), args...);
		}
	};
	
	

	template<class RL, class...ArgsType>
	struct MLuaExtFunction : GenericFunction{
		typedef RL(*TFUNC)(ArgsType...);
		TFUNC m_func;
		MLuaExtFunction(const char* name, TFUNC func) : GenericFunction(name), m_func(func) {};
		~MLuaExtFunction() {};
		inline virtual int call(lua_State *L) {
			push2lua(L, LuaFunHelpRead<sizeof...(ArgsType), RL, mjTL::MTypeList<ArgsType...>>::Apply(L, m_func));
			return 1;
		};
	};

	template<class...Args>
	struct MLuaExtFunction<void, Args...> : GenericFunction{
		typedef void(*TFUNC)(Args...);
		TFUNC m_func;
		MLuaExtFunction(const char* name, TFUNC func) : GenericFunction(name), m_func(func) {};
		~MLuaExtFunction() {};
		inline virtual int call(lua_State *L) {
			LuaFunHelpRead<sizeof...(Args), void, mjTL::MTypeList<Args...>>::Apply(L, m_func);
			return 0;
		};
	};

	//
	// 将一个函数和一个类注册在一起
	//
	template<class T,class RL,class...ArgsType>
	inline void registerExternalFunction(lua_State* L, const char* name, RL(*Fun)(ArgsType...)){
		luaL_getmetatable(L, ClassName<T>::getName());
		if (lua_istable(L, -1)) {
			lua_pushstring(L, name);
			new (lua_newuserdata(L, sizeof(MLuaExtFunction<RL, ArgsType...>))) MLuaExtFunction<RL,ArgsType...>(name, Fun);
			lua_pushcclosure(L, &proxyMethodCall, 1);
			lua_rawset(L, -3);
		}
		else {
			printf("please register class %s\n", ClassName<T>::getName());
		}
		lua_pop(L, 1);
	}

#pragma endregion 注册外部函数


	//
	//+-----------------------------------------------------------------------------------------------
	//
	// 注册普通成员函数
	//
#pragma region 注册普通成员函数

	template<int N, class Obj,class RL, class T>
	struct LuaMemFunHelpRead;

	//
	// 有返回值
	//
	template<class Obj, class RL, class...ArgsType>
	struct LuaMemFunHelpRead<0, Obj, RL, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static RL Apply(lua_State* L, Obj* obj, RL(Obj::*Fun)(ArgsType...), Args...args){
			return ApplyRun(obj, Fun, args...);
		}
		template<class...Args>
		static RL ApplyRun(Obj* obj, RL(Obj::*Fun)(ArgsType...), Args...args){
			return (obj->*Fun)(args...);
		}
	};

	template<class Obj, class RL, class...ArgsType>
	struct LuaMemFunHelpRead<1, Obj,RL, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static RL Apply(lua_State* L, Obj* obj, RL(Obj::*Fun)(ArgsType...), Args...args){
			typedef typename mjTL::index_type<mjTL::MTypeList<ArgsType...>, 0>::type __type;
			return ApplyRun(obj,Fun, read2cpp<__type>(L, 2), args...);
		}
		template<class...Args>
		static RL ApplyRun(Obj* obj, RL(Obj::*Fun)(ArgsType...), Args...args){
			return (obj->*Fun)(args...);
		}
	};

	template<int N, class Obj, class RL, class...ArgsType>
	struct LuaMemFunHelpRead<N, Obj, RL, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static RL Apply(lua_State* L, Obj* obj, RL(Obj::*Fun)(ArgsType...), Args...args){
			typedef typename mjTL::index_type<mjTL::MTypeList<ArgsType...>, N - 1>::type __type;
			return LuaMemFunHelpRead<N - 1, Obj, RL, mjTL::MTypeList<ArgsType...>>::Apply(L, obj, Fun, read2cpp<__type>(L, N + 1), args...);
		}
	};


	//
	// 无返回值
	//

	template<class Obj, class...ArgsType>
	struct LuaMemFunHelpRead<0, Obj, void, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static void Apply(lua_State* L, Obj* obj, void(Obj::*Fun)(ArgsType...), Args...args){
			ApplyRun(obj, Fun, args...);
		}
		template<class...Args>
		static void ApplyRun(Obj* obj, void(Obj::*Fun)(ArgsType...), Args...args){
			(obj->*Fun)(args...);
		}
	};


	template<class Obj, class...ArgsType>
	struct LuaMemFunHelpRead<1, Obj,void, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static void Apply(lua_State* L, Obj* obj, void(Obj::*Fun)(ArgsType...), Args...args){
			typedef typename mjTL::index_type<mjTL::MTypeList<ArgsType...>, 0>::type __type;
			ApplyRun(obj,Fun, read2cpp<__type>(L, 2), args...);
		}
		template<class...Args>
		static void ApplyRun(Obj* obj, void(Obj::*Fun)(ArgsType...), Args...args){
			(obj->*Fun)(args...);
		}
	};

	template<int N,class Obj,class...ArgsType>
	struct LuaMemFunHelpRead<N, Obj,void, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static void Apply(lua_State* L, Obj* obj, void(Obj::*Fun)(ArgsType...), Args...args){
			typedef typename mjTL::index_type<mjTL::MTypeList<ArgsType...>, N - 1>::type __type;
			LuaMemFunHelpRead<N - 1, Obj, void, mjTL::MTypeList<ArgsType...>>::Apply(L, obj,Fun, read2cpp<__type>(L, N + 1), args...);
		}
	};

	template<class T,class RL,class...ArgsType>
	struct MMethodClass : GenericMethod {
		typedef RL(T::* TFUNC)(ArgsType...);
		TFUNC m_func;
		MMethodClass(const char* name, TFUNC func) : GenericMethod(name), m_func(func) {};
		~MMethodClass() {};
		inline virtual int call(lua_State *L) {
			T* obj = read2cpp<T*>(L, 1);
			push2lua<RL>(L, LuaMemFunHelpRead<sizeof...(ArgsType), T, RL, mjTL::MTypeList<ArgsType...>>::Apply(L, obj, m_func));
			return 1;
		};
	};

	template<class T,  class...ArgsType>
	struct MMethodClass<T,void,ArgsType...> : GenericMethod {
		typedef void(T::* TFUNC)(ArgsType...);
		TFUNC m_func;
		MMethodClass(const char* name, TFUNC func) : GenericMethod(name), m_func(func) {};
		~MMethodClass() {};
		inline virtual int call(lua_State *L) {
			T* obj = read2cpp<T*>(L, 1);
			LuaMemFunHelpRead<sizeof...(ArgsType), T, void, mjTL::MTypeList<ArgsType...>>::Apply(L, obj, m_func);
			return 0;
		};
	};


	template<typename T, typename RL, typename...ArgsType>
	inline void registerMethodEx(lua_State* L, const char* name, RL(T::*func)(ArgsType...)) {
		luaL_getmetatable(L, ClassName<T>::getName());

		if (lua_istable(L, -1)) {
			lua_pushstring(L, name);
			new (lua_newuserdata(L, sizeof(MMethodClass<T, RL, ArgsType...>)))
				MMethodClass<T, RL, ArgsType...>(name, func);
			lua_pushcclosure(L, &proxyMethodCall, 1);
			lua_rawset(L, -3);
		}
		else {
			printf("please register class %s\n", ClassName<T>::getName());
		}
		lua_pop(L, 1);
	}

#pragma endregion 注册普通成员函数

	//
	//+-----------------------------------------------------------------------------------------------------------
	//
	// 注册const成员函数
	//
#pragma region 注册const成员函数

	template<int N, class Obj, class RL, class T>
	struct LuaConstMemFunHelpRead;

	template<class Obj, class RL, class...ArgsType>
	struct LuaConstMemFunHelpRead<0, Obj, RL, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static RL Apply(lua_State* L, Obj* obj, RL(Obj::*Fun)(ArgsType...)const, Args...args){
			return ApplyRun(obj, Fun, args...);
		}
		template<class...Args>
		static RL ApplyRun(Obj* obj, RL(Obj::*Fun)(ArgsType...)const, Args...args){
			return (obj->*Fun)(args...);
		}
	};

	template<class Obj, class RL, class...ArgsType>
	struct LuaConstMemFunHelpRead<1, Obj, RL, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static RL Apply(lua_State* L, Obj* obj, RL(Obj::*Fun)(ArgsType...)const, Args...args){
			typedef typename mjTL::index_type<mjTL::MTypeList<ArgsType...>, 0>::type __type;
			return ApplyRun(obj, Fun, read2cpp<__type>(L, 2), args...);
		}
		template<class...Args>
		static RL ApplyRun(Obj* obj, RL(Obj::*Fun)(ArgsType...)const, Args...args){
			return (obj->*Fun)(args...);
		}
	};

	template<int N, class Obj, class RL, class...ArgsType>
	struct LuaConstMemFunHelpRead<N, Obj, RL, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static RL Apply(lua_State* L, Obj* obj, RL(Obj::*Fun)(ArgsType...)const, Args...args){
			typedef typename mjTL::index_type<mjTL::MTypeList<ArgsType...>, N - 1>::type __type;
			return LuaConstMemFunHelpRead<N - 1, Obj, RL, mjTL::MTypeList<ArgsType...>>::Apply(L, Fun, read2cpp<__type>(L, N + 1), args...);
		}
	};

	template<class Obj, class...ArgsType>
	struct LuaConstMemFunHelpRead<0, Obj, void, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static void Apply(lua_State* L, Obj* obj, void(Obj::*Fun)(ArgsType...)const, Args...args){
			ApplyRun(obj, Fun, args...);
		}
		template<class...Args>
		static void ApplyRun(Obj* obj, void(Obj::*Fun)(ArgsType...)const, Args...args){
			(obj->*Fun)(args...);
		}
	};

	template<class Obj, class...ArgsType>
	struct LuaConstMemFunHelpRead<1, Obj, void, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static void Apply(lua_State* L, Obj* obj, void(Obj::*Fun)(ArgsType...)const, Args...args){
			typedef typename mjTL::index_type<mjTL::MTypeList<ArgsType...>, 0>::type __type;
			ApplyRun(obj, Fun, read2cpp<__type>(L, 2), args...);
		}
		template<class...Args>
		static void ApplyRun(Obj* obj, void(Obj::*Fun)(ArgsType...)const, Args...args){
			(obj->*Fun)(args...);
		}
	};

	template<int N, class Obj, class...ArgsType>
	struct LuaConstMemFunHelpRead<N, Obj, void, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static void Apply(lua_State* L, Obj* obj, void(Obj::*Fun)(ArgsType...)const, Args...args){
			typedef typename mjTL::index_type<mjTL::MTypeList<ArgsType...>, N - 1>::type __type;
			LuaConstMemFunHelpRead<N - 1, Obj, void, mjTL::MTypeList<ArgsType...>>::Apply(L, obj, Fun, read2cpp<__type>(L, N + 1), args...);
		}
	};

	template<class T, class RL, class...ArgsType>
	struct MMethodConstClass : GenericMethod {
		typedef RL(T::* TFUNC)(ArgsType...)const;
		TFUNC m_func;
		MMethodConstClass(const char* name, TFUNC func) : GenericMethod(name), m_func(func) {};
		~MMethodConstClass() {};
		inline virtual int call(lua_State *L) {
			T* obj = read2cpp<T*>(L, 1);
			push2lua<RL>(L, LuaConstMemFunHelpRead<sizeof...(ArgsType), T, RL, mjTL::MTypeList<ArgsType...>>::Apply(L, obj, m_func));
			return 1;
		};
	};

	template<class T, class...ArgsType>
	struct MMethodConstClass<T, void, ArgsType...> : GenericMethod{
		typedef void(T::* TFUNC)(ArgsType...)const;
		TFUNC m_func;
		MMethodConstClass(const char* name, TFUNC func) : GenericMethod(name), m_func(func) {};
		~MMethodConstClass() {};
		inline virtual int call(lua_State *L) {
			T* obj = read2cpp<T*>(L, 1);
			LuaConstMemFunHelpRead<sizeof...(ArgsType), T, void, mjTL::MTypeList<ArgsType...>>::Apply(L, obj, m_func);
			return 0;
		};
	};


	template<typename T, typename RL, typename...ArgsType>
	inline void registerConstMethodEx(lua_State* L, const char* name, RL(T::*func)(ArgsType...)const) {
		luaL_getmetatable(L, ClassName<T>::getName());

		if (lua_istable(L, -1)) {
			lua_pushstring(L, name);
			new (lua_newuserdata(L, sizeof(MMethodConstClass<T, RL, ArgsType...>)))
				MMethodConstClass<T, RL, ArgsType...>(name, func);
			lua_pushcclosure(L, &proxyMethodCall, 1);
			lua_rawset(L, -3);
		}
		else {
			printf("please register class %s\n", ClassName<T>::getName());
		}
		lua_pop(L, 1);
	}
#pragma endregion 注册const成员函数

	//
	//+---------------------------------------------------------------------------------
	//
	// 注册自由函数函数
	//
#pragma region 注册自由函数函数

	template<int N, class RL, class T>
	struct LuaFreeFunHelpRead;

	template<class RL, class...ArgsType>
	struct LuaFreeFunHelpRead<0, RL, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static RL Apply(lua_State* L, RL(*Fun)(ArgsType...), Args... args){
			return ApplyRun(Fun, args...);
		}
		template<class...Args>
		static RL ApplyRun(RL(*Fun)(ArgsType...), Args... args){
			return (*Fun)(args...);
		}
	};

	template<class RL, class...ArgsType>
	struct LuaFreeFunHelpRead<1, RL, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static RL Apply(lua_State* L, RL(*Fun)(ArgsType...), Args...args){
			typedef typename mjTL::index_type<mjTL::MTypeList<ArgsType...>, 0>::type __type;
			return ApplyRun(Fun, read2cpp<__type>(L, 1), args...);
		}
		template<class...Args>
		static RL ApplyRun(RL(*Fun)(ArgsType...), Args...args){
			return (*Fun)(args...);
		}
	};

	template<int N, class RL, class...ArgsType>
	struct LuaFreeFunHelpRead<N, RL, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static RL Apply(lua_State* L, RL(*Fun)(ArgsType...), Args...args){
			typedef typename mjTL::index_type<mjTL::MTypeList<ArgsType...>, N - 1>::type __type;
			return LuaFreeFunHelpRead<N - 1, RL, mjTL::MTypeList<ArgsType...>>::Apply(L, Fun, read2cpp<__type>(L, N), args...);
		}
	};


	template<class...ArgsType>
	struct LuaFreeFunHelpRead<1, void, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static void Apply(lua_State* L, void(*Fun)(ArgsType...), Args...args){
			typedef typename mjTL::index_type<mjTL::MTypeList<ArgsType...>, 0>::type __type;
			ApplyRun(Fun, read2cpp<__type>(L, 1), args...);
		}
		template<class...Args>
		static void ApplyRun(void(*Fun)(ArgsType...), Args...args){
			(*Fun)(args...);
		}
	};

	template<int N, class...ArgsType>
	struct LuaFreeFunHelpRead<N, void, mjTL::MTypeList<ArgsType...>>{
		template<class...Args>
		static void Apply(lua_State* L, void(*Fun)(ArgsType...), Args...args){
			typedef typename mjTL::index_type<mjTL::MTypeList<ArgsType...>, N - 1>::type __type;
			LuaFreeFunHelpRead<N - 1, void, mjTL::MTypeList<ArgsType...>>::Apply(L, Fun, read2cpp<__type>(L, N), args...);
		}
	};

	template<typename RL,typename...ArgsType >
	struct MFunctionClass : GenericFunction{
		typedef RL(*TFUNC)(ArgsType...);
		TFUNC m_func;
		MFunctionClass(const char* name, TFUNC func) : GenericFunction(name), m_func(func) {};
		~MFunctionClass() {};
		inline virtual int call(lua_State *L) {
			push2lua<RL>(L, LuaFreeFunHelpRead<sizeof...(ArgsType), RL, mjTL::MTypeList<ArgsType...>>::Apply(L, m_func));
			return 1;
		};
	};

	template<typename...ArgsType >
	struct MFunctionClass<void, ArgsType...> : GenericFunction{
		typedef void(*TFUNC)(ArgsType...);
		TFUNC m_func;
		MFunctionClass(const char* name, TFUNC func) : GenericFunction(name), m_func(func) {};
		~MFunctionClass() {};
		inline virtual int call(lua_State *L) {
			LuaFreeFunHelpRead<sizeof...(ArgsType), void, mjTL::MTypeList<ArgsType...>>::Apply(L, m_func);
			return 0;
		};
	};

	template<typename RL,typename...ArgsType>
	inline void registerFunctionEx(lua_State* L, const char* name, RL(*func)(ArgsType...)) {
		new (lua_newuserdata(L, sizeof(MFunctionClass<RL, ArgsType...>))) MFunctionClass<RL, ArgsType...>(name, func);
		lua_pushcclosure(L, proxyFunctionCall, 1);
		lua_setglobal(L, name);
	}

#pragma endregion 注册自由函数函数

	//
	//+-------------------------------------------------------------------------------------------
	//
	// C++ 操作Lua函数
	//
#pragma region C++ 操作Lua函数

	template<class T, class...ArgsType>
	struct LuaFunHelp;

	template<class T>
	struct LuaFunHelp<T>{
		static void Apply(lua_State* L, T val){
			push2lua(L, val);
		}
	};

	template<class T, class...ArgsType>
	struct LuaFunHelp
	{
		static void Apply(lua_State* L, const T& val, ArgsType... args){
			push2lua(L, val);
			LuaFunHelp<ArgsType...>::Apply(L, args...);
		}
	};
	
	template<class R>
	struct MLuaFunEx{
		MLuaFunEx(lua_State* L, const char* funName) :m_luaState(L), mFunName(funName){
			lua_getglobal(L, funName);
			if (lua_isfunction(m_luaState, -1)) {
				m_ref = luaL_ref(L, LUA_REGISTRYINDEX);
				b_isvalid = true;
			}
			else {
				printf("%s is not a lua function!\n", funName);
				b_isvalid = false;
			}
		}

		~MLuaFunEx() {
			luaL_unref(m_luaState, LUA_REGISTRYINDEX, m_ref);
			m_luaState = NULL;
		}

		bool IsValid(){
			return b_isvalid;
		}

		R operator()(){
			if (!b_isvalid){
				return R();
			}
			lua_pushcclosure(m_luaState, error_log, 0);
			int stackTop = lua_gettop(m_luaState);
			lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, m_ref);
			lua_pcall(m_luaState, 0, 1, stackTop);
			R result = read2cpp<R>(m_luaState, -1);
			lua_settop(m_luaState, -3);
			return result;
		}

		template<class...ArgsType>
		R operator()(ArgsType... args){
			if (!b_isvalid){
				return R();
			}
			lua_pushcclosure(m_luaState, error_log, 0);
			int stackTop = lua_gettop(m_luaState);
			lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, m_ref);
			LuaFunHelp<ArgsType...>::Apply(m_luaState, args...);
			lua_pcall(m_luaState, sizeof...(ArgsType), 1, stackTop);
			R result = read2cpp<R>(m_luaState, -1);
			lua_settop(m_luaState, -3);
			return result;
		}

	private:
		const char*  mFunName;
		bool		 b_isvalid{ false };
		int			 m_ref{ 0 };
		lua_State*	 m_luaState{ nullptr };
	};

	template<>
	struct MLuaFunEx<void>{
		MLuaFunEx(lua_State* L, const char* funName) :m_luaState(L), mFunName(funName){
			lua_getglobal(L, funName);
			if (lua_isfunction(m_luaState, -1)) {
				m_ref = luaL_ref(L, LUA_REGISTRYINDEX);
				b_isvalid = true;
			}
			else {
				printf("%s is not a lua function!\n", funName);
				b_isvalid = false;
			}
		}

		~MLuaFunEx() {
			luaL_unref(m_luaState, LUA_REGISTRYINDEX, m_ref);
			m_luaState = NULL;
		}

		bool IsValid(){
			return b_isvalid;
		}

		void operator()(){
			if (!b_isvalid){
				return;
			}
			lua_pushcclosure(m_luaState, error_log, 0);
			int stackTop = lua_gettop(m_luaState);
			lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, m_ref);
			lua_pcall(m_luaState, 0, 1, stackTop);
			lua_settop(m_luaState, -3);
		}

		template<class... ArgsType>
		void operator()(ArgsType... args){
			if (!b_isvalid){
				return;
			}
			lua_pushcclosure(m_luaState, error_log, 0);
			int stackTop = lua_gettop(m_luaState);
			lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, m_ref);
			LuaFunHelp<ArgsType...>::Apply(m_luaState, args...);
			lua_pcall(m_luaState, sizeof...(ArgsType), 1, stackTop);
			lua_settop(m_luaState, -3);
		}

	private:
		const char*  mFunName;
		bool		 b_isvalid{ false };
		int			 m_ref{ 0 };
		lua_State*	 m_luaState{ nullptr };
	};
#pragma endregion C++ 操作Lua函数



	//
	// Lua 对象
	// 不支持递归表
	//
#pragma region LuaObject

	template<class T>
	struct __value_lua__{
		static void to_lua(lua_State* L, const T& val){
			push2lua(L, val);
		}

		static T to_cpp(lua_State* L, int index){
			return read2cpp<T>(L, index);
		}
	};

	template<>
	struct __value_lua__<std::string>{
		static void to_lua(lua_State* L, const std::string& val){
			push2lua(L, val.c_str());
		}

		static std::string to_cpp(lua_State* L, int index){
			return read2cpp<const char*>(L, index);
		}
	};

	template<>
	struct __value_lua__<const char*>{
		static void to_lua(lua_State* L, const char* val){
			push2lua(L, val);
		}

		static const char* to_cpp(lua_State* L, int index){
			return read2cpp<const char*>(L, index);
		}
	};

	template<>
	struct __value_lua__<char*>{
		static void to_lua(lua_State* L, char* val){
			push2lua(L, val);
		}

		static char* to_cpp(lua_State* L, int index){
			return read2cpp<char*>(L, index);
		}
	};


	template<class T>
	struct MTLLuaToValue{
		static T ToValue(lua_State* L){
			if (lua_isuserdata(L, -1)){
				UserData<T>* ud = static_cast<UserData<T>*>(lua_touserdata(L, -1));
				return *(ud->m_objPtr);
			}
			else{
				return T();
			}
		}

		static void FromValue(lua_State* L, T val, const char* name){
			if (lua_isuserdata(L, -1)){
				UserData<T>* ud = static_cast<UserData<T>*>(lua_touserdata(L, -1));
				*ud->m_objPtr = val;
			}
		}
	};

	template<class T>
	struct MTLLuaToValue<T*>{
		static T* ToValue(lua_State* L){
			if (lua_isuserdata(L, -1)){
				UserData<T>* ud = static_cast<UserData<T>*>(lua_touserdata(L, -1));
				return ud->m_objPtr;
			}
			else{
				return nullptr;
			}
		}

		static void FromValue(lua_State* L, T* val, const char* name){
			if (lua_isuserdata(L, -1)){
				UserData<T>* ud = static_cast<UserData<T>*>(lua_touserdata(L, -1));
				ud->m_objPtr = val;
			}
		}
	};
			

	template<>
	struct MTLLuaToValue<unsigned __int64>{
		static unsigned __int64 ToValue(lua_State* L){
			return lua_tointeger(L, -1);
		}
		static void FromValue(lua_State* L, unsigned __int64 val, const char* name){
			lua_pushinteger(L, val);
			lua_setglobal(L, name);
		}
	};

	template<>
	struct MTLLuaToValue<__int64> : MTLLuaToValue<unsigned __int64>{};

	template<>
	struct MTLLuaToValue<unsigned __int32> : MTLLuaToValue<unsigned __int64>{};

	template<>
	struct MTLLuaToValue<__int32> : MTLLuaToValue<unsigned __int64>{};

	template<>
	struct MTLLuaToValue<unsigned __int16> : MTLLuaToValue<unsigned __int64>{};

	template<>
	struct MTLLuaToValue<__int16> : MTLLuaToValue<unsigned __int64>{};

	template<>
	struct MTLLuaToValue<unsigned long> : MTLLuaToValue<unsigned __int64>{};

	template<>
	struct MTLLuaToValue<long> : MTLLuaToValue<unsigned __int64>{};


	template<>
	struct MTLLuaToValue<double>{
		static double ToValue(lua_State* L){
			return lua_tonumber(L, -1);
		}
		static void FromValue(lua_State* L, double val, const char* name){
			lua_pushnumber(L, val);
			lua_setglobal(L, name);
		}
	};

	template<>
	struct MTLLuaToValue<float> : MTLLuaToValue<double>{};

	template<>
	struct MTLLuaToValue<const char*>{
		static const char* ToValue(lua_State* L){
			return lua_tostring(L, -1);
		}
		static void FromValue(lua_State* L, const char* val, const char* name){
			lua_pushstring(L, val);
			lua_setglobal(L, name);
		}
	};

	template<>
	struct MTLLuaToValue<char*> {
		static char* ToValue(lua_State* L){
			return const_cast<char*>(lua_tostring(L, -1));
		}
		static void FromValue(lua_State* L, const char* val, const char* name){
			lua_pushstring(L, val);
			lua_setglobal(L, name);
		}
	};

	template<>
	struct MTLLuaToValue<std::string>{
		static std::string ToValue(lua_State* L){
			return lua_tostring(L, -1);
		}
		static void FromValue(lua_State* L, const std::string& val, const char* name){
			lua_pushstring(L, val.c_str());
			lua_setglobal(L, name);
		}
	};

	template<>
	struct MTLLuaToValue<bool>{
		static bool ToValue(lua_State* L){
			return lua_toboolean(L, -1);
		}
		static void FromValue(lua_State* L, bool val, const char* name){
			lua_pushboolean(L, val);
			lua_setglobal(L, name);
		}
	};

	template<class T>
	struct MTLLuaToValue<std::vector<T>>{
		static std::vector<T> ToValue(lua_State* L){
			std::vector<T> res;
			int n = luaL_len(L, -1);
			for (int i = 0; i < n; ++i){
				lua_rawgeti(L, -1, i + 1);
				T val = MTLLuaToValue<T>::ToValue(L);
				res.push_back(val);
				lua_pop(L, 1);
			}
			return res;
		}

		static void FromValue(lua_State* L, const std::vector<T>& val, const char* name){
			if (!lua_istable(L, -1)){
				return;
			}
			int n = luaL_len(L, -1);
			if (n > val.size()){
				for (int i = 0; i < n; ++i){
					lua_pushinteger(L, i + 1);
					lua_pushnil(L);
					lua_settable(L, -3);
				}
			}
			for (int i = 0; i < val.size(); ++i){
				lua_pushinteger(L, i + 1);
				__value_lua__<T>::to_lua(L, val.at(i));
				lua_settable(L, -3);
			}
		}
	};

	template<class K,class V>
	struct MTLLuaToValue<std::map<K,V>>{
		static std::map<K, V> ToValue(lua_State* L){
			std::map<K, V> res;	
			if (!lua_istable(L, -1)){
				return res;
			}
			int id = lua_gettop(L);
			lua_pushnil(L);
			while (lua_next(L, id)){
				K key = __value_lua__<K>::to_cpp(L, -2);
				V val = __value_lua__<V>::to_cpp(L, -1);
				res[key] = val;
				lua_pop(L, 1);
			}
			return res;
		}

		static void FromValue(lua_State* L, const std::map<K, V>& val, const char* name){
			if (!lua_istable(L, -1)){
				return;
			}
			int n = luaL_len(L, -1);
			for (auto& m : val){
				__value_lua__<K>::to_lua(L, m.first);
				__value_lua__<V>::to_lua(L, m.second);
				lua_settable(L, -3);
			}
		}
	};




	struct LuaObject{
		lua_State* m_luaState{ nullptr };
		const char* mName{ nullptr };
		std::string m_tableName;
		LuaObject(lua_State* L, const char* name) :m_luaState(L), mName(name){			
		}

		~LuaObject(){
			m_luaState = NULL;
		}

		template<class T>
		T get(){
			if (m_tableName.empty()){
				lua_getglobal(m_luaState, mName);
				return MTLLuaToValue<T>::ToValue(m_luaState);
			}
			else{
				lua_getglobal(m_luaState, m_tableName.c_str());
				lua_pushstring(m_luaState, mName);
				lua_gettable(m_luaState, -2);
				return MTLLuaToValue<T>::ToValue(m_luaState);
			}
		}

		template<class T>
		void set(T val){
			if (m_tableName.empty()){
				lua_getglobal(m_luaState, mName);
				MTLLuaToValue<T>::FromValue(m_luaState, val, mName);
			}
			else{
				lua_getglobal(m_luaState, m_tableName.c_str());
				lua_pushstring(m_luaState, mName);
				__value_lua__<T>::to_lua(m_luaState, val);
				lua_rawset(m_luaState, -3);
			}
		}

		bool is_table() const{
			if (is_nil()){
				return false;
			}
			lua_getglobal(m_luaState, mName);
			return lua_istable(m_luaState, -1);
		}

		bool is_nil() const{
			if (m_luaState == nullptr || mName == nullptr){
				return true;
			}
			if (m_tableName.empty()){
				lua_getglobal(m_luaState, mName);
				return lua_isnil(m_luaState, -1);
			}
			else{
				lua_getglobal(m_luaState, m_tableName.c_str());
				lua_pushstring(m_luaState, mName);
				lua_gettable(m_luaState, -2);
				return lua_isnil(m_luaState, -1);
			}
		}


		//
		// 这对关联表的一些简单接口
		//
		LuaObject operator[](const char* key) const{
			if (is_table() == false){
				return LuaObject(nullptr, nullptr);
			}
			lua_pushstring(m_luaState, key);
			lua_gettable(m_luaState, -2);
			LuaObject obj(m_luaState, key);
			obj.m_tableName = mName;
			return obj;
		}

		template<class T>
		void set_value(const char* key, T val){
			if (m_tableName.empty()){
				if (is_table() == false){
					return;
				}
				lua_pushstring(m_luaState, key);
				__value_lua__<T>::to_lua(m_luaState, val);
				lua_rawset(m_luaState, -3);
			}
			else{
				lua_getglobal(m_luaState, m_tableName.c_str());
				lua_pushstring(m_luaState, key);
				__value_lua__<T>::to_lua(m_luaState, val);
				lua_rawset(m_luaState, -3);
			}
		}

		template<class T>
		T get_value(const char* key){
			if (is_table() == false){
				return T();
			}
			lua_pushstring(m_luaState, key);
			lua_gettable(m_luaState, -2);
			return __value_lua__<T>::to_cpp(m_luaState, -1);
		}


		void erase_key(const char* key){
			if (is_table() == false){
				return;
			}
			lua_pushstring(m_luaState, key);
			lua_pushnil(m_luaState);
			lua_rawset(m_luaState, -3);
		}

		//
		// 针对序的一些简单接口
		//
		template<class T>
		void set_value(int index, T val){
			if (is_table() == false){
				return;
			}
			__value_lua__<T>::to_lua(m_luaState, val);
			lua_rawseti(m_luaState, -2, index);
		}

		template<class T>
		T get_value(int index) const{
			if (is_table() == false){
				return T();
			}
			lua_rawgeti(m_luaState, -2, index);
			return __value_lua__<T>::to_cpp(m_luaState,-1);
		}

		int array_len() const{
			if (is_table() == false){
				return 0;
			}
			return  luaL_len(m_luaState, -1); 
		}

		void array_clear(){
			if (is_table() == false){
				return;
			}
			int n = luaL_len(m_luaState, -1);
			for (int i = 0; i < n; ++i){
				lua_pushinteger(m_luaState, i + 1);
				lua_pushnil(m_luaState);
				lua_settable(m_luaState, -3);
			}
		}
	};

#pragma endregion LuaObject
}
