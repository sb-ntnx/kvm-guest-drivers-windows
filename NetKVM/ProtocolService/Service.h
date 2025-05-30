#pragma once

class CServiceManager
{
  public:
    CServiceManager(bool bQueryOnly = false)
    {
        m_Handle = OpenSCManager(NULL, NULL, bQueryOnly ? SC_MANAGER_ENUMERATE_SERVICE : SC_MANAGER_ALL_ACCESS);
        if (!m_Handle)
        {
            Log("%s - failed, error %d", __FUNCTION__, GetLastError());
        }
    }
    ~CServiceManager()
    {
        if (m_Handle)
        {
            CloseServiceHandle(m_Handle);
        }
    }
    bool Install(LPCTSTR name, LPCTSTR binary)
    {
        CString s;
        s.Format(_T("\"%s\" %s"), binary, name);
        SC_HANDLE h = CreateService(m_Handle,
                                    name,
                                    name,
                                    SERVICE_STOP | SERVICE_START | SERVICE_QUERY_STATUS,
                                    ServiceType(),
                                    SERVICE_DEMAND_START,
                                    SERVICE_ERROR_NORMAL,
                                    s,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL,
                                    NULL);
        if (h)
        {
            Log("%s: service installed", __FUNCTION__);
            CloseServiceHandle(h);
        }
        else
        {
            Log("%s: error %d", __FUNCTION__, GetLastError());
        }
        return h != NULL;
    }
    SC_HANDLE Handle() const
    {
        return m_Handle;
    }
    static ULONG ServiceType()
    {
        return SERVICE_WIN32_OWN_PROCESS;
    }

  protected:
    SC_HANDLE m_Handle;
};

class CService
{
  public:
    CService(LPCWSTR name, bool bQueryOnly = false) : m_Handle(NULL), m_ServiceManager(bQueryOnly)
    {
        ULONG flags = SERVICE_QUERY_STATUS;
        if (!bQueryOnly)
        {
            flags |= SERVICE_STOP | SERVICE_START | DELETE | SERVICE_USER_DEFINED_CONTROL;
        }
        if (m_ServiceManager.Handle())
        {
            m_Handle = OpenService(m_ServiceManager.Handle(), name, flags);
        }
        if (!m_Handle)
        {
            Log("%s %S - failed, error %d", __FUNCTION__, name, GetLastError());
        }
    }
    ~CService()
    {
        if (m_Handle)
        {
            CloseServiceHandle(m_Handle);
        }
    }
    ULONG Stop()
    {
        if (!m_Handle)
        {
            return ERROR_ACCESS_DENIED;
        }
        if (!ControlService(m_Handle, SERVICE_CONTROL_STOP, &m_Status))
        {
            ULONG error = GetLastError();
            Log("%s %u", __FUNCTION__, error);
            return error;
        }
        Log("%s done", __FUNCTION__);
        return ERROR_SUCCESS;
    }
    ULONG Start()
    {
        if (!m_Handle)
        {
            return ERROR_ACCESS_DENIED;
        }
        if (!StartService(m_Handle, 0, NULL))
        {
            ULONG error = GetLastError();
            Log("%s %u", __FUNCTION__, error);
            return error;
        }
        Log("%s done", __FUNCTION__);
        return ERROR_SUCCESS;
    }
    ULONG Query(ULONG &state)
    {
        if (!m_Handle)
        {
            return ERROR_ACCESS_DENIED;
        }
        if (!QueryServiceStatus(m_Handle, &m_Status))
        {
            return GetLastError();
        }
        state = m_Status.dwCurrentState;
        Log("%s = %s(0x%X)", __FUNCTION__, StateName(state), state);
        return ERROR_SUCCESS;
    }
    bool Delete()
    {
        bool b;
        if (!m_Handle)
        {
            return false;
        }
        b = !!DeleteService(m_Handle);
        if (!b)
        {
            ULONG error = GetLastError();
            Log("%s %u", __FUNCTION__, error);
        }
        else
        {
            Log("%s OK", __FUNCTION__);
        }
        return b;
    }
    bool NeedActionToStart()
    {
        ULONG state;
        if (Query(state))
        {
            return false;
        }
        return state == SERVICE_STOPPED || state == SERVICE_STOP_PENDING;
    }
    bool NeedActionToStop()
    {
        ULONG state;
        if (Query(state))
        {
            return false;
        }
        return state == SERVICE_RUNNING;
    }
    static LPCSTR StateName(ULONG state)
    {
        LPCSTR names[] = {"Unknown",
                          "Stopped",
                          "Start Pending",
                          "Stop Pending",
                          "Running",
                          "Continue Pending",
                          "Pause Pending",
                          "Paused"};
        if (state >= ELEMENTS_IN(names))
        {
            state = 0;
        }
        return names[state];
    }
    ULONG NotifyStatusChange(ULONG dwNotifyMask, PSERVICE_NOTIFY pNotify)
    {
        if (m_Handle)
        {
            ULONG state;
            if (!Query(state) && (m_Status.dwServiceType == SERVICE_KERNEL_DRIVER ||
                                  m_Status.dwServiceType == SERVICE_FILE_SYSTEM_DRIVER))
            {
                Log("%s: Not possible for kernel drivers", __FUNCTION__);
                return ERROR_INVALID_FLAGS;
            }
            return NotifyServiceStatusChange(m_Handle, dwNotifyMask, pNotify);
        }
        else
        {
            return ERROR_INVALID_HANDLE;
        }
    }
    void Control(UCHAR code)
    {
        if (m_Handle)
        {
            SERVICE_STATUS status;
            if (!ControlService(m_Handle, code, &status))
            {
                Log("%s: error %d", __FUNCTION__, GetLastError());
            }
        }
    }

