#include <Windows.h>

#pragma section(".mbd2", read, write)
#pragma section(".mbd3", read, write)

extern "C"
{
    __declspec(allocate(".mbd2")) volatile unsigned char Padding[0x2000] = { 2 };
    __declspec(allocate(".mbd3")) __declspec(dllexport) volatile signed char ReadTarget = -1;
    __declspec(allocate(".mbd3")) __declspec(dllexport) volatile signed char WriteTarget = 0;
    __declspec(allocate(".mbd3")) __declspec(dllexport) volatile unsigned char* HeapPointer = nullptr;

#pragma code_seg(push, membp_code, ".mbc")
    __declspec(dllexport) __declspec(noinline) void ReadSequence()
    {
        const auto value = ReadTarget;
        ExitProcess(static_cast<UINT>(static_cast<unsigned char>(value)));
    }

    __declspec(dllexport) __declspec(noinline) void WriteSequence()
    {
        WriteTarget = 0x5A;
        ExitProcess(0x5A);
    }

    __declspec(dllexport) __declspec(noinline) void ExecSequence()
    {
        ExitProcess(0x33);
    }

    __declspec(noinline) void start()
    {
        ExitProcess(0);
    }

    __declspec(dllexport) __declspec(noinline) void WriteHeap()
    {
        *HeapPointer = 0xBC;

        ExitProcess(static_cast<UINT>(*HeapPointer));
    }

    __declspec(dllexport) __declspec(noinline) void StartHeap()
    {
        HANDLE hHeap = GetProcessHeap();
        HeapPointer = (unsigned char*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, 1024);

        WriteHeap();
    }
#pragma code_seg(pop, membp_code)
}
