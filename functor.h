#ifndef DNET_FUNCTOR
#define DNET_FUNCTOR

#include "smartptr.h"

#include <boost/noncopyable.hpp>

class Closure0 : private boost::noncopyable {
public:
  virtual void Run() const = 0;
};

template<typename Owner> class Closure0_Member_NC : public Closure0 {
private:
  Owner *owner;
  void (Owner::*function)(void);

public:
  virtual void Run() const { (owner->*function)(); };
  
  Closure0_Member_NC(Owner *owner, void (Owner::*function)(void)) : owner(owner), function(function) { };
};

template<typename Owner> class Closure0_Member_C : public Closure0 {
private:
  const Owner *owner;
  void (Owner::*function)(void) const;

public:
  virtual void Run() const { (owner->*function)(); };
  
  Closure0_Member_C(const Owner *owner, void (Owner::*function)(void) const) : owner(owner), function(function) { };
};

template<typename Owner> smart_ptr<Closure0> NewFunctor(Owner *owner, void (Owner::*function)(void)) {
  return smart_ptr<Closure0>(new Closure0_Member_NC<Owner>(owner, function));
};

template<typename Owner> smart_ptr<Closure0> NewFunctor(const Owner *owner, void (Owner::*function)(void) const) {
  return smart_ptr<Closure0>(new Closure0_Member_C<Owner>(owner, function));
};

#endif
