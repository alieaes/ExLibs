#pragma once

#ifndef __HDR_EXT_MODULE_BASE__
#define __HDR_EXT_MODULE_BASE__

#include "Module/ExModuleDefines.hpp"
#include "String/EXString.hpp"

namespace Ext
{
    namespace Module
    {
        class cModuleBase
        {
        public:
            cModuleBase();
            virtual ~cModuleBase();

            std::wstring                             GetModuleName() { return _sModuleName; }
            std::wstring                             GetModuleGroup() { return _sModuleGroup; }

            virtual bool                             NotifyModule( const XString& sNotifyJobs ) = 0;

            bool                                     ModuleInit();
            bool                                     ModuleStart();
            bool                                     ModuleStop();
            bool                                     ModuleFinal();

            eModuleState                             GetModuleState();

            bool                                     IsStop() const { return _isStop; }
        protected:
            virtual bool                             moduleInit()   = 0;
            virtual bool                             moduleStart()  = 0;
            virtual bool                             moduleStop()   = 0;
            virtual bool                             moduleFinal()  = 0;

        private:
            void                                     setModuleState( eModuleState state );

            std::wstring                             _sModuleName;
            std::wstring                             _sModuleGroup;

            bool                                     _isStop         = false;
            std::atomic_bool                         _isRunning      = false;

            std::mutex                               _lckState;
            eModuleState                             _eState         = MODULE_STATE_NONE;

            friend class                             cModuleManager;
        };

        typedef std::shared_ptr< cModuleBase > spModuleBase;
    }
}

#endif