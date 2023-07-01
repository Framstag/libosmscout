#ifndef LIBOSMSCOUT_SIGNAL_H
#define LIBOSMSCOUT_SIGNAL_H

/*
 This source is part of the libosmscout library
 Copyright (C) 2023 Lukas Karas

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#include <osmscout/lib/CoreImportExport.h>

#include <functional>
#include <mutex>
#include <set>
#include <memory>

namespace osmscout {

  template<typename... Args>
  class Slot;

  template<typename... Args>
  class Signal
  {
  private:
    struct Connection
    {
      std::mutex mutex;
      Slot<Args...>* slot = nullptr;
      Signal<Args...>* signal = nullptr;
    };

  private:
    // Note: when signal and slot mutexes are locked at the same time, always lock signal's one first
    std::mutex mutex;
    std::vector<std::shared_ptr<Connection>> connections;

  public:
    friend class Slot<Args...>;

    Signal() = default;
    Signal(const Signal&) = delete;
    Signal(Signal&&) = delete;

    Signal& operator=(const Signal&) = delete;
    Signal& operator=(Signal&&) = delete;

    virtual ~Signal();

    void Emit(const Args&... args);

    void Connect(Slot<Args...> &slot);

    void Disconnect(Slot<Args...> &slot);

    void Disconnect();
  };

  template<typename... Args>
  class Slot
  {
  private:
    // Note: when signal and slot mutexes are locked at the same time, always lock signal's one first
    std::mutex mutex;
    const std::function<void(const Args&...)> callback;
    std::vector<std::shared_ptr<typename Signal<Args...>::Connection>> connections;

  public:
    friend class Signal<Args...>;

    explicit Slot(const std::function<void(const Args&...)> &callback);

    Slot(const Slot&) = delete;
    Slot(Slot&&) = delete;

    Slot& operator=(const Slot&) = delete;
    Slot& operator=(Slot&&) = delete;

    virtual ~Slot();

    void Disconnect();

  private:
    void Call(const Args&... args) const;
  };

  template<typename... Args>
  Slot<Args...>::Slot(const std::function<void(const Args&...)> &callback):
    callback(callback)
  {}

  template<typename... Args>
  void Slot<Args...>::Call(const Args&... args) const
  {
    callback(args...);
  }

  template<typename... Args>
  void Signal<Args...>::Emit(const Args&... args)
  {
    std::unique_lock lock(mutex);
    for (const auto &con: connections) {
      std::unique_lock lockCon(con->mutex);
      if (auto slot = con->slot; slot != nullptr) {
        slot->Call(args...);
      }
    }
  }

  template<typename... Args>
  void Signal<Args...>::Connect(Slot<Args...> &slot)
  {
    std::unique_lock lock(mutex);
    std::unique_lock slotLock(slot.mutex);

    auto con=std::make_shared<Connection>();
    con->slot = &slot;
    con->signal = this;
    connections.push_back(con);
    slot.connections.push_back(con);
  }

  template<typename... Args>
  void Signal<Args...>::Disconnect(Slot<Args...> &slot)
  {
    std::unique_lock lock(mutex);
    std::unique_lock slotLock(slot.mutex);

    if (auto it = connections.find([&slot](auto const &con) -> bool { return con && con->slot == &slot; });
      it != connections.end()) {
      connections.erase(it);
    }
    if (auto it = slot.connections.find([this](auto const &con) -> bool { return con && con->signal == this; });
      it != slot.connections.end()) {
      slot.connections.erase(it);
    }
  }

  template<typename... Args>
  void Signal<Args...>::Disconnect()
  {
    std::unique_lock lock(mutex);
    for (auto &con: connections) {
      std::unique_lock lockCon(con->mutex);
      con->signal = nullptr;
      con->slot = nullptr;
    }
    connections.clear();
  }

  template<typename... Args>
  void Slot<Args...>::Disconnect()
  {
    std::unique_lock lock(mutex);
    for (auto &con: connections) {
      std::unique_lock lockCon(con->mutex);
      con->signal = nullptr;
      con->slot = nullptr;
    }
    connections.clear();
  }

  template<typename... Args>
  Signal<Args...>::~Signal() {
    Disconnect();
  }

  template<typename... Args>
  Slot<Args...>::~Slot() {
    Disconnect();
  }

} // namespace

#endif //LIBOSMSCOUT_SIGNAL_H
