#ifndef DNET_SOCKET
#define DNET_SOCKET

#include <string>

#include "util.h"
#include "noncopyable.h"

class Socket : boost::noncopyable {
public:
  string receiveline();
  void send(const string &val);

  bool isDead() const;
};

class Listener : boost::noncopyable {
  int sock;
  
public:
  smart_ptr<Socket> consumeNewConnection();

  Listener(int port);
  ~Listener();
};

#endif
