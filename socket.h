#ifndef DNET_SOCKET
#define DNET_SOCKET

#include "smartptr.h"

using namespace std;

class Socket : boost::noncopyable {
private:
  int sock;

  string buffer;
public:
  string receiveline();
  void sendline(const string &val);

  bool isDead() const;

  Socket(int sock);
  ~Socket();
};

class Listener : boost::noncopyable {
  int sock;
  
public:
  smart_ptr<Socket> consumeNewConnection();

  Listener(int port);
  ~Listener();
};

#endif
