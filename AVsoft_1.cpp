#include <windows.h>
#include <stack>

#define READER 10
#define WRITER 5
#define ITERATION 3

HANDLE work_Mutex, read_Mutex;
HANDLE write_Event;
INT ReadCounter = 0;
INT DataCounter = 1;
std::stack <INT> SharedInfo;

VOID Error(CONST LPCWSTR Message);
INT CreateEvents(HANDLE& EVENT);
INT CreateMutexes(HANDLE& MUTEX);
HANDLE* CreateThreads(INT ThreadCount, DWORD WINAPI ThreadProc(LPVOID pParam));
HANDLE* CloseThreads(INT ThreadCount, HANDLE* THREAD);

INT PrepareData();
VOID UseData(INT ID, CONST LPCSTR str, INT data, LPCSTR mode);
DWORD WINAPI READER_thread(LPVOID pParam);
DWORD WINAPI WRITER_thread(LPVOID pParam);

INT main() {
    HANDLE* Readers;
    HANDLE* Writers;
    if (CreateEvents(write_Event))
        ExitProcess(1);
    if (CreateMutexes(work_Mutex))
        ExitProcess(1);
    if (CreateMutexes(read_Mutex))
        ExitProcess(1);
    Writers = CreateThreads(WRITER, WRITER_thread);
    Readers = CreateThreads(READER, READER_thread);
	WaitForMultipleObjects(WRITER, Writers, TRUE, INFINITE);
	WaitForMultipleObjects(READER, Readers, TRUE, INFINITE);
	SharedInfo = std::stack <INT> ();
    Readers = CloseThreads(READER, Readers);
    Writers = CloseThreads(WRITER, Writers);
	CloseHandle(work_Mutex);
	CloseHandle(read_Mutex);
	CloseHandle(write_Event);
	ExitProcess(0);
}

VOID Error(CONST LPCWSTR Message) {
    TCHAR ErrorMSG[256];
    CONST HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteConsole(hStdOut, Message, lstrlen(Message), NULL, NULL);
    wsprintf(ErrorMSG, TEXT("LastError = %d\r\n"), GetLastError());
    WriteConsole(hStdOut, ErrorMSG, lstrlen(ErrorMSG), NULL, NULL);
    ExitProcess(0);
}

INT CreateEvents(HANDLE& EVENT) {
    EVENT = CreateEvent(
        NULL,
        FALSE,
        TRUE,
        NULL
    );
    if (EVENT == NULL) {
        Error(TEXT("Failed to create event.\r\n"));
        return 1;
    }
    return 0;
}

INT CreateMutexes(HANDLE& MUTEX) {
    MUTEX = CreateMutex(
        NULL,
        FALSE,
        NULL
    );
    if (MUTEX == NULL) {
        Error(TEXT("Failed to create mutex.\r\n"));
        return 1;
    }
    return 0;
}

HANDLE* CreateThreads(INT ThreadCount, DWORD WINAPI ThreadProc(LPVOID pParam)) {
    if (ThreadCount) {
        HANDLE* THREAD = new HANDLE[ThreadCount];
        DWORD dwThreadID;
        for (INT i = 0; i < ThreadCount; i++) {
            THREAD[i] = CreateThread(
                NULL,
                NULL,
                ThreadProc,
                (LPVOID)(i + 1),
                NULL,
                &dwThreadID);
            if (THREAD[i] == NULL) {
                Error(TEXT("Failed to create thread.\r\n"));
                delete[] THREAD;
                return NULL;
            }
        }
        return THREAD;
    }
    else
        return NULL;
}

HANDLE* CloseThreads(INT ThreadCount, HANDLE* THREAD) {
    for (INT i = 0; i < ThreadCount; i++)
        CloseHandle(THREAD[i]);
    delete[] THREAD;
    return NULL;
}

INT PrepareData() {
	return DataCounter++;
}

VOID UseData(INT ID, CONST LPCSTR str, INT data, LPCSTR mode) {
	TCHAR Message[256];
	CONST HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	wsprintf(Message, TEXT("%S #%d %S: %d\n"), str, ID, mode, data);
	WriteConsole(hStdOut, Message, lstrlen(Message), NULL, NULL);
}

DWORD WINAPI READER_thread(LPVOID pParam) {
    INT readerID = (INT)pParam;
    INT data;
    for (INT i = 0; i < ITERATION; i++) {
        WaitForSingleObject(work_Mutex, INFINITE);
        WaitForSingleObject(read_Mutex, INFINITE);
        ReleaseMutex(work_Mutex);
        if (++ReadCounter == 1) {
            WaitForSingleObject(write_Event, INFINITE);
            ResetEvent(write_Event);
        }
        ReleaseMutex(read_Mutex);
        if (!SharedInfo.empty())
            data = SharedInfo.top();
        else
            data = NULL;
        WaitForSingleObject(read_Mutex, INFINITE);
        if (--ReadCounter == 0) 
            SetEvent(write_Event);
        ReleaseMutex(read_Mutex);
        UseData(readerID, "READER", data, "scan");
    }
    ExitThread(0);
}

DWORD WINAPI WRITER_thread(LPVOID pParam) {
    INT writerID = (INT)pParam;
    INT data;
    for (INT i = 0; i < ITERATION; i++) {
        data = PrepareData();
        WaitForSingleObject(work_Mutex, INFINITE);
        WaitForSingleObject(write_Event, INFINITE);
        ResetEvent(write_Event);
        ReleaseMutex(work_Mutex);
        SharedInfo.push(data);
        SetEvent(write_Event);
        UseData(writerID, "WRITER", data, "put");
    }
    ExitThread(0);
}