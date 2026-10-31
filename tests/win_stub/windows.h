/* Host-test stand-in for <windows.h>: only what TrackWidth uses. */
#ifndef TW_TEST_WINDOWS_H
#define TW_TEST_WINDOWS_H
#include <stddef.h>
#include <stdint.h>
typedef void *HANDLE,*LPVOID,*HWND;
typedef unsigned long DWORD;
typedef long LONG;
typedef int BOOL;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef BOOL (*WNDENUMPROC)(HWND,LPARAM);
#define WINAPI
#define CALLBACK
#define TRUE 1
#define FALSE 0
#define GW_OWNER 4
#define WM_CLOSE 0x10
#define MEM_COMMIT 0x1000
#define PAGE_GUARD 0x100
#define PAGE_NOACCESS 0x01
#define PAGE_READONLY 0x02
#define PAGE_READWRITE 0x04
#define PAGE_WRITECOPY 0x08
#define PAGE_EXECUTE_READ 0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_EXECUTE_WRITECOPY 0x80
typedef struct {void *BaseAddress;void *AllocationBase;DWORD AllocationProtect;size_t RegionSize;DWORD State;DWORD Protect;DWORD Type;} MEMORY_BASIC_INFORMATION;
size_t VirtualQuery(const void*,MEMORY_BASIC_INFORMATION*,size_t);
BOOL EnumWindows(WNDENUMPROC,LPARAM);
DWORD GetWindowThreadProcessId(HWND,DWORD*);
DWORD GetCurrentProcessId(void);
BOOL IsWindowVisible(HWND);
HWND GetWindow(HWND,unsigned);
BOOL PostMessageW(HWND,unsigned,WPARAM,LPARAM);
HANDLE CreateThread(void*,size_t,DWORD(*)(LPVOID),void*,DWORD,DWORD*);
BOOL CloseHandle(HANDLE);
LONG InterlockedIncrement(volatile LONG*);
LONG InterlockedDecrement(volatile LONG*);
#define HEAP_ZERO_MEMORY 8
HANDLE GetProcessHeap(void);
void *HeapAlloc(HANDLE,DWORD,size_t);
BOOL HeapFree(HANDLE,DWORD,void*);
LONG InterlockedExchangeAdd(volatile LONG*,LONG);
void Sleep(DWORD);
#define MAX_PATH 260
typedef long long LONGLONG;
typedef void *PVOID;
uint64_t GetTickCount64(void);
typedef void *HMODULE;
typedef const char *LPCSTR;
#define GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS 4
#define GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT 2
BOOL GetModuleHandleExA(DWORD,LPCSTR,HMODULE*);
DWORD GetModuleFileNameA(HMODULE,char*,DWORD);
unsigned GetPrivateProfileIntA(const char*,const char*,int,const char*);
BOOL WritePrivateProfileStringA(const char*,const char*,const char*,const char*);
#endif
