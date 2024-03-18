#include "stdafx.h"
#include "EXFile.hpp"

#include <filesystem>

///////////////////////////////////////////////////////////////////////////

bool Ext::File::IsExistFile( const XString& sFileFullPath )
{
    return std::filesystem::exists( sFileFullPath.toWString() );
}

bool Ext::File::IsExistDir( const XString& sDirPath )
{
    return std::filesystem::is_directory( sDirPath.toWString() );
}

///////////////////////////////////////////////////////////////////////////

