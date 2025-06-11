#include "stdafx.h"
#include "EXIPC.hpp"

#include "EXProcess.hpp"

///////////////////////////////////////////////////////////////////////////

constexpr size_t SHM_SIZE = sizeof( Ext::IPC::stIPCData );

Ext::IPC::CIPCMutex::CIPCMutex( std::wstring sMutexName, int nTimeout /*= 0*/ )
{
    _sMutexName = sMutexName;
    _nTimeout = nTimeout;
}

Ext::IPC::CIPCMutex::~CIPCMutex()
{
    Release();
}

bool Ext::IPC::CIPCMutex::Create()
{
    _hMutex = OpenMutex( SYNCHRONIZE, FALSE, _sMutexName.c_str() );

    if( ::GetLastError() == ERROR_ALREADY_EXISTS )
        _hMutex = OpenMutex( SYNCHRONIZE, FALSE, _sMutexName.c_str() );

    if( _hMutex == nullptr )
        return false;
    else
        return true;
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

    HANDLE hMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, _info.sIPCName.c_str() );

    while( _isStop == false )
    {
        Sleep( 10 );

        // NOTE : 종료해야하는지?
        if( hMapFile == nullptr )
            hMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, _info.sIPCName.c_str() );

        if( _info.hMutex )
            WaitForSingleObject( _info.hMutex, INFINITE );

        void* pBuf = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );

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

        if( _info.hMutex != nullptr )
            ReleaseMutex( _info.hMutex );
    }
}

void Ext::IPC::CIPCServer::worker()
{
    while( _isStop == false )
    {
        auto data = _queue.DeQueue();

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
            CIPCMutex reqMutex( GetRequestIPCName( data.sSenderIPCName ), INFINITE );

            if( reqMutex.Open() == true )
            {
                HANDLE hMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, GetGlobalName( GetRequestIPCName( data.sSenderIPCName ) ).c_str() );

                void* pBuf = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );
                reqData = malloc( data.requestSize );
                memcpy( reqData, pBuf, data.requestSize );

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

        {
            CIPCMutex reqMutex( data.sSenderIPCName, INFINITE );

            if( reqMutex.Open() == true )
            {
                HANDLE hMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, GetGlobalName( data.sSenderIPCName ).c_str() );
                void* pBuf = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );
                auto* pIPCData = reinterpret_cast< stIPCData* >( pBuf );

                pIPCData->hasResponse = true;
                pIPCData->hasRequest = false;
                pIPCData->nSenderPID = _getpid();

                //if( _info.sIPCName.size() < 256 )
                std::copy( _info.sIPCName.begin(), _info.sIPCName.end(), std::begin( pIPCData->sSenderIPCName ) );

                pIPCData->isInit = true;
                pIPCData->responseSize = data.responseSize;

                if( data.responseSize > sizeof( pIPCData->resData ) || pIPCData->bCheckIPC == true )
                {
                    HANDLE hMapDataFile = nullptr;

                    CIPCMutex resMutex( GetResponseIPCName( data.sSenderIPCName ), INFINITE );
                    // 데드락 조심
                    if( resMutex.Open() == true )
                    {
                        hMapDataFile = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, static_cast< DWORD >( SHM_SIZE ), GetResponseIPCName( data.sSenderIPCName ).c_str() );

                        if( hMapDataFile != nullptr )
                        {
                            void* pDataBuf = MapViewOfFile( hMapDataFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );
                            memcpy( pDataBuf, resData, data.responseSize );

                            if( pDataBuf != nullptr )
                                UnmapViewOfFile( pDataBuf );
                        }
                    }

                    resMutex.Release();

                    if( hMapDataFile != nullptr )
                        CloseHandle( hMapDataFile );
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

    HANDLE hMapFile = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, static_cast< DWORD >( SHM_SIZE ), GetGlobalName( sIPCName ).c_str() );

    void* pBuf = nullptr;

    do
    {
        if( hMapFile == nullptr )
            break;

        pBuf = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );

        if( pBuf == nullptr )
        {
            CloseHandle( hMapFile );
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

        stIPCInfo info;
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

    if( hMapFile != nullptr )
        CloseHandle( hMapFile );

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
    //HANDLE hClientMapFile = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, static_cast< DWORD >( SHM_SIZE ), sIPCClientName.c_str() );

    do
    {
        CIPCMutex ServerMutex( sIPCName, timeOutSec );

        if( ServerMutex.Open() == false )
            break;

        HANDLE hMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, sIPCNameFinal.c_str() );

        if( hMapFile == nullptr )
            break;

        while( nCheckMs < timeOutSec * 100 )
        {
            do
            {
                void* pBuf = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );
                auto* pServerData = reinterpret_cast< stIPCData* >( pBuf );

                if( pServerData->isInit == false || pServerData->hasRequest == true )
                    break;

                bool isUsingMmap = false;

                if( requestSize > sizeof( pServerData->reqData ) )
                    isUsingMmap = true;

                pServerData->hasResponse = false;

                pServerData->nSenderPID = _getpid();
                pServerData->requestSize = requestSize;
                std::copy( sIPCClientName.begin(), sIPCClientName.end(), std::begin( pServerData->sSenderIPCName ) );
                pServerData->hasRequest = true;
                pServerData->needResponse = true;
                pServerData->timeout = std::time( nullptr ) + timeOutSec;
                pServerData->bCheckIPC = bCheckIPC;

                if( isUsingMmap == true || bCheckIPC == 1 )
                {
                    CIPCMutex reqMutex( GetRequestIPCName( sIPCClientName ), INFINITE );

                    if( reqMutex.Open() == true )
                    {
                        HANDLE hMapFile = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, static_cast< DWORD >( requestSize ), GetGlobalName( GetRequestIPCName( sIPCClientName ) ).c_str() );

                        void* pBuf = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );
                        pBuf = malloc( requestSize );
                        memcpy( requestData, pBuf, requestSize );

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
                    memcpy( pServerData->reqData, requestData, requestSize );
                }


            } while( false );
        }

    } while( false );

    return isSuccess;
}