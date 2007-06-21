
#include "debug.h"
#include "os.h"

#include "minizip/unzip.h"

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>

using namespace std;
using namespace xercesc;

int main(int argc, char *argv[]) {
  set_exename("ods2csv.exe");
  
  CHECK(argc == 4);
  
  unzFile unzf = unzOpen(argv[2]);
  CHECK(unzf);
  
  XMLPlatformUtils::Initialize();
  
  XMLPlatformUtils::Terminate();
  
  unzClose(unzf);

  CHECK(0);
}
