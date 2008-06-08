#ifndef DNET_IMAGE
#define DNET_IMAGE






#include <vector>

using namespace std;

class Image {
public:
  int x;
  int y;
  
  vector<vector<unsigned long> > c;
};

Image imageFromPng(const string &fname);

#endif
