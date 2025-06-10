#include "cipc.h"

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <iostream>
#include <signal.h>

#if defined(__APPLE__) || defined(__FreeBSD__)
const char* AppName = getprogname();
#elif defined(_GNU_SOURCE)
const char* AppName = program_invocation_name;
#else
const char* AppName = "?";
#endif

bool IS_IPC_LOGGING = false;
eIPCResult LAST_RESULT_CODE = IPC_RESULT_UNKNOWN;

using namespace nsIPC;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string GetIPCStrError( eIPCResult err )
{
    if( IPC_RESULT_STRING.contains( err ) == true )
        return IPC_RESULT_STRING.value( err );
    else
        return "";
}

void ERR_LOG( const std::string& str, int nErr )
{
    if( IS_IPC_LOGGING == true )
        std::cerr << "[IPC" << str << "result=" << (int)LAST_RESULT_CODE << "|" << GetIPCStrError( LAST_RESULT_CODE ) << "|err=" << nErr << "|" << strerror( nErr ) << std::endl;
}

void setIPCLastError( eIPCResult err )
{
    LAST_RESULT_CODE = err;
}

void cleanupIPC( const std::string& sIPCName, stIPCData* data, int shmfd, sem_t* sem = SEM_FAILED )
{
    if( data != nullptr )
        munmap( data, sizeof( data ) );

    if( shmfd != -1 )
    {
        close( shmfd );

        if( sIPCName.empty() == false )
            shm_unlink( sIPCName.c_str() );
    }

    if( sem != SEM_FAILED )
        sem_close( sem );

    if( sIPCName.empty() == false )
        sem_unlink( sIPCName.c_str() );
}

void* openMmapIPC( int nShmfd, size_t size, bool* isSuccess = nullptr )
{
    void* data = nullptr;

    if( isSuccess != nullptr )
        *isSuccess = false;

    do
    {
        if( nShmfd == -1 )
        {
            setIPCLastError( IPC_RESULT_SHM_OPEN_FAILED );
            ERR_LOG( "[IPC] Not Open Shm", 0 );
            break;
        }

        data = mmap( nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, nShmfd, 0 );
        if( data == MAP_FAILED )
        {
            ERR_LOG( "[IPC] Error mapping shared memory", errno );
            break;
        }

        if( isSuccess != nullptr )
            *isSuccess = true;

    } while( false );

    return data;
}

bool isExistIPC( const std::string& sIPCName )
{
    bool isExist = false;

    do
    {
        if( mapIPC.isEmpty() == true )
            break;

        for( const auto& ipc : mapIPC )
        {
            if( ipc.sIPCName.compare( sIPCName ) == 0 || ipc.sDataIPCName.compare( sIPCName ) == 0 )
            {
                isExist = true;
                break;
            }
        }

    } while( false );

    return isExist;
}

int createShmIPC( const std::string& sIPCName )
{
    int nShmfd = -1;

    do
    {
        if( isExistIPC( sIPCName ) == true )
        {
            ERR_LOG( "[IPC] Already Exist IPC", errno );
            break;
        }

        nShmfd = shm_open( sIPCName.c_str(), O_CREAT | O_RDWR, 0777 );
        if( nShmfd == -1 )
        {
            ERR_LOG( "[IPC] Error Creating Shared Memory", errno );
            break;
        }

        if( ftruncate( nShmfd, sizeof( stIPCData ) ) == -1 )
        {
            if( errno != 22 )
            {
                ERR_LOG( "[IPC] Error setting size of shared memory", errno );
                cleanupIPC( sIPCName, nullptr, nShmfd );
                break;
            }
        }

    } while( false );

    return nShmfd;
}

int openShmIPC( const std::string& sIPCName )
{
    int nShmfd = -1;

    do
    {
        nShmfd = shm_open( sIPCName.c_str(), O_RDWR, 0666 );
        if( nShmfd == -1 )
        {
            ERR_LOG( "[IPC] Error Creating Shared Memory", errno );
            break;
        }

    } while( false );

    return nShmfd;
}

sem_t* createSemIPC( const std::string& sIPCName )
{
    sem_unlink( sIPCName.c_str() );

    sem_t* sem = sem_open( sIPCName.c_str(), O_CREAT, 0666, 1 );
    if ( sem == SEM_FAILED )
    {
        ERR_LOG( "[IPC] SEM Open Error", errno );
        cleanupIPC( sIPCName, nullptr, -1 );
    }

    return sem;
}

sem_t* openSemIPC( const std::string& sIPCName )
{
    sem_t* sem = sem_open( sIPCName.c_str(), 0 );
    if ( sem == SEM_FAILED )
    {
        ERR_LOG( "[IPC] SEM Open Error", errno );
        cleanupIPC( sIPCName, nullptr, -1 );
    }

    return sem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CIPCSem
{
public:
    CIPCSem( const std::string& sIPCName, bool isDeleteOnClose = false ) { _sIPCName = sIPCName; _isDeleteOnClose = isDeleteOnClose; }

    ~CIPCSem()
    {
        if( _isDeleteOnClose == true )
            Destroy();
        else
            Close();
    }

    sem_t* Create()
    {
        _sem = createSemIPC( _sIPCName );
        return _sem;
    }

    sem_t* Open()
    {
        _sem = openSemIPC( _sIPCName );
        return _sem;
    }

    void Close()
    {
        if( _sem != SEM_FAILED )
            sem_close( _sem );
    }

    void Destroy()
    {
        Close();

        if( _sIPCName.empty() == false )
            sem_unlink( _sIPCName.c_str() );
    }

    sem_t* Get() { return _sem; }
    bool IsVaild() { return _sem != SEM_FAILED; }

private:
    std::string _sIPCName;
    sem_t*      _sem             = SEM_FAILED;
    bool        _isDeleteOnClose = false;
};

class CIPCShm
{
public:
    CIPCShm( const std::string& sIPCName, bool isDeleteOnClose = false ) { _sIPCName = sIPCName; _isDeleteOnClose = isDeleteOnClose; }

    ~CIPCShm()
    {
        if( _isDeleteOnClose == true )
            Destroy();
        else
            Close();
    }

    int Create()
    {
        _shmfd = createShmIPC( _sIPCName );
        return _shmfd;
    }

    int Open()
    {
        _shmfd = openShmIPC( _sIPCName );
        return _shmfd;
    }

    void Close()
    {
        if( _shmfd != -1 )
            close( _shmfd );
    }

    void Destroy()
    {
        Close();

        if( _sIPCName.empty() == false )
            shm_unlink( _sIPCName.c_str() );

        _shmfd = -1;
    }

    int Get() { return _shmfd; }
    bool IsVaild() { return _shmfd != -1; }

private:
    std::string _sIPCName;
    int         _shmfd           = -1;
    bool        _isDeleteOnClose = false;
};

class CIPCMmap
{
public:
    CIPCMmap( int nShmfd ) { _shmfd = nShmfd; }

    ~CIPCMmap()
    {
        Close();
    }

    void* Open( size_t size )
    {
        _data = openMmapIPC( _shmfd, size );
        return _data;
    }

    void Close()
    {
        if( _data != nullptr )
            munmap( _data, sizeof( _data ) );
    }

    void* Get() { return _data; }
    bool IsVaild() { return _data != nullptr; }

private:
    void*       _data        = nullptr;
    int         _shmfd       = -1;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string getClientIPCName()
{
    return "/iMonSHM" + std::to_string( getpid() ) + AppName;
}

std::string getDataIPCName( const std::string& sIPCName )
{
    return sIPCName + "DATA";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CIPCServer::CIPCServer( stIPCInfo& info )
{
    _info = info;
    _thWorker = std::thread( [this](){ main(); } );
}

CIPCServer::~CIPCServer()
{
    _isStop = true;

    _queue.Stop();

    if( _thWorker.joinable() == true )
        _thWorker.join();

    for( auto& th : _vecThread )
    {
        if( th->joinable() == true )
            th->join();
    }
}

void CIPCServer::main()
{
    for( int idx = 1; idx < _info.nMaxThreadCount; idx++ )
        _vecThread.push_back( std::make_shared<std::thread>( std::thread( [ this ](){ worker(); } ) ) );

    CIPCShm shm( _info.sIPCName );
    CIPCSem sem( _info.sIPCName );

    if( shm.Open() == -1 )
        return;

    if( sem.Open() == SEM_FAILED )
        return;

    CIPCMmap mmap( shm.Get() );

    if( mmap.Open( sizeof( stIPCData ) ) == nullptr )
        return;

    while( _isStop == false )
    {
        usleep( 10 * 1000 );

        SemaphoreGuard guard( sem.Get() );
        stIPCData* data = (stIPCData*) mmap.Get();
        if( data->hasRequest == true )
        {
            if( kill( data->nSenderPID, 0 ) != 0 )
            {
                data->hasRequest = false;
                data->hasResponse = false;
                data->nSenderPID = 0;
                data->needResponse = false;
                continue;
            }

            //std::cout << "[IPC] reqData size = " << sizeof( data->reqData ) << std::endl;
            //std::cout << "[IPC] struct size = " << sizeof( _info ) << std::endl;
            //std::cout << "[IPC] TEST = pid=" << data->nSenderPID << std::endl;

            stIPCData qData = *data;
            _queue.EnQueue( qData );

            data->hasRequest = false;
            data->hasResponse = false;
            data->nSenderPID = 0;
            data->needResponse = false;
        }
    }
}

void CIPCServer::worker()
{
    while( _isStop == false )
    {
        auto data = _queue.DeQueue();

        if( _isStop == true )
            break;

        if( data.hasRequest == false )
            continue;

        bool isUsingMmap = false;

        if( data.requestSize > sizeof( data.reqData ) || data.nCheckIPC == 1 )
            isUsingMmap = true;

        void* reqData = nullptr;
        if( isUsingMmap == true )
        {
            CIPCShm serverDataShm( _info.sDataIPCName );
            CIPCSem serverDataSem( _info.sDataIPCName );

            if( serverDataShm.Open() == -1 )
                continue;

            if( serverDataSem.Open() == SEM_FAILED )
                continue;

            SemaphoreGuard serverDataGuard( serverDataSem.Get() );
            CIPCMmap serverMmap( serverDataShm.Get() );
            void* serverData = serverMmap.Open( sizeof( data.requestSize ) );
            reqData = malloc( data.requestSize );
            memcpy( reqData, serverData, data.requestSize );
            //reqData = serverData;
        }
        else
        {
            reqData = (void*)&data.reqData;
        }

        void* resData = nullptr;

        if( _info.fnCallback != nullptr && data.nCheckIPC == 0 )
            _info.fnCallback( _info.sIPCName, reqData, data.requestSize, resData, data.responseSize, _info.pContext );

        if( isUsingMmap == true )
        {
            if( reqData != nullptr )
                free( reqData );
        }

        if( data.nCheckIPC != 0 )
        {
            quint64 quTestMsg = 0xF1F2F3F4F5F6F7F8;
            resData = (void*)&quTestMsg;
            data.responseSize = sizeof( quint64 );
        }

        if( data.needResponse == false && data.nCheckIPC == 0 )
            continue;

        if( data.responseSize != 0 && resData == nullptr )
            continue;

        CIPCShm clientShm( data.sSenderIPCName );
        CIPCSem clientSem( data.sSenderIPCName );

        if( clientShm.Open() == -1 )
            continue;

        if( clientSem.Open() == SEM_FAILED )
            continue;

        CIPCMmap clientMmap( clientShm.Get() );
        {
            SemaphoreGuard guard( clientSem.Get() );
            if( clientMmap.Open( sizeof( stIPCData ) ) == nullptr )
                continue;

            stIPCData* client = (stIPCData*)clientMmap.Get();

            client->hasResponse = true;
            client->hasRequest = false;
            client->nSenderPID = getpid();
            client->sSenderIPCName = _info.sIPCName;
            client->isInit = true;
            client->responseSize = data.responseSize;

            if( data.responseSize > sizeof( client->resData ) || data.nCheckIPC == 1 )
            {
                CIPCShm clientDataShm( getDataIPCName( data.sSenderIPCName ) );
                CIPCSem clientDataSem( getDataIPCName( data.sSenderIPCName ) );

                if( clientDataShm.Open() == -1 )
                    continue;

                if( clientDataSem.Open() == SEM_FAILED )
                    continue;

                SemaphoreGuard clientDataGuard( clientDataSem.Get() );
                CIPCMmap clientMmap( clientDataShm.Get() );
                void* clientData = clientMmap.Open( sizeof( data.responseSize ) );
                memcpy( clientData, resData, data.responseSize );
            }
            else
            {
                memcpy( client->resData, resData, data.responseSize );
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SetLoggingIPC( bool isLogging )
{
    IS_IPC_LOGGING = isLogging;
}

bool CreateIPC( const std::string& sIPCName, fnCallback fnCallback, void *pContext, int nMaxThreadCount /*= IPC_DEFAULT_MAX_THREAD_COUNT*/, int timeOutSec /*= IPC_DEFAULT_TIMEOUT*/ )
{
    bool isSuccess = false;
    stIPCShm shm;
    stIPCData* ipcData = nullptr;

    do
    {
        CIPCShm shmfd( sIPCName );
        shmfd.Create();

        if( shmfd.Get() == -1 )
            break;

        ipcData = (stIPCData*) openMmapIPC( shmfd.Get(), sizeof( stIPCData ), &isSuccess );

        shm.sDataIPCName = getDataIPCName( sIPCName );

        CIPCShm dataShmfd( shm.sDataIPCName );
        dataShmfd.Create();

        if( dataShmfd.Get() == -1 )
            break;

        CIPCSem sem( sIPCName );

        if( sem.Create() == SEM_FAILED )
            break;

        CIPCSem semData( shm.sDataIPCName );

        if( semData.Create() == SEM_FAILED )
            break;

        semData.Close();

        {
            SemaphoreGuard guard( sem.Get() );

            ipcData->isInit = true;

            stIPCInfo info;
            info.sIPCName        = sIPCName;
            info.sDataIPCName    = shm.sDataIPCName;
            info.fnCallback      = fnCallback;
            info.nMaxThreadCount = nMaxThreadCount;
            info.pContext        = pContext;
            info.nTimeoutSec     = timeOutSec;

            shm.sIPCName         = sIPCName;
            shm.spIPCServer      = std::make_shared< CIPCServer >( info );

            mapIPC.insert( sIPCName, shm );
        }

        isSuccess = true;

    } while( false );

    return isSuccess;
}

bool DestroyIPC( const std::string& sIPCName )
{
    bool isSuccess = false;

    do
    {
        if( mapIPC.contains( sIPCName ) == false )
            break;

        stIPCShm shm = mapIPC[ sIPCName ];

        CIPCShm shmfd( sIPCName );
        CIPCSem sem( sIPCName );

        cleanupIPC( sIPCName, nullptr, shmfd.Open(), sem.Open() );

        CIPCShm dataShmfd( shm.sDataIPCName );
        CIPCSem dataSem( shm.sDataIPCName );

        cleanupIPC( shm.sDataIPCName, nullptr, dataShmfd.Open(), dataSem.Open() );

        mapIPC.remove( sIPCName );

        isSuccess = true;

    } while( false );

    return isSuccess;
}

bool SendIPC( const std::string& sIPCName, void *requestData, size_t requestSize, void *responseData, size_t& responseSize, int timeOutSec /*= IPC_DEFAULT_TIMEOUT*/, int nCheckIPC /*= 0*/ )
{
    bool isSuccess = false;
    int nCheckMs = 0;

    do
    {
        CIPCShm  clientShm( getClientIPCName(), true );
        CIPCShm  clientDataShm( getDataIPCName( getClientIPCName() ), true );
        CIPCSem  clientSem( getClientIPCName(), true );
        CIPCSem  clientDataSem( getDataIPCName( getClientIPCName() ), true );

        if( clientShm.Create() == -1 )
            break;

        if( clientDataShm.Create() == -1 )
            break;

        if( clientSem.Create() == SEM_FAILED )
            break;

        if( clientDataSem.Create() == SEM_FAILED )
            break;

        bool isSendSuccess = false;
        {
            CIPCShm serverShm( sIPCName );
            CIPCSem serverSem( sIPCName );

            if( serverShm.Open() == -1 )
                break;

            if( serverSem.Open() == SEM_FAILED )
                break;

            while( nCheckMs < timeOutSec * 100 )
            {
                do
                {
                    SemaphoreGuard guard( serverSem.Get() );

                    CIPCMmap serverMmap( serverShm.Get() );
                    stIPCData* server = (stIPCData*) serverMmap.Open( sizeof( stIPCData ) );

                    if( server->isInit == false || server->hasRequest == true )
                        break;

                    bool isUsingMmap = false;

                    if( requestSize > sizeof( server->reqData ) )
                        isUsingMmap = true;

                    server->hasResponse     = false;
                    server->nSenderPID      = getpid();
                    server->requestSize     = requestSize;
                    server->sSenderIPCName  = getClientIPCName();
                    server->hasRequest      = true;
                    server->needResponse    = true;
                    server->timeout         = std::time( nullptr ) + timeOutSec;
                    server->nCheckIPC       = nCheckIPC;

                    if( isUsingMmap == true || nCheckIPC == 1 )
                    {
                        CIPCShm serverDataShm( getDataIPCName( sIPCName ) );

                        if( serverDataShm.Open() == -1 )
                            break;

                        CIPCSem serverDataSem( getDataIPCName( sIPCName ) );

                        if( serverDataSem.Open() == SEM_FAILED )
                            break;

                        SemaphoreGuard dataGuard( serverDataSem.Get() );

                        CIPCMmap serverDataMmap( serverDataShm.Get() );

                        if( serverDataMmap.Open( requestSize ) == nullptr )
                            break;

                        memcpy( serverDataMmap.Get(), requestData, requestSize );
                    }
                    else
                    {
                        memcpy( server->reqData, requestData, requestSize );
                    }

                    isSendSuccess = true;

                } while( false );

                if( isSendSuccess == true )
                    break;

                usleep( 10 * 1000 );
                nCheckMs++;
            }
        }

        if( isSendSuccess == false )
            break;

        bool isRecvSuccess = false;

        while( nCheckMs < timeOutSec * 100 )
        {
            do
            {
                SemaphoreGuard guard( clientSem.Get() );

                CIPCMmap clientMmap( clientShm.Get() );
                stIPCData* client = (stIPCData*) clientMmap.Open( sizeof( stIPCData ) );
                if( client->hasRequest == false && client->hasResponse == true )
                {
                    bool isUsingMmap = false;

                    if( client->responseSize > sizeof( client->resData ) )
                        isUsingMmap = true;

                    responseSize = client->responseSize;

                    if( isUsingMmap == true || nCheckIPC == 1 )
                    {
                        CIPCMmap clientDataMmap( clientDataShm.Get() );

                        SemaphoreGuard guard( clientDataSem.Get() );

                        if( clientDataMmap.Open( client->responseSize ) == nullptr )
                            break;

                        memcpy( responseData, clientDataMmap.Get(), client->responseSize );
                    }
                    else
                    {
                        memcpy( responseData, client->resData, client->responseSize );
                    }

                    isRecvSuccess = true;
                }

            } while( false );

            if( isRecvSuccess == true )
                break;

            usleep( 10 * 1000 );
            nCheckMs++;
        }

        if( isSendSuccess == true && isRecvSuccess == true )
            isSuccess = true;

    } while( false );

    return isSuccess;
}

bool CheckIPC( const std::string& sIPCName, int nCheckIPC )
{
    quint64 quTestMsg = 0xF1F2F3F4F5F6F7F8;
    quint64 quAnswer = 0;
    size_t quAnswerSize = sizeof( quAnswer );

    if( SendIPC( sIPCName, (void*)&quTestMsg, sizeof( quTestMsg ), (void*)&quAnswer, quAnswerSize, IPC_DEFAULT_TIMEOUT, nCheckIPC ) == false )
        return false;

    if( quTestMsg != quAnswer )
        return false;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

