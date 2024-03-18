#include "stdafx.h"
#include "EXSQLite.hpp"

#include <fstream>

#include "json.hpp"
#include "Base/ExtConfig.hpp"
#include "String/EXFormat.hpp"
#include "Util/EXFile.hpp"

Ext::SQLite::cStmt::cStmt()
    : _stmt( NULLPTR )
{
}

Ext::SQLite::cStmt::cStmt( sqlite3_stmt* stmt )
    : _stmt( stmt )
{
}

Ext::SQLite::cStmt::~cStmt()
{
    if( _stmt != NULLPTR )
        sqlite3_reset( _stmt.get() );

    if( _stmt != NULLPTR )
        _stmt.reset();
}

Ext::SQLite::cStmt& Ext::SQLite::cStmt::operator=( const Ext::SQLite::cStmt& rhs )
{
    _stmt = rhs._stmt;
    return *this;
}

bool Ext::SQLite::cStmt::Next()
{
    bool isSuccess = false;
    int rc = 0;

    do
    {
        if( _db == NULLPTR )
            break;

        // TODO : LOCK 등에 의해 값을 가져올 수 없는 상황을 대비해야 함.
        rc = sqlite3_step( Get() );

        if( rc == SQLITE_ROW )
            isSuccess = true;

        if( rc == SQLITE_OK )
            isSuccess = true;

    } while( false );

    return isSuccess;
}

XString Ext::SQLite::cStmt::Value( const XString& sColumnName )
{
    XString sRet;
    int rc = 0;

    do
    {
        if( _db == NULLPTR )
            break;

        for( int nIdx = 0; nIdx < GetColumnCnt(); ++nIdx )
        {
            XString s = sqlite3_column_name( Get(), nIdx );
            if( _wcsicmp( s.c_str(), sColumnName.c_str() ) == 0 )
            {
                // wstring NULLPTR 예외를 피하기 위함, 좋은 방법이 있다면 바꾸도록 해야함
                auto s = sqlite3_column_text( Get(), nIdx );
                if( s == NULL )
                    break;

                sRet = static_cast< const wchar_t* >( sqlite3_column_text16( Get(), nIdx ) );
                break;
            }
        }

    } while( false );

    return sRet;
}

///////////////////////////////////////////////////////////////////////////


Ext::SQLite::cDBTransaction::cDBTransaction( sqlite3* db )
{
    _db = db;
    Begin();
}

Ext::SQLite::cDBTransaction::~cDBTransaction()
{
    Commit();
}

bool Ext::SQLite::cDBTransaction::Begin()
{
    bool isSuccess = false;
    int rc = 0;

    do
    {
        if( _db == NULLPTR )
            break;

        char* cErr = 0;
        rc = sqlite3_exec( _db, "BEGIN TRANSACTION", NULL, NULL, &cErr );

        XString sErr = sqlite3_errmsg( _db );
        if( rc == SQLITE_ROW || rc == SQLITE_OK || rc == SQLITE_DONE )
            isSuccess = true;

    } while( false );

    return isSuccess;
}

bool Ext::SQLite::cDBTransaction::Commit()
{
    bool isSuccess = false;
    int rc = 0;

    do
    {
        if( _db == NULLPTR )
            break;

        char* cErr = 0;
        rc = sqlite3_exec( _db, "COMMIT", NULL, NULL, &cErr );

        XString sErr = sqlite3_errmsg( _db );
        if( rc == SQLITE_ROW || rc == SQLITE_OK || rc == SQLITE_DONE )
            isSuccess = true;

    } while( false );

    return isSuccess;
}

bool Ext::SQLite::cDBTransaction::RollBack()
{
    bool isSuccess = false;
    int rc = 0;

    do
    {
        if( _db == NULLPTR )
            break;

        char* cErr = 0;
        rc = sqlite3_exec( _db, "ROLLBACK", NULL, NULL, &cErr );

        XString sErr = sqlite3_errmsg( _db );
        if( rc == SQLITE_ROW || rc == SQLITE_OK || rc == SQLITE_DONE )
            isSuccess = true;

    } while( false );

    return isSuccess;
}


Ext::SQLite::cSQLiteDB::cSQLiteDB( sqlite3* db )
{
    _db = db;
}

void Ext::SQLite::cSQLiteDB::SetDB( sqlite3* db )
{
    _db = db;
}

sqlite3* Ext::SQLite::cSQLiteDB::Data()
{
    return _db;
}

bool Ext::SQLite::cSQLiteDB::PrepareSQL( cStmt& stmt, const XString& sQuery )
{
    bool isSuccess = false;
    int rc = 0;
    auto s = stmt.Get();

    do
    {
        if( _db == NULLPTR )
            break;

        rc = sqlite3_prepare16_v2( _db, sQuery.c_str(), -1, &s, NULL );
        XString sErr = sqlite3_errmsg( _db );

        if( rc != SQLITE_OK )
            break;

        stmt = cStmt( s );

        isSuccess = true;

    } while( false );

    return isSuccess;
}

bool Ext::SQLite::cSQLiteDB::BindValue(Ext::SQLite::cStmt stmt, const XString& sBind, const XString& sValue )
{
    bool isSuccess = false;
    int rc = 0;

    do
    {
        if( _db == NULLPTR )
            break;

        auto idx = sqlite3_bind_parameter_index( stmt.Get(), sBind.toString().c_str() );
        // https://zoningout.tistory.com/121
        rc = sqlite3_bind_text16( stmt.Get(), idx, sValue.toWString().c_str(), -1, SQLITE_TRANSIENT );

        if( rc != SQLITE_OK )
            break;

        isSuccess = true;

    } while( false );

    return isSuccess;
}