  protected:
    CServiceManager m_ServiceManager;
    SC_HANDLE m_Handle;
    SERVICE_STATUS m_Status;
};

class CEvent
{
  public:
    CEvent(bool Manual)
    {
        m_Handle = CreateEvent(NULL, Manual, false, NULL);
    }
    ~CEvent()
    {
        if (m_Handle)
        {
            CloseHandle(m_Handle);
        }
    }
    ULONG Wait(ULONG Millies = INFINITE)
    {
        return WaitForSingleObject(m_Handle, Millies);
    }
    void Set()
    {
        SetEvent(m_Handle);
    }

  private:
    HANDLE m_Handle;
};

class CMutex
{
  public:
    CMutex()
    {
        m_Handle = CreateMutex(NULL, false, NULL);
    }
    ~CMutex()
    {
        if (m_Handle)
        {
            CloseHandle(m_Handle);
        }
    }
    ULONG Wait(ULONG Millies = INFINITE)
    {
        return WaitForSingleObject(m_Handle, Millies);
    }
    void Release()
    {
        ReleaseMutex(m_Handle);
    }

  private:
    HANDLE m_Handle;
};

class CMutexProtect
{
  public:
    CMutexProtect(CMutex &Mutex) : m_Mutex(Mutex)
    {
        Mutex.Wait();
    }
    ~CMutexProtect()
    {
        m_Mutex.Release();
    }

  private:
    CMutex &m_Mutex;
};

class CServiceImplementation
{
  public:
    CServiceImplementation(LPCTSTR name) : m_Name(name)
    {
        Register(this);
    }
    LPCTSTR ServiceName() const
    {
        return m_Name;
    }
    static bool CheckInMain()
    {
        char *data;
        ULONG done;
        if (WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSSessionId, &data, &done) &&
            done == 4)
        {
            ULONG session = *(ULONG *)data;
            Log("The application runs in session %d", session);
            WTSFreeMemory(data);
            INT_PTR count = m_Registered.GetCount();
            if (session == 0 && count)
            {
                SERVICE_TABLE_ENTRY *table = new SERVICE_TABLE_ENTRY[count + 1];
                table[count].lpServiceName = NULL;
                table[count].lpServiceProc = NULL;
                for (INT_PTR i = 0; i < count; ++i)
                {
                    CServiceImplementation *obj = m_Registered.GetAt(i);
                    table[i].lpServiceName = obj->m_Name.GetBuffer();
                    table[i].lpServiceProc = [](DWORD argc, LPTSTR *argv) {
                        CServiceImplementation *obj = NULL;
                        if (!argc || CServiceImplementation::Registered().GetCount() == 1)
                        {
                            obj = CServiceImplementation::Registered().GetAt(0);
                        }
                        else
                        {
                            obj = FindObject(argv[0]);
                        }
                        if (obj)
                        {
                            obj->ServiceMain(argc, argv);
                        }
                        else
                        {
                            Log("Nothing found to run: %d args", argc);
                        }
                    };
                }
                StartServiceCtrlDispatcher(table);
                delete[] table;
                return true;
            }
        }
        return false;
    }
    bool Install()
    {
        CString s = BinaryPath();
        CServiceManager mgr;
        bool b = mgr.Install(ServiceName(), s);
        return b;
    }
    bool Installed()
    {
        CService service(ServiceName(), true);
        ULONG state;
        return !service.Query(state);
    }
    bool Uninstall()
    {
        CService service(ServiceName());
        return service.Delete();
    }
    static const CAtlArray<CServiceImplementation *> &Registered()
    {
        return m_Registered;
    }
    static CString BinaryPath()
    {
        CString s;
        HMODULE hModule = GetModuleHandle(NULL);
        WCHAR path[MAX_PATH];
        GetModuleFileName(hModule, path, MAX_PATH);
        s = path;
        return s;
    }
    void Control(UCHAR code)
    {
        CService service(ServiceName());
        service.Control(code);
    }

  protected:
    CString m_Name;
    void SetState(ULONG State)
    {
        m_State.Set(State);
    }
    class CServiceState
    {
      public:
        CServiceState() : m_StopEvent(true)
        {
            status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
            status.dwCurrentState = SERVICE_STOPPED;
            status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SESSIONCHANGE;
            status.dwWaitHint = 30000;
            status.dwCheckPoint = 0;
            status.dwWin32ExitCode = NO_ERROR;
            status.dwServiceSpecificExitCode = 0;
        }
        SERVICE_STATUS_HANDLE hService = NULL;
        SERVICE_STATUS status;
        void Set(ULONG state)
        {
            status.dwCurrentState = state;
            Log("%s: %s", __FUNCTION__, CService::StateName(state));
            SetServiceStatus(hService, &status);
            if (state == SERVICE_STOPPED)
            {
                m_StopEvent.Set();
            }
        }
        void Stoppable(bool b)
        {
            if (b)
            {
                status.dwControlsAccepted |= SERVICE_ACCEPT_STOP;
            }
            else
            {
                status.dwControlsAccepted &= ~SERVICE_ACCEPT_STOP;
            }
            Log("%s = %d", __FUNCTION__, b);
            SetServiceStatus(hService, &status);
        }
        bool Stoppable()
        {
            bool b = (status.dwControlsAccepted & SERVICE_ACCEPT_STOP) != 0;
            Log("%s = %d", __FUNCTION__, b);
            return b;
        }
        CEvent m_StopEvent;
    };
    CServiceState m_State;
    void ServiceMain(_In_ ULONG dwArgc, _In_ LPTSTR *lpszArgv)
    {
        // clang-format off
        UNREFERENCED_PARAMETER(dwArgc);
        UNREFERENCED_PARAMETER(lpszArgv);
        m_State.hService = RegisterServiceCtrlHandlerEx(
            m_Name,
            [](DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) -> DWORD
        {
            UNREFERENCED_PARAMETER(lpEventData);
            UNREFERENCED_PARAMETER(dwEventType);
            CServiceImplementation *obj = (CServiceImplementation *)lpContext;
            return obj->ControlHandler(dwControl, dwEventType, lpEventData);
        },
            this);
        if (m_State.hService)
        {
            m_State.Set(SERVICE_START_PENDING);
            if (OnStart())
            {
                m_State.Set(SERVICE_RUNNING);
            }
            m_State.m_StopEvent.Wait();
        }
        else
        {
            Log("Failed to register %S service, error %d", ServiceName(), GetLastError());
        }
        // clang-format on
    }
    virtual DWORD ControlHandler(ULONG dwControl, ULONG dwEventType, LPVOID lpEventData)
    {
        DWORD res = ERROR_CALL_NOT_IMPLEMENTED;
        UNREFERENCED_PARAMETER(dwEventType);
        UNREFERENCED_PARAMETER(lpEventData);
        Log("%s: %s", __FUNCTION__, Name<eServiceControl>(dwControl));
        switch (dwControl)
        {
            case SERVICE_CONTROL_STOP:
                if (!m_State.Stoppable())
                {
                    res = ERROR_CALL_NOT_IMPLEMENTED;
                }
                else if (m_State.status.dwCurrentState == SERVICE_RUNNING)
                {
                    res = NO_ERROR;
                    m_State.Set(SERVICE_STOP_PENDING);
                    if (OnStop())
                    {
                        m_State.Set(SERVICE_STOPPED);
                    }
                }
                else
                {
                    Log("%s: got STOP, current state %s(%d)",
                        __FUNCTION__,
                        CService::StateName(m_State.status.dwCurrentState),
                        m_State.status.dwCurrentState);
                }
                break;
            default:
                break;
        }
        return res;
    }
    virtual bool OnStop()
    {
        // if more processing required, return false
        // then later use m_State.Set(SERVICE_STOPPED)
        return true;
    }
    virtual bool OnStart()
    {
        // if more processing required, return false
        // then later use m_State.Set(SERVICE_RUNNING)
        // or m_State.Set(SERVICE_STOPPED)
        return true;
    }
    static void Register(CServiceImplementation *obj)
    {
        m_Registered.Add(obj);
    }
    static CServiceImplementation *FindObject(LPCTSTR name)
    {
        for (size_t i = 0; i < m_Registered.GetCount(); ++i)
        {
            CServiceImplementation *obj = m_Registered.GetAt(i);
            CString s = name;
            if (!s.CompareNoCase(obj->ServiceName()))
            {
                return obj;
            }
        }
        return NULL;
    }
    static CAtlArray<CServiceImplementation *> m_Registered;
};

class CThreadOwner
{
  public:
    CThreadOwner()
    {
    }
    ~CThreadOwner()
    {
        StopThread(true);
        while (IsThreadRunning())
        {
            Sleep(10);
        }
    }
    bool IsThreadRunning() const
    {
        return m_ThreadHandle != NULL;
    }
    enum tThreadState
    {
        tsNotRunning,
        tsRunning,
        tsAborted,
        tsDeleting
    };
    bool StartThread()
    {
        if (IsThreadRunning())
        {
            return false;
        }
        InterlockedCompareExchange(&m_State, tsRunning, tsNotRunning);
        m_ThreadHandle = (HANDLE)_beginthread(_ThreadProc, 0, this);
        if (IsThreadRunning())
        {
            return true;
        }
        else
        {
            InterlockedCompareExchange(&m_State, tsNotRunning, tsRunning);
            return false;
        }
    }
    virtual tThreadState StopThread(bool bDeleting = false)
    {
        LONG val;
        if (bDeleting)
        {
            val = InterlockedCompareExchange(&m_State, tsDeleting, tsRunning);
        }
        else
        {
            val = InterlockedCompareExchange(&m_State, tsAborted, tsRunning);
        }
        return (tThreadState)val;
    }

  private:
    LONG m_State = tsNotRunning;

  protected:
    tThreadState ThreadState()
    {
        return (tThreadState)m_State;
    }
    HANDLE m_ThreadHandle = NULL;
    virtual void ThreadProc() = 0;
    virtual void ThreadTerminated(tThreadState previous)
    {
        UNREFERENCED_PARAMETER(previous);
        m_ThreadHandle = NULL;
    }
    static void __cdecl _ThreadProc(PVOID param)
    {
        CThreadOwner *pOwner = (CThreadOwner *)param;
        pOwner->ThreadProc();
        LONG val = InterlockedExchange(&pOwner->m_State, tsNotRunning);
        pOwner->ThreadTerminated((tThreadState)val);
    }
};
