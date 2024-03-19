#pragma once

#ifndef __HDR_EXT_UTIL__
#define __HDR_EXT_UTIL__

#include <random>

#include "EXString.hpp"
#include "Base/Singleton.hpp"

#define EXT_ENUM_START( x )              \
class c##x                               \
{                                        \
public:                                  \
    c##x()                               \
    {

#define EXT_ENUM_ADD( e, x )             \
        _map[ e ] = x;

#define EXT_ENUM_END( x )                \
    }                                    \
    ~c##x() {}                           \
    XString Get( x e )                   \
    {                                    \
        if( _map.contains( e ) == true ) \
            return _map[ e ];            \
        else                             \
            return "";                   \
    }                                    \
    void Set( x e, const XString& s )    \
    {                                    \
        _map[ e ] = s;                   \
    }                                    \
private:                                 \
    std::map< x, XString > _map;         \
};

#define EXT_ENUM_GET( x, e )             \
    c##x().Get( e )

namespace Ext
{
    enum eCaseType
    {
        CASE_TYPE_NONE = 0,
        CASE_TYPE_UPPER = 1,
        CASE_TYPE_LOWER = 2
    };

    enum eCaseSensitivity
    {
        CASE_SENSITIVE = 0,
        CASE_INSENSITIVE = 1
    };

    namespace Util
    {
        inline bool isTrue( const wchar_t* c )       { return c == L"Y" || c == L"y" || c == L"True" || c == L"TRUE" || c == L"1"; }
        inline bool isTrue( const std::wstring& s )  { return s == L"Y" || s == L"y" || s == L"True" || s == L"TRUE" || s == L"1"; }

        inline bool isTrue( const char* c )          { return c == "Y" || c == "y" || c == "True" || c == "TRUE" || c == "1"; }
        inline bool isTrue( const std::string& s )   { return s == "Y" || s == "y" || s == "True" || s == "TRUE" || s == "1"; }

        std::wstring                             CreateGUID( eCaseType eCase = CASE_TYPE_NONE );

        double                                   CalcPercentageIncrease( double dInit, double dFinal );

        template < typename T >
        class cRandom
        {
        public:
            cRandom( T nStart, T nEnd )
            {
                SetRange( nStart, nEnd );
                Reset();
            }

            ~cRandom()
            {
            }

            void SetRange( T nStart, T nEnd )
            {
                if( nStart < nEnd )
                {
                    _nRangeStart = nStart;
                    _nRangeEnd = nEnd;
                }
                else
                {
                    _nRangeStart = nEnd;
                    _nRangeEnd = nStart;
                }

                _dist = std::uniform_int_distribution<T>( _nRangeStart, _nRangeEnd );
            }

            void Reset()
            {
                _gen.seed( std::random_device{}( ) );
            }

            T Generate()
            {
                return _dist( _gen );
            }

        private:
            T                                    _nRangeStart;
            T                                    _nRangeEnd;
            std::mt19937                         _gen;
            std::uniform_int_distribution< T >   _dist;
        };
    }
}

#endif
