#ifndef DNET_DEBUG
#define DNET_DEBUG

/*************
 * CHECK/TEST macros
 */
 
int dprintf( const char *bort, ... ) __attribute__((format(printf,1,2)));

extern int frameNumber;

void crash() __attribute__((__noreturn__));
#define CHECK(x) while(1) { if(!(x)) { dprintf("Error at %d, %s:%d - %s\n", frameNumber, __FILE__, __LINE__, #x); crash(); } break; }
#define TEST(x) CHECK(x)
#define printf FAILURE

#endif
