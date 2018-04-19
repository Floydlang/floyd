//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

namespace scm {
namespace detail {

struct none_t;

template <typename... Ts>
struct pack {};

template <typename Pack>
struct pack_size;

template <typename... Ts>
struct pack_size<pack<Ts...>>
{
    static constexpr auto value = sizeof...(Ts);
};

template <typename Pack>
constexpr auto pack_size_v = pack_size<Pack>::value;

template <typename Pack>
struct pack_last
{
    using type = none_t;
};

template <typename T, typename ...Ts>
struct pack_last<pack<T, Ts...>>
    : pack_last<pack<Ts...>>
{};

template <typename T>
struct pack_last<pack<T>>
{
    using type = T;
};

template <typename Pack>
using pack_last_t = typename pack_last<Pack>::type;

} // namespace detail
} // namespace scm
