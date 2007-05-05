
#include "merger_util.h"
#include "parse.h"
#include "util.h"

#include <boost/regex.hpp>

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

string splice(const string &source, float value) {
  int replacements = 0;
  vector<string> radidam = tokenize(source, " ");
  for(int i = 0; i < radidam.size(); i++) {
    static const boost::regex expression("MERGE\\(([0-9]*)\\)");
    boost::smatch match;
    if(boost::regex_match(radidam[i], match, expression)) {
      if(match[1].matched) {
        radidam[i] = StringPrintf("%f", atof(match[1].str().c_str()) * value);
      } else {
        radidam[i] = StringPrintf("%f", value);
      }
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

string suffix(const string &name, int position) {
  CHECK(position >= 1);
  CHECK(name.size() >= 1);
  CHECK(count(name.begin(), name.end(), '.') >= position);
  int sps = -1;
  for(int i = name.size() - 1; i >= 0; --i) {
    if(name[i] == '.')
      position--;
    if(!position) {
      sps = i;
      break;
    }
  }
  CHECK(sps != -1);
  sps++;
  return string(name.begin() + sps, find(name.begin() + sps, name.end(), '.'));
}
