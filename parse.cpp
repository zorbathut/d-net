
#include "parse.h"

#include "debug.h"
#include "util.h"

#include <iostream>

using namespace std;

vector< string > tokenize(const string &in, const string &kar) {
  string::const_iterator cp = in.begin();
  vector< string > oot;
  while(cp != in.end()) {
    while(cp != in.end() && count(kar.begin(), kar.end(), *cp))
      cp++;
    if(cp != in.end())
      oot.push_back(string(cp, find_first_of(cp, in.end(), kar.begin(), kar.end())));
    cp = find_first_of(cp, in.end(), kar.begin(), kar.end());
  };
  return oot;
};

vector< int > sti(const vector< string > &foo) {
  int i;
  vector< int > bar;
  for(i = 0; i < foo.size(); i++) {
    bar.push_back(atoi(foo[i].c_str()));
  }
  return bar;
};

string kvData::debugOutput() const {
  string otp;
  otp = "\"";
  otp += category;
  otp += "\" : ";
  for(map<string, string>::const_iterator itr = kv.begin(); itr != kv.end(); itr++) {
    otp += "{ \"" + itr->first + "\" -> \"" + itr->second + "\" }, ";
  }
  return otp;
}

string kvData::consume(string key) {
  if(kv.count(key) != 1) {
    dprintf("Failed to read key \"%s\" in object \"%s\"\n", key.c_str(), category.c_str());
    CHECK(kv.count(key) == 1);
  }
  string out = kv[key];
  kv.erase(kv.find(key));
  return out;
}

bool kvData::isDone() const {
  return kv.size() == 0;
}
void kvData::shouldBeDone() const {
  CHECK(isDone());
}

istream &getLineStripped(istream &ifs, string *out) {
  while(getline(ifs, *out)) {
    *out = string(out->begin(), find(out->begin(), out->end(), '#'));
    while(out->size() && isspace(*out->begin()))
      out->erase(out->begin());
    while(out->size() && isspace((*out)[out->size()-1]))
      out->erase(out->end() - 1);
    if(out->size())
      return ifs;
  }
  return ifs;
}

istream &getkvData(istream &ifs, kvData *out) {
  out->kv.clear();
  out->category.clear();
  {
    string line;
    getLineStripped(ifs, &line);
    if(!ifs)
      return ifs; // only way to return failure without an assert
    vector<string> tok = tokenize(line, " ");
    CHECK(tok.size() == 2);
    CHECK(tok[1] == "{");
    out->category = tok[0];
  }
  {
    string line;
    while(getLineStripped(ifs, &line)) {
      if(line == "}")
        return ifs;
      vector<string> tok = tokenize(line, "=");
      CHECK(tok.size() == 1 || tok.size() == 2);
      string dat;
      if(tok.size() == 1)
        dat = "";
      else
        dat = tok[1];
      if(!out->kv.count(tok[0]))
        out->kv[tok[0]] = dat;
      else
        out->kv[tok[0]] += "\n" + dat;
    }
  }
  CHECK(0);
  return ifs; // this will be failure
}

string stringFromKvData(const kvData &kvd) {
  string rv;
  rv += StringPrintf("%s {\n", kvd.category.c_str());
  for(map<string, string>::const_iterator itr = kvd.kv.begin(); itr != kvd.kv.end(); itr++) {
    vector<string> remu = tokenize(itr->second, "\n");
    for(int i = 0; i < remu.size(); i++) {
      for(int j = 0; j < remu[i].size(); j++) {
        CHECK(remu[i][j] != '=');
        CHECK(remu[i][j] != '#');
        if(isalpha(remu[i][j]))
          continue;
        if(isdigit(remu[i][j]))
          continue;
        if(remu[i][j] >= 32 && remu[i][j] <= 126)
          continue;
        CHECK(0);
      }
      rv += StringPrintf("  %s=%s\n", itr->first.c_str(), remu[i].c_str());
    }
  }
  rv += "}\n";
  return rv;
}

char fromHex(char in) {
  in = tolower(in);
  if(isdigit(in))
    return in - '0';
  else if(in >= 'a' && in <= 'f')
    return in - 'a' + 10;
  else
    CHECK(0);
}