bool Ext::SQLite::cSQLiteDB::ExecuteSQL( cStmt& stmt )
{
    bool isSuccess = false;
    int rc = 0;
    auto s = stmt.Get();

    do
    {
        if( _db == NULLPTR )
            break;

        rc = sqlite3_step( s );

        XString sErr = sqlite3_errmsg( _db );
        if( rc == SQLITE_ROW || rc == SQLITE_OK || rc == SQLITE_DONE )
            isSuccess = true;

        rc = sqlite3_reset( s );

        stmt.SetColumnCnt( sqlite3_column_count( s ) );
        stmt.SetDB( _db );

    } while( false );

    return isSuccess;
}

Ext::SQLite::cSQLiteMgr::cSQLiteMgr()
{
}

bool Ext::SQLite::cSQLiteMgr::InitDB( XString sDBName, XString sFilePath )
{
    bool isSuccess = false;

    do
    {

        DB_INFO dbInfo;
        dbInfo.sDBName = sDBName;
        dbInfo.sFilePath = sFilePath;

        isSuccess = InitDB( dbInfo );

    } while( false );

    return isSuccess;
}

bool Ext::SQLite::cSQLiteMgr::InitDB( DB_INFO dbInfo )
{
    bool isSuccess = false;

    do
    {
        if( dbInfo.sFilePath.IsEmpty() == true || dbInfo.sDBName.IsEmpty() == true )
            break;

        if( IsExistDBInfo( dbInfo.sDBName ) == true )
        {
            isSuccess = true;
            break;
        }

        bool isExist = false;

        // TODO : 파일이 존재하는 경우엔 Migration을 진행하도록 함
        if( File::IsExistFile( dbInfo.sFilePath ) == true )
            isExist = true;

        sqlite3* db;
        sqlite3_stmt* res;

        int rc = sqlite3_open16( dbInfo.sFilePath.c_str(), &db );

        if( rc != SQLITE_OK )
        {
            sqlite3_close( db );
            break;
        }

        spSQLiteDB sp = spSQLiteDB( new cSQLiteDB );
        sp->SetDB( db );
        dbInfo.spSQLite = sp;
        _mapNameToInfo[ dbInfo.sDBName ] = dbInfo;

        nlohmann::basic_json<> JsonDB;

        if( dbInfo.isUseWALMode == true )
        {
            cStmt stmt;
            XString sQuery = "PRAGMA journal_mode=WAL;";

            sp->PrepareSQL( stmt, sQuery );
            sp->ExecuteSQL( stmt );
        }

        if( File::IsExistFile( dbInfo.sJsonPath ) == true )
        {
            std::ifstream i;
            i.open( dbInfo.sJsonPath.toWString() );

            nlohmann::json json;

            i >> json;

            JsonDB = json[ dbInfo.sDBName ];
        }

        if( JsonDB.is_null() == true )
            break;

        if( isExist == false )
        {
            cDBTransaction t( sp->Data() );
            cStmt stmt;
            XString sQuery = "PRAGMA encoding = \"UTF-16le\";";

            sp->PrepareSQL( stmt, sQuery );
            sp->ExecuteSQL( stmt );

            sQuery = "CREATE TABLE TBL_DEFAULT( id TEXT NOT NULL, rev TEXT NOT NULL );";

            sp->PrepareSQL( stmt, sQuery );
            sp->ExecuteSQL( stmt );

            auto JsonCreate = JsonDB[ "Create" ];
            int nRev = JsonCreate[ "Rev" ].get<int>();

            for( int idx = 1; idx <= nRev; idx++ )
            {
                XString sQueryTmp = JsonCreate[ std::to_string( idx ) ].get<std::string>();
                sp->PrepareSQL( stmt, sQueryTmp );
                sp->ExecuteSQL( stmt );
            }

            sQuery = Ext::Format::Format( L"INSERT OR REPLACE INTO TBL_DEFAULT(id, rev) VALUES( \"{}\", \"{}\" ) ", dbInfo.sDBName, nRev );

            sp->PrepareSQL( stmt, sQuery );
            sp->ExecuteSQL( stmt );
        }

        isSuccess = true;

    } while( false );

    return isSuccess;
}

Ext::SQLite::spSQLiteDB Ext::SQLite::cSQLiteMgr::GetDB( XString sDBName )
{
    Ext::SQLite::spSQLiteDB db = NULLPTR;

    if( _mapNameToInfo.count( sDBName ) > 0 )
        db = _mapNameToInfo[ sDBName ].spSQLite;

    return db;
}

bool Ext::SQLite::cSQLiteMgr::Close( XString sDBName )
{
    bool isSuccess = false;
    auto db = GetDB( sDBName );

    do
    {
        if( db != NULLPTR )
            isSuccess = sqlite3_close( db->Data() ) == SQLITE_OK;

    } while( false );

    return isSuccess;
}

bool Ext::SQLite::cSQLiteMgr::IsExistDBInfo( XString sDBName )
{
    return _mapNameToInfo.count( sDBName ) > 0 ? true : false;
}