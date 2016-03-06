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
void exec_query(sqlite3 * db, const char * sql)
{
   char *zErrMsg = 0;
   int rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
     std::cerr << "SQL error: " << zErrMsg << std::endl;
     sqlite3_free(zErrMsg);
     exit(2);
   }
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

void process()
{
  sqlite3* db_ = open_database("test1.db");

  exec_query(db_, "SELECT * from COMPANY"); 

  sqlite3_close(db_);
}

int main()
{

  return 0;
}
