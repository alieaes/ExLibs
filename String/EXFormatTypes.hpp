#pragma once

#ifndef __HDR_EXT_FORMAT_TYPES__
#define __HDR_EXT_FORMAT_TYPES__

#include "EXFormat.hpp"

EXT_FORMAT_TYPE_ADD_START()

EXT_FORMAT_TYPE_ADD_MAKE( double, std::to_wstring )
EXT_FORMAT_TYPE_ADD_MAKE( float, std::to_wstring )
EXT_FORMAT_TYPE_ADD_MAKE( DWORD, std::to_wstring )
EXT_FORMAT_TYPE_ADD_MAKE( UINT, std::to_wstring )
EXT_FORMAT_TYPE_ADD_MAKE( std::wstring, std::wstring )

EXT_FORMAT_TYPE_ADD_END()

#endif
