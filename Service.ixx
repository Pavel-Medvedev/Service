#include <fstream>
#include <Windows.h>
export module Service;

class Service
{
public:
	Service(std::string);
	Service(Service&);
	~Service();
	LPCWSTR GetName() { return serviceName; };
	Service operator=(Service);
private:
	std::fstream log;
	LPCWSTR serviceName;
	SERVICE_STATUS ServiceStatus;
	SERVICE_STATUS_HANDLE hServiceStausHandler;
	HANDLE hServiceEvent;
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;
	DWORD dwCounter = NULL;

	friend void WINAPI ServiceMain(DWORD dwArgc, LPSTR* lpArgv);
	friend void WINAPI ServiceControlHnadler(DWORD dwControl);
	friend void ServiceReportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
	friend void ServiceInit(DWORD dwArgc, LPSTR* lpArgv);
	friend void ServiceInstal();
	friend void ServiceDelete();
	friend void ServiceStart();
	friend void ServiceStop();
};
Service service("Service");
void WINAPI ServiceMain(DWORD dwArgc, LPSTR* lpArgv);
void WINAPI ServiceControlHnadler(DWORD dwControl);
void ServiceReportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
void ServiceInit(DWORD dwArgc, LPSTR* lpArgv);
void ServiceInstal();
void ServiceDelete();
void ServiceStart();
void ServiceStop();




Service::Service(std::string serviceName)
{
	this->serviceName = std::wstring(serviceName.begin(), serviceName.end()).c_str();
	hServiceStausHandler = RegisterServiceCtrlHandler(this->serviceName, (LPHANDLER_FUNCTION)ServiceControlHnadler);
	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	hService = OpenService(hSCManager, this->serviceName, SERVICE_ALL_ACCESS);
	log.open(serviceName + "Log.txt", 9);
	if (!log)
		log.open(serviceName + "Log.txt", 1);
}

Service::Service(Service& other)
{
	serviceName = other.serviceName;
	int strLength
		= WideCharToMultiByte(CP_UTF8, 0, serviceName, -1,
			nullptr, 0, nullptr, nullptr);
	std::string str(strLength, 0);
	WideCharToMultiByte(CP_UTF8, 0, serviceName, -1, &str[0],
		strLength, nullptr, nullptr);
	hServiceStausHandler = RegisterServiceCtrlHandler(this->serviceName, (LPHANDLER_FUNCTION)ServiceControlHnadler);
	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	hService = OpenService(hSCManager, this->serviceName, SERVICE_ALL_ACCESS);
	log.open(str + "Log.txt", 9);
	if (!log)
		log.open(str + "Log.txt", 1);
}

Service::~Service()
{
	log.close();
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);

}

Service Service::operator=(Service other) {
	serviceName = other.serviceName;
	int strLength
		= WideCharToMultiByte(CP_UTF8, 0, serviceName, -1,
			nullptr, 0, nullptr, nullptr);
	std::string str(strLength, 0);
	WideCharToMultiByte(CP_UTF8, 0, serviceName, -1, &str[0],
		strLength, nullptr, nullptr);
	hServiceStausHandler = RegisterServiceCtrlHandler(this->serviceName, (LPHANDLER_FUNCTION)ServiceControlHnadler);
	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	hService = OpenService(hSCManager, this->serviceName, SERVICE_ALL_ACCESS);
	log.open(str + "Log.txt", 9);
	if (!log)
		log.open(str + "Log.txt", 1);
	return other;
}


void WINAPI ServiceMain(DWORD dwArgc, LPSTR* lpArgv) {
	service.ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	service.ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP;
	service.ServiceStatus.dwServiceSpecificExitCode = 0;
	ServiceReportStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
	SetServiceStatus(service.hServiceStausHandler, &service.ServiceStatus);
	ServiceInit(dwArgc, lpArgv);
}


