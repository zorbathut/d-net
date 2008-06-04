#ifndef DNET_SMARTPTR
#define DNET_SMARTPTR

#include "debug.h"

#include <boost/noncopyable.hpp>

template <typename B, typename D> B *upcast(D *d) { return static_cast<D *>(static_cast<B *>(d)); } 

template<typename T> class smart_ptr {
  template <typename U> friend class smart_ptr;
  T *ptr;
  int *ct;

public:
  void reset() {
    if(ptr) {
      (*ct)--;
      if(*ct == 0) {
        delete ct;
        delete ptr;
      }
      ct = NULL;
      ptr = NULL;
    }
  }
  void reset(T *pt) {
    reset();
    if(pt) {
      ptr = pt;
      ct = new int(1);
    }
  }
  
  bool empty() const {
    return !ptr;
  }
  
  T *get() {
    CHECK(ptr);
    return ptr;
  }
  const T *get() const {
    CHECK(ptr);
    return ptr;
  }

  T *operator->() {
    return get();
  }
  const T *operator->() const {
    return get();
  }
  
  T &operator*() {
    return *get();
  }
  const T &operator*() const {
    return *get();
  }

  smart_ptr<T> &operator=(const smart_ptr<T> &x) {
    if(this != &x) {
      reset();
      if(x.ptr) {
        ptr = x.ptr;
        ct = x.ct;
        (*ct)++;
      }
    }
    return *this;
  }
  template<typename U> smart_ptr<T> &operator=(const smart_ptr<U> &x) {
    CHECK(this != reinterpret_cast<const smart_ptr<T> *>(&x)); // oh boy oh boy
    reset();
    if(x.ptr) {
      ptr = upcast<T, U>(x.ptr);  // I honestly don't remember why I needed upcast<> for this anymore, but I'm going to assume for now that I had a good reason
      ct = x.ct;
      (*ct)++;
    }
    return *this;
  }

  smart_ptr() {
    ct = NULL;
    ptr = NULL;
  }
  smart_ptr(const smart_ptr<T> &x) {
    ct = NULL;
    ptr = NULL;
    *this = x;
  }
  template<typename U> smart_ptr(const smart_ptr<U> &x) {
    ct = NULL;
    ptr = NULL;
    *this = x;
  }
  explicit smart_ptr<T>(T *pt) {
    ct = NULL;
    ptr = NULL;
    reset(pt);
  }
  ~smart_ptr() {
    reset();
  }
};

// basically it's a smart_ptr without a copy operator, and thus no refcounting
template<typename T> class scoped_ptr : boost::noncopyable {
  T *ptr;

public:
  void reset() {
    delete ptr;
    ptr = NULL;
  }
  void reset(T *pt) {
    reset();
    ptr = pt;
  }
  
  bool empty() const {
    return !ptr;
  }
  
  T *get() {
    CHECK(ptr);
    return ptr;
  }
  const T *get() const {
    CHECK(ptr);
    return ptr;
  }

  T *operator->() {
    return get();
  }
  const T *operator->() const {
    return get();
  }
  
  T &operator*() {
    return *get();
  }
  const T &operator*() const {
    return *get();
  }

  smart_ptr() {
    ptr = NULL;
  }
  explicit smart_ptr<T>(T *pt) {
    ptr = pt;
  }
  ~smart_ptr() {
    reset();
  }
}

#endif
