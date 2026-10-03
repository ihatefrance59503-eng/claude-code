// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
//#pragma once
//#ifdef _KERNEL_MODE
//#include <ntddk.h>
//#include <ntdef.h>
//#else
//#include <Windows.h>
//#include <utility>
//#endif
//
///*
// *  Copyright 2022 Barracudach
// *
// * Licensed under the Apache License, Version 2.0 (the "License");
// * you may not use this file except in compliance with the License.
// * You may obtain a copy of the License at
// *
// *     http://www.apache.org/licenses/LICENSE-2.0
// *
// * Unless required by applicable law or agreed to in writing, software
// * distributed under the License is distributed on an "AS IS" BASIS,
// * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// * See the License for the specific language governing permissions and
// * limitations under the License.
// */
//
// // === FAQ === documentation is available at https://github.com/Barracudach
////Supports 2 modes: kernelmode and usermode(x64)
////For kernel- disable Control Flow Guard (CFG) /guard:cf 
////usermode c++17 and above
////kernelmode c++14 and above
//
//
//
//#define spoof CallSpoofer::SpoofFunction spoof(_AddressOfReturnAddress());
//#ifdef _KERNEL_MODE
//#define spoof_c(ret_type,name) (CallSpoofer::SafeCall<ret_type,std::remove_reference_t<decltype(*name)>>(name))
//#else
//#define SPOOF_CALL(name) (CallSpoofer::SafeCall(name))
//#endif
//
//
//#define MAX_FUNC_BUFFERED 100
//#define SHELLCODE_GENERATOR_SIZE 500
//
//namespace CallSpoofer
//{
//#ifdef _KERNEL_MODE
//	typedef unsigned __int64  uintptr_t, size_t;
//#pragma region std::forward
//
//#pragma endregion 
//
//#else
//	using namespace std;
//#endif
//
//}
//
//
//namespace CallSpoofer
//{
//	class SpoofFunction
//	{
//	public:
//		uintptr_t temp = 0;
//		const uintptr_t xor_key = 0xff00ff00ff00ff00;
//		void* ret_addr_in_stack = 0;
//
//		SpoofFunction(void* addr) :ret_addr_in_stack(addr)
//		{
//			temp = *(uintptr_t*)ret_addr_in_stack;
//			temp ^= xor_key;
//			*(uintptr_t*)ret_addr_in_stack = 0;
//		}
//		~SpoofFunction()
//		{
//			temp ^= xor_key;
//			*(uintptr_t*)ret_addr_in_stack = temp;
//		}
//	};
//
//#ifdef _KERNEL_MODE
//	__forceinline PVOID LocateShellCode(PVOID func, size_t size = 500)
//	{
//		void* addr = ExAllocatePoolWithTag(NonPagedPool, size, (ULONG)"File");
//		if (!addr)
//			return nullptr;
//		return memcpy(addr, func, size);
//	}
//#else
//	__forceinline PVOID LocateShellCode(PVOID func, size_t size = SHELLCODE_GENERATOR_SIZE)
//	{
//		void* addr = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
//		if (!addr)
//			return nullptr;
//		return memcpy(addr, func, size);
//	}
//#endif
//
//#ifdef _KERNEL_MODE
//	template <typename RetType, typename Func, typename ...Args>
//	RetType
//#else
//	template <typename Func, typename ...Args>
//	typename std::invoke_result<Func, Args...>::type
//#endif
//		__declspec(safebuffers)ShellCodeGenerator(Func f, Args&... args)
//	{
//#ifdef _KERNEL_MODE
//		using this_func_type = decltype(ShellCodeGenerator<RetType, Func, Args&...>);
//		using return_type = RetType;
//#else
//		using this_func_type = decltype(ShellCodeGenerator<Func, Args&...>);
//		using return_type = typename std::invoke_result<Func, Args...>::type;
//#endif
//		const uintptr_t xor_key = 0xff00ff00ff00ff00;
//		void* ret_addr_in_stack = _AddressOfReturnAddress();
//		uintptr_t temp = *(uintptr_t*)ret_addr_in_stack;
//		temp ^= xor_key;
//		*(uintptr_t*)ret_addr_in_stack = 0;
//
//		if constexpr (std::is_same<return_type, void>::value)
//		{
//			f(args...);
//			temp ^= xor_key;
//			*(uintptr_t*)ret_addr_in_stack = temp;
//		}
//		else
//		{
//			return_type&& ret = f(args...);
//			temp ^= xor_key;
//			*(uintptr_t*)ret_addr_in_stack = temp;
//			return ret;
//		}
//	}
//
//
//
//#ifdef _KERNEL_MODE
//	template<typename RetType, class Func>
//#else
//	template<class Func >
//#endif
//	class SafeCall
//	{
//		Func* funcPtr;
//
//	public:
//		SafeCall(Func* func) : funcPtr(func) {}
//
//		template<typename... Args>
//		__forceinline decltype(auto) operator()(Args&&... args)
//		{
//			spoof;
//
//			using return_type =
//#ifdef _KERNEL_MODE
//				RetType;
//#else
//				typename std::invoke_result<Func, Args...>::type;
//#endif
//
//			// 🔴 IMPORTANT FIX:
//			// Use uintptr_t ONLY for identity (no decltype, no function pointer types)
//			uintptr_t self_addr =
//				reinterpret_cast<uintptr_t>(
//					&ShellCodeGenerator<RetType, Func*, Args...>
//					);
//
//			// 🔴 storage must also be uintptr_t (NOT function pointer types)
//			static uintptr_t orig_generator[MAX_FUNC_BUFFERED]{};
//			static uintptr_t alloc_generator[MAX_FUNC_BUFFERED]{};
//			static size_t count{};
//
//			uintptr_t p_shellcode = 0;
//
//			unsigned index = 0;
//			while (orig_generator[index])
//			{
//				if (orig_generator[index] == self_addr)
//				{
//					p_shellcode = alloc_generator[index];
//					break;
//				}
//				index++;
//			}
//
//			if (!p_shellcode)
//			{
//				void* allocated = LocateShellCode((void*)self_addr);
//
//				p_shellcode = reinterpret_cast<uintptr_t>(allocated);
//
//				orig_generator[count] = self_addr;
//				alloc_generator[count] = p_shellcode;
//				count++;
//			}
//
//			using shell_fn = return_type(*)(Func*, Args...);
//
//			return reinterpret_cast<shell_fn>(p_shellcode)(funcPtr, std::forward<Args>(args)...);
//		}
//	};
//}