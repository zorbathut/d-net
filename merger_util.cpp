
#include "merger_util.h"
#include "parse.h"

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

string splice(const string &source, const string &splicetext) {
  int replacements = 0;
  vector<string> radidam = tokenize(source, " ");
  for(int i = 0; i < radidam.size(); i++) {
    if(radidam[i] == "MERGE") {
      radidam[i] = splicetext;
      replacements++;
    }
  }
  CHECK(replacements == 1);
  string out;
  for(int i = 0; i < radidam.size(); i++) {
    if(i)
      out += " ";
    out += radidam[i];
  }
  return out;
}

string suffix(const string &name) {
  CHECK(count(name.begin(), name.end(), '.'));
  return strrchr(name.c_str(), '.') + 1;
}
