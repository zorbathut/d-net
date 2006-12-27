#ifndef DNET_VECEDIT
#define DNET_VECEDIT

#include "functor.h"
#include "dvec2.h"

#include <boost/noncopyable.hpp>

class Vecedit : private boost::noncopyable {
  smart_ptr<Closure0> resync_gui_callback;
  
  // x center, y center, zoom per pixel
  float xc;
  float yc;
  float zpp;
  
  Dvec2 dv2;
  
public:

  bool changed() const;

  void render() const;

  void clear();
  void load(const string &filename);
  void save(const string &filename);
  
  explicit Vecedit(const smart_ptr<Closure0> &resync_gui_callback);
};

#endif
