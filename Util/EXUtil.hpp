#ifndef __HDR_EXT_UTIL__
#define __HDR_EXT_UTIL__

namespace Ext
{
    namespace Util
    {
        inline bool isTrue( const wchar_t* c )       { return c == L"Y" || c == L"y" || c == L"True" || c == L"TRUE" || c == L"1"; }
        inline bool isTrue( const std::wstring& s )  { return s == L"Y" || s == L"y" || s == L"True" || s == L"TRUE" || s == L"1"; }

        inline bool isTrue( const char* c )          { return c == "Y" || c == "y" || c == "True" || c == "TRUE" || c == "1"; }
        inline bool isTrue( const std::string& s )   { return s == "Y" || s == "y" || s == "True" || s == "TRUE" || s == "1"; }

        std::wstring CreateGUID();
    }
}

#endif
