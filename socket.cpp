
#include "socket.h"

#include "debug.h"

#include <winsock.h>

class InitWinsock {
public:
  InitWinsock() {
    dprintf("Starting winsock");
    WSADATA wsaData;
    CHECK(WSAStartup(MAKEWORD(1, 1), &wsaData) == 0);
  }
  ~InitWinsock() {
    dprintf("Closing winsock");
    WSACleanup();
  }
} iws;  // this whole shebang doesn't exist in BSD

string Socket::receiveline() {
  char data[1024];
  int recvamount;
  
  // read as much as we can
  while(recvamount = recv(sock, data, sizeof(data), 0)) {
    if(recvamount == -1)
      break;
    CHECK(recvamount >= 0);
    buffer += string(data, data + recvamount);
  }
  
  while(1) {
    if(find(buffer.begin(), buffer.end(), '\n') != buffer.end()) {
      string rline = string(buffer.begin(), find(buffer.begin(), buffer.end(), '\n'));
      if(rline.size() && rline[rline.size() - 1] == '\r')
        rline.erase(rline.end() - 1);
      buffer.erase(buffer.begin(), find(buffer.begin(), buffer.end(), '\n') + 1);
      return rline;
    }
    recvamount = recv(sock, data, sizeof(data), 0);
    if(recvamount == -1)  // wheeeee
      continue;
    CHECK(recvamount >= 0);
    buffer += string(data, data + recvamount);
  }
}

void Socket::sendline(const string &str) {
  string tst = str;
  if(!tst.size() || tst[tst.size() - 1] != '\n')
    tst += '\n';
  string rst;
  for(int i = 0; i < tst.size(); i++) {
    if(tst[i] == '\n')
      rst += "\r\n";
    else
      rst += tst[i];
  }
  int cpt = 0;
  while(cpt != rst.size()) {
    int rv = send(sock, rst.c_str() + cpt, rst.size() - cpt, 0);
    CHECK(rv >= 0);
    cpt += rv;
  }
}
    
Socket::Socket(int sock) : sock(sock) {
    //CHECK(fcntl(sock, F_SETFL, O_NONBLOCK) == 0); // BSD
  { int nonblocking = 1; CHECK(ioctlsocket(sock, FIONBIO, (unsigned long*) &nonblocking) == 0); } // Windows
}
Socket::~Socket() {
  closesocket(sock);
}

Listener::Listener(int port) {
  CHECK(port >= 0 && port < 65536);
  
  CHECK((sock = socket(PF_INET, SOCK_STREAM, 0)) != -1);

  //CHECK(fcntl(sock, F_SETFL, O_NONBLOCK) == 0); // BSD
  { int nonblocking = 1; CHECK(ioctlsocket(sock, FIONBIO, (unsigned long*)&nonblocking) == 0); } // Windows
  
  {
    int yes = 1;
    CHECK(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(const char *)&yes,sizeof(int)) == 0);
  }
  
  struct sockaddr_in my_addr;    // my address information
    
  my_addr.sin_family = AF_INET;         // host byte order
  my_addr.sin_port = htons(port);     // short, network byte order
  my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
  memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct
  
  CHECK(bind(sock, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == 0);
  
  listen(sock, 10);
}

smart_ptr<Socket> Listener::consumeNewConnection() {
  struct sockaddr_in their_addr;
  int sin_size = sizeof(their_addr);
  
  int new_fd;
  new_fd = accept(sock, (struct sockaddr *)&their_addr, &sin_size);
  
  if(new_fd == -1) {
    return smart_ptr<Socket>();
  }
  
  return smart_ptr<Socket>(new Socket(new_fd));
}

Listener::~Listener() {
  closesocket(sock);  // close in BSD
}

/*
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    while(1) {  // main accept() loop
        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
                                                       &sin_size)) == -1) {
            perror("accept");
            continue;
        }
        printf("server: got connection from %s\n",
                                           inet_ntoa(their_addr.sin_addr));
        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            if (send(new_fd, "Hello, world!\n", 14, 0) == -1)
                perror("send");
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

    return 0;
*/