void WINAPI ServiceControlHnadler(DWORD dwControl) {
	DWORD errorCode = NO_ERROR;
	switch (dwControl)
	{
	case SERVICE_CONTROL_INTERROGATE:
		break;
	case SERVICE_CONTROL_STOP:
		service.ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		break;
	case SERVICE_CONTROL_SHUTDOWN:
		service.ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		break;
	default:
		errorCode = ERROR_CALL_NOT_IMPLEMENTED;
		break;
	}
	ServiceReportStatus(service.ServiceStatus.dwCurrentState, errorCode, 0);
}

void ServiceReportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint) {
	service.ServiceStatus.dwCurrentState = dwCurrentState;
	service.ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
	service.ServiceStatus.dwWaitHint = dwWaitHint;
	if (dwCurrentState == SERVICE_START_PENDING)
		service.ServiceStatus.dwControlsAccepted = 0;
	else
		service.ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	if (dwCurrentState == SERVICE_RUNNING || dwCurrentState == SERVICE_STOPPED)
		service.ServiceStatus.dwCheckPoint = 0;
	else
		service.ServiceStatus.dwCheckPoint = service.dwCounter++;
	SetServiceStatus(service.hServiceStausHandler, &service.ServiceStatus);
}



void ServiceInit(DWORD dwArgc, LPSTR* lpArgv) {
	service.hServiceEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (service.hServiceEvent == NULL)
		ServiceReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
	else
		ServiceReportStatus(SERVICE_RUNNING, NO_ERROR, 0);

	while (true)
	{
		WaitForSingleObject(service.hServiceEvent, INFINITE);
		ServiceReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
	}
}

void ServiceInstal() {

	DWORD dwModuleFileName = 0;
	TCHAR szPath[MAX_PATH];
	dwModuleFileName = GetModuleFileName(NULL, szPath, MAX_PATH);
	service.hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	service.hService = CreateService(
		service.hSCManager,
		service.serviceName,
		service.serviceName,
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		szPath,
		NULL, NULL, NULL, NULL, NULL
	);
}

void ServiceDelete() {
	service.log << "start\t";
	DeleteService(service.hService);
	service.log << GetLastError();
}

void ServiceStart() {
	SERVICE_STATUS_PROCESS SvcStatusProcess;
	STARTUPINFO startupInfo;
	PROCESS_INFORMATION processInfo;
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	ZeroMemory(&processInfo, sizeof(processInfo));
	DWORD dwBytesNeeded = NULL;
	service.log << "start\t";
	QueryServiceStatusEx(service.hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&SvcStatusProcess, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded);
	while (SvcStatusProcess.dwCurrentState == SERVICE_STOP_PENDING)
		QueryServiceStatusEx(service.hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&SvcStatusProcess, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded);
	if (SvcStatusProcess.dwCurrentState != SERVICE_RUNNING) {
		StartService(service.hService, NULL, NULL);
		CreateProcess(TEXT("C:\\Windows\\System32\\cmd.exe"), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);
	}
	service.log << processInfo.dwProcessId << '\t';
	QueryServiceStatusEx(service.hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&SvcStatusProcess, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded);
	ServiceReportStatus(SvcStatusProcess.dwCurrentState, NO_ERROR, 3000);
	service.log << SvcStatusProcess.dwCurrentState << std::endl;
}

void ServiceStop() {
	SERVICE_STATUS_PROCESS SvcStatusProcess;
	DWORD dwBytes;
	ControlService(service.hService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&SvcStatusProcess);
	while (SvcStatusProcess.dwCurrentState != SERVICE_STOPPED)
		QueryServiceStatusEx(service.hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&SvcStatusProcess, sizeof(SERVICE_STATUS_PROCESS), &dwBytes);
}

export bool Start(int argc, CHAR* argv[]) {
	if (lstrcmpiA(argv[1], "install") == 0)
		ServiceInstal();
	else if (lstrcmpiA(argv[1], "start") == 0)
		ServiceStart();
	else if (lstrcmpiA(argv[1], "stop") == 0)
		ServiceStop();
	else if (lstrcmpiA(argv[1], "delete") == 0)
		ServiceDelete();
	SERVICE_TABLE_ENTRY DispatchTable[] = {
		{(LPWSTR)service.GetName(), (LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{NULL, NULL}
	};
	StartServiceCtrlDispatcher(DispatchTable);
	return true;
}