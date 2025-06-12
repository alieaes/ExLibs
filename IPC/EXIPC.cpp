#include "stdafx.h"
#include "EXIPC.hpp"

#include "EXProcess.hpp"

///////////////////////////////////////////////////////////////////////////

constexpr size_t SHM_SIZE = sizeof( Ext::IPC::stIPCData );

Ext::IPC::CIPCMutex::CIPCMutex( std::wstring sMutexName, int64_t nTimeout /*= 0*/ )
{
    _sMutexName = sMutexName;

    if( nTimeout <= 0 )
        nTimeout = INFINITE;

    _nTimeout = nTimeout;
}

Ext::IPC::CIPCMutex::~CIPCMutex()
{
    Release();
}

bool Ext::IPC::CIPCMutex::Create()
{
    _hMutex = CreateMutex( NULL, FALSE, _sMutexName.c_str() );

    if( ::GetLastError() == ERROR_ALREADY_EXISTS )
        _hMutex = OpenMutex( SYNCHRONIZE, FALSE, _sMutexName.c_str() );

    if( _hMutex == nullptr )
        return false;
    else
    {
        Release();
        return true;
    }
}

bool Ext::IPC::CIPCMutex::Open()
{
    bool isSuccess = false;
    _hMutex = OpenMutex( SYNCHRONIZE, FALSE, _sMutexName.c_str() );

    DWORD dwRet = 0;

    if( _hMutex )
    {
        dwRet = WaitForSingleObject( _hMutex, _nTimeout * 1000 );
        if( dwRet == WAIT_OBJECT_0 )
            isSuccess = true;
    }
    
    return isSuccess;
}

void Ext::IPC::CIPCMutex::Release()
{
    if( _hMutex != nullptr )
        ReleaseMutex( _hMutex );
}

void Ext::IPC::CIPCMutex::Close()
{
    if( _hMutex != nullptr )
        CloseHandle( _hMutex );
}

/////////////////////////////////////////////////////////////////////////////////////////////////
///
/////////////////////////////////////////////////////////////////////////////////////////////////

Ext::IPC::CIPCServer::CIPCServer( stIPCInfo& info )
{
    _info = info;
    _thWorker = std::thread( [ this ]() { main(); } );
}

Ext::IPC::CIPCServer::~CIPCServer()
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

    CloseMutex();

    if( _info.hMapping != nullptr )
        CloseHandle( _info.hMapping );
}

void Ext::IPC::CIPCServer::CloseMutex()
{
    if( _isStop == false )
        return;

    if( _info.hMutex != nullptr )
    {
        ReleaseMutex( _info.hMutex );
        CloseHandle( _info.hMutex );
        _info.hMutex = nullptr;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
///
/////////////////////////////////////////////////////////////////////////////////////////////////

void Ext::IPC::CIPCServer::main()
{
    for( int idx = 1; idx < _info.nMaxThreadCount; idx++ )
        _vecThread.push_back( std::make_shared<std::thread>( std::thread( [ this ]() { worker(); } ) ) );

    HANDLE hMapFile = nullptr;

    while( _isStop == false )
    {
        Sleep( 10 );

        if( _info.hMutex )
            WaitForSingleObject( _info.hMutex, INFINITE );

        // NOTE : 종료해야하는지?
        if( hMapFile == nullptr )
            hMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, GetGlobalName( _info.sIPCName ).c_str() );

        void* pBuf = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );

        if( pBuf == nullptr )
        {
            if( hMapFile != nullptr )
            {
                CloseHandle( hMapFile );
                hMapFile = nullptr;
            }

            if( _info.hMutex != nullptr )
                ReleaseMutex( _info.hMutex );

            continue;
        }

        void* reqBuffer = nullptr;
        size_t reqSize = 0;

        auto* pIPCData = reinterpret_cast< stIPCData* >( pBuf );

        if( pIPCData->hasRequest == true )
        {
            if( Process::IsRunningProcess( pIPCData->nSenderPID ) == false )
            {
                pIPCData->clear();

                if( _info.hMutex != nullptr )
                    ReleaseMutex( _info.hMutex );

                if( pBuf != nullptr )
                    UnmapViewOfFile( pBuf );

                continue;
            }

            stIPCData qData = *pIPCData;
            _queue.EnQueue( qData );

            pIPCData->clear();
        }

        if( pBuf != nullptr )
            UnmapViewOfFile( pBuf );

        if( hMapFile != nullptr )
        {
            CloseHandle( hMapFile );
            hMapFile = nullptr;
        }

        if( _info.hMutex != nullptr )
            ReleaseMutex( _info.hMutex );
    }
}

void Ext::IPC::CIPCServer::worker()
{
    while( _isStop == false )
    {
        auto data = _queue.DeQueue();
        HANDLE hMapDataFile = nullptr;

        if( _isStop == true )
            break;

        if( data.hasRequest == false )
            continue;

        bool isUsingMmap = false;

        if( data.requestSize > sizeof( data.reqData ) || data.bCheckIPC == true )
            isUsingMmap = true;

        void* reqData = nullptr;

        if( isUsingMmap == true )
        {
            CIPCMutex reqMutex( GetRequestIPCName( data.sSenderIPCName ), data.timeout );

            if( reqMutex.Open() == true )
            {
                HANDLE hMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, GetGlobalName( GetRequestIPCName( data.sSenderIPCName ) ).c_str() );

                void* pBuf = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );

                if ( pBuf != nullptr )
                {
                    reqData = malloc( data.requestSize );
                    memcpy( reqData, pBuf, data.requestSize );
                }

                if( pBuf != nullptr )
                    UnmapViewOfFile( pBuf );
            }
            else
            {
                // ERROR
            }
        }
        else
        {
            reqData = ( void* ) &data.reqData;
        }

        void* resData = nullptr;

        if( _info.fnCallback != nullptr && data.bCheckIPC == false )
            _info.fnCallback( _info.sIPCName, reqData, data.requestSize, resData, data.responseSize, _info.pContext );

        if( isUsingMmap == true )
        {
            if( reqData != nullptr )
                free( reqData );
        }

        if( data.bCheckIPC == true )
        {
            unsigned long long quTestMsg = 0xF1F2F3F4F5F6F7F8;
            resData = ( void* ) &quTestMsg;
            data.responseSize = sizeof( unsigned long long );
        }

        if( data.needResponse == false && data.bCheckIPC == false )
            continue;

        if( data.responseSize == 0 && resData == nullptr )
            continue;

        CIPCMutex resMutex( GetResponseIPCName( data.sSenderIPCName ), data.timeout );

        if( resMutex.Create() == false )
            continue;

        {
            CIPCMutex reqMutex( data.sSenderIPCName, data.timeout );

            if( reqMutex.Open() == true )
            {
                HANDLE hMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, GetGlobalName( data.sSenderIPCName ).c_str() );
                void* pBuf = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );

                if( pBuf == nullptr )
                    break;

                auto* pIPCData = reinterpret_cast< stIPCData* >( pBuf );

                pIPCData->hasResponse = true;
                pIPCData->hasRequest = false;
                pIPCData->nSenderPID = GetCurrentProcessId();

                //if( _info.sIPCName.size() < 256 )
                std::copy( _info.sIPCName.begin(), _info.sIPCName.end(), std::begin( pIPCData->sSenderIPCName ) );

                pIPCData->isInit = true;
                pIPCData->responseSize = data.responseSize;

                if( data.responseSize > sizeof( pIPCData->resData ) || data.bCheckIPC == true )
                {
                    // 데드락 조심
                    if( resMutex.Open() == true )
                    {
                        hMapDataFile = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, static_cast< DWORD >( SHM_SIZE ), GetGlobalName( GetResponseIPCName( data.sSenderIPCName ) ).c_str() );

                        if( hMapDataFile != nullptr )
                        {
                            void* pDataBuf = MapViewOfFile( hMapDataFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );

                            if( pDataBuf != nullptr )
                                memcpy( pDataBuf, resData, data.responseSize );

                            if( pDataBuf != nullptr )
                                UnmapViewOfFile( pDataBuf );
                        }
                    }

                    resMutex.Release();
                }
                else
                {
                    memcpy( pIPCData->resData, resData, data.responseSize );
                }

                if( pBuf != nullptr )
                    UnmapViewOfFile( pBuf );

                if( _info.hMutex != nullptr )
                    ReleaseMutex( _info.hMutex );
            }
            else
            {
                // ERROR
            }
        }

        {
            int nCheckTime = 0;

            while ( nCheckTime < data.timeout )
            {
                HANDLE hMapFile = OpenFileMapping( FILE_MAP_READ, FALSE, GetGlobalName( data.sSenderIPCName ).c_str() );

                if( hMapFile == nullptr )
                    break;

                void* pBuf = MapViewOfFile( hMapFile, FILE_MAP_READ, 0, 0, SHM_SIZE );

                if( pBuf == nullptr )
                {
                    if( hMapFile != nullptr )
                        CloseHandle( hMapFile );

                    break;
                }

                auto* pIPCData = reinterpret_cast< stIPCData* >( pBuf );

                if( pIPCData->isDone == true )
                {
                    if( hMapFile != nullptr )
                        CloseHandle( hMapFile );

                    if( pBuf != nullptr )
                        UnmapViewOfFile( pBuf );

                    break;
                }

                if( hMapFile != nullptr )
                    CloseHandle( hMapFile );

                if( pBuf != nullptr )
                    UnmapViewOfFile( pBuf );
            }

            if( hMapDataFile != nullptr )
                CloseHandle( hMapDataFile );
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////
///
/////////////////////////////////////////////////////////////////////////////////////////////////

std::wstring Ext::IPC::GetRequestIPCName( const std::wstring& sIPCName )
{
    return sIPCName + L"ReqDATA";
}

std::wstring Ext::IPC::GetResponseIPCName( const std::wstring& sIPCName )
{
    return sIPCName + L"ResDATA";
}

std::wstring Ext::IPC::GetGlobalName( const std::wstring& sIPCName )
{
    return L"Global\\" + sIPCName;
}

bool Ext::IPC::CreateIPC( const std::wstring& sIPCName, const fnCallback& fnCallback, void* pContext, int nMaxThreadCount /*= IPC_DEFAULT_MAX_THREAD_COUNT*/, int timeOutSec /*= IPC_DEFAULT_TIMEOUT*/ )
{
    bool isSuccess = false;

    stIPCShm shm;
    stIPCInfo info;

    info.hMapping = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, static_cast< DWORD >( SHM_SIZE ), GetGlobalName( sIPCName ).c_str() );

    void* pBuf = nullptr;

    do
    {
        if( info.hMapping == nullptr )
            break;

        pBuf = MapViewOfFile( info.hMapping, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );

        if( pBuf == nullptr )
        {
            CloseHandle( info.hMapping );
            break;
        }

        auto* pIPCData = reinterpret_cast< stIPCData* >( pBuf );
        pIPCData->clear();

        HANDLE hMutex = CreateMutex( NULL, TRUE, sIPCName.c_str() );

        if( ::GetLastError() == ERROR_ALREADY_EXISTS )
            hMutex = OpenMutex( SYNCHRONIZE, FALSE, sIPCName.c_str() );

        if( hMutex == nullptr )
            break;

        pIPCData->isInit = true;

        info.sIPCName = sIPCName;
        info.fnCallback = fnCallback;
        info.nMaxThreadCount = nMaxThreadCount;
        info.pContext = pContext;
        info.nTimeoutSec = timeOutSec;
        info.hMutex = hMutex;

        shm.sIPCName = sIPCName;
        shm.spIPCServer = std::make_shared< CIPCServer >( info );

        if( hMutex != nullptr )
            ReleaseMutex( hMutex );

        mapIPC[ sIPCName ] = shm;

        isSuccess = true;

    } while( false );

    if( pBuf != nullptr )
        UnmapViewOfFile( pBuf );

    return isSuccess;
}

bool Ext::IPC::SendIPC( const std::wstring& sIPCName, void* requestData, size_t requestSize, void* responseData, size_t& responseSize, int timeOutSec, bool bCheckIPC )
{
    bool isSuccess = false;
    std::wstring sIPCNameFinal = L"Global\\" + sIPCName;
    std::wstring sIPCClientName = sIPCName + std::to_wstring( _getpid() ) + L"Client" + std::to_wstring( ++uIPC );
    std::wstring sReqIPCName = GetRequestIPCName( sIPCClientName );
    std::wstring sResIPCName = GetResponseIPCName( sIPCClientName );

    int nCheckMs = 0;
    HANDLE hClientMapFile = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, static_cast< DWORD >( SHM_SIZE ), GetGlobalName( sIPCClientName ).c_str() );
    HANDLE hReqMapFile = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, static_cast< DWORD >( requestSize ), GetGlobalName( GetRequestIPCName( sIPCClientName ) ).c_str() );

    CIPCMutex reqMutex( GetRequestIPCName( sIPCClientName ), timeOutSec );
    CIPCMutex reqClientMutex( sIPCClientName, timeOutSec );

    do
    {
        if( reqMutex.Create() == false )
            break;

        if( reqClientMutex.Create() == false )
            break;

        if( hClientMapFile == nullptr )
            break;

        bool isSendSuccess = false;

        while( nCheckMs < timeOutSec * 100 )
        {
            do
            {
                CIPCMutex ServerMutex( sIPCName, timeOutSec );

                if( ServerMutex.Open() == false )
                    break;

                HANDLE hMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, sIPCNameFinal.c_str() );

                if( hMapFile == nullptr )
                    break;

                void* pBuf = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );

                if( pBuf == nullptr )
                {
                    if( hMapFile != nullptr )
                        CloseHandle( hMapFile );

                    break;
                }

                auto* pServerData = reinterpret_cast< stIPCData* >( pBuf );

                if( pServerData->isInit == false || pServerData->hasRequest == true )
                {
                    if( pBuf != nullptr )
                        UnmapViewOfFile( pBuf );

                    if( hMapFile != nullptr )
                        CloseHandle( hMapFile );

                    break;
                }

                bool isUsingMmap = false;

                if( requestSize > sizeof( pServerData->reqData ) )
                    isUsingMmap = true;

                pServerData->hasResponse = false;

                pServerData->nSenderPID = GetCurrentProcessId();
                pServerData->requestSize = requestSize;
                std::copy( sIPCClientName.begin(), sIPCClientName.end(), std::begin( pServerData->sSenderIPCName ) );
                pServerData->hasRequest = true;
                pServerData->needResponse = true;
                pServerData->timeout = std::time( nullptr ) + timeOutSec;
                pServerData->bCheckIPC = bCheckIPC;

                if( isUsingMmap == true || bCheckIPC == true )
                {
                    if( reqMutex.Open() == true )
                    {
                        void* pReqBuf = MapViewOfFile( hReqMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );

                        if( pReqBuf == nullptr )
                        {
                            if( pBuf != nullptr )
                                UnmapViewOfFile( pBuf );

                            if( hMapFile != nullptr )
                                CloseHandle( hMapFile );

                            break;
                        }

                        requestData = malloc( requestSize );
                        memcpy( requestData, pReqBuf, requestSize );

                        if( pReqBuf != nullptr )
                            UnmapViewOfFile( pReqBuf );
                    }
                    else
                    {
                        // ERROR
                    }
                }
                else
                {
                    memcpy( pServerData->reqData, requestData, requestSize );
                }

                isSendSuccess = true;

                if( pBuf != nullptr )
                    UnmapViewOfFile( pBuf );

                if( hMapFile != nullptr )
                    CloseHandle( hMapFile );

            } while( false );

            reqMutex.Release();

            if( isSendSuccess == true )
                break;

            Sleep( 10 );
            nCheckMs++;
        }

        bool isRecvSuccess = false;

        while( nCheckMs < timeOutSec * 100 )
        {
            HANDLE hMapFile = nullptr;
            HANDLE hResMapFile = nullptr;
            void* pClientBuf = nullptr;
            void* pClientResBuf = nullptr;

            do
            {
                if( reqClientMutex.Open() == true )
                {
                    hMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, GetGlobalName( sIPCClientName ).c_str() );
                    pClientBuf = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );

                    if( pClientBuf == nullptr )
                        break;

                    auto* pClientIPCData = reinterpret_cast< stIPCData* >( pClientBuf );

                    if( pClientIPCData->hasRequest == false && pClientIPCData->hasResponse == true )
                    {
                        bool isUsingMmap = false;

                        if( pClientIPCData->responseSize > sizeof( pClientIPCData->resData ) )
                            isUsingMmap = true;

                        responseSize = pClientIPCData->responseSize;

                        if( isUsingMmap == true || bCheckIPC == true )
                        {
                            CIPCMutex reqMutex( GetResponseIPCName( sIPCClientName ), timeOutSec );

                            if( reqMutex.Open() == true )
                            {
                                hResMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, GetGlobalName( GetResponseIPCName( sIPCClientName ) ).c_str() );

                                pClientResBuf = MapViewOfFile( hResMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );

                                if( pClientResBuf == nullptr )
                                    break;

                                //responseData = malloc( responseSize );
                                memcpy( responseData, pClientResBuf, responseSize );

                                if( pClientResBuf != nullptr )
                                    UnmapViewOfFile( pClientResBuf );
                            }
                        }
                        else
                        {
                            memcpy( responseData, pClientIPCData->resData, pClientIPCData->responseSize );
                        }

                        pClientIPCData->isDone = true;
                        isRecvSuccess = true;
                    }
                }

            } while( false );

            reqClientMutex.Release();

            if( hMapFile != nullptr )
                CloseHandle( hMapFile );

            if( hResMapFile != nullptr )
                CloseHandle( hResMapFile );

            if( pClientBuf != nullptr )
                UnmapViewOfFile( pClientBuf );

            if( pClientResBuf != nullptr )
                UnmapViewOfFile( pClientResBuf );

            if( isRecvSuccess == true )
                break;

            Sleep( 10 );
            nCheckMs++;
        }

        if( isSendSuccess == true && isRecvSuccess == true )
            isSuccess = true;

    } while( false );

    if( hReqMapFile != nullptr )
        CloseHandle( hReqMapFile );

    if( hClientMapFile != nullptr )
        CloseHandle( hClientMapFile );

    return isSuccess;
}