#include <sqlite3.h> 

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <unordered_set>
#include <string>
#include <cassert>
#include <thread>
#include <mutex>



sqlite3_stmt* prepare_query(sqlite3 * db, const char * const sql)
{
  sqlite3_stmt * prepared;
 
  int rc = sqlite3_prepare_v2(db, sql, -1, &prepared, NULL);
 
  if(rc != SQLITE_OK) {
    std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
    exit(2);
  }
 
  return prepared;
}

void reset_query(sqlite3 * db, sqlite3_stmt * prepared)
{
  int rc = sqlite3_reset(prepared);

  if(rc != SQLITE_OK) {
    std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
    exit(2);
  }
}

std::vector<std::string> read_data(sqlite3 * db, sqlite3_stmt * prepared)
{
  std::vector<std::string> ret;
  reset_query(db, prepared);

  int rc = sqlite3_step(prepared);

  while (rc == SQLITE_ROW) {
    ret.push_back( (const char *)sqlite3_column_text(prepared, 1) ); 
    rc = sqlite3_step(prepared);
  }

  if (rc != SQLITE_DONE) {
    std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
    exit(2);
  }

  return ret;
}

sqlite3* open_database(const char * dbname)
{
  sqlite3 *db;
  int rc = sqlite3_open(dbname, &db);
  if( rc ){
    std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
    exit(1);
  }
  return db;
}

void process_data(sqlite3* db_, sqlite3_stmt* stmt_)
{
  std::vector<std::string> d = read_data(db_, stmt_);
  for (const auto & s : d) {
    std::cout << s << std::endl;
  }
}

void process()
{
  sqlite3* db_ = open_database("test1.db");
  sqlite3_stmt * stmt_ = prepare_query(db_, "select * from company");

  process_data(db_, stmt_); 

  sqlite3_finalize(stmt_);

  sqlite3_close(db_);
}

int main()
{
  process();
  return 0;
}

