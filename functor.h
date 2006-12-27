#ifndef DNET_FUNCTOR
#define DNET_FUNCTOR

#include "smartptr.h"

#include <boost/noncopyable.hpp>

class Closure0 : private boost::noncopyable {
public:
  virtual void Run() const = 0;
};

template<typename Owner> class Closure0_Member : public Closure0 {
private:
  Owner *owner;
  void (Owner::*function)(void);

public:
  virtual void Run() const { (owner->*function)(); };
  
  Closure0_Member(Owner *owner, void (Owner::*function)(void)) : owner(owner), function(function) { };
};

template<typename Owner> smart_ptr<Closure0> NewFunctor(Owner *owner, void (Owner::*function)(void)) {
  return smart_ptr<Closure0>(new Closure0_Member<Owner>(owner, function));
};

#endif
