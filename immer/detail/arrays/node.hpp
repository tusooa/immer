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

#include <immer/detail/util.hpp>
#include <immer/detail/combine_standard_layout.hpp>

#include <limits>

namespace immer {
namespace detail {
namespace arrays {

template <typename T, typename MemoryPolicy>
struct node
{
    using memory      = MemoryPolicy;
    using heap        = typename MemoryPolicy::heap::type;
    using transience  = typename memory::transience_t;
    using refs_t      = typename memory::refcount;
    using ownee_t     = typename transience::ownee;
    using node_t      = node;
    using edit_t      = typename transience::edit;

    struct data_t
    {
        aligned_storage_for<T> buffer;
    };

    using impl_t = combine_standard_layout_t<data_t,
                                             refs_t,
                                             ownee_t>;

    impl_t impl;

    constexpr static std::size_t sizeof_n(size_t count)
    {
        return offsetof(impl_t, d.buffer) + sizeof(T) * count;
    }

    refs_t& refs() const
    {
        return auto_const_cast(get<refs_t>(impl));
    }

    const ownee_t& ownee() const { return get<ownee_t>(impl); }
    ownee_t& ownee()             { return get<ownee_t>(impl); }

    const T* data() const { return reinterpret_cast<const T*>(&impl.d.buffer); }
    T* data()             { return reinterpret_cast<T*>(&impl.d.buffer); }

    bool can_mutate(edit_t e) const
    {
        return refs().unique()
            || ownee().can_mutate(e);
    }

    static void delete_n(node_t* p, size_t sz, size_t cap)
    {
        destroy_n(p->data(), sz);
        heap::deallocate(sizeof_n(cap), p);
    }


    static node_t* make_n(size_t n)
    {
        return new (heap::allocate(sizeof_n(n))) node_t{};
    }

    static node_t* make_e(edit_t e, size_t n)
    {
        auto p = make_n(n);
        p->ownee() = e;
        return p;
    }

    template <typename Iter>
    static node_t* copy_n(size_t n, Iter first, Iter last)
    {
        auto p = make_n(n);
        try {
            std::uninitialized_copy(first, last, p->data());
            return p;
        } catch (...) {
            heap::deallocate(sizeof_n(n), p);
            throw;
        }
    }

    static node_t* copy_n(size_t n, node_t* p, size_t count)
    {
        return copy_n(n, p->data(), p->data() + count);
    }

    template <typename Iter>
    static node_t* copy_e(edit_t e, size_t n, Iter first, Iter last)
    {
        auto p = copy_n(n, first, last);
        p->ownee() = e;
        return p;
    }

    static node_t* copy_e(edit_t e, size_t n, node_t* p, size_t count)
    {
        return copy_e(e, n, p->data(), p->data() + count);
    }
};

} // namespace arrays
} // namespace detail
} // namespace immer
