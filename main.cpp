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

#include <stdlib.h>

#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/reg.h>   /* For constants ORIG_EAX etc */
int status1;
int status2;

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
    //exec_query(db,
    //       "CREATE TABLE IF NOT EXISTS COMPANY("  \
    //       "ID             INT     NOT NULL," \
    //       "NAME           TEXT    NOT NULL," \
    //       "AGE            INT     NOT NULL," \
    //       "ADDRESS        CHAR(50)," \
    //       "SALARY         REAL );"); 
    // 
    // 
    //exec_query(db,
    //         "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "  \
    //         "VALUES (1, 'Paul', 32, 'California', 20000.00 ); " \
    //         "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "  \
    //         "VALUES (2, 'Allen', 25, 'Texas', 15000.00 ); "     \
    //         "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
    //         "VALUES (3, 'Teddy', 23, 'Norway', 20000.00 );" \
    //         "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
    //         "VALUES (4, 'Mark', 25, 'Rich-Mond ', 65000.00 );");

    exec_query(db, "SELECT * from COMPANY"); 
    exec_query(db, "SELECT * from COMPANY"); 
  }

  struct CallbackData
  {
    CallbackData(std::vector<std::string>& d, sqlite3* db) 
      : tortured_databases_(d), db_(db)
    {}

    std::vector<std::string> & tortured_databases_;
    sqlite3* db_;
  };

  static void trace_callback(void * arg, const char * query)
  {
    CallbackData * d = static_cast<CallbackData*>(arg);
    std::string n(sqlite3_db_filename(d->db_, "main"));
    d->tortured_databases_.push_back(n);
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
  std::vector<std::string> process()
  {
    //std::srand(std::time(0));

    std::vector<CallbackData> dbs;
    std::vector<std::string> tortured_databases;

    dbs.emplace_back(CallbackData(tortured_databases, open_database("test1.db")));
    //dbs.emplace_back(CallbackData(tortured_databases, open_database("test2.db")));
    //dbs.emplace_back(CallbackData(tortured_databases, open_database("test3.db")));
    //dbs.emplace_back(CallbackData(tortured_databases, open_database("test4.db")));
    //dbs.emplace_back(CallbackData(tortured_databases, open_database("test5.db")));

    //for (size_t i = 0; i<dbs.size(); ++i) {
    //  sqlite3_trace(dbs[i].db_, trace_callback, &dbs[i]);
    //}

    for (size_t i = 0; i<dbs.size(); ++i) {
      torture_database(dbs[i].db_);  
    }

    for (size_t i = 0; i<dbs.size(); ++i) {
      sqlite3_close(dbs[i].db_);
    }
    return tortured_databases;
  }
};

//#define FORKING_ENABLED
//#define TRACING_ENABLED

#ifdef TRACING_ENABLED    
void trace(pid_t child, std::vector<std::string>& files, std::mutex & mutex)
{
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
        struct regs {
#ifdef __x86_64__
          unsigned long 
#endif
          long
            err, 
            syscall, 
            arg0;
        } regs_ = { 
#ifdef __x86_64__
          regs.rax, regs.orig_rax, regs.rdi
#else
          regs.eax, regs.orig_eax, regs.ebx
#endif
        };
        if (regs_.err == (unsigned long)-ENOSYS) { //syscall_enter stop
          if (regs_.syscall == __NR_open) {
            std::string file_name;
            long word = ptrace(PTRACE_PEEKDATA, pid, regs_.arg0, NULL);
            char * tmp = (char *)&word;
            while (*tmp != '\0') {
              file_name.push_back(*tmp);
              ++tmp;
              if (tmp == (char *)(&word + 1)) {
                regs_.arg0 += sizeof(word);
                word = ptrace(PTRACE_PEEKDATA, pid, regs_.arg0, NULL);
                tmp = (char *)&word;
              }
            }
            {
              std::lock_guard<std::mutex> l(mutex);
              files.push_back(file_name);
            }
          }
        }
        else { //syscall_exit stop
        }
    }
  }
}
#endif


#ifdef FORKING_ENABLED
std::vector<std::string> read_from_pipe(int pipefd)
{
  std::vector<std::string> ret;

  char buf;
  if (read(pipefd, &buf, 1) <= 0) 
    return ret;

  assert(buf != 0);

  ret.push_back(std::string());
  ret.back().push_back(buf);

  while (read(pipefd, &buf, 1) > 0) {
    if (buf == 0) {
      ret.push_back(std::string());
    }
    else {
      ret.back().push_back(buf);
    } 
  }

  return ret;
}

void write_to_pipe(int pipefd, const std::vector<std::string>& files)
{
  for (std::vector<std::string>::const_iterator iter = files.begin();
        iter != files.end();
        ++iter) {
    write(pipefd, iter->c_str(), iter->size() + 1); 
  }
}

void search_unnecessary_openings(const std::vector<std::string>& opened_files, 
                                 const std::vector<std::string>& tortured)
{
  std::unordered_set<std::string> tort(tortured.begin(), tortured.end());
  std::unordered_set<std::string> opened(opened_files.begin(), opened_files.end());
  for (std::unordered_set<std::string>::const_iterator iter = opened.begin();
        iter != opened.end();
        ++iter) {
    std::unordered_set<std::string>::const_iterator find_it = tort.find(*iter);
    if (find_it == tort.end()) {
      std::cout << "file: " << *iter << " unnecessary opened" << std::endl;
    }
  }
}
#endif
#ifdef FORKING_ENABLED
void reader_func(int child_socket, const std::vector<std::string>& opened_files, std::mutex& mutex)
{
  std::vector<std::string> tortured = read_from_pipe(child_socket);
  std::lock_guard<std::mutex> lk(mutex);
  search_unnecessary_openings(opened_files, tortured);
}
#endif

int main()
{
#ifdef FORKING_ENABLED
  int pipefd[2];
  pipe(pipefd);
  pid_t child = fork();
  if (child == 0) {
    close(pipefd[0]);
#ifdef TRACING_ENABLED    
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    raise(SIGSTOP);
#endif
#endif
    SqlModelling modelling;
    std::vector<std::string> tortured = modelling.process();
#ifdef FORKING_ENABLED    
    write_to_pipe(pipefd[1], tortured);
    close(pipefd[1]);
  }
  else {
    close(pipefd[1]);
#ifdef TRACING_ENABLED
    std::vector<std::string> files;
    std::mutex mutex;
    std::thread thread(reader_func, pipefd[0], std::ref(files), std::ref(mutex));
    trace(child, files, mutex);
    close(pipefd[0]);
    thread.join();
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
