#ifndef DNET_PARSE
#define DNET_PARSE

#include <map>
#include <string>
#include <vector>

using namespace std;

vector< string > tokenize( const string &in, const string &kar );
vector< int > sti( const vector< string > &foo );
  
class kvData {
public:
  
  string category;
  map<string, string> kv;

  string debugOutput() const;

  string consume(string key);
  bool isDone() const;
  void shouldBeDone() const;

};

istream &getLineStripped(istream &ifs, string *out);
istream &getkvData(istream &ifs, kvData *out);

string stringFromKvData(const kvData &kvd);

char fromHex(char in);

#endif
