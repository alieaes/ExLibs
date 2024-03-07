#ifndef __HDR_EXT_CONVERTER__
#define __HDR_EXT_CONVERTER__

#include <codecvt>
#include <sstream>
#include <vector>

#include "Windows.h"

namespace Ext
{
    namespace Convert
    {
        std::wstring                             a2ws( const std::string& str );
        std::wstring                             s2ws( const std::string& str );
        wchar_t*                                 c2wc( const char* c );
        std::string                              ws2s( const std::wstring& wstr );
        char*                                    wc2c( const wchar_t* wc );
        std::wstring                             u82ws( const std::string& utf8 );
        std::string                              ws2u8( const std::wstring& utf16 );
    }
}

#endif