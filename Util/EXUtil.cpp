#include "stdafx.h"
#include "EXUtil.hpp"

#include "UUID/uuid_v4.h"

std::wstring Ext::Util::CreateGUID( eCaseType eCase /*= CASE_TYPE_NONE*/ )
{
    UUIDv4::UUIDGenerator<std::mt19937_64> uuid;
    std::string sUUID = uuid.getUUID().str();

    if( eCase == CASE_TYPE_UPPER )
        std::ranges::transform(sUUID, sUUID.begin(), ::toupper );

    return std::basic_string<wchar_t>( sUUID.begin(), sUUID.end() );
}
