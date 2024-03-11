#pragma once

#ifndef __HDR_EXT_MODULE_MANAGER__
#define __HDR_EXT_MODULE_MANAGER__

#include "ExLibs/Base/Singleton.hpp"
#include "String/EXString.hpp"

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
        typedef std::map< XString, spModuleBase > mapNameToModule;

        class cModuleManager
        {
        public:
            cModuleManager();
            ~cModuleManager();

        public:
            bool                                     RegisterModule( const XString& sModuleName, const XString& sModuleGroup, cModuleBase* cModule );
            bool                                     RegisterModule( const XString& sModuleName, const XString& sModuleGroup, spModuleBase spModule );

            bool                                     UnRegisterModule( const XString& sModuleName );

            bool                                     NotifyAllModule( const XString& sNotifyJobs );
            bool                                     NotifyGroupModule( const XString& sGroup, const XString& sNotifyJobs );

            bool                                     IsExistModule( const XString& sModuleName );

            spModuleBase                             GetModuleBase( const XString& sModuleName );

        private:
            bool                                     insertModule( const XString& sModuleName, const XString& sModuleGroup, const spModuleBase& spModule );
            bool                                     insertModule( const XString& sModuleName, const spModuleBase& spModule );
            bool                                     deleteModule( const XString& sModuleName );

            void                                     notifyAllModule();

        private:
            bool                                     _isStop                 = false;

            mapNameToModule                          _mapModuleNameToModule;
            std::shared_mutex                        _lckModule;

            Queue::eventQueue< MODULE_NOTIFY_INFO >  _notifyQueue;

            std::thread                              _thNotify;
        };

        typedef Ext::Singletons::Singleton< cModuleManager > tdStModuleManager;

        template< typename T >
        std::shared_ptr< T > GetModule( const XString& sModuleName )
        {
            auto stModule = tdStModuleManager::GetInstance();

                do
                {
                    if( stModule->IsExistModule( sModuleName ) == false )
                        break;

                    return std::dynamic_pointer_cast< T >( stModule->GetModuleBase( sModuleName ) );

                } while( false );

                return std::shared_ptr< T >();
        }
    }
}

#endif