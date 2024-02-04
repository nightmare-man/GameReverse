#include <stdio.h>
#include <stdlib.h>
#include "windows.h"
/*
这里面有几个坑，
首先我们可以直接写一个THREADPROC，里面用MessageBox吗？
这样注入到目标进程后，调用会失败的。

首先回顾下编译和装载：
编译的时候，内部符号（已经确定位置的符号）会以相对地址的形式呈现，
而外部引入的函数，会在导入地址表里注册，然后以间接相对地址的形式出现 例如call [rva]

在链接的时候，将导入地址表里的对应项填入即可。

而动态链接库则在第一次调用的时候执行链接。

因此，如果我们选择直接写MessageBox再注入， 运行的时候（我们的注入器已经经过编译链接了）THREADPROC，
里的call 肯定是call [iat item] 。但是这个iat item是注入器里的地址。在目标进程中，还是不是iat item不知道
更别说仍然是MesasgeBox的iat。同样的，我们给MessageBox传入的立即数参数，作为注入器的静态储存区的地址，在目标
进程中也不正确。

因此 当我们要想目标进程注入的时候，我们必须将地址换成目标进程的。
但是有以下便利我们可以参考。
几乎所有进程都会调用kernel32.dll，并且 里面的函数地址在所有进程都相同。比如LoadLibrary GetProcessAddr
因此我们可以直接使用这两个地址，但是不要以符号的形式使用，因为符号只是编译时的地址，运行后可能会被改变。 （后补充，这一句的说法是错误的，因为符号地址随着编译链接，在运行时也是正确的）
因此我们使用GetProcessAddress来获得这两个函数的地址。

获得这两个函数在目标进程的地址后，就可以加载其他库，调用其中的函数。

最后我们将整个线程函数写入目标进程， 将其参数写入目标进程。
在这之前得分配内存。


*/
typedef struct _THREAD_PARAM {
	FARPROC pFunc[2];
	WCHAR szBuf[3][128];
	char szBuf1[128];
} THREAD_PARAM, *PTHREAD_PARAM;

typedef HMODULE(WINAPI* PFLOADLIBRARY)(LPCWSTR libname);
typedef FARPROC(WINAPI* PFGETPROCADDRESS)(HMODULE hmodule, LPCSTR funcname);
typedef int(WINAPI* PFMESSAGE)(HWND h, LPCWSTR txt, LPCWSTR caption, UINT type);
 
DWORD WINAPI THREADPROC(LPVOID lparam) {
	
	PTHREAD_PARAM param = (PTHREAD_PARAM)lparam;
	HMODULE hmodule = NULL;
	FARPROC pfunc = NULL;
	hmodule = ((PFLOADLIBRARY)(param->pFunc[0]))(param->szBuf[0]);
	pfunc = ((PFGETPROCADDRESS)(param->pFunc[1]))(hmodule, param->szBuf1);
	int ret = ((PFMESSAGE)pfunc)(NULL, param->szBuf[1], param->szBuf[2], MB_OK);
	while (1) {
		
	}
	return 0;
}

DWORD InjectCode(DWORD pid) {
	HANDLE target_p = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	THREAD_PARAM param;
	HMODULE hmod = GetModuleHandleW(L"kernel32.dll");

	param.pFunc[0] = GetProcAddress(hmod, "LoadLibraryW");
	param.pFunc[1] = GetProcAddress(hmod, "GetProcAddress");
	printf("%p", param.pFunc[1]);
	printf("%p", (void*)GetProcAddress);
	wcscpy_s(param.szBuf[0], 11, L"user32.dll");
	wcscpy_s(param.szBuf[1], 15, L"这是一个inject box");
	wcscpy_s(param.szBuf[2], 6, L"也许是标题");
	strcpy_s(param.szBuf1, 12, "MessageBoxW");
	LPVOID param_addr = VirtualAllocEx(target_p, NULL, sizeof(param), MEM_COMMIT, PAGE_READWRITE);
	LPVOID thread_func_addr = VirtualAllocEx(target_p, NULL, 4096, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	SIZE_T ret = 0;
	int real_ret = -1;
	real_ret=WriteProcessMemory(target_p, param_addr, (LPVOID)&param, sizeof(param), &ret);
	real_ret=WriteProcessMemory(target_p, thread_func_addr, (LPVOID)THREADPROC, 256, &ret);
	HANDLE h = CreateRemoteThread(target_p, NULL, 0, (LPTHREAD_START_ROUTINE)thread_func_addr, param_addr, NULL, NULL);
	WaitForSingleObject(h, INFINITE);
	CloseHandle(h);
	CloseHandle(target_p);
	return 0;
}


DWORD SetDebugPriviledge() {
	HANDLE token;
	DWORD ret = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token);
	if (ret == 0) {
		printf("获取打开令牌失败！");
		CloseHandle(token);
		return -1;
	}
	LUID luid;
	ret = LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);
	if (ret == 0) {
		CloseHandle(token);
		printf("获取debug权限luid失败");
		return -1;
	}
	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	ret = AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), NULL, NULL);
	if (ret == 0) {
		CloseHandle(token);
		printf("调整安全令牌失败，无法获取debug权限\n");
		return -1;
	}
	BOOL suc = (GetLastError() == ERROR_SUCCESS);
	if (suc) {
		printf("debug权限开启成功!\n");
	}

	return 0;
}
int main() {
	MessageBox(NULL, L"nihao", L"qq", MB_OK);
	SetDebugPriviledge();
	DWORD a = 0;
	//printf("%u", (unsigned int)InjectCode -(unsigned int)THREADPROC);
	scanf_s("%ld", &a);
	InjectCode(a);
	return 0;
}