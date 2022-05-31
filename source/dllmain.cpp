#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include <cleo_redux_sdk.h>

#include <Hooking.Patterns.h>
#include <Trampoline.h>
#include <MemoryMgr.h>

class MemoryOperationsPlugin {
public:
	MemoryOperationsPlugin() {
		Log("Init MemoryOperations");
		RegisterCommand("GET_IMAGE_BASE", GetImageBase, "mem");
		RegisterCommand("FIND_PATTERN", GetPattern, "mem");
		RegisterCommand("ALLOC_NEAR", AllocTrampoline, "mem");
		RegisterCommand("READ_RELATIVE_OFFSET", ReadRelativeOffset, "mem");
		RegisterCommand("WRITE_RELATIVE_OFFSET", WriteRelativeOffset, "mem");
	}

	static HandlerResult GetImageBase(Context ctx)
	{
		SetIntParam(ctx, (isize)GetModuleHandleA(NULL));
		return HandlerResult::CONTINUE;
	}

	static HandlerResult GetPattern(Context ctx)
	{
		char buf[STR_MAX_LEN];
		GetStringParam(ctx, buf, sizeof(buf));
		auto index = GetIntParam(ctx);
		auto pattern = hook::pattern(buf);
		if (!pattern.count_hint(index + 1).empty())
		{
			UpdateCompareFlag(ctx, true);
			SetIntParam(ctx, (isize)pattern.get(index).get<void>(0));
		}
		else
		{
			UpdateCompareFlag(ctx, false);
			SetIntParam(ctx, 0);
		}
		return HandlerResult::CONTINUE;
	}

	static HandlerResult AllocTrampoline(Context ctx)
	{
#ifdef _WIN64
		SetIntParam(ctx, (isize)Trampoline::MakeTrampoline(GetModuleHandle(nullptr))->RawSpace(GetIntParam(ctx)));
#else
		SetIntParam(ctx, 0);
#endif
		return HandlerResult::CONTINUE;
	}

	static HandlerResult ReadRelativeOffset(Context ctx)
	{
		auto ib = (isize)GetModuleHandleA(NULL);
		DWORD dwProtect;
		void* val;
		intptr_t dstAddr = GetIntParam(ctx);
		if (dstAddr < ib)
			dstAddr += ib;
		VirtualProtect((void*)dstAddr, 4, PAGE_EXECUTE_READWRITE, &dwProtect);
		Memory::ReadOffsetValue(dstAddr, val);
		VirtualProtect((void*)dstAddr, 4, dwProtect, &dwProtect);
		SetIntParam(ctx, (isize)val);
		return HandlerResult::CONTINUE;
	}

	static HandlerResult WriteRelativeOffset(Context ctx)
	{
		auto ib = (isize)GetModuleHandleA(NULL);
		DWORD dwProtect;
		intptr_t dstAddr = GetIntParam(ctx);
		intptr_t value = GetIntParam(ctx);
		if (dstAddr < ib)
			dstAddr += ib;
		if (value < ib)
			value += ib;
		VirtualProtect((void*)dstAddr, 4, PAGE_EXECUTE_READWRITE, &dwProtect);
		Memory::WriteOffsetValue(dstAddr, *(intptr_t*)&value);
		VirtualProtect((void*)dstAddr, 4, dwProtect, &dwProtect);
		return HandlerResult::CONTINUE;
	}

} MemoryOperationsPlugin;
