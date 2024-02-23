#pragma once

#ifndef __HDR_EXT_MODULE_BASE__
#define __HDR_EXT_MODULE_BASE__

#include "Module/ExModuleDefines.hpp"

namespace Ext
{
    namespace Module
    {
        class cModuleBase
        {
        public:
            cModuleBase();
            ~cModuleBase();

            std::wstring                             GetModuleName() { return _sModuleName; }
            std::wstring                             GetGroup() { return _sModuleGroup; }

            void                                     SetModuleName( const std::wstring& sModuleName ) { _sModuleName = sModuleName; }
            void                                     SetGroup( const std::wstring& sGroup ) { _sModuleGroup = sGroup; }

            virtual bool                             NotifyModule( const std::wstring& sNotifyJobs ) = 0;

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

            bool                                     _isStop         = false;
            std::atomic_bool                         _isRunning      = false;

            std::mutex                               _lckState;
            eModuleState                             _eState         = MODULE_STATE_NONE;

            std::wstring                             _sModuleName;
            std::wstring                             _sModuleGroup;
        };

        typedef std::shared_ptr< cModuleBase > spModuleBase;
    }
}

#endif