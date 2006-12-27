#ifndef DNET_VECEDIT
#define DNET_VECEDIT

#include "functor.h"

#include <boost/noncopyable.hpp>

class Vecedit : private boost::noncopyable {
  smart_ptr<Closure0> resync_gui_callback;
  
public:

  bool changed() const;

  void render() const;

  void clear();
  void load(const string &filename);
  void save(const string &filename);
  
  explicit Vecedit(const smart_ptr<Closure0> &resync_gui_callback);
};

#endif
