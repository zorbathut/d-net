#ifndef DNET_PARSE
#define DNET_PARSE

#include <iostream>
#include <string>
#include <map>
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

};

istream &getLineStripped(istream &ifs, string &out);
istream &getkvData(istream &ifs, kvData &out);

#endif
