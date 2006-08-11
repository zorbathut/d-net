#ifndef DNET_DEBUG
#define DNET_DEBUG

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
 
int dprintf(const char *bort, ...) __attribute__((format(printf,1,2)));

extern int frameNumber;

void CrashHandler(const char *fname, int line);
void PrintDebugStack();
void crash() __attribute__((__noreturn__));
#define CHECK(x) do { if(!(x)) { dprintf("Error at %d, %s:%d - %s\n", frameNumber, __FILE__, __LINE__, #x); PrintDebugStack(); CrashHandler(__FILE__, __LINE__); crash(); } } while(0)
#define TEST(x) CHECK(x)
#define printf FAILURE

#endif
