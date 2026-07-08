#include "stdafx.h"
#include "EXString.hpp"

#include <algorithm>
#include <cwctype>
#include <Windows.h>

#ifdef USING_QTLIB
#include <qdatetime.h>
#endif

std::string XString::toString() const
{
    return Ext::Convert::ws2s( _str );
}

std::wstring XString::toWString() const
{
    return _str;
}

#ifdef QSTRING_H
QString XString::toQString() const
{
    return QString::fromStdWString( _str );
}
#endif

bool XString::toBool() const
{
    if( auto tmp = toLower(); tmp == L"y" || tmp == L"true" || tmp == L"1" || tmp == L"yes" )
        return true;

    return false;
}

int XString::toInt() const
{
    try
    {
        return std::stoi( _str );
    }
    catch( const std::exception& )
    {
        return 0;
    }
}

long XString::toLong() const
{
    try
    {
        return std::stol( _str );
    }
    catch( const std::exception& )
    {
        return 0;
    }
}

XString XString::toLower() const
{
    std::wstring tmp = _str;
    std::ranges::transform( tmp, tmp.begin(), towlower );

    return tmp;
}

XString XString::toUpper() const
{
    std::wstring tmp = _str;
    std::ranges::transform( tmp, tmp.begin(), towupper );

    return tmp;
}

std::vector<char> XString::toCharByte() const
{
    std::string tmp = Ext::Convert::ws2s( _str );

    return std::vector<char>( tmp.begin(), tmp.end() );
}

bool XString::IsEmpty() const
{
    return _str.empty();
}

bool XString::IsDigit() const
{
    if( IsEmpty() == true )
        return false;

    return std::ranges::all_of( _str, []( wchar_t c ) { return std::iswdigit( c ) != 0; } );
}

size_t XString::size() const
{
    return _str.size();
}

int XString::count( const XString& find ) const
{
    int nCount = 0;

    if( find.IsEmpty() == true )
        return nCount;

    size_t sizeFind = std::string::npos;
    size_t idx = 0;

    while( true )
    {
        sizeFind = _str.find( find.toWString(), idx );
        if( sizeFind != std::string::npos )
        {
            nCount++;
            idx = sizeFind + find.size();
        }
        else
        {
            break;
        }
    }

    return nCount;
}

XString XString::substr( size_t nDst ) const
{
    if( nDst == std::string::npos )
        return "";

    return _str.substr( nDst );
}

XString XString::substr( size_t nSrc, size_t nSize ) const
{
    if( nSrc == std::string::npos )
        return "";

    return _str.substr( nSrc, nSize );
}

size_t XString::find_last_of( const XString& xs ) const
{
    return _str.find_last_of( xs.toWString() );
}

size_t XString::find( const XString& xs ) const
{
    return _str.find( xs.toWString() );
}

size_t XString::rfind( const XString& xs ) const
{
    return _str.rfind( xs.toWString() );
}

const wchar_t* XString::c_str() const
{
    return _str.c_str();
}

std::vector<XString> XString::split( const XString& sSplit ) const
{
    int nCnt = count( sSplit );
    size_t nSize = sSplit.size();

    std::vector<XString> vecList;
    vecList.reserve( nCnt );

    std::string::size_type st = _str.find( sSplit.toWString() );
    std::string::size_type nCurrent = 0;

    while( st != std::string::npos )
    {
        vecList.push_back( _str.substr( nCurrent, st - nCurrent ) );

        nCurrent = st + nSize;
        st = _str.find( sSplit.toWString(), st + nSize );
    }

    return vecList;
}

XString XString::replaceAll( const XString& xa, const XString& xb ) const
{
    if( xa.IsEmpty() == true )
        return _str;

    std::wstring tmp = _str;

    for( size_t idx = tmp.find( xa.toWString() ); idx != std::wstring::npos; idx = tmp.find( xa.toWString(), idx + xb.size() ) )
    {
        tmp.replace( idx, xa.size(), xb.toWString() );
    }

    return tmp;
}

XString XString::replace( const XString& xa, const XString& xb ) const
{
    std::wstring tmp = _str;
    size_t pos = tmp.find( xa.toWString() );

    if( pos != std::string::npos )
        tmp.replace( pos, xa.size(), xb.toWString() );

    return tmp;
}

int XString::compare( const XString& xs, bool isCaseInsensitive /*= false*/ ) const
{
    if( isCaseInsensitive == true )
    {
        const std::wstring xa{ toLower() };
        const std::wstring xb{ xs.toLower() };

        return xa.compare( xb );
    }
    else
    {
        return _str.compare( xs.toWString() );
    }
}

bool XString::contains( const XString& xs, bool isCaseInsensitive /*= false*/ ) const
{
    if( isCaseInsensitive == true )
    {
        const std::wstring tmp{ toLower() };
        return tmp.find( xs.toLower().toWString() ) != std::string::npos;
    }
    return _str.find( xs.toWString() ) != std::string::npos;
}

bool XString::startswith( const XString& xs, bool isCaseInsensitive /*= false*/ ) const
{
    if( isCaseInsensitive == true )
    {
        const std::wstring tmp{ toLower() };
        return tmp.starts_with( xs.toLower() );
    }
    return _str.starts_with( xs );
}

bool XString::endswith( const XString& xs, bool isCaseInsensitive /*= false*/ ) const
{
    if( isCaseInsensitive == true )
    {
        const std::wstring tmp{ toLower() };
        return tmp.ends_with( xs.toLower() );
    }
    return _str.ends_with( xs );
}

void XString::clear()
{
    _str.clear();
}

std::wstring XString::s2ws( const std::string& s )
{
    return Ext::Convert::s2ws( s );
}

std::wstring XString::c2ws( const char* c )
{
    if( c == NULL )
        return L"";

    return Ext::Convert::s2ws( std::string( c ) );
}
