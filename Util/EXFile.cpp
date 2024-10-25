#include "stdafx.h"
#include "EXFile.hpp"

#include <filesystem>

///////////////////////////////////////////////////////////////////////////

namespace Ext::File
{
    bool IsExistFile( const XString& sFileFullPath )
    {
        return std::filesystem::exists( sFileFullPath.toWString() );
    }

    bool IsExistDir( const XString& sDirPath )
    {
        return std::filesystem::is_directory( sDirPath.toWString() );
    }

    XString NormalizePath( const XString& sFullPath )
    {
#if defined(WIN32) || defined(_WIN32) 
        XString sSeparator = "\\";
        XString sSeparatorTarget = "/";
#else 
        XString sSeparator = "/";
        XString sSeparatorTarget = "\\";
#endif
        XString sNormalizePath = sFullPath.replaceAll( sSeparatorTarget, sSeparator );

        if( sNormalizePath.endswith( sSeparator ) == true )
            sNormalizePath = sNormalizePath.substr( 0, sNormalizePath.size() - 2 );

        return sNormalizePath;
    }

    cExtFile::cExtFile()
    {
    }

    cExtFile::cExtFile( const XString& sFullPath )
    {
        reset();

        _sFullPath = sFullPath;
        devidePath( sFullPath );
    }

    void cExtFile::SetPath( const XString& sFullPath )
    {
        reset();

        _sFullPath = sFullPath;
        devidePath( sFullPath );
    }

    bool cExtFile::IsExist()
    {
        return std::filesystem::exists( FullPath().toWString() );
    }

    XString cExtFile::Path()
    {
        return _sFilePath;
    }

    XString cExtFile::Name()
    {
        return _sFileName;
    }

    XString cExtFile::FullPath()
    {
        if( _sFileName.IsEmpty() == true && _sFilePath.IsEmpty() == true )
            return "";
        else if( _sFileName.IsEmpty() == true )
            return _sFilePath;
        else
            return _sFilePath + EXT_PATH_SEPARATOR + _sFileName;
    }

    void cExtFile::reset()
    {
        _isExist = false;
        _eType = FILE_TYPE_NONE;

        _sFilePath.clear();
        _sFileName.clear();
        _sFullPath.clear();
    }

    void cExtFile::devidePath()
    {
        devidePath( _sFullPath );
    }

    void cExtFile::devidePath( const XString& sFullPath )
    {
        XString sNormalizePath = NormalizePath( sFullPath );
        _sFullPath = sFullPath;

        if( std::filesystem::exists( sFullPath.toWString() ) == false )
        {
            _isExist = false;
            return;
        }

        if( std::filesystem::is_directory( sNormalizePath.toWString() ) == true )
        {
            _sFilePath = sFullPath;
            _sFileName.clear();
            _eType = FILE_TYPE_DIR;
        }
        else
        {
            auto fnDividePath = [ this, &sNormalizePath ]() {
                auto pos = sNormalizePath.rfind( EXT_PATH_SEPARATOR );
                if( pos != std::string::npos )
                {
                    _sFilePath = sNormalizePath.substr( 0, pos - 1 );
                    _sFileName = sNormalizePath.substr( pos, std::string::npos );
                }
            };

            if( std::filesystem::is_regular_file( sNormalizePath.toWString() ) == true )
            {
                _eType = FILE_TYPE_FILE;
                fnDividePath();
            }
            else if( std::filesystem::is_character_file( sNormalizePath.toWString() ) == true )
            {
                _eType = FILE_TYPE_CHARACTER;
                fnDividePath();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////

