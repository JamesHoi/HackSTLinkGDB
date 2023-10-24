#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCKAPI_ 
#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include "detours.h"
#pragma comment(lib, "detours.lib")

using namespace std;
typedef int (WSAAPI *fnSend)(
    _In_ SOCKET     s,
    _In_ char* buf,
    _In_ int        len,
    _In_ int        flags
);
fnSend oldSendfn;
HANDLE hProcess = NULL;
DWORD pid;


#define VERIFY_CODE "$4f2e4b2e3a3078302c3078302c3078302c3078302c307831312c3078342c3078612c3078300a#ab"
int WSAAPI newSendfn(SOCKET s, char* buf, int len, int flags)
{
    if(len == 82)
    {
        len = 80;
        memcpy(buf, VERIFY_CODE, len);
    }
    return ((fnSend)oldSendfn)(s, buf, len, flags);
}

#define BASE_ADDRESS 0x400000
#define CHECK_ST_RVA 0xA5EC
void Init(void)
{
    /* Patch GDB Verify ST device */
    pid = GetCurrentProcessId();
	hProcess = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
    uint8_t data = 1;
    BOOL ret = WriteProcessMemory(hProcess, (LPVOID)(BASE_ADDRESS + CHECK_ST_RVA), &data, 1, 0);
	if (ret == false)
		MessageBoxW(NULL, L"Patch Error.", L"ImagePatch", MB_OK | MB_ICONERROR);

    /* Hook send function */
    oldSendfn = (fnSend)GetProcAddress(GetModuleHandle(L"ws2_32.dll"), "send");
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach((void**)&oldSendfn, newSendfn);
    if (DetourTransactionCommit() != NO_ERROR)
    {
        MessageBoxW(NULL, L"API Function Hook Error.", L"Init", MB_OK | MB_ICONERROR);
    }
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        Init();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

__declspec(dllexport)void WINAPI Dummy()
{

}