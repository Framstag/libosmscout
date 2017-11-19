#ifndef OSMSCOUT_GPX_OPTIONAL_H
#define OSMSCOUT_GPX_OPTIONAL_H

// Copyright (C) 2015 Justin - https://codereview.stackexchange.com/users/49895/justin
// cc by-sa 3.0
// https://codereview.stackexchange.com/questions/103737/c-optional-implementation


// TODO: use std::optional after transition to c++14

#include <osmscout/private/GPXImportExport.h>

#include <stdexcept>

namespace osmscout {

template<typename T>
class OSMSCOUT_GPX_API Optional {
public:
  const static Optional<T> empty;

  static Optional<T> of(const T &v);

  static Optional<T> of(T *v);

  T get() const;

  bool hasValue() const;

  ~Optional() { if (value != nullptr) delete value; }

private:
  Optional(T *v) : value{v} {}

  const T *value;
};

template<typename T>
const Optional<T> Optional<T>::empty = Optional::of(nullptr);



template<typename T>
Optional<T> Optional<T>::of(const T &t) {
  return Optional<T>{new T(t)};
}

template<typename T>
Optional<T> Optional<T>::of(T *t) {
  if (t != nullptr) return Optional<T>{new T(*t)};
  return Optional<T>{nullptr};
}

template<typename T>
bool Optional<T>::hasValue() const {
  return value != nullptr;
}

template<typename T>
T Optional<T>::get() const {
  if (!hasValue()) throw std::runtime_error("Tried to get the value from an optional that has no value");
  return *value;
}

template<typename T>
bool operator==(const Optional<T> &t1, const Optional<T> &t2) {
  if (!t1.hasValue() && !t2.hasValue()) return true;
  else if (!t1.hasValue() || !t2.hasValue()) return false;
  return t1.get() == t2.get();
}

template<typename T>
bool operator!=(const Optional<T> &t1, const Optional<T> &t2) {
  return !(t1 == t2);
}
}

#endif //OSMSCOUT_GPX_OPTIONAL_H
