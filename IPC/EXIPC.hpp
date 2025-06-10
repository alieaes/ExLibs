#pragma once

#ifndef __HDR_EXT_IPC__
#define __HDR_EXT_IPC__

#include <functional>
#include <map>
#include <thread>

#include "ExDataStruct.hpp"
#include "EXString.hpp"

namespace Ext
{
    namespace IPC
    {
        constexpr int IPC_DEFAULT_MAX_THREAD_COUNT = 10;
        constexpr int IPC_DEFAULT_TIMEOUT = 10;

        struct stIPCData
        {
            // client -> server
            unsigned int     reqData[ 128 ];
            size_t           requestSize    = 0;
            bool             hasRequest     = false;

            // server -> client
            unsigned int     resData[ 128 ];
            size_t           responseSize   = 0;
            bool             hasResponse    = false;
            bool             needResponse   = false;
            wchar_t          sSenderIPCName[ 256 ];

            unsigned int     nSenderPID     = 0;
            int64_t          timeout        = 0;

            bool             isInit         = false;

            bool             bCheckIPC      = false;

            void clear()
            {
                isInit       = true;

                nSenderPID   = 0;

                hasRequest   = false;
                hasResponse  = false;
                needResponse = false;

                bCheckIPC    = false;
            }
        };

        typedef std::unique_ptr< stIPCData > spIPCData;
        typedef std::function< void( const std::wstring sIPCName, void* requestData, size_t& requestSize, void* responseData, size_t& responseSize, void* pContext ) > fnCallback;
        typedef std::shared_ptr< std::thread > spIPCThread;

        struct stIPCInfo
        {
            std::wstring      sIPCName;
            std::wstring      sDataIPCName;
            fnCallback        fnCallback;
            void*             pContext;
            int               nMaxThreadCount = IPC_DEFAULT_MAX_THREAD_COUNT;
            int               nTimeoutSec     = IPC_DEFAULT_TIMEOUT;
        };

        enum eIPCResult
        {
            IPC_RESULT_UNKNOWN = 0,

            // SUCCESS
            IPC_RESULT_SUCCESS = 1,

            // CIPC SERVER FAILED
            IPC_RESULT_FAILED_IPC_NAME_EXIST = 10,

            // CIPC SERVER SKIP
            IPC_RESULT_SKIP_TERMINATE_CLIENT = 21,
            IPC_RESULT_SKIP_NOT_REQUEST = 22,
            IPC_RESULT_SKIP_EMPTY_RESPONSE = 23,

            // CIPC CLIENT FAILED
            IPC_RESULT_FAILED_TIMEOUT = 30,
            IPC_RESULT_FAILED_SERVER_NOT_OPEN = 31,

            // SHM FAILED
            IPC_RESULT_SHM_CREATE_FAILED = 40,
            IPC_RESULT_TRUNCATE_FAILED = 41,
            IPC_RESULT_SHM_OPEN_FAILED = 42,

            // SEM FAILED
            IPC_RESULT_SEM_CREATE_FAILED = 51,
            IPC_RESULT_SEM_OPEN_FAILED = 52,

            // MMAP FAILED
            IPC_RESULT_MMAP_OPEN_FAILED = 61
        };

        class CIPCServer
        {
        public:
            CIPCServer( stIPCInfo& info );
            ~CIPCServer();

        private:
            void                            main();
            void                            worker();

            std::vector< spIPCThread >      _vecThread;
            Queue::eventQueue< stIPCData >  _queue;

            bool                            _isStop = false;

            std::thread                     _thWorker;

            stIPCInfo                       _info;
        };

        typedef std::shared_ptr< CIPCServer > spIPCServer;

        struct stIPCShm
        {
            std::wstring      sIPCName;
            std::wstring      sDataIPCName;
            spIPCServer       spIPCServer;
        };

        static std::map< std::string, stIPCShm > mapIPC;

        std::wstring                     GetDataIPCName( std::wstring sIPCName );

        eIPCResult                       GetIPCLastError();
        std::string                      GetIPCStrError( eIPCResult err );
        bool                             CreateIPC( const std::wstring& sIPCName, fnCallback fnCallback, void* pContext, int nMaxThreadCount = IPC_DEFAULT_MAX_THREAD_COUNT, int timeOutSec = IPC_DEFAULT_TIMEOUT );
        bool                             DestroyIPC( const std::wstring& sIPCName );
        bool                             SendIPC( const std::wstring& sIPCName, void* requestData, size_t requestSize, void* responseData, size_t& responseSize, int timeOutSec = IPC_DEFAULT_TIMEOUT, int nCheckIPC = 0 );
        bool                             CheckIPC( const std::wstring& sIPCName, int nCheckIPC );
    }
}

#endif
