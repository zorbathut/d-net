
#include "parse.h"

#include "util.h"

using namespace std;

vector< string > tokenize( const string &in, const string &kar ) {
	string::const_iterator cp = in.begin();
	vector< string > oot;
	while( cp != in.end() ) {
		while( cp != in.end() && count( kar.begin(), kar.end(), *cp ) )
			cp++;
		if( cp != in.end() )
			oot.push_back( string( cp, find_first_of( cp, in.end(), kar.begin(), kar.end() ) ) );
		cp = find_first_of( cp, in.end(), kar.begin(), kar.end() );
	};
	return oot;
};

vector< int > sti( const vector< string > &foo ) {
	int i;
	vector< int > bar;
	for( i = 0; i < foo.size(); i++ ) {
		bar.push_back( atoi( foo[ i ].c_str() ) );
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
    CHECK(kv.count(key) == 1);
    string out = kv[key];
    kv.erase(kv.find(key));
    return out;
}

istream &getLineStripped(istream &ifs, string &out) {
    while(getline(ifs, out)) {
        out = string(out.begin(), find(out.begin(), out.end(), '#'));
        while(out.size() && isspace(*out.begin()))
            out.erase(out.begin());
        while(out.size() && isspace(out[out.size()-1]))
            out.erase(out.end() - 1);
        if(out.size())
            return ifs;
    }
    return ifs;
}

istream &getkvData(istream &ifs, kvData &out) {
    out.kv.clear();
    out.category.clear();
    {
        string line;
        getLineStripped(ifs, line);
        if(!ifs)
            return ifs; // only way to return failure without an assert
        vector<string> tok = tokenize(line, " ");
        CHECK(tok.size() == 2);
        CHECK(tok[1] == "{");
        out.category = tok[0];
    }
    {
        string line;
        while(getLineStripped(ifs, line)) {
            if(line == "}")
                return ifs;
            vector<string> tok = tokenize(line, "=");
            CHECK(tok.size() == 1 || tok.size() == 2);
            CHECK(!out.kv.count(tok[0]));
            if(tok.size() == 1)
                out.kv[tok[0]] = "";
            else
                out.kv[tok[0]] = tok[1];
        }
    }
    CHECK(0);
    return ifs; // this will be failure
}
