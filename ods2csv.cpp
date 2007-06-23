
#include "debug.h"
#include "os.h"

#include "minizip/unzip.h"

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMTreeWalker.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

#include <vector>

using namespace std;
using namespace xercesc;

class XSargh {
public :
  
  const XMLCh* get() const { return xs; }
  
  XSargh(const char *in) { xs = XMLString::transcode(in); }
	XSargh(const string &in) { xs = XMLString::transcode(in.c_str()); }

  ~XSargh() { XMLString::release(&xs); }

private :
  XMLCh*   xs;
};

#define X(str) XSargh(str).get()

string FX(const XMLCh *d) {
	char *dat = XMLString::transcode(d);
	string out = dat;
	XMLString::release(&dat);
	return out;
};

string attrib(DOMTreeWalker *walkr, const string &attr) {
  DOMNamedNodeMap *mp = walkr->getCurrentNode()->getAttributes();
  CHECK(mp);
  DOMNode *n = mp->getNamedItem(X(attr));
  CHECK(n);
  return FX(n->getNodeValue());
};

vector<vector<string> > extractDataFromTable(DOMTreeWalker *walkor) {
  CHECK(FX(walkor->getCurrentNode()->getNodeName()) == "table:table");
  CHECK(walkor->firstChild());
  CHECK(walkor->parentNode());
  
  vector<vector<string> > rv;
  
  return rv;
}

vector<vector<string> > extractData(const vector<unsigned char> &data, const string &tablename) {
  XMLPlatformUtils::Initialize();
  
  vector<vector<string> > rv;
  
  {
    MemBufInputSource mbis(&data[0], data.size(), "extracted_data");
    XercesDOMParser parse;
    parse.parse(mbis);
    
    dprintf("Content parsed\n");
    
    DOMDocument *doc = parse.getDocument();
    DOMTreeWalker *walkor = doc->createTreeWalker(doc, DOMNodeFilter::SHOW_ALL, NULL, true);
    
    CHECK(walkor->firstChild());
    CHECK(FX(walkor->getCurrentNode()->getNodeName()) == "office:document-content");
    CHECK(walkor->firstChild());
    
    bool foundbody = false;
    do {
      if(FX(walkor->getCurrentNode()->getNodeName()) == "office:body") {
        CHECK(!foundbody);
        foundbody = true;
        CHECK(walkor->firstChild());
        CHECK(FX(walkor->getCurrentNode()->getNodeName()) == "office:spreadsheet");
        CHECK(walkor->firstChild());
        do {
          CHECK(FX(walkor->getCurrentNode()->getNodeName()) == "table:table");
          if(attrib(walkor, "table:name") == tablename) {
            CHECK(rv.empty());
            rv = extractDataFromTable(walkor);
            CHECK(!rv.empty());
          }
        } while(walkor->nextSibling());
        CHECK(walkor->parentNode());
        CHECK(walkor->parentNode());
      }
    } while(walkor->nextSibling());
    CHECK(foundbody);
    dprintf("prerelease\n");
    walkor->release();
    //doc->release();  // enabling this makes it crash. why? because xerces is weird
  }
  
  XMLPlatformUtils::Terminate();
  
  CHECK(0);
}

int main(int argc, char *argv[]) {
  set_exename("ods2csv.exe");
  
  CHECK(argc == 4);
  
  vector<unsigned char> file;
  {
    unzFile unzf = unzOpen(argv[2]);
    CHECK(unzf);
    
    CHECK(unzLocateFile(unzf, "content.xml", 1) == UNZ_OK);
    
    unz_file_info inf;
    CHECK(unzGetCurrentFileInfo(unzf, &inf, NULL, 0, NULL, 0, NULL, 0) == UNZ_OK);
    CHECK(inf.uncompressed_size < (128 << 20)); // what
    
    CHECK(unzOpenCurrentFile(unzf) == UNZ_OK);
    
    file.resize(inf.uncompressed_size);
    CHECK(unzReadCurrentFile(unzf, &file[0], file.size()) == file.size());
    
    CHECK(unzCloseCurrentFile(unzf) == UNZ_OK);
    unzClose(unzf);
    
    dprintf("Content read, %d bytes\n", file.size());
  }
  
  vector<vector<string> > dat = extractData(file, argv[3]);

  {
    FILE *of = fopen(argv[1], "wb");
    for(int i = 0; i < dat.size(); i++) {
      for(int j = 0; j < dat[i].size(); j++) {
        if(j)
          fprintf(of, ",");
        fprintf(of, "\"%s\"", dat[i][j].c_str());
      }
      fprintf(of, "\n");
    }
    fclose(of);
  }
}
