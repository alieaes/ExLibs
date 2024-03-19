#pragma once

#ifndef __HDR_EXT_FORMAT__
#define __HDR_EXT_FORMAT__

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ExLibs/Base/ExtConfig.hpp"
#include "ExLibs/Util/EXUtil.hpp"

#define EXT_FORMAT_TYPE_ADD_START()                     \
    namespace Ext                                       \
    {                                                   \
        namespace Format                                \
        {

#define EXT_FORMAT_TYPE_ADD_MAKE_START( type )          \
            std::wstring format_as( type value )        \
            {

#define EXT_FORMAT_TYPE_ADD_MAKE_END()                  \
            }

#define EXT_FORMAT_TYPE_ADD_MAKE( type, impl )          \
            inline std::wstring format_as( type value ) \
            {                                           \
                return impl( value );                   \
            } 
            
#define EXT_FORMAT_TYPE_ADD_END()                       \
        }                                               \
    } 

#include "String/EXFormatTypes.hpp"

/*
 * Required Include File : ExtConfig, EXUtil, EXFormatTypes
 * Using : Format( "{} {0} {} {2:b}", 10, 20, 1 ) -> "10 10 20 true"
 */

namespace Ext
{
    namespace Format
    {
        template<typename T>
        std::wstring format_as( T Value )
        {
            return L"unknown type";
        }

        inline std::wstring format_as( int Value )
        {
            return std::to_wstring( Value );
        }

        inline std::wstring format_as( const wchar_t* Value )
        {
            return Value;
        }

        inline std::wstring format_as( wchar_t* Value )
        {
            return Value;
        }

        inline std::wstring format_as( const char* Value )
        {
            std::string tempString( Value );
            std::wstring wideString( tempString.begin(), tempString.end() );
            return std::wstring( tempString.begin(), tempString.end() );
        }

#ifdef QSTRING_H
        inline std::wstring format_as( QString Value )
        {
            return Value.toStdWString();
        }
#endif

        class cFormat
        {
        public:
            template< typename ... Args >
            std::wstring Format( const std::wstring& sFormatString, Args... args )
            {
                format_args_impl( args... );
                const std::wstring& sRet = format_trans( sFormatString.c_str() );
                return sRet;
            }

        private:
            template<typename T>
            void format_args_impl( T value )
            {
                const std::wstring& s = format_as( value );
                _vecArgs.push_back( s );
            }

            template<typename T, typename... Args>
            void format_args_impl( T value, Args... args )
            {
                const std::wstring& s = format_as( value );
                _vecArgs.push_back( s );
                format_args_impl( args... );
            }

            std::wstring format_trans( const wchar_t* sFormatString ) const
            {
                std::wstring sRet;

                bool isPlaceHolder = false;
                bool isType = false;
                bool isBracket = false;

                std::wstring sType;

                short nIndex = -1;
                short nArgsIdx = 0;

                while( *sFormatString != NULL )
                {
                    if( isBracket == true )
                    {
                        if( *sFormatString == L'}' )
                            sFormatString++;

                        if( *sFormatString == L'}' )
                        {
                            sRet += L"{}";
                            isBracket = false;
                            isPlaceHolder = false;
                            sFormatString++;
                            continue;
                        }
                        else
                        {
                            sRet += L"{";
                            isBracket = false;
                            isPlaceHolder = false;
                        }
                    }

                    if( isPlaceHolder == true && isType == true )
                    {
                        if( *sFormatString != L'{' && *sFormatString != L'}' )
                        {
                            sType += *sFormatString;
                            sFormatString++;
                            continue;
                        }

                        if( *sFormatString == L'}' )
                        {
                            if( sType == L"b" || sType == L"B" )
                            {
                                if( nIndex == -1 )
                                {
                                    if( _vecArgs.size() >= ( nArgsIdx + 1 ) )
                                    {
                                        sRet += Util::isTrue( _vecArgs[ nArgsIdx++ ] ) == true ? L"true" : L"false";
                                        sFormatString++;
                                        isPlaceHolder = false;
                                        isType = false;
                                        sType.clear();
                                        continue;
                                    }
                                    else
                                    {
                                        isPlaceHolder = false;
                                        isType = false;
                                        sType.clear();
                                        sFormatString++;
                                    }
                                }
                                else
                                {
                                    if( _vecArgs.size() >= ( nIndex + 1 ) )
                                    {
                                        sRet += Util::isTrue( _vecArgs[ nIndex ] ) == true ? L"true" : L"false";
                                        sFormatString++;
                                        isPlaceHolder = false;
                                        nIndex = -1;
                                        sType.clear();
                                        continue;
                                    }
                                    else
                                    {
                                        nIndex = -1;
                                        isType = false;
                                        sType.clear();
                                        isPlaceHolder = false;
                                        sFormatString++;
                                    }
                                }
                            }
                            else
                            {
                                nIndex = -1;
                                sType.clear();
                                isPlaceHolder = false;
                                isType = false;
                                sFormatString++;
                            }
                                
                        }
                    }

                    if( isPlaceHolder == true )
                    {
                        if( *sFormatString == L'{' )
                        {
                            isBracket = true;
                            sFormatString++;
                            continue;
                        }
                        else if( *sFormatString == L'}' )
                        {
                            if( isType == false && nIndex == -1 )
                            {
                                if( _vecArgs.size() >= ( nArgsIdx + 1 ) )
                                {
                                    sRet += _vecArgs[ nArgsIdx++ ];
                                    sFormatString++;
                                    isPlaceHolder = false;
                                    continue;
                                }
                                else
                                {
                                    isPlaceHolder = false;
                                    sFormatString++;
                                }
                            }
                            else if( isType == false && nIndex > -1 )
                            {
                                if( _vecArgs.size() >= ( nIndex + 1 ) )
                                {
                                    sRet += _vecArgs[ nIndex ];
                                    sFormatString++;
                                    isPlaceHolder = false;
                                    nIndex = -1;
                                    continue;
                                }
                                else
                                {
                                    nIndex = -1;
                                    isPlaceHolder = false;
                                    sFormatString++;
                                }
                            }
                        }
                        else
                        {
                            if( *sFormatString == L':' )
                            {
                                isType = true;
                                sFormatString++;
                            }
                            else
                            {
                                short nIdx = -1;

                                if( *sFormatString >= L'0' && *sFormatString <= L'9' )
                                {
                                    nIdx = *sFormatString - L'0';

                                    if( nIndex == -1 )
                                        nIndex = nIdx;
                                    else
                                        nIndex = ( nIndex * 10 ) + nIdx;

                                    sFormatString++;
                                    continue;
                                }
                            }

                        }
                    }
                    else
                    {
                        if( *sFormatString == L'{' )
                        {
                            isPlaceHolder = true;
                            *sFormatString++;
                        }
                        else
                        {
                            sRet += *sFormatString;
                            *sFormatString++;
                        }
                    }
                }

                return sRet;
            }

            std::vector< std::wstring > _vecArgs;
        };

        template< typename ... Args >
        XString Format( const XString& sFormatString, Args... args )
        {
            return cFormat().Format( sFormatString.toWString(), args... );
        }
    }
}

#endif
