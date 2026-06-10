#pragma once

#include <cstdint>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

namespace AqualinkAutomate::History
{

	//=========================================================================
	// Minimal RAII wrappers over the SQLite C API.
	//
	// These exist so the history service never hand-manages a sqlite3* / stmt
	// lifetime: a connection closes in ~SqliteDb, a prepared statement finalises
	// in ~SqliteStmt, and a transaction rolls back in ~SqliteTransaction unless
	// Commit() was called.  Errors throw std::runtime_error carrying the SQLite
	// message; the service catches and logs them so a storage hiccup never takes
	// down the main loop.  No ORM, no third-party wrapper.
	//=========================================================================

	class SqliteDb
	{
	public:
		// Opens (creating if needed) the database at `path`. Use ":memory:" for an
		// in-memory database (tests). Throws std::runtime_error on failure.
		explicit SqliteDb(const std::string& path);
		~SqliteDb();

		SqliteDb(const SqliteDb&) = delete;
		SqliteDb& operator=(const SqliteDb&) = delete;
		SqliteDb(SqliteDb&&) = delete;
		SqliteDb& operator=(SqliteDb&&) = delete;

		sqlite3* Handle() const noexcept { return m_Db; }

		// Execute one or more semicolon-separated statements (no bound params, no
		// result rows). Throws std::runtime_error on error.
		void Exec(const std::string& sql);

		// rowid of the most recent successful INSERT on this connection.
		std::int64_t LastInsertRowId() const noexcept;

	private:
		sqlite3* m_Db{ nullptr };
	};

	class SqliteStmt
	{
	public:
		SqliteStmt(SqliteDb& db, const std::string& sql);
		~SqliteStmt();

		SqliteStmt(const SqliteStmt&) = delete;
		SqliteStmt& operator=(const SqliteStmt&) = delete;
		SqliteStmt(SqliteStmt&&) = delete;
		SqliteStmt& operator=(SqliteStmt&&) = delete;

		// 1-based parameter binding (matches SQLite's convention).
		void Bind(int index, std::int64_t value);
		void Bind(int index, double value);
		void Bind(int index, const std::string& value);

		// Advances the statement. Returns true when a row is available
		// (SQLITE_ROW), false when finished (SQLITE_DONE). Throws on error.
		bool Step();

		// Reset so the statement can be re-bound and re-stepped.
		void Reset();

		// 0-based column accessors (valid after Step() returned true).
		std::int64_t ColumnInt64(int column);
		double ColumnDouble(int column);
		std::string ColumnText(int column);

	private:
		SqliteDb& m_Db;
		sqlite3_stmt* m_Stmt{ nullptr };
	};

	// RAII transaction: BEGIN on construction, COMMIT on Commit(), otherwise
	// ROLLBACK in the destructor.
	class SqliteTransaction
	{
	public:
		explicit SqliteTransaction(SqliteDb& db);
		~SqliteTransaction();

		SqliteTransaction(const SqliteTransaction&) = delete;
		SqliteTransaction& operator=(const SqliteTransaction&) = delete;

		void Commit();

	private:
		SqliteDb& m_Db;
		bool m_Committed{ false };
	};

}
// namespace AqualinkAutomate::History
