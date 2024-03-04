#include "stdafx.h"
#include "EXUtil.hpp"

#include "uuid_v4.h"

std::wstring Ext::Util::CreateGUID( bool isUpper /*= false*/ )
{
    UUIDv4::UUIDGenerator<std::mt19937_64> uuid;
    std::string sUUID = uuid.getUUID().str();

    if( isUpper == true )
        std::transform( sUUID.begin(), sUUID.end(), sUUID.begin(), ::toupper );

    return std::wstring( sUUID.begin(), sUUID.end() );
}
