#pragma once

#include <algorithm>
#include <type_traits>


namespace intrusive_map {
struct base_node {
  base_node() : parent_(this) {}
  base_node(base_node const&) noexcept {}
  base_node(base_node&& rhs) noexcept;
  base_node& operator=(base_node&& rhs) noexcept;
  ~base_node();

  void swap(base_node& b);
  void unlink();
  void insert_left(base_node* left_son);
  void insert_right(base_node* right_son);
  void relink_parent(base_node* new_son) const;
  bool is_left() const;
  bool is_right() const;
  base_node* next() const;
  base_node* prev() const;

  base_node* parent_{nullptr};
  base_node* left_{nullptr};
  base_node* right_{nullptr};
};

struct default_tag {};
struct left_tag {};
struct right_tag {};

template <typename Tag>
struct opportunity_tag {
  using type = Tag;
};

template <>
struct opportunity_tag<left_tag> {
  using type = right_tag;
};

template <>
struct opportunity_tag<right_tag> {
  using type = left_tag;
};

template <typename Left, typename Right, typename Tag>
struct map_key {
  using key_t = void;
};

template <typename Left, typename Right>
struct map_key<Left, Right, left_tag> {
  using key_t = Left;
};

template <typename Left, typename Right>
struct map_key<Left, Right, right_tag> {
  using key_t = Right;
};

template <typename Left, typename Right, typename Tag>
struct map_value {
  using val_t = void;
};

template <typename Left, typename Right>
struct map_value<Left, Right, left_tag> {
  using val_t = Right;
};

template <typename Left, typename Right>
struct map_value<Left, Right, right_tag> {
  using val_t = Left;
};

template <typename Tag = default_tag>
struct map_node : public base_node {};

struct empty_bimap_node : map_node<left_tag>, map_node<right_tag> {};

template <typename Left, typename Right>
struct bimap_node : empty_bimap_node {
  bimap_node() = default;
  template<typename L, typename R>
  bimap_node(L&& left, R&& right)
      : left_value_(std::forward<L>(left)), right_value_(std::forward<R>(right)) {}

  Left left_value_;
  Right right_value_;

  template <typename Tag,
            std::enable_if_t<std::is_same_v<left_tag, Tag>, bool> = true>
  const Left& get_key() const {
    return left_value_;
  }

  template <typename Tag,
            std::enable_if_t<std::is_same_v<left_tag, Tag>, bool> = true>
  const Right& get_value() const {
    return right_value_;
  }

  template <typename Tag,
            std::enable_if_t<std::is_same_v<right_tag, Tag>, bool> = true>
  const Right& get_key() const {
    return right_value_;
  }

  template <typename Tag,
            std::enable_if_t<std::is_same_v<right_tag, Tag>, bool> = true>
  const Left& get_value() const {
    return left_value_;
  }
};

template <typename Left, typename Right, typename Tag>
bimap_node<Left, Right> const* upcast(base_node const* ptr) {
  return static_cast<bimap_node<Left, Right> const*>(
      static_cast<map_node<Tag> const*>(ptr));
}

template <typename Left, typename Right, typename Tag>
bimap_node<Left, Right>* upcast(base_node* ptr) {
  return static_cast<bimap_node<Left, Right>*>(
      static_cast<map_node<Tag>*>(ptr));
}

template <typename Tag>
empty_bimap_node const* upcast_to_empty_bimap_node(base_node const* ptr) {
  return static_cast<empty_bimap_node const*>(static_cast<map_node<Tag> const*>(ptr));
}

template <typename Tag>
base_node const* downcast(empty_bimap_node const* ptr) {
  return static_cast<base_node const*>(static_cast<map_node<Tag> const*>(ptr));
}

template <typename Left, typename Right, typename Tag>
base_node* downcast(bimap_node<Left, Right>* ptr) {
  return static_cast<base_node*>(static_cast<map_node<Tag>*>(ptr));
}
} //namespace intrusive_map
