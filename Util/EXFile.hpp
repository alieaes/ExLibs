#pragma once

#ifndef __HDR_EXT_UTIL_FILE__
#define __HDR_EXT_UTIL_FILE__

#include "EXString.hpp"
#include "EXUtil.hpp"

#if defined(WIN32) || defined(_WIN32) 
    #define EXT_PATH_SEPARATOR "\\" 
#else 
    #define EXT_PATH_SEPARATOR "/" 
#endif

namespace Ext
{
    namespace File
    {
        bool                                     IsExistFile( const XString& sFileFullPath );
        bool                                     IsExistDir( const XString& sDirPath );
        XString                                  NormalizePath( const XString& sFullPath );

        enum EXT_FILE_TYPE
        {
            FILE_TYPE_NONE      = 0,
            FILE_TYPE_FILE      = 1,
            FILE_TYPE_DIR       = 2,
            FILE_TYPE_SYMLINK   = 3,
            FILE_TYPE_CHARACTER = 4
        };

        EXT_ENUM_START( EXT_FILE_TYPE )
        EXT_ENUM_ADD( FILE_TYPE_NONE,      "알수없음" )
        EXT_ENUM_ADD( FILE_TYPE_FILE,      "파일" )
        EXT_ENUM_ADD( FILE_TYPE_DIR,       "디렉토리" )
        EXT_ENUM_ADD( FILE_TYPE_SYMLINK,   "심볼릭 링크" )
        EXT_ENUM_ADD( FILE_TYPE_CHARACTER, "문자 장치 파일" )
        EXT_ENUM_END( EXT_FILE_TYPE )

        class cExtFile
        {
        public:
            cExtFile();
            cExtFile( const XString& sFullPath );

            void                                     SetPath( const XString& sFullPath );
            bool                                     IsExist();
            XString                                  Path();
            XString                                  Name();
            XString                                  FullPath();

        private:
            void                                     reset();

            void                                     devidePath();
            void                                     devidePath( const XString& sFullPath );

        private:
            // 존재여부는 IsExist로 확인하고, 이건 매번 확인 시 캐시값으로 남겨둠.
            bool                                     _isExist = false;
            EXT_FILE_TYPE                            _eType = FILE_TYPE_NONE;

            XString                                  _sFullPath;
            XString                                  _sFilePath;
            XString                                  _sFileName;
        };
    }
}

#endif
