
#include "merger_util.h"
#include "parse.h"
#include "util.h"

#include <boost/regex.hpp>

vector<string> parseCsv(const string &in) {
  //dprintf("%s\n", in.c_str());
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
    } else {
      static const boost::regex expression(".*MERGE.*");
      CHECK(!boost::regex_match(radidam[i], expression));
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
    static const boost::regex expression("MERGE(?:\\(([0-9]*)\\))?");
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
  if(count(name.begin(), name.end(), '.') == 0 && position == 1)
    return name;
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

void checkForExtraMerges(const kvData &kvd) {
  for(map<string, string>::const_iterator itr = kvd.kv.begin(); itr != kvd.kv.end(); itr++) {
    if(itr->second.find("MERGE") != string::npos) {
      dprintf("Unexpected MERGE\n");
      dprintf("%s\n", stringFromKvData(kvd).c_str());
      CHECK(0);
    }
  }
}

string findName(const string &thistoken, const set<string> &table) {
  string tts;
  for(int i = 0; i < thistoken.size(); i++)
    if(isalpha(thistoken[i]) || thistoken[i] == '.')
      tts += tolower(thistoken[i]);
  CHECK(tts.size());
  
  string ct;
  for(set<string>::const_iterator itr = table.begin(); itr != table.end(); itr++) {
    CHECK(itr->size());
    string tx;
    for(int i = 0; i < itr->size(); i++)
      if(isalpha((*itr)[i]) || thistoken[i] == '.')
        tx += tolower((*itr)[i]);
    //dprintf("Comparing %s and %s\n", tx.c_str(), tts.c_str());
    if(tx == tts) {
      CHECK(!ct.size());
      ct = *itr;
    }
  }
  return ct;
}
