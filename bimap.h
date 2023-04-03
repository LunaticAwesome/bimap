#pragma once

#include "bimap_node.h"
#include "intusive_map.h"
#include <cstddef>

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap {
private:
  using left_t = Left;
  using right_t = Right;

  using node_t = intrusive_map::bimap_node<Left, Right>;

  intrusive_map::empty_bimap_node root_{};
  intrusive_map::intrusive_map<Left, Right, intrusive_map::left_tag,
                               CompareLeft>
      left_map_;
  intrusive_map::intrusive_map<Left, Right, intrusive_map::right_tag,
                               CompareRight>
      right_map_;
  size_t size_{0};

public:
  using right_iterator =
      intrusive_map::map_iterator<left_t, right_t, intrusive_map::right_tag>;
  using left_iterator =
      intrusive_map::map_iterator<left_t, right_t, intrusive_map::left_tag>;

  // Создает bimap не содержащий ни одной пары.
  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight())
      : root_(), left_map_(root_, std::move(compare_left)),
        right_map_(root_, std::move(compare_right)) {}

  // Конструкторы от других и присваивания
  bimap(bimap const& other) : bimap() {
    for (left_iterator it = other.begin_left(); it != other.end_left(); ++it) {
      insert(*it, it.get_value());
    }
  }
  bimap(bimap&& other) noexcept
      : root_(std::move(other.root_)), size_(other.size_), left_map_(root_),
        right_map_(root_) {}

  // root_ не надо свапать, так как left_map_.swap свапает его часть с left_tag
  // а righ_map_ его часть с right_tag
  void swap(bimap& rhs) {
    left_map_.swap(rhs.left_map_);
    right_map_.swap(rhs.right_map_);
    std::swap(size_, rhs.size_);
  }

  bimap& operator=(bimap const& other) {
    if (this != &other) {
      bimap tmp(other);
      swap(tmp);
    }
    return *this;
  }
  bimap& operator=(bimap&& other) noexcept {
    if (this != &other) {
      bimap tmp(std::move(other));
      swap(tmp);
    }
    return *this;
  }

  // Деструктор. Вызывается при удалении объектов bimap.
  // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
  // (включая итераторы ссылающиеся на элементы следующие за последними).
  ~bimap() {
    if (left_map_.root_.left_) {
      recursive_delete(left_map_.root_.left_);
    }
  }
  // Вставка пары (left, right), возвращает итератор на left.
  // Если такой left или такой right уже присутствуют в bimap, вставка не
  // производится и возвращается end_left().
  left_iterator insert(left_t const& left, right_t const& right) {
    return insert_impl(left, right);
  }
  left_iterator insert(left_t const& left, right_t&& right) {
    return insert_impl(left, std::move(right));
  }
  left_iterator insert(left_t&& left, right_t const& right) {
    return insert_impl(std::move(left), right);
  }
  left_iterator insert(left_t&& left, right_t&& right) {
    return insert_impl(std::move(left), std::move(right));
  }

  // Удаляет элемент и соответствующий ему парный.
  // erase невалидного итератора неопределен.
  // erase(end_left()) и erase(end_right()) неопределены.
  // Пусть it ссылается на некоторый элемент e.
  // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
  left_iterator erase_left(left_iterator it) {
    left_iterator ret = left_map_.erase(it);
    right_map_.erase(it.flip());
    delete (upcast_left(it.ptr_));
    size_--;
    return ret;
  }
  // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе
  // не делает ничего Возвращает была ли пара удалена
  bool erase_left(left_t const& left) {
    left_iterator it = find_left(left);
    if (it == end_left()) {
      return false;
    }
    erase_left(it);
    return true;
  }

  right_iterator erase_right(right_iterator it) {
    return erase_left(it.flip()).flip();
  }
  bool erase_right(right_t const& right) {
    right_iterator it = find_right(right);
    if (it == end_right()) {
      return false;
    }
    erase_right(it);
    return true;
  }

  // erase от ренжа, удаляет [first, last), возвращает итератор на последний
  // элемент за удаленной последовательностью
  left_iterator erase_left(left_iterator first, left_iterator last) {
    while (first != last) {
      erase_left(first++);
    }
    return first;
  }
  right_iterator erase_right(right_iterator first, right_iterator last) {
    while (first != last) {
      erase_right(first++);
    }
    return first;
  }

  // Возвращает итератор по элементу. Если не найден - соответствующий end()
  left_iterator find_left(left_t const& left) const {
    return left_map_.find(left);
  }
  right_iterator find_right(right_t const& right) const {
    return right_map_.find(right);
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует -- бросает std::out_of_range
  right_t const& at_left(left_t const& key) const {
    left_iterator it = find_left(key);
    if (it == end_left()) {
      throw std::out_of_range("invalid key");
    }
    return it.get_value();
  }
  left_t const& at_right(right_t const& key) const {
    right_iterator it = find_right(key);
    if (it == end_right()) {
      throw std::out_of_range("invalid key");
    }
    return it.get_value();
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует, добавляет его в bimap и на противоположную
  // сторону кладет дефолтный элемент, ссылку на который и возвращает
  // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
  // соответствующий ему элемент на запрашиваемый (смотри тесты)

  template <typename T = left_t, typename = std::enable_if_t<
                                     std::is_same_v<T, left_t> &&
                                     std::is_default_constructible_v<right_t>>>
  right_t const& at_left_or_default(left_t const& key) {
    intrusive_map::base_node* left_it = left_map_.find_impl(key);
    if (left_map_.cmp(left_it, key) == 0) {
      return upcast_left(left_it)->right_value_;
    } else {
      right_t def = right_t();
      intrusive_map::base_node* right_it = right_map_.find_impl(def);
      if (right_map_.cmp(right_it, def) == 0) {
        upcast_right(right_it)->left_value_ = key;
        left_map_.erase_impl(left_it);
        left_map_.insert(*upcast_left(left_it));
        return upcast_right(right_it)->right_value_;
      } else {
        return insert(key, std::move(def)).get_value();
      }
    }
  }
  template <typename T = right_t, typename = std::enable_if_t<
                                      std::is_same_v<T, right_t> &&
                                      std::is_default_constructible_v<left_t>>>
  left_t const& at_right_or_default(right_t const& key) {
    intrusive_map::base_node* right_it = right_map_.find_impl(key);
    if (right_map_.cmp(right_it, key) == 0) {
      return upcast_left(right_it)->left_value_;
    } else {
      left_t def = left_t();
      intrusive_map::base_node* left_it = left_map_.find_impl(def);
      if (left_map_.cmp(left_it, def) == 0) {
        upcast_left(left_it)->right_value_ = key;
        left_map_.erase_impl(right_it);
        right_map_.insert(*upcast_right(right_it));
        return upcast_left(left_it)->left_value_;
      } else {
        return *insert(std::move(def), key);
      }
    }
  }

  // lower и upper bound'ы по каждой стороне
  // Возвращают итераторы на соответствующие элементы
  // Смотри std::lower_bound, std::upper_bound.
  left_iterator lower_bound_left(const left_t& left) const {
    return left_map_.lower_bound(left);
  }
  left_iterator upper_bound_left(const left_t& left) const {
    return left_map_.upper_bound(left);
  }

  right_iterator lower_bound_right(const right_t& right) const {
    return right_map_.lower_bound(right);
  }
  right_iterator upper_bound_right(const right_t& right) const {
    return right_map_.upper_bound(right);
  }

  // Возващает итератор на минимальный по порядку left.
  left_iterator begin_left() const {
    return left_map_.begin();
  }
  // Возващает итератор на следующий за последним по порядку left.
  left_iterator end_left() const {
    return left_map_.end();
  }

  // Возващает итератор на минимальный по порядку right.
  right_iterator begin_right() const {
    return right_map_.begin();
  }
  // Возващает итератор на следующий за последним по порядку right.
  right_iterator end_right() const {
    return right_map_.end();
  }

  // Проверка на пустоту
  bool empty() const {
    return size_ == 0;
  }

  // Возвращает размер бимапы (кол-во пар)
  std::size_t size() const {
    return size_;
  }

  // операторы сравнения
  friend bool operator==(bimap const& a, bimap const& b) {
    if (a.size_ != b.size_) {
      return false;
    }
    left_iterator it1 = a.begin_left();
    left_iterator it2 = b.begin_left();
    for (; it1 != a.end_left() && it2 != b.end_left(); ++it1, ++it2) {
      if (*it1 != *it2 || it1.get_value() != it2.get_value()) {
        return false;
      }
    }
    return true;
  }
  friend bool operator!=(bimap const& a, bimap const& b) {
    return !operator==(a, b);
  }

private:
  template <typename L, typename R>
  left_iterator insert_impl(L&& left, R&& right) noexcept {
    left_iterator it = end_left();
    auto* left_ptr = left_map_.find_impl(left);
    auto* right_ptr = right_map_.find_impl(right);
    if (left_map_.cmp(left_ptr, left) != 0 &&
        right_map_.cmp(right_ptr, right) != 0) {
      auto* node = new intrusive_map::bimap_node<Left, Right>(
          std::forward<L>(left), std::forward<R>(right));
      it = left_iterator(left_map_.insert_impl(left_ptr, *node));
      right_map_.insert_impl(right_ptr, *node);
      size_++;
    }
    return it;
  }

  void recursive_delete(intrusive_map::base_node* ptr) {
    if (ptr == nullptr) {
      return;
    }
    recursive_delete(ptr->left_);
    recursive_delete(ptr->right_);
    delete (upcast_left(ptr));
  }

  intrusive_map::bimap_node<Left, Right> const*
  upcast_left(intrusive_map::base_node const* p) {
    return intrusive_map::upcast<Left, Right, intrusive_map::left_tag>(p);
  }

  intrusive_map::bimap_node<Left, Right>*
  upcast_left(intrusive_map::base_node* p) {
    return intrusive_map::upcast<Left, Right, intrusive_map::left_tag>(p);
  }

  intrusive_map::bimap_node<Left, Right> const*
  upcast_right(intrusive_map::base_node const* p) {
    return intrusive_map::upcast<Left, Right, intrusive_map::right_tag>(p);
  }

  intrusive_map::bimap_node<Left, Right>*
  upcast_right(intrusive_map::base_node* p) {
    return intrusive_map::upcast<Left, Right, intrusive_map::right_tag>(p);
  }
};
