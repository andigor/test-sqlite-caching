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
#include <atomic>


std::mutex mut;
std::atomic_int atom;

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
//  reset_query(db, prepared);
  int rc;
  size_t counter = 0;
  while (true) {
//    mut.lock();
    int expected = 0;
    while (!atom.compare_exchange_weak(
          expected
          ,1 
          ,std::memory_order_acquire 
          ,std::memory_order_relaxed
          ) ) {
      expected = 0;
    }
    ++counter;
    rc = sqlite3_step(prepared);
    if (rc != SQLITE_ROW)
    {
 //     mut.unlock();
      atom.store(0);
      break;
    }
    const char * txt = (const char *)sqlite3_column_text(prepared, 1);
    ret.push_back( txt  ); 
    atom.store(0, std::memory_order_release);
  //  mut.unlock();
  }

  if (rc != SQLITE_DONE) {
    std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
    exit(2);
  }
 
  std::cout << "counter: " << counter << std::endl;
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

void process_data(sqlite3* db_, sqlite3_stmt* stmt_, size_t & s)
{
  std::vector<std::string> d = read_data(db_, stmt_);
  s = d.size();
  //std::cout << "size: " << d.size() << std::endl;
//  for (const auto & s : d) {
//    std::cout << s << std::endl;
//  }
}

void process(size_t & s)
{
  sqlite3* db_ = open_database("test1.db");
  sqlite3_stmt * stmt_ = prepare_query(db_, "select * from company");

  process_data(db_, stmt_, s); 

  sqlite3_finalize(stmt_);

  sqlite3_close(db_);
}

void process_mt()
{
  sqlite3* db_ = open_database("test1.db");
  sqlite3_stmt * stmt_ = prepare_query(db_, "select * from company");

  size_t s1, s2;
  std::thread thr1(process_data, db_, stmt_, std::ref(s1));
  std::thread thr2(process_data, db_, stmt_, std::ref(s2));

  thr1.join();
  thr2.join();

  std::cout << "s1: " << s1 << " s2: " << s2 << " sum: " << s1 + s2 << std::endl;

  sqlite3_finalize(stmt_);

  sqlite3_close(db_);
}

int main()
{
  process_mt();
  return 0;
}

