#include "stdafx.h"
#include "EXString.hpp"

#include <algorithm>
#include <codecvt>
#include <Windows.h>

#ifdef USING_QTLIB
#include <qdatetime.h>
#endif

#pragma warning(disable:4996)

std::string XString::toString() const
{
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> cvt;

    auto tmp = cvt.to_bytes( _str );
    return tmp;
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
    int n = 0;
    try
    {
        n= std::stoi( _str );
    }
    catch( [[maybe_unused]] const std::invalid_argument& ia )
    {
    }

    return n;
}

long XString::toLong() const
{
    return std::stol( _str );
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
    std::wstring tmp = _str;

    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    std::string string = converter.to_bytes( tmp );
    std::vector< char > ret( string.begin(), string.end() );

    return ret;
}

bool XString::IsEmpty() const
{
    return _str.empty();
}

bool XString::IsDigit() const
{
    bool isDigit = false;
    int n = toInt();

    do
    {
        if( n == 0 )
        {
            if( compare( "0" ) == 0 )
            {
                isDigit = true;
                break;
            }

            isDigit = false;
        }
        else
        {
            isDigit = true;
        }
    }
    while( false );

    return isDigit;
}

bool XString::Endl() const
{
    return _nPos > size() ? true : false;
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
            idx = sizeFind + 1;
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
    if( nSrc == std::string::npos || nSize == std::string::npos )
        return "";

    return _str.substr( nSrc, nSize );
}

size_t XString::find_last_of( XString xs ) const
{
    return _str.find_last_of( xs.toWString() );
}

size_t XString::find( XString xs ) const
{
    return _str.find( xs.toWString() );
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
    std::wstring tmp = _str;

    for( size_t idx = tmp.find( xa.toWString() ); idx >= 0; idx = tmp.find( xa.toWString() ) )
    {
        if( idx == std::string::npos )
            break;

        tmp.replace( idx, xa.size(), xb.toWString() );
    }

    return tmp;
}

XString XString::replace( const XString& xa, const XString& xb ) const
{
    std::wstring tmp = _str;
    tmp.replace( tmp.find( xa.toWString() ), xa.size(), xb.toWString() );

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
        return tmp.find( xs.toLower() ) != std::string::npos;
    }
    return _str.find( xs.toLower() ) != std::string::npos;
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
    _nPos = 0;
}
