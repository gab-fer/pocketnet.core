// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_BASEREPOSITORY_H
#define POCKETDB_BASEREPOSITORY_H

#include <utility>
#include <util/system.h>
#include <unordered_map>
#include <boost/algorithm/string/replace.hpp>

#include "shutdown.h"
#include "pocketdb/SQLiteDatabase.h"
#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/stmt.h"

namespace PocketDb
{
    using namespace std;
    using namespace PocketHelpers;

    struct Pagination {
        int TopHeight;
        int PageStart;
        int PageSize;
        string OrderBy;
        bool OrderDesc;
    };

    class BaseRepository
    {
    private:
        unordered_map<string, shared_ptr<Stmt>> _statements;

    protected:
        SQLiteDatabase& m_database;

        string EscapeValue(string value);

        UniValue JSONError(int code, const string& message)
        {
            UniValue error(UniValue::VOBJ);
            error.pushKV("code", code);
            error.pushKV("message", message);
            return error;
        }

        // General method for SQL operations
        // Locked with shutdownMutex
        // Timeouted for ReadOnly connections
        template<typename T>
        void SqlTransaction(const string& func, T execute)
        {
            try
            {
                int64_t nTime1 = GetTimeMicros();

                if (!m_database.BeginTransaction())
                    throw JSONError(500, strprintf("%s: can't begin transaction\n", func));

                // We are running SQL logic with timeout only for read-only connections
                if (m_database.IsReadOnly())
                {
                    auto timeoutValue = chrono::seconds(gArgs.GetArg("-sqltimeout", 10));
                    run_with_timeout(
                        [&]()
                        {
                            execute();
                        },
                        timeoutValue,
                        [&]()
                        {
                            m_database.InterruptQuery();
                            LogPrintf("Function `%s` failed with execute timeout\n", func);
                            throw JSONError(408, "Sql request timeout");
                        }
                    );
                }
                else
                {
                    execute();
                }
                
                if (!m_database.CommitTransaction())
                    throw JSONError(500, strprintf("%s: can't commit transaction\n", func));

                int64_t nTime2 = GetTimeMicros();

                BenchLog(func, 0.001 * (nTime2 - nTime1));
            }
            catch (const exception& ex)
            {
                m_database.AbortTransaction();
                throw JSONError(500, ex.what());
            }
        }

        void SqlTransaction(const string& func, const function<Stmt&()>& prepare, const function<void(Stmt&)>& execute, bool logForce = false)
        {
            try
            {
                bool timeouted = false;
                int64_t nTime1 = GetTimeMicros();

                if (!m_database.BeginTransaction())
                    throw JSONError(500, strprintf("%s: can't begin transaction\n", func));

                // Prepare transaction binds
                auto& stmt = prepare();

                if (logForce || gArgs.GetChainName() == CBaseChainParams::REGTEST)
                    LogPrintf("Debug logging sql query for `%s`:\n%s\n", func, stmt.Log());

                // We are running SQL logic with timeout only for read-only connections
                if (m_database.IsReadOnly())
                {
                    auto timeoutValue = chrono::seconds(gArgs.GetArg("-sqltimeout", 10));

                    run_with_timeout(
                        [&]()
                        {
                            execute(stmt);
                        },
                        timeoutValue,
                        [&]()
                        {
                            m_database.InterruptQuery();
                            LogPrintf("Warning! Function `%s` failed with execute timeout:\n%s\n", func, stmt.Log());
                            timeouted = true;
                        }
                    );
                }
                else
                {
                    execute(stmt);
                }
                
                if (!m_database.CommitTransaction())
                    throw JSONError(500, strprintf("%s: can't commit transaction\n", func));

                int64_t nTime2 = GetTimeMicros();
                BenchLog(func, 0.001 * (nTime2 - nTime1));

                if (m_database.IsReadOnly() && timeouted)
                    throw JSONError(408, "Sql request timeout");
            }
            catch (const UniValue& objError)
            {
                m_database.AbortTransaction();
                throw JSONError(500, objError.write());
            }
            catch (const exception& ex)
            {
                m_database.AbortTransaction();
                throw JSONError(500, ex.what());
            }
        }

        Stmt SqlSingleton(const string& sql);

        Stmt& Sql(const string& sql);

        void SetLastInsertRowId(int64_t value);

        int64_t GetLastInsertRowId();

        void BenchLog(const string& func, double time);

    public:

        explicit BaseRepository(SQLiteDatabase& db) : m_database(db)
        {
        }

        virtual ~BaseRepository() = default;

        virtual void Init() { }

        virtual void Destroy()
        {
            _statements.clear();
        }
    };
}

#endif //POCKETDB_BASEREPOSITORY_H