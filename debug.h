#ifndef DNET_DEBUG
#define DNET_DEBUG






#include <deque>
#include <string>

using namespace std;

/*************
 * CHECK/TEST macros
 */
 
class StackPrinter {
public:
  virtual void Print() const = 0;

  StackPrinter();
  virtual ~StackPrinter();
};

class StackString : public StackPrinter {
public:
  void Print() const;

  StackString(const string &str);

private:
  string str_;
};

deque<string> &dbgrecord();

void registerCrashFunction(void (*)());
void unregisterCrashFunction(void (*)());
 
void disableStackTrace();

extern void *stackStart;

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// Almost everything after here is necessary for the vector header patch
int rdprintf(const char *bort, ...) __attribute__((format(printf,1,2)));
int rdprintf();
#define dprintf(format, args...) rdprintf("%10.10s:%4d: " format, __FILE__, __LINE__, ## args)

extern int frameNumber;

void CrashHandler(const char *fname, int line);
void HandleFailure(const char *fname, int line) __attribute__((__noreturn__));
#define CHECK(x, args...) (__builtin_expect(!!(x), 1) ? (void)(1) : (dprintf("Error at %d, %s:%d - %s\n", frameNumber, __FILE__, __LINE__, #x), dprintf("" args), CrashHandler(__FILE__, __LINE__), HandleFailure(__FILE__, __LINE__)))
// And here would be the end



#define printf FAILURE
 
#endif
