﻿#ifndef __HDR_EXT_XSTRING__
#define __HDR_EXT_XSTRING__

#define EXT_USING_XSTRING
#define _CRT_SECURE_NO_WARNINGS

#include <codecvt>
#include <sstream>
#include <vector>
#include <stdlib.h>

#include "Windows.h"

#include "EXConverter.hpp"

class XString
{
public:
    XString()
    {
        _str = std::wstring();
    }

    XString( const char* c )
    {
        if( c == NULL )
            return;

        _str = c2ws( c );
    }

    XString( const wchar_t* wc )
    {
        if( wc == NULL )
            return;

        _str = wc;
    }

    XString( const unsigned char* c )
    {
        if( c == NULL )
            return;

        std::string s = reinterpret_cast< const char* >( c );
        _str = std::wstring( s.begin(), s.end() );
    }

    XString( DWORD dw )
    {
        _str = std::to_wstring( dw );
    }

    XString( int n )
    {
        _str = std::to_wstring( n );
    }

    XString( unsigned int n )
    {
        _str = std::to_wstring( n );
    }

    XString( long n )
    {
        _str = std::to_wstring( n );
    }

    XString( ULONGLONG ull )
    {
        _str = std::to_wstring( ull );
    }

    XString( double d )
    {
        _str = std::to_wstring( d );
    }

    XString( const std::wstring& ws )
    {
        _str = ws;
    }

    XString( const std::string& s )
    {
        s2ws( s );
    }

    XString( const std::wstringstream& ws )
    {
        _str = ws.rdbuf()->str();
    }

#ifdef QSTRING_H
    XString( const QString& qs )
    {
        _str = qs.toStdWString();
    }
#endif // QSTRING_H

    ~XString()
    = default;

    auto operator<=>( XString const& ) const = default;

    /////////////// XString <- X ///////////////
    XString operator = ( std::wstring& ws )          // XString xs = std::wstring( L"TEST" );
    {
        _str = ws;
        return _str;
    }

    XString operator = ( std::string& s )            // XString xs = std::string( "TEST" );
    {
        _str = s2ws( s );
        return _str;
    }

    XString operator = ( const char* c )             // XString xs = "TEST";
    {
        _str = c2ws( c );
        return _str;
    }

    XString operator = ( const unsigned char* c )             // XString xs = "TEST";
    {
        std::string s = static_cast< std::string >( reinterpret_cast< const char* >( c ) ); // new style
        _str = s2ws( s );
        return _str;
    }

    XString operator = ( const wchar_t* wc )         // XString xs = L"TEST";
    {
        _str = wc;
        return _str;
    }

    XString operator = ( const XString& wc )         // XString xs = L"TEST";
    {
        _str = wc.toWString();
        return _str;
    }

    XString operator = ( const std::wstringstream& ws )
    {
        _str = ws.rdbuf()->str();
        return _str;
    }

#ifdef QSTRING_H
    XString operator = ( QString& qs )               // XString xs = QString( "TEST" );
    {
        _str = qs.toStdWString();
        return _str;
    }
#endif  // QSTRING_H
    ////////////////////////////////////////////

    /////////////// XString -> X ///////////////
    operator std::string() const
    {
        if( _str.empty() ) return {};

        int nSize = WideCharToMultiByte( CP_UTF8, 0, _str.c_str(), ( int )_str.size(), NULL, 0, NULL, NULL );
        std::string result( nSize, 0 );
        WideCharToMultiByte( CP_UTF8, 0, _str.c_str(), ( int )_str.size(), &result[ 0 ], nSize, NULL, NULL );
        return result;
    }
    //operator std::wstring() const { return _str; }

    operator LPCWSTR() const { return _str.c_str(); }

#ifdef QSTRING_H
    operator QString() const { return QString::fromStdWString( _str ); }
#endif  // QSTRING_H
    ////////////////////////////////////////////

    XString operator+= ( int n )
    {
        _str += std::to_wstring( n );
        return _str;
    }

