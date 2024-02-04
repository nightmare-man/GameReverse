#include <stdio.h>
#include "windows.h"

typedef int(*__cdecl LOAD_FUNC)(const char* className, const char* lpszDriverPath);
typedef void (*__cdecl SetHandle)();
typedef void (*__cdecl MouseMoveRELATIVE)(LONG dx, LONG dy);
typedef void (*__cdecl MouseMoveABSOLUTE)(LONG dx, LONG dy);
typedef void (*__cdecl MouseRightButtonDown)();
typedef void (*__cdecl MouseRightButtonUp)();
typedef int (*__cdecl UnloadNTDriver)(const char* szSvrName);
int main()
{

    WCHAR buf[100]; 
    /*
    *需要知道的是wchar_t是宽字符，也就是unicode编码的字符，在windows
    * 中的实现是utf-16即每个字符两个字节，需要两个字节全为0才结束
    * 要使用配套的wscanf wsprintf  ，以下用的是wscanf_s 需要在知道
    * buf的数组大小，而不是实际的字符长度
    * 
    * 而char 是单字符编码，对于非ascii字符，用的是多个char编码一个字符，
    * 如果系统是gbk编码，则当scanf读入中文时，实际上读入的是gbk编码的多个
    * 的char，保存到buf中，输出的时候printf向终端发送的也是多个字节的char，
    * 如果终端能够支持gbk编码，则会解码成对应的字符，不支持就是乱码或者多个ascii
    * 这正是为什么scanf printf能支持超过ascii范围的原因，对他们来说不存在编码的概念
    * 只知道一个基本字符是1个字节，由终端自己重组
    * 
    * 同样的 wscanf 和wprintf只不过是换成了两个字节罢了
        */
    wscanf_s(L"%ls", buf, (unsigned int)_countof(buf));
    HMODULE m = LoadLibraryEx(buf, NULL, NULL);
   
    
    auto func1= (LOAD_FUNC)GetProcAddress(m, "LoadNTDriver");
    auto func2 = (SetHandle)GetProcAddress(m, "SetHandle");
    auto func3 = (MouseMoveRELATIVE)GetProcAddress(m, "MouseMoveRELATIVE");
    auto func4= (MouseRightButtonDown)GetProcAddress(m, "MouseRightButtonDown");
    auto func5 = (MouseRightButtonUp)GetProcAddress(m, "MouseRightButtonUp");
    auto func6 = (UnloadNTDriver)GetProcAddress(m, "UnloadNTDriver");
    auto func7 = (MouseMoveABSOLUTE)GetProcAddress(m, "MouseMoveABSOLUTE");
    func1("kmclass", "d:\\data\\1.sys");
    func2();
    if(func3!=NULL){
        
                func7(100, 100);
                func4();
                func5();
                //Sleep(1000);
       
    }else{
        printf("get func error\n");
    }
    func6("kmclass");
    return 0;
}

