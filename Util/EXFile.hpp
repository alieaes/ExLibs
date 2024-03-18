#pragma once

#ifndef __HDR_EXT_UTIL_FILE__
#define __HDR_EXT_UTIL_FILE__

#include "EXString.hpp"

namespace Ext
{
    namespace File
    {
        bool                                     IsExistFile( const XString& sFileFullPath );
        bool                                     IsExistDir( const XString& sDirPath );
    }
}

#endif
