
#include <cstdio>
#include <fstream>
#include <vector>
#include <iterator>
#include <set>

#include "debug.h"
#include "parse.h"
#include "util.h"

using namespace std;

class Params {
  set<string> flags;
  set<string> unusedflags;
  
public:
  void init(const vector<string> &items);

  bool check(const string &str);
  void override();

  string stringize() const;

  ~Params();
};

void Params::init(const vector<string> &items) {
  flags = set<string>(items.begin(), items.end());
  CHECK(flags.size() == items.size());
  unusedflags = flags;
}

bool Params::check(const string &str) {
  unusedflags.erase(str);
  return flags.count(str);
}
void Params::override() {
  unusedflags.clear();
}

string Params::stringize() const {
  string out;
  if(flags.size()) {
    for(set<string>::const_iterator itr = flags.begin(); itr != flags.end(); itr++) {
      if(itr != flags.begin())
        out += ", ";
      out += *itr;
    }
  } else {
    out += "(none)";
  }
  return out;
}

Params::~Params() {
  if(unusedflags.size())
    for(set<string>::const_iterator itr = unusedflags.begin(); itr != unusedflags.end(); itr++)
      dprintf("Unused flag: %s\n", itr->c_str());
  CHECK(unusedflags.size() == 0);
}

int main() {
  dprintf("Generating functor.h");
  
  vector<char> source;
  {
    ifstream sourcefile("functor.h.in");
    CHECK(sourcefile);
    sourcefile >> noskipws;
    copy(istream_iterator<char>(sourcefile), istream_iterator<char>(), back_inserter(source));
  }
  
  FILE *funcfile = fopen("functor.h.tmp", "w");
  
  fprintf(funcfile,
    "#ifndef DNET_FUNCTOR\n"
    "#define DNET_FUNCTOR\n"
    "\n"
    "// Generated by functor_gen.cpp. Do not modify this file directly!\n"
    "\n"
    "#include \"smartptr.h\"\n"
    "\n"
    "#include <boost/noncopyable.hpp>\n"
    "\n"
  );
  
  const int maxparams = 2;
  
  bool first = true;
  for(int i = maxparams; i >= 0; i--) {
    for(int phase = 0; phase < 2; phase++) {
      int curofs = 0;
      while(curofs != source.size()) {
        if(source[curofs] == '%') {
          CHECK(source[curofs + 1] == '%');
          curofs += 2;
          int endtoken = find(source.begin() + curofs, source.end(), '%') - source.begin();
          CHECK(source[endtoken] == '%' && source[endtoken + 1] == '%');
          string inttoken = string(source.begin() + curofs, source.begin() + endtoken);
          
          Params params;
          if(count(inttoken.begin(), inttoken.end(), '(')) {
            CHECK(count(inttoken.begin(), inttoken.end(), '(') == 1);
            CHECK(count(inttoken.begin(), inttoken.end(), ')') == 1);
            CHECK(inttoken[inttoken.size() - 1] == ')');
            string paramlist = string(find(inttoken.begin(), inttoken.end(), '(') + 1, inttoken.end() - 1); // at worst this is "nothing"
            inttoken = string(inttoken.begin(), find(inttoken.begin(), inttoken.end(), '('));
            params.init(tokenize(paramlist, ", "));
          }
          
          {
            string dbgout = inttoken;
            dbgout += ", params ";
            dbgout += params.stringize();
            dprintf("%s\n", dbgout.c_str());
          }
          
          if(inttoken == "CALLBACK_OR_CLOSURE") {
            if(phase == 0)
              fprintf(funcfile, "Closure");
            else if(phase == 1)
              fprintf(funcfile, "Callback");
            else
              CHECK(0);
          } else if(inttoken == "RETURNVALUE") {
            if(phase == 0)
              fprintf(funcfile, "void");
            else if(phase == 1)
              fprintf(funcfile, "Returnvalue");
            else
              CHECK(0);
          } else if(inttoken == "TEMPLATE") {
            // Flags: blankonfirst, prefixtemplate, prefixbracket, hasowner, hastypename, hastype, hasparam, hasdefaults, trailingvoids, nullstatic, nullnothing
            bool null = !i && !params.check("hasowner");
            
            if(params.check("blankonfirst") && first) {
              params.override();
            } else if(params.check("nullstatic") && null) {   // params check must be first so it touches the token and we know it's not being ignored
              fprintf(funcfile, "static");
              params.override();
            } else if(params.check("nullnothing") && null) {
              params.override();
            } else {
              if(params.check("prefixtemplate"))
                fprintf(funcfile, "template");
              if(params.check("prefixtemplate") || params.check("prefixbracket"))
                fprintf(funcfile, "<");
              
              vector<string> items;
              if(params.check("hasowner"))
                items.push_back("Owner");
              if(params.check("hasreturnvalue") && phase == 1)
                items.push_back("Returnvalue");
              
              for(int j = 0; j < i; j++)
                items.push_back(StringPrintf("T%d", j));
              
              if(!items.size()) { // touch stuff that would otherwise go untouched
                params.check("hastypename");
                params.check("hastype");
                params.check("hasparam");
                params.check("hasdefaults");
              }
              
              for(int j = 0; j < items.size(); j++) {
                if(j)
                  fprintf(funcfile, ", ");
                if(params.check("hastypename"))
                  fprintf(funcfile, "typename ");
                if(params.check("hastype"))
                  fprintf(funcfile, "%s ", items[j].c_str());
                if(params.check("hasparam")) {
                  CHECK(items[j][0] == 'T');
                  fprintf(funcfile, "param%s ", items[j].c_str() + 1);
                }
                if(params.check("hasdefaults") && first)  // hasdefaults only on first
                  if(items[j][0] == 'T')
                    fprintf(funcfile, "= void");
              }
              
              if(params.check("trailingvoids")) {
                for(int j = i; j < maxparams; j++) {
                  if(j || items.size())
                    fprintf(funcfile, ", ");
                  fprintf(funcfile, "void");
                }
              }
              
              if(params.check("prefixtemplate") || params.check("prefixbracket"))
                fprintf(funcfile, "> ");
            }
          } else {
            dprintf("Intercepted token is %s\n", inttoken.c_str());
            CHECK(0);
          }
          
          curofs = endtoken + 2;
        } else {
          int npos = find(source.begin() + curofs, source.end(), '%') - source.begin();
          fprintf(funcfile, "%s", string(source.begin() + curofs, source.begin() + npos).c_str());
          curofs = npos;
        }
      }
    }
    first = false;
  }
  
  fprintf(funcfile, "\n");
  fprintf(funcfile, "#endif\n");
  
  fclose(funcfile);
  
  // this is pretty much awful
  system("mv functor.h.tmp functor.h");
}
