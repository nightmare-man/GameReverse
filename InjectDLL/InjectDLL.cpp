#include <stdio.h>
#include "windows.h"

DWORD InjectFunc(DWORD pid, LPCWSTR path) {
    HANDLE target_pro = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (target_pro == NULL) {
        printf("打开进程失败!");
        return -1;
    }
    SIZE_T buff_size = (wcslen(path)+1) *sizeof(WCHAR);
    LPVOID remote_buf = VirtualAllocEx(target_pro, NULL, buff_size, MEM_COMMIT, PAGE_READWRITE);
    if (remote_buf == NULL) {
        printf("分配内存失败!");
        CloseHandle(target_pro);
        return -1;
    }
    SIZE_T real_write_size = 0;
    WriteProcessMemory(target_pro, remote_buf, (LPCVOID)path, buff_size, &real_write_size);
    if (real_write_size < buff_size) {
        printf("写入失败!");
        CloseHandle(target_pro);
        return -1;
    }
    HMODULE m = GetModuleHandleW(L"Kernel32.dll");
    if (m == NULL) {
        printf("获取Kernel32.dll失败!\n");
        CloseHandle(target_pro);
        return -1;
    }
    LPTHREAD_START_ROUTINE start_func = (LPTHREAD_START_ROUTINE)GetProcAddress(m, "LoadLibraryW");
    if (start_func == NULL) {
        printf("获取LoadLibraryW失败\n");
        CloseHandle(target_pro);
        return -1;
    }
    HANDLE rthread = CreateRemoteThread(target_pro, NULL, 1024 * 1024, start_func, remote_buf, NULL, NULL);
    if (rthread == NULL) {
        printf("创建远程线程失败！\n");
        CloseHandle(target_pro);
        return -1;
    }
    WaitForSingleObject(rthread, INFINITE);
    CloseHandle(rthread);
    CloseHandle(target_pro);
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
    BOOL suc =( GetLastError() == ERROR_SUCCESS);
    if (suc) {
        printf("debug权限开启成功!\n");
    }
  
    return 0;
}

int main()
{
    WCHAR buf[100];
    DWORD pid = 0;
    printf("请输入dll路径\n");
    //这里的路径要用绝对路径，或者目标程序的相对路径，因为是在目标程序
    //创建线程，调用loadlibrary，因此是目标程序的相对路径
    wscanf_s(L"%ls", buf, (SIZE_T)_countof(buf));
    scanf_s("%d", &pid);
    SetDebugPriviledge();
    InjectFunc(pid, buf);
    while (1) {

    }
    return 0;
}


/*
以下引用自chatgpt

在Windows中，访问令牌（Access Token）和安全描述符（Security Descriptor）是用于管理进程和对象访问权限的关键概念。它们通常一起配合工作，确保系统的安全性。下面解释了它们的基本概念和角色：

访问令牌（Access Token）：

角色： 访问令牌是一个数据结构，包含了有关进程或用户的安全信息，如用户的安全标识符（SID）、特权（Privileges）、用户组、以及其他与安全相关的信息。
生成： 当用户登录系统时，系统会为其创建一个访问令牌，该令牌包含用户的安全信息。
作用： 访问令牌被附加到进程，用于标识进程的安全上下文。进程的权限和行为会受到访问令牌的限制。
安全描述符（Security Descriptor）：

角色： 安全描述符是一个数据结构，包含了关于对象（如文件、进程、线程等）的安全信息。它包括了拥有者、主要组、自主访问控制列表（DACL）和系统访问控制列表（SACL）等信息。
生成： 当对象被创建时，系统会为其分配一个默认的安全描述符。安全描述符可以随后被修改。
作用： 安全描述符定义了对象的安全属性，控制着哪些用户或进程可以访问对象，以及以何种权限进行访问。
访问令牌和安全描述符的协作：

当一个进程（源进程）尝试访问另一个对象（目标对象，如进程、文件等）时，Windows会检查源进程的访问令牌和目标对象的安全描述符。
访问令牌中包含了源进程的安全上下文信息，包括用户的身份、权限和其他信息。
目标对象的安全描述符定义了它的安全属性，包括对象的拥有者、访问控制列表（DACL）等。
Windows会根据这两者的信息，判断源进程是否有权访问目标对象以及允许的操作。
访问权限检查：

当进行访问权限检查时，Windows会检查源进程的访问令牌，查看它是否包含了执行操作所需的权限。
同时，Windows会检查目标对象的安全描述符，查看访问令牌中的用户是否在对象的 DACL 中，以及是否有足够的权限进行所需的操作。
总体来说，访问令牌和安全描述符协同工作，确保系统的访问控制和安全性。访问令牌定义了进程的安全上下文，而安全描述符定义了对象的安全属性，两者共同决定了对象的访问权限。
*/
