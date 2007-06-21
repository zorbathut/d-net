
#include "debug.h"

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>

using namespace xercesc;

int main() {
  XMLPlatformUtils::Initialize();
  
  dprintf("lolz\n");
  XMLPlatformUtils::Terminate();
}
