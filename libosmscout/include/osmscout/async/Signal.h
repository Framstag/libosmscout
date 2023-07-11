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
#include <vector>

namespace osmscout {

  template<typename... Args>
  class Slot;

  /** Signal and Slot is tool for connecting source of events and its consumers.
   * It is similar to Qt's signal, but it lacks some advanced functionality,
   * like asynchronous connection. Slot callback is called in thread context of the signal.
   * Locking or asynchronicity have to be solved differently.
   *
   * Signal and Slot are thread safe. Their live cycle may be independent.
   *
   * @tparam Args
   */
  template<typename... Args>
  class Signal
  {
  private:
    struct Connection
    {
      // Note: when just signal or slot is locked, always lock connection when accessing it
      mutable std::mutex mutex;
      Slot<Args...>* slot = nullptr;
      Signal<Args...>* signal = nullptr;
    };

  private:
    // Note: when signal and slot mutexes are locked at the same time, always lock signal's one first
    mutable std::mutex mutex;
    std::vector<std::shared_ptr<Connection>> connections;

  public:
    friend class Slot<Args...>;

    Signal() = default;
    Signal(const Signal&) = delete;
    Signal(Signal&&) = delete;

    Signal& operator=(const Signal&) = delete;
    Signal& operator=(Signal&&) = delete;

    virtual ~Signal();

    /** Emit signal.
     * All connected slots are called.
     * @param args
     */
    void Emit(const Args&... args) const;

    /** Connect Signal to given Slot
     * @param slot
     */
    void Connect(Slot<Args...> &slot);

    /** Disconnect from specific slot.
     * When there is no connection with given slot, Method is no-op.
     * @param slot
     */
    void Disconnect(Slot<Args...> &slot);

    /** Disconnect from all slots
     */
    void Disconnect();
  };

  template<typename... Args>
  class Slot
  {
  private:
    // Note: when signal and slot mutexes are locked at the same time, always lock signal's one first
    mutable std::mutex mutex;
    const std::function<void(const Args&...)> callback;
    std::vector<std::shared_ptr<typename Signal<Args...>::Connection>> connections;

  public:
    friend class Signal<Args...>;

    /** Construct slot with callback function.
     * @param callback
     */
    explicit Slot(const std::function<void(const Args&...)> &callback);

    Slot(const Slot&) = delete;
    Slot(Slot&&) = delete;

    Slot& operator=(const Slot&) = delete;
    Slot& operator=(Slot&&) = delete;

    virtual ~Slot();

    /** Disconnect from all signals
     */
    void Disconnect();

  private:
    /** Call the callback. Lock of specific connection have to be hold,
     * to be sure that Slot is not destructed before returning from the call.
     * @param args
     */
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
  void Signal<Args...>::Emit(const Args&... args) const
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