    XString operator+= ( double d )
    {
        _str += std::to_wstring( d );
        return _str;
    }

    XString operator+= ( XString xs )
    {
        _str += xs.string();
        return _str;
    }

    XString operator +=( char c )
    {
        _str += std::to_wstring( c );
        return _str;
    }

    XString operator +=( const char* c )
    {
        _str += c2ws( c );
        return _str;
    }

    XString operator += ( std::string& s )
    {
        _str += s2ws( s );
        return _str;
    }

    XString operator +( XString& xs )
    {
        std::wstring tmp = _str + xs.toWString();
        return tmp;
    }

    XString operator +( std::wstring& xs )
    {
        std::wstring tmp = _str + xs;
        return tmp;
    }

    XString operator +( const char* c )
    {
        std::wstring tmp = _str + c2ws( c );
        return tmp;
    }

    XString operator +( const std::wstring& xs )
    {
        return _str + xs;
    }

    XString operator +( const std::string& xs )
    {
        std::wstring tmp = _str + s2ws( xs );
        return tmp;
    }

    bool operator ==( std::wstring& xs ) const
    {
        return xs.compare( _str ) == 0 ? true : false;
    }

    bool operator ==( const XString& xs ) const
    {
        return _str.compare( xs.toWString() ) == 0 ? true : false;
    }

    bool operator ==( XString& xs ) const
    {
        return _str.compare( xs.toWString() ) == 0 ? true : false;
    }

    bool operator ==( const std::wstring& xs ) const
    {
        return xs.compare( _str ) == 0 ? true : false;
    }

    bool operator ==( const wchar_t* wc ) const
    {
        return _str.compare( wc ) == 0 ? true : false;
    }

    XString operator *() const
    {
        if( _nPos > size() )
            return NULL;
        return _str[ _nPos ];
    }

    XString operator ++()
    {
        return _str[ _nPos++ ];
    }

    XString operator ++( int )
    {
        if( _nPos + 1 > size() )
        {
            ++_nPos;
            return NULL;
        }
        return _str[ ++_nPos ];
    }

    XString operator --( int )
    {
        if( _nPos - 1 < size() )
        {
            --_nPos;
            return NULL;
        }
        return _str[ --_nPos ];
    }

    std::string                      toString() const;
    std::wstring                     toWString() const;
#ifdef QSTRING_H
    QString                          toQString() const;
#endif
    // 값이 존재한다고 TRUE가 되지 않음, 1/Yes/True/y 의 값이여야 TRUE로 반환됨
    bool                             toBool() const;
    int                              toInt() const;
    long                             toLong() const;

    XString                          toLower() const;
    XString                          toUpper() const;

    std::vector< char >              toCharByte() const;

    bool                             IsEmpty() const;
    bool                             IsDigit() const;
    bool                             Endl() const;

    size_t                           size() const;
    int                              count( const XString& find ) const;

    XString                          substr( size_t nDst ) const;
    XString                          substr( size_t nSrc, size_t nSize ) const;

    size_t                           find_last_of( XString xs ) const;
    size_t                           find( XString xs ) const;
    size_t                           rfind( XString xs ) const;

    const wchar_t*                   c_str() const;

    std::wstring                     string() { return _str; }

    std::vector< XString >           split( const XString& sSplit ) const;

    XString                          replaceAll( const XString& xa, const XString& xb ) const;
    XString                          replace( const XString& xa, const XString& xb ) const;

    int                              compare( const XString& xs, bool isCaseInsensitive = false ) const;

    bool                             contains( const XString& xs, bool isCaseInsensitive = false ) const;

    bool                             startswith( const XString& xs, bool isCaseInsensitive = false ) const;
    bool                             endswith( const XString& xs, bool isCaseInsensitive = false ) const;

    void                             clear();

protected:

private:
    std::wstring                     s2ws( const std::string& s );
    std::wstring                     c2ws( const char* c );

    std::wstring                     _str;
    int                              _nPos = 0;
};

#endif
