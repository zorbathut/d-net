
#include "debug.h"
#include "os.h"

#include "minizip/unzip.h"

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMTreeWalker.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

#include <vector>

using namespace std;
using namespace xercesc;

inline string FX( const XMLCh *d ) {
	char *dat = XMLString::transcode( d );
	string out = dat;
	XMLString::release( &dat );
	return out;
};

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
  
  XMLPlatformUtils::Initialize();
  
  {
    MemBufInputSource mbis(&file[0], file.size(), argv[2]);
    XercesDOMParser parse;
    parse.parse(mbis);
    
    dprintf("Content parsed\n");
    
    DOMDocument *doc = parse.getDocument();
    DOMTreeWalker *walkor = doc->createTreeWalker(doc, DOMNode::ELEMENT_NODE, NULL, true);
    
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
          CHECK(walkor->firstChild());
          
          CHECK(walkor->parentNode());
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

  dprintf("ENDING YOU AND ALL YOU LOVE\n");
  CHECK(0);
}
