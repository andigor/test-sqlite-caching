#include <sqlite3.h> 

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>

#include <stdlib.h>

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

void torture_database(sqlite3 * db)
{
  exec_query(db,
         "CREATE TABLE IF NOT EXISTS COMPANY("  \
         "ID             INT     NOT NULL," \
         "NAME           TEXT    NOT NULL," \
         "AGE            INT     NOT NULL," \
         "ADDRESS        CHAR(50)," \
         "SALARY         REAL );"); 
   
   
  exec_query(db,
           "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "  \
           "VALUES (1, 'Paul', 32, 'California', 20000.00 ); " \
           "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "  \
           "VALUES (2, 'Allen', 25, 'Texas', 15000.00 ); "     \
           "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
           "VALUES (3, 'Teddy', 23, 'Norway', 20000.00 );" \
           "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
           "VALUES (4, 'Mark', 25, 'Rich-Mond ', 65000.00 );");

  exec_query(db, "SELECT * from COMPANY"); 
}

void trace_callback(void * , const char * query)
{
  std::cout << query << std::endl;
}

sqlite3* open_database(const char * dbname)
{
  sqlite3 *db;
  int rc = sqlite3_open(dbname, &db);
  if( rc ){
    std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
    exit(1);
  }
  sqlite3_trace(db, trace_callback, NULL);
  return db;
}


void process()
{
  std::srand(std::time(0));
  std::vector<sqlite3*> dbs;

  dbs.push_back(open_database("test1.db"));
  dbs.push_back(open_database("test2.db"));
  dbs.push_back(open_database("test3.db"));
  dbs.push_back(open_database("test4.db"));
  dbs.push_back(open_database("test5.db"));

  for (int i = 0; i<dbs.size(); ++i) {
    torture_database(dbs[std::rand() % dbs.size()]);  
  }

  for (int i = 0; i<dbs.size(); ++i)
    sqlite3_close(dbs[i]);
}



int main()
{
  process();

  return 0;
}
