#pragma once

#ifndef __HDR_EXT_SQLITE__
#define __HDR_EXT_SQLITE__

#include "EXString.hpp"
#include "Base/Singleton.hpp"
#include "SQLite/include/sqlite3.h"

namespace Ext
{
    namespace SQLite
    {
        class cStmt
        {
        public:
            cStmt();
            cStmt( sqlite3_stmt* stmt );
            ~cStmt();

            cStmt& operator = ( const cStmt& rhs );

            sqlite3_stmt*                          Get() const { return _stmt.get(); }
            int                                    GetColumnCnt() const { return _nColumnCnt; }

            void                                   SetColumnCnt( int nCnt ) { _nColumnCnt = nCnt; }
            void                                   SetDB( sqlite3* db ) { _db = db; }

            bool                                   Next();
            XString                                Value( const XString& sColumnName );

        private:
            std::shared_ptr<sqlite3_stmt>          _stmt;
            int                                    _nColumnCnt = 0;
            sqlite3*                               _db;
        };

        class cDBTransaction
        {
        public:
            cDBTransaction( sqlite3* db );
            ~cDBTransaction();

            bool                                   Begin();
            bool                                   Commit();
            bool                                   RollBack();

        private:
            sqlite3* _db;
        };


        class cSQLiteDB
        {
        public:
            cSQLiteDB( sqlite3* db );
            cSQLiteDB() {}

            void                                   SetDB( sqlite3* db );
            sqlite3*                               Data();
            bool                                   PrepareSQL( cStmt& stmt, const XString& sQuery );
            bool                                   BindValue( cStmt stmt, const XString& sBind, const XString& sValue );
            bool                                   ExecuteSQL( cStmt& stmt );

        private:
            sqlite3* _db;
        };

        typedef std::shared_ptr<cSQLiteDB> spSQLiteDB;

        typedef struct _dbInfo
        {
            auto operator<=>( _dbInfo const& ) const = default;

            XString                                 sDBName;
            XString                                 sFilePath;
            XString                                 sJsonPath;

            spSQLiteDB                              spSQLite;

            bool                                    isUseWALMode = false;
        } DB_INFO;

        class cSQLiteMgr
        {
        public:
            cSQLiteMgr();

            bool                                   InitDB( XString sDBName, XString sFilePath );
            bool                                   InitDB( DB_INFO dbInfo );
            spSQLiteDB                             GetDB( XString sDBName );
            bool                                   Close( XString sDBName );


        protected:
            bool                                   IsExistDBInfo( XString sDBName );

        private:
            std::map< XString, DB_INFO >           _mapNameToInfo;
        };

    }
}

typedef Ext::Singletons::Singleton<Ext::SQLite::cSQLiteMgr> TyStSqliteMgr;

#endif