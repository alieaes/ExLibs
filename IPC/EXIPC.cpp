#include "stdafx.h"
#include "EXIPC.hpp"

#include "EXProcess.hpp"

///////////////////////////////////////////////////////////////////////////

constexpr size_t SHM_SIZE = sizeof( Ext::IPC::stIPCData );


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
}

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

        HANDLE hMutex = OpenMutex( SYNCHRONIZE, FALSE, _info.sIPCName.c_str() );

        if( hMutex )
            WaitForSingleObject( hMutex, INFINITE );

        void* pBuf = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );

        void* reqBuffer = nullptr;
        size_t reqSize = 0;

        auto* pIPCData = reinterpret_cast< stIPCData* >( pBuf );

        if( pIPCData->hasRequest == true )
        {
	        if( Process::IsRunningProcess( pIPCData->nSenderPID ) == false )
	        {
                pIPCData->clear();

                if( hMutex != nullptr )
                    ReleaseMutex( hMutex );

                continue;
	        }

            stIPCData qData = *pIPCData;
            _queue.EnQueue( qData );

            pIPCData->clear();
        }

        if( hMutex != nullptr )
            ReleaseMutex( hMutex );
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
            HANDLE hMutex = OpenMutex( SYNCHRONIZE, FALSE, _info.sDataIPCName.c_str() );

            if( hMutex )
            {
                WaitForSingleObject( hMutex, INFINITE );

                HANDLE hMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, _info.sDataIPCName.c_str() );

                void* pBuf = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );
                reqData = malloc( data.requestSize );
                memcpy( reqData, pBuf, data.requestSize );

                ReleaseMutex( hMutex );
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
            HANDLE hMutex = OpenMutex( SYNCHRONIZE, FALSE, data.sSenderIPCName );

            if( hMutex )
            {
                WaitForSingleObject( hMutex, INFINITE );

                HANDLE hMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, data.sSenderIPCName );
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
                    // 데드락 조심
                    HANDLE hDataMutex = OpenMutex( SYNCHRONIZE, FALSE, GetDataIPCName( data.sSenderIPCName ).c_str() );
                    if( hDataMutex )
                    {
                        WaitForSingleObject( hDataMutex, INFINITE );
                        HANDLE hMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, GetDataIPCName( data.sSenderIPCName ).c_str() );
                        void* pBuf = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );
                        memcpy( pBuf, resData, data.responseSize );
                    }

                    if( hDataMutex != nullptr )
                        ReleaseMutex( hDataMutex );
                }
                else
                {
                    memcpy( pIPCData->resData, resData, data.responseSize );
                }

                if( hMutex != nullptr )
                    ReleaseMutex( hMutex );
            }
            else
            {
                // ERROR
            }
        }
    }
}

std::wstring Ext::IPC::GetDataIPCName( std::wstring sIPCName )
{
    return sIPCName + L"DATA";
}

bool Ext::IPC::CreateIPC( const std::wstring& sIPCName, fnCallback fnCallback, void* pContext, int nMaxThreadCount /*= IPC_DEFAULT_MAX_THREAD_COUNT*/, int timeOutSec /*= IPC_DEFAULT_TIMEOUT*/ )
{
    bool isSuccess = false;
    stIPCShm shm;

    std::wstring sIPCNameFinal = L"Global\\" + sIPCName;
    shm.sDataIPCName = GetDataIPCName( sIPCNameFinal );

    HANDLE hMapFile     = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, static_cast< DWORD >( SHM_SIZE ), sIPCNameFinal.c_str() );
    //HANDLE hMapDataFile = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, static_cast< DWORD >( SHM_SIZE ), shm.sDataIPCName.c_str() );

    do
    {
        if( hMapFile == nullptr ) 
        {
            //std::cerr << "CreateFileMapping failed: " << GetLastError() << std::endl;
            break;
        }

        void* pBuf = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE );

        if( pBuf == nullptr ) 
        {
            CloseHandle( hMapFile );
            break;
        }

        auto* pIPCData = reinterpret_cast< stIPCData* >( pBuf );
        pIPCData->clear();

        HANDLE hMutex = CreateMutex( NULL, TRUE, sIPCNameFinal.c_str() );

        if( ::GetLastError() == ERROR_ALREADY_EXISTS )
            break;

        if( hMutex == nullptr )
            break;

        pIPCData->isInit = true;

        stIPCInfo info;
        info.sIPCName = sIPCName;
        info.sDataIPCName = shm.sDataIPCName;
        info.fnCallback = fnCallback;
        info.nMaxThreadCount = nMaxThreadCount;
        info.pContext = pContext;
        info.nTimeoutSec = timeOutSec;

        shm.sIPCName = sIPCName;
        shm.spIPCServer = std::make_shared< CIPCServer >( info );

        if( hMutex != nullptr )
            ReleaseMutex( hMutex );

        isSuccess = true;

    } while( false );

    if( hMapFile != nullptr )
        CloseHandle( hMapFile );

    //if( hMapDataFile != nullptr )
    //    CloseHandle( hMapDataFile );

    return isSuccess;
}
