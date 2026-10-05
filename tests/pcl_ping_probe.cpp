// Diagnostic only: send the SDK's registered PCL ping to the game's window.
// No input keys, character actions, private data or plugin configuration.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
DWORD pid=0;unsigned sent=0;
BOOL CALLBACK ping(HWND window,LPARAM msg){DWORD owner=0;GetWindowThreadProcessId(window,&owner);if(owner==pid&&IsWindowVisible(window)&&PostMessageW(window,UINT(msg),0,0))++sent;return TRUE;}
int main(){auto snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(snapshot==INVALID_HANDLE_VALUE)return 2;PROCESSENTRY32W e{};e.dwSize=sizeof(e);if(Process32FirstW(snapshot,&e))do{if(!_wcsicmp(e.szExeFile,L"Dungeons-Win64-Shipping.exe")){if(pid){CloseHandle(snapshot);return 3;}pid=e.th32ProcessID;}}while(Process32NextW(snapshot,&e));CloseHandle(snapshot);if(!pid)return 4;auto message=RegisterWindowMessageW(L"PC_Latency_Stats_Ping");if(!message)return 5;EnumWindows(ping,LPARAM(message));std::cout<<"Diagnostic pings queued: "<<sent<<"\n";return sent?0:6;}
