#ifndef LIBOSMSCOUT_CANCELABLEFUTURE_H
#define LIBOSMSCOUT_CANCELABLEFUTURE_H

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

#include <future>
#include <functional>
#include <optional>
#include <vector>

namespace osmscout {

  /** Future provides mechanism to access result of asynchronous computation.
   * Instead of std::future, this one provides callbacks. So the consumer
   * of the value doesn't need to be blocked.
   *
   * @tparam T
   */
  template<typename T>
  class CancelableFuture
  {
  public:
    using DoneCallback = std::function<void(T const&)>;
    using CancelCallback = std::function<void()>;

    struct State
    {
      std::mutex mutex;
      std::optional<T> value;
      bool canceled=false;
      std::vector<DoneCallback> callbacks;
      std::vector<CancelCallback> cancelCallbacks;

      void Cancel()
      {
        std::unique_lock lock(mutex);
        if (canceled || value) {
          return;
        }
        canceled=true;
        for (const auto &callback: cancelCallbacks) {
          callback();
        }
      }
    };

    class Promise
    {
    private:
      std::shared_ptr<State> state=std::make_shared<State>();
    public:
      Promise() = default;

      virtual ~Promise()
      {
        state->Cancel();
      };

      Promise(const Promise&) = delete;
      Promise(Promise&&) = default;

      Promise& operator=(const Promise&) = delete;
      Promise& operator=(Promise&&) = default;

      CancelableFuture<T> Future() const {
        return CancelableFuture<T>(state);
      }

      void Cancel()
      {
        state->Cancel();
      }

      bool IsCanceled()
      {
        std::unique_lock lock(state->mutex);
        return state->canceled;
      }

      void SetValue(const T &value)
      {
        std::unique_lock lock(state->mutex);
        if (state->canceled || state->value) {
          return;
        }
        state->value=value;
        for (const auto &callback: state->callbacks) {
          callback(value);
        }
      }
    };

  private:
    std::shared_ptr<State> state;

    CancelableFuture(const std::shared_ptr<State> &state): state(state)
    {
      // no code
    };

  public:
    CancelableFuture(const CancelableFuture &) = default;
    CancelableFuture(CancelableFuture &&) = default;

    virtual ~CancelableFuture() = default;

    CancelableFuture& operator=(const CancelableFuture &) = default;
    CancelableFuture& operator=(CancelableFuture &&) = default;

    /** Cancel the corresponding execution.
     * Cancel callbacks are executed in context of caller.
     */
    void Cancel()
    {
      state->Cancel();
    }

    /**
     * Callback triggered on future complete.
     * When future is calceled, it is never called.
     * It is called from thread of value producer.
     * When future is completed already, callback is called immediately in thread of caller.
     *
     * @param callback
     */
    void OnComplete(const DoneCallback &callback)
    {
      std::unique_lock lock(state->mutex);
      if (state->value) {
        callback(state->value.value());
      } else if (!state->canceled) {
        state->callbacks.push_back(std::move(callback));
      }
    }

    /**
     * Callback triggered when future is canceled.
     * It is called from thread that is canceling the execution.
     * When future is completed already, callback is called immediately in thread of caller.
     *
     * @param callback
     */
    void OnCancel(const CancelCallback &callback)
    {
      std::unique_lock lock(state->mutex);
      if (state->canceled) {
        callback();
      } else if (!state->value) {
        state->cancelCallbacks.push_back(std::move(callback));
      }
    };

    std::optional<T> Value()
    {
      std::unique_lock lock(state->mutex);
      return state->value;
    }

    bool IsCanceled()
    {
      std::unique_lock lock(state->mutex);
      return state->canceled;
    }
  };

}

#endif //LIBOSMSCOUT_CANCELABLEFUTURE_H
