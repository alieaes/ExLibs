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
            virtual ~cModuleBase();

            std::wstring                             GetModuleName() { return _sModuleName; }
            std::wstring                             GetModuleGroup() { return _sModuleGroup; }

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

            /*
            *  override sample
            *  setModuleName  -> _sModuleName  = (MODULE_NAME)
            *  setModuleGroup -> _sModuleGroup = (MODULE_GROUP)
            */

            virtual void                             setModuleName() = 0;
            virtual void                             setModuleGroup() = 0;

            std::wstring                             _sModuleName;
            std::wstring                             _sModuleGroup;

        private:
            void                                     setModuleState( eModuleState state );

            bool                                     _isStop         = false;
            std::atomic_bool                         _isRunning      = false;

            std::mutex                               _lckState;
            eModuleState                             _eState         = MODULE_STATE_NONE;
        };

        typedef std::shared_ptr< cModuleBase > spModuleBase;
    }
}

#endif