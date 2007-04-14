
#include "merger_util.h"

vector<string> parseCsv(const string &in) {
  dprintf("%s\n", in.c_str());
  vector<string> tok;
  {
    string::const_iterator bg = in.begin();
    while(bg != in.end()) {
      string::const_iterator nd = find(bg, in.end(), ',');
      tok.push_back(string(bg, nd));
      bg = nd;
      if(bg != in.end())
        bg++;
    }
  }
  for(int i = 0; i < tok.size(); i++) {
    if(tok[i].size() >= 2 && tok[i][0] == '"' && tok[i][tok[i].size() - 1] == '"') {
      tok[i].erase(tok[i].begin());
      tok[i].erase(tok[i].end() - 1);
    }
  }
  return tok;
}
