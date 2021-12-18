#ifndef OSMSCOUT_SCOPEGUARD_H
#define OSMSCOUT_SCOPEGUARD_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2021  Lukas Karas

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

namespace osmscout {

/**
 * ScopeGuard utility calls its constructor parameter (callable type)
 * in it's destructor (on the end of the scope)
 *
 * Examples:
 * ```
 *   ScopeGuard guard([]() noexcept {
 *     log.Debug() << "End of scope";
 *   });
 * ```
 *
 * @tparam CB noexcept `callable`.
 */
template <typename CB>
class ScopeGuard
{
private:
  CB cb;
  static_assert(std::is_nothrow_invocable_v<CB>, "Callback must be a nothrow (noexcept) callable");
public:
  explicit ScopeGuard(CB cb) noexcept(noexcept(std::move(cb))): cb{std::move(cb)}
  {}

  ScopeGuard(const ScopeGuard &) = delete;
  ScopeGuard(ScopeGuard && other) = delete;
  ScopeGuard& operator=(const ScopeGuard&) = delete;
  ScopeGuard& operator=(ScopeGuard&&) = delete;

  ~ScopeGuard() noexcept {
    cb();
  }
};
}

#endif // OSMSCOUT_SCOPEGUARD_H
