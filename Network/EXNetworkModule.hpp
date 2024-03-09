#pragma once

#ifndef __HDR_EXT_NETWORK_MODULE__
#define __HDR_EXT_NETWORK_MODULE__

#include "Module/EXModuleManager.hpp"
#include "Module/EXModuleBase.hpp"
#include "Network/EXNetwork.hpp"

#include "Network/include/Socket/Socket.h"
#include "Network/include/Socket/TCPClient.h"
#include "Network/include/Socket/TCPServer.h"

namespace Ext
{
    namespace Network
    {
        class cModuleNetwork : public Module::cModuleBase
        {
        public:
            cModuleNetwork();
            ~cModuleNetwork() override;
            bool NotifyModule( const std::wstring& sNotifyJobs ) override;

            bool                                     NewConnection();
            bool                                     GetConnection();
            bool                                     CloseConnection();

        protected:
            bool                                     moduleInit() override;
            bool                                     moduleStart() override;
            bool                                     moduleStop() override;
            bool                                     moduleFinal() override;
            void                                     setModuleName() override;
            void                                     setModuleGroup() override;

        public:
        };
    }
}

#endif