/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#ifndef LRU_POLICY_H_
#define LRU_POLICY_H_

template<typename FullKey,
	 typename Payload,
         typename PayloadTraits
         >
struct lru_policy_traits
{
  typedef bi::list_member_hook<> policy_hook_type;
  typedef trie< FullKey, Payload, PayloadTraits, policy_hook_type> parent_trie;
  typedef typename bi::list< parent_trie,
                             bi::member_hook< parent_trie,
                                              policy_hook_type,
                                              &parent_trie::policy_hook_ > > policy_container;

  
  class policy : public policy_container
  {
  public:
    policy ()
      : max_size_ (100)
    {
    }

    inline void
    update (typename parent_trie::iterator item)
    {
      // do relocation
      policy_container::splice (policy_container::end (),
                                *this,
                                policy_container::s_iterator_to (*item));
    }
  
    inline void
    insert (typename parent_trie::iterator item)
    {
      if (policy_container::size () >= max_size_)
        {
          typename parent_trie::iterator oldItem = &(*policy_container::begin ());
          policy_container::pop_front ();
          oldItem->erase ();
        }
      
      policy_container::push_back (*item);
    }
  
    inline void
    lookup (typename parent_trie::iterator item)
    {
      // do relocation
      policy_container::splice (policy_container::end (),
                                *this,
                                policy_container::s_iterator_to (*item));
    }
  
    inline void
    erase (typename parent_trie::iterator item)
    {
      policy_container::erase (policy_container::s_iterator_to (*item));
    }

    inline void
    set_max_size (size_t max_size)
    {
      max_size_ = max_size;
    }

    inline size_t
    get_max_size () const
    {
      return max_size_;
    }

  private:
    size_t max_size_;
  };
};

#endif