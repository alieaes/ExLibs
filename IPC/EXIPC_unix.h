/*
    ===========================================================
       cipc.h
    ===========================================================
       version : 1.0.0
    ===========================================================
       date    : 2025-02-14
    ===========================================================
*/
#ifndef CIPC_H
#define CIPC_H

#include <string>
#include <sys/types.h>
#include <thread>
#include <QMap>
#include <semaphore.h>
#include <QVector>
#include <QQueue>
#include <QHash>

namespace nsIPC
{
    class SemaphoreGuard
    {
    public:
        SemaphoreGuard( sem_t* s ) : sem( s ) { sem_wait( sem ); }
        ~SemaphoreGuard()                     { sem_post( sem ); }
    private:
        sem_t*                     sem;
    };

    template< typename T >
    class eventQueue
    {
    public:
        eventQueue() {};
        ~eventQueue()
        {
            Stop();
        };

        T DeQueue()
        {
            std::unique_lock< std::mutex > lck( _lck );

            while( _queue.empty() == true )
                _cvQueue.wait( lck );

            if( _isStop == true )
                return T();

            T ret = _queue.front();
            _queue.pop_front();

            return ret;
        }

        void EnQueue( const T& item )
        {
            std::unique_lock< std::mutex > lck( _lck );

            _queue.push_back( item );
            _cvQueue.notify_one();
        }

        void Stop()
        {
            _isStop = true;
            _cvQueue.notify_all();
        }

        int Size()
        {
            return _queue.size();
        }

    private:
        std::condition_variable                  _cvQueue;
        QQueue< T >                              _queue;
        std::mutex                               _lck;

        bool                                     _isStop = false;

    };
}

struct stIPCData
{
    unsigned int     reqData[ 128 ];
    size_t           requestSize   = 0;
    bool             hasRequest    = false;

    unsigned int     resData[ 128 ];
    size_t           responseSize  = 0;
    bool             hasResponse   = false;
    bool             needResponse  = false;
    std::string      sSenderIPCName;

    pid_t            nSenderPID    = 0;
    int64_t          timeout       = 0;

    bool             isInit        = false;

    int              nCheckIPC     = 0;

    void clear()
    {
        isInit = true;
    }
};

typedef std::unique_ptr< stIPCData > spIPCData;
typedef std::function< void( const std::string sIPCName, void* requestData, size_t& requestSize, void* responseData, size_t& responseSize, void* pContext ) > fnCallback;
typedef std::shared_ptr< std::thread > spIPCThread;

const int IPC_DEFAULT_MAX_THREAD_COUNT = 10;
const int IPC_DEFAULT_TIMEOUT          = 10;

struct stIPCInfo
{
    std::string      sIPCName;
    std::string      sDataIPCName;
    fnCallback       fnCallback;
    void*            pContext;
    int              nMaxThreadCount  = IPC_DEFAULT_MAX_THREAD_COUNT;
    int              nTimeoutSec      = IPC_DEFAULT_TIMEOUT;
};

class CIPCServer
{
public:
    CIPCServer( stIPCInfo& info );
    ~CIPCServer();

private:
    void                            main();
    void                            worker();

    QVector< spIPCThread >          _vecThread;
    nsIPC::eventQueue< stIPCData >  _queue;
    bool                            _isStop    = false;
    std::thread                     _thWorker;
    stIPCInfo                       _info;
};

typedef std::shared_ptr< CIPCServer > spIPCServer;

struct stIPCShm
{
    std::string      sIPCName;
    std::string      sDataIPCName;
    spIPCServer      spIPCServer;
};

enum eIPCResult
{
    IPC_RESULT_UNKNOWN                 = 0,

    // SUCCESS
    IPC_RESULT_SUCCESS                 = 1,

    // CIPC SERVER FAILED
    IPC_RESULT_FAILED_IPC_NAME_EXIST   = 10,

    // CIPC SERVER SKIP
    IPC_RESULT_SKIP_TERMINATE_CLIENT   = 21,
    IPC_RESULT_SKIP_NOT_REQUEST        = 22,
    IPC_RESULT_SKIP_EMPTY_RESPONSE     = 23,

    // CIPC CLIENT FAILED
    IPC_RESULT_FAILED_TIMEOUT          = 30,
    IPC_RESULT_FAILED_SERVER_NOT_OPEN  = 31,

    // SHM FAILED
    IPC_RESULT_SHM_CREATE_FAILED       = 40,
    IPC_RESULT_TRUNCATE_FAILED         = 41,
    IPC_RESULT_SHM_OPEN_FAILED         = 42,

    // SEM FAILED
    IPC_RESULT_SEM_CREATE_FAILED       = 51,
    IPC_RESULT_SEM_OPEN_FAILED         = 52,

    // MMAP FAILED
    IPC_RESULT_MMAP_OPEN_FAILED        = 61
};

static const QHash< eIPCResult, const char*> IPC_RESULT_STRING = {
    {IPC_RESULT_UNKNOWN,                "Unknown error"},

    // SUCCESS
    {IPC_RESULT_SUCCESS,                "Success"},

    // CIPC SERVER FAILED
    {IPC_RESULT_FAILED_IPC_NAME_EXIST,  "IPC name already exists"},

    // CIPC SERVER SKIP
    {IPC_RESULT_SKIP_TERMINATE_CLIENT,  "Skip: Client process terminated"},
    {IPC_RESULT_SKIP_NOT_REQUEST,       "Skip: No request available"},
    {IPC_RESULT_SKIP_EMPTY_RESPONSE,    "Skip: Empty response"},

    // CIPC CLIENT FAILED
    {IPC_RESULT_FAILED_TIMEOUT,         "Operation timed out"},
    {IPC_RESULT_FAILED_SERVER_NOT_OPEN, "Server is not open"},

    // SHM FAILED
    {IPC_RESULT_SHM_CREATE_FAILED,      "Failed to create shared memory"},
    {IPC_RESULT_TRUNCATE_FAILED,        "Failed to truncate shared memory"},
    {IPC_RESULT_SHM_OPEN_FAILED,        "Failed to open shared memory"},

    // SEM FAILED
    {IPC_RESULT_SEM_CREATE_FAILED,      "Failed to create semaphore"},
    {IPC_RESULT_SEM_OPEN_FAILED,        "Failed to open semaphore"},

    // MMAP FAILED
    {IPC_RESULT_MMAP_OPEN_FAILED,       "Failed to open memory mapping"}
};

static QMap< std::string, stIPCShm > mapIPC;

eIPCResult                       GetIPCLastError();
std::string                      GetIPCStrError( eIPCResult err );
void                             SetLoggingIPC( bool isLogging );
bool                             CreateIPC( const std::string& sIPCName, fnCallback fnCallback, void* pContext, int nMaxThreadCount = IPC_DEFAULT_MAX_THREAD_COUNT, int timeOutSec = IPC_DEFAULT_TIMEOUT );
bool                             DestroyIPC( const std::string& sIPCName );
bool                             SendIPC( const std::string& sIPCName, void* requestData, size_t requestSize, void* responseData, size_t& responseSize, int timeOutSec = IPC_DEFAULT_TIMEOUT, int nCheckIPC = 0 );
bool                             CheckIPC( const std::string& sIPCName, int nCheckIPC );

#endif // CIPC_H
