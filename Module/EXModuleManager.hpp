#pragma once

#ifndef __HDR_EXT_MODULE_MANAGER__
#define __HDR_EXT_MODULE_MANAGER__

#include "ExLibs/Base/Singleton.hpp"
#include "EXModuleBase.hpp"

#include <shared_mutex>

#include "Util/ExDataStruct.hpp"

/*
 * RegisterModule   : ModuleInit
 * UnRegisterModule : ModuleFinal
 */
namespace Ext
{
    namespace Module
    {
        typedef std::map< std::wstring, spModuleBase > mapNameToModule;

        class cModuleManager
        {
        public:
            cModuleManager();
            ~cModuleManager();

        public:
            bool                                     RegisterModule( const std::wstring& sModuleName, cModuleBase* cModule );
            bool                                     RegisterModule( const std::wstring& sModuleName, spModuleBase spModule );

            bool                                     UnRegisterModule( const std::wstring& sModuleName );

            bool                                     NotifyAllModule( const std::wstring& sNotifyJobs );
            bool                                     NotifyGroupModule( const std::wstring& sGroup, const std::wstring& sNotifyJobs );

            bool                                     IsExistModule( const std::wstring& sModuleName );
        private:
            spModuleBase                             getModule( const std::wstring& sModuleName );

            bool                                     insertModule( const std::wstring& sModuleName, const spModuleBase& spModule );
            bool                                     deleteModule( const std::wstring& sModuleName );

            void                                     notifyAllModule();

        private:
            bool                                     _isStop                 = false;

            mapNameToModule                          _mapModuleNameToModule;
            std::shared_mutex                        _lckModule;

            Queue::eventQueue< MODULE_NOTIFY_INFO >  _notifyQueue;

            std::thread                              _thNotify;

        };

        typedef Ext::Singletons::Singleton< cModuleManager > tdStModuleManager;
    }
}

#endif