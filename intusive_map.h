#pragma once

#include "bimap_node.h"
#include <functional>
#include <iostream>

template <typename L, typename R, typename C1, typename C2>
struct bimap;

namespace intrusive_map {
template <typename Left, typename Right, typename Tag>
struct map_iterator;

// Эту intrusive_map можно превратить в нормальную intrusive_map, где
// key_t == Left, val_t == Right,
// где вместо insert(bimap_node<Left, Right>) будет insert(Left, Right),
// если просто добавить дополнительный класс, который будет создавать
// bimap_node и удалять его соответственно, и делать вызовы методов этой map.
// В целом, я не хочу чтобы ей можно было пользоваться без какой-либо обертки.
template <typename Left, typename Right, typename Tag = left_tag,
          typename Compare = std::less<Left>>
class intrusive_map : private Compare {
  using key_t = typename map_key<Left, Right, Tag>::key_t;
  using val_t = typename map_value<Left, Right, Tag>::val_t;

  template <typename L, typename R, typename C1, typename C2>
  friend struct ::bimap;

public:
  using iterator = map_iterator<Left, Right, Tag>;

  intrusive_map() = delete;
  intrusive_map(empty_bimap_node& root)
      : root_(static_cast<base_node&>(static_cast<map_node<Tag>&>(root))) {}
  intrusive_map(intrusive_map const&) = delete;
  intrusive_map(intrusive_map&&) = default;
  intrusive_map(empty_bimap_node& root, Compare compare)
      : Compare(std::move(compare)),
        root_(static_cast<base_node&>(static_cast<map_node<Tag>&>(root))) {}

  void swap(intrusive_map& rhs) {
    //    base_node* ptr = rhs_root.right_;
    //    rhs_root.insert_right(root.right_);
    //    root.insert_right(ptr);
    base_node* ptr = rhs.root_.left_;
    rhs.root_.insert_left(root_.left_);
    root_.insert_left(ptr);
  }

  iterator insert(bimap_node<Left, Right>& val) {
    return iterator(insert_impl(find_impl(val.template get_key<Tag>()), val));
  }

  iterator erase(iterator it) {
    return iterator(erase_impl(it.ptr_));
  }

  iterator find(key_t const& val) const {
    iterator it = iterator(find_impl(val));
    if (cmp(it, val) == 0) {
      return it;
    } else {
      return end();
    }
  }

  iterator begin() const {
    iterator it = end();
    while (it.ptr_->left_) {
      it.ptr_ = it.ptr_->left_;
    }
    return it;
  }

  iterator end() const {
    return intrusive_map::iterator(&root_);
  }

  iterator lower_bound(key_t const& val) const {
    iterator it = iterator(find_impl(val));
    if (cmp(it, val) == -1) {
      ++it;
    }
    return it;
  }

  iterator upper_bound(key_t const& val) const {
    iterator it = iterator(find_impl(val));
    if (cmp(it, val) <= 0) {
      ++it;
    }
    return it;
  }

private:
  // можно не хранить ссылку на root_, а передавать его в методах,
  // но это выглядит очень неприятно
  base_node& root_;
  // Возвращает указатель на элемент, ключ которого скорее всего равен, т.е
  // или его left_ == nullptr и *it > val, или right_ == nullptr и *it < val,
  // или *it == val, сравнения выполняются в терминах функции cmp
  base_node* find_impl(key_t const& val) const {
    base_node* it = const_cast<base_node*>(&root_);
    while (true) {
      int cmp_val = cmp(it, val);
      if (cmp_val == 1) {
        if (it->left_ == nullptr) {
          break;
        }
        it = it->left_;
      } else if (cmp_val == 0) {
        break;
      } else {
        if (it->right_ == nullptr) {
          break;
        }
        it = it->right_;
      }
    }
    return it;
  }

  // Удаляет элемент по указателю
  base_node* erase_impl(base_node const* it) {
    base_node* ret = it->next();
    if (it->left_ == nullptr && it->right_ == nullptr) {
      it->relink_parent(nullptr);
    } else if (it->left_ == nullptr) {
      it->relink_parent(it->right_);
    } else if (it->right_ == nullptr) {
      it->relink_parent(it->left_);
    } else {
      erase_impl(ret);
      it->relink_parent(ret);
      ret->insert_left(it->left_);
      ret->insert_right(it->right_);
    }
    return ret;
  }

  int cmp(iterator a, key_t const& key_b) const {
    return cmp(a.ptr_, key_b);
  }
  int cmp(base_node const* a, key_t const& key_b) const {
    if (a == &root_) {
      return 1;
    }
    key_t const& key_a = upcast<Left, Right, Tag>(a)->template get_key<Tag>();
    return cmp(key_a, key_b);
  }
  // returns -1 key_a < key_b; 0 key_a == key_b; 1 key_a > key_b
  // root_->key == +inf
  int cmp(key_t const& key_a, key_t const& key_b) const {
    if (this->operator()(key_a, key_b)) {
      return -1;
    }
    if (this->operator()(key_b, key_a)) {
      return 1;
    }
    return 0;
  }

  // Делает вставку по указателю,
  // можно использовать только если it был получен из find_impl
  // с тем же val.get_key(), т. к. может сломать инвариант
  base_node* insert_impl(base_node* it, bimap_node<Left, Right>& val) {
    int cmp_val = cmp(it, val.template get_key<Tag>());
    if (cmp_val == 1) {
      it->insert_left(downcast<Left, Right, Tag>(&val));
      it = it->left_;
    } else if (cmp_val == 0) {
      it = &root_;
    } else {
      it->insert_right(downcast<Left, Right, Tag>(&val));
      it = it->right_;
    }
    return it;
  }
};

template <typename Left, typename Right, typename Tag>
struct map_iterator {
  using key_t = typename map_key<Left, Right, Tag>::key_t;
  using val_t = typename map_value<Left, Right, Tag>::val_t;

  template <typename L, typename R, typename T, typename C>
  friend class intrusive_map;

  template <typename L, typename R, typename C1, typename C2>
  friend struct ::bimap;

  friend struct map_iterator<Left, Right, typename opportunity_tag<Tag>::type>;

  map_iterator() = default;
  map_iterator(std::nullptr_t) = delete;
  map_iterator(map_iterator const& rhs) = default;
  map_iterator& operator=(map_iterator const& rhs) = default;
  key_t const& operator*() const {
    return upcast<Left, Right, Tag>(ptr_)->template get_key<Tag>();
  }

  key_t const* operator->() const {
    return &upcast<Left, Right, Tag>(ptr_)->template get_key<Tag>();
  }

  val_t const& get_value() const {
    return upcast<Left, Right, Tag>(ptr_)->template get_value<Tag>();
  }

  map_iterator& operator++() {
    ptr_ = ptr_->next();
    return *this;
  }

  map_iterator operator++(int) {
    map_iterator ret = *this;
    ++*this;
    return ret;
  }

  map_iterator& operator--() {
    ptr_ = ptr_->prev();
    return *this;
  }

  map_iterator operator--(int) {
    map_iterator ret = *this;
    --*this;
    return ret;
  }

  friend bool operator==(map_iterator const& a, map_iterator const& b) {
    return a.ptr_ == b.ptr_;
  }

  friend bool operator!=(map_iterator const& a, map_iterator const& b) {
    return a.ptr_ != b.ptr_;
  }

  map_iterator<Left, Right, typename opportunity_tag<Tag>::type> flip() {
    return map_iterator<Left, Right, typename opportunity_tag<Tag>::type>(
        upcast_to_empty_bimap_node<Tag>(ptr_));
  }

private:
  base_node const* ptr_{nullptr};
  explicit map_iterator(base_node* ptr) : ptr_(ptr) {}
  explicit map_iterator(base_node const* ptr) : ptr_(ptr) {}
  explicit map_iterator(empty_bimap_node const* val)
      : ptr_(downcast<Tag>(val)) {}
};
} // namespace intrusive_map
