#include "stdafx.h"
#include "EXIPC.hpp"

///////////////////////////////////////////////////////////////////////////

constexpr size_t SHM_SIZE = sizeof( Ext::File::stIPCData );


Ext::File::CIPCServer::CIPCServer( stIPCInfo& info )
{
    _info = info;
    _thWorker = std::thread( [ this ]() { main(); } );
}

Ext::File::CIPCServer::~CIPCServer()
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

void Ext::File::CIPCServer::main()
{
    for( int idx = 1; idx < _info.nMaxThreadCount; idx++ )
        _vecThread.push_back( std::make_shared<std::thread>( std::thread( [ this ]() { worker(); } ) ) );

    while( _isStop == false )
    {
        Sleep( 10 );

        HANDLE hMutex = CreateMutex( NULL, TRUE, _info.sIPCName.c_str() );
    }
}

bool Ext::File::CreateIPC( const std::wstring& sIPCName, fnCallback fnCallback, void* pContext, int nMaxThreadCount /*= IPC_DEFAULT_MAX_THREAD_COUNT*/, int timeOutSec /*= IPC_DEFAULT_TIMEOUT*/ )
{
    bool isSuccess = false;
    stIPCShm shm;

    std::wstring sIPCNameFinal = L"Global\\" + sIPCName;
    shm.sDataIPCName = sIPCNameFinal + L"DATA";

    HANDLE hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        static_cast< DWORD >( SHM_SIZE ),
        sIPCNameFinal.c_str()
    );

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

    return isSuccess;
}
