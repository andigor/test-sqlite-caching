#include <sqlite3.h> 

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <unordered_set>
#include <string>

#include <stdlib.h>

#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/reg.h>   /* For constants ORIG_EAX etc */

class SqlModelling 
{
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

  struct CallbackData
  {
    CallbackData(std::unordered_set<std::string>& d, sqlite3* db) 
      : tortured_databases_(d), db_(db)
    {}

    std::unordered_set<std::string> & tortured_databases_;
    sqlite3* db_;
  };

  static void trace_callback(void * arg, const char * query)
  {
    CallbackData * d = static_cast<CallbackData*>(arg);
    std::string n(sqlite3_db_filename(d->db_, "main"));
    d->tortured_databases_.insert(n);
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

  public:
  std::unordered_set<std::string> process()
  {
    std::srand(std::time(0));

    std::vector<CallbackData> dbs;
    std::unordered_set<std::string> tortured_databases;

    dbs.emplace_back(CallbackData(tortured_databases, open_database("test1.db")));
    dbs.emplace_back(CallbackData(tortured_databases, open_database("test2.db")));
    dbs.emplace_back(CallbackData(tortured_databases, open_database("test3.db")));
    dbs.emplace_back(CallbackData(tortured_databases, open_database("test4.db")));
    dbs.emplace_back(CallbackData(tortured_databases, open_database("test5.db")));

    for (size_t i = 0; i<dbs.size(); ++i) {
      sqlite3_trace(dbs[i].db_, trace_callback, &dbs[i]);
    }

    for (size_t i = 0; i<dbs.size(); ++i) {
      torture_database(dbs[std::rand() % dbs.size()].db_);  
    }

    for (size_t i = 0; i<dbs.size(); ++i) {
      sqlite3_close(dbs[i].db_);
    }
    return tortured_databases;
  }
};

int main()
{

#ifdef FORKING_ENABLED
  pid_t child = fork();
  if (child == 0) {
#ifdef TRACING_ENABLED    
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    raise(SIGSTOP);
#endif
#endif
    SqlModelling modelling;
    std::unordered_set<std::string> tortured = modelling.process();
    for (const auto & s : tortured) {
      std::cout << s << std::endl;
    }
#ifdef FORKING_ENABLED    
  }
  else {
#ifdef TRACING_ENABLED    
    waitpid(-1, NULL, __WALL);

    ptrace(PTRACE_ATTACH, child, 0, 0);
    ptrace(PTRACE_SETOPTIONS, child, NULL, PTRACE_O_TRACEFORK 
                                         | PTRACE_O_TRACEVFORK 
                                         | PTRACE_O_TRACECLONE 
                                         | PTRACE_O_TRACEEXIT 
                                         | PTRACE_O_EXITKILL);

    pid_t pid = child;
    while (true) {
      int status;

      ptrace(PTRACE_SYSCALL, pid, 0, 0);
      pid = waitpid(-1, &status, __WALL);

      if(WIFEXITED(status))
      { 
        break;
      }
      else if (WIFSTOPPED(status))
      {   
          struct user_regs_struct regs;
          ptrace(PTRACE_GETREGS, pid, 0, &regs);
          if (regs.rax == (unsigned long)-ENOSYS) { //syscall_enter stop
            if (regs.orig_rax == __NR_open) {
              std::string file_name;
              long word = ptrace(PTRACE_PEEKDATA, pid, regs.rdi, NULL);
              char * tmp = (char *)&word;
              while (*tmp != '\0') {
                file_name.push_back(*tmp);
                ++tmp;
                if (tmp == (char *)(&word + 1)) {
                  regs.rdi += sizeof(word);
                  word = ptrace(PTRACE_PEEKDATA, pid, regs.rdi, NULL);
                  tmp = (char *)&word;
                }
                }
              }
              std::cout << "file_name: " << file_name << std::endl;
            }
          }
          else { //syscall_exit stop
          }
      }
    }
#else
    for (;;) {
      int status;
      waitpid(child, &status, __WNOTHREAD);
      if (WIFEXITED(status)) {
        break;
      }
    }
#endif
  }
#endif

  return 0;
}
