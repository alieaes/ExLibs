#pragma once

#ifndef __HDR_EXT_MODULE_DEFINES__
#define __HDR_EXT_MODULE_DEFINES__

#include "String/EXString.hpp"

namespace Ext
{
    namespace Module
    {
        enum eModuleState
        {
            MODULE_STATE_NONE        = 0,
            MODULE_STATE_INIT        = 1,
            MODULE_STATE_START       = 2,
            MODULE_STATE_STOP        = 3,
            MODULE_STATE_FINAL       = 4
        };

        struct MODULE_NOTIFY_INFO
        {
            XString sGroup;
            XString sJobs;

            MODULE_NOTIFY_INFO()
            {
                
            }

            MODULE_NOTIFY_INFO( const std::wstring& _sGroup, const std::wstring& _sJobs )
            {
                sGroup = _sGroup;
                sJobs = _sJobs;
            }

            MODULE_NOTIFY_INFO( const XString& _sGroup, const XString& _sJobs )
            {
                sGroup = _sGroup;
                sJobs = _sJobs;
            }

            MODULE_NOTIFY_INFO( const std::wstring& _sJobs )
            {
                sJobs = _sJobs;
            }

            MODULE_NOTIFY_INFO( const XString& _sJobs )
            {
                sJobs = _sJobs;
            }

            bool isEmpty() const
            {
                return sGroup.IsEmpty() == true && sJobs.IsEmpty() == true;
            }
        };
    }
}

#endif