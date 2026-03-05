#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>

DWORD GetProcessIdByName(const std::wstring& processName)
{
    DWORD pid = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 processEntry = { sizeof(PROCESSENTRY32) };
        if (Process32First(hSnapshot, &processEntry))
        {
            do
            {
                if (processName.compare(processEntry.szExeFile) == 0)
                {
                    pid = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &processEntry));
        }
        CloseHandle(hSnapshot);
    }
    return pid;
}

void InjectDll(DWORD processid, const std::string& dllPath)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processid);
    if (hProcess != NULL)
    {
        // Allocate memory in the remote process
        LPVOID remoteMemory = VirtualAllocEx(hProcess, NULL, dllPath.size(), MEM_COMMIT, PAGE_READWRITE);
        if (remoteMemory != NULL)
        {
            // Write the DLL file path to the memory
            WriteProcessMemory(hProcess, remoteMemory, dllPath.c_str(), dllPath.size(), NULL);
            // Create a remote thread to load the DLL
            DWORD threadId;
            HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, remoteMemory, 0, &threadId);
            if (hThread != NULL)
            {
                WaitForSingleObject(hThread, INFINITE);
                CloseHandle(hThread);
            }

        // Free the memory
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
    }
    CloseHandle(hProcess);
}
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <dll path>" << std::endl;
        return 1;
    }
    std::string dllPath = argv[1];
    DWORD processid = GetProcessIdByName(L"Msango.bin");
    if (processid > 0)
    {
        InjectDll(processid, dllPath);
    }
    return 0;
}
