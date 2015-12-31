#include <sqlite3.h> 

#include <iostream>

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

void process()
{
   sqlite3 *db;

   int rc = sqlite3_open("test.db", &db);

   if( rc ){
     std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
     exit(1);
   }
   
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

   sqlite3_close(db);
}



int main()
{
  process();
  return 0;
}
