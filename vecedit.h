#ifndef DNET_VECEDIT
#define DNET_VECEDIT

#include "functor.h"

#include <boost/noncopyable.hpp>

class Vecedit : private boost::noncopyable {
  smart_ptr<Closure0> resync_gui_callback;
  
public:
  
  void render() const;
  
  explicit Vecedit(const smart_ptr<Closure0> &resync_gui_callback);
};

#endif
