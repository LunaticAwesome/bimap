#include "bimap_node.h"

#include <algorithm>

intrusive_map::base_node::base_node(base_node&& rhs) noexcept
    : parent_(rhs.parent_), left_(rhs.left_), right_(rhs.right_) {
  if (left_) {
    left_->parent_ = this;
  }
  if (right_) {
    right_->parent_ = this;
  }
  rhs.relink_parent(this);
  rhs.unlink();
}

intrusive_map::base_node&
intrusive_map::base_node::operator=(base_node&& rhs) noexcept {
  if (this != &rhs) {
    base_node tmp(std::move(rhs));
    swap(tmp);
  }
  return *this;
}

intrusive_map::base_node::~base_node() {
  unlink();
}

void intrusive_map::base_node::unlink() {
  right_ = nullptr;
  left_ = nullptr;
  parent_ = nullptr;
}

void intrusive_map::base_node::swap(intrusive_map::base_node& b) {
  std::swap(right_, b.right_);
  std::swap(left_, b.left_);
  std::swap(parent_, b.parent_);
}

void intrusive_map::base_node::insert_left(base_node* left_son) {
  if (left_son) {
    left_son->parent_ = this;
  }
  left_ = left_son;
}
void intrusive_map::base_node::insert_right(base_node* right_son) {
  if (right_son) {
    right_son->parent_ = this;
  }
  right_ = right_son;
}
bool intrusive_map::base_node::is_left() const {
  return parent_ && parent_->left_ == this;
}
bool intrusive_map::base_node::is_right() const {
  return parent_ && parent_->right_ == this;
}
void intrusive_map::base_node::relink_parent(base_node* p) const {
  if (is_left()) {
    parent_->left_ = p;
  }
  if (is_right()) {
    parent_->right_ = p;
  }
  if (p) {
    p->parent_ = parent_;
  }
}
intrusive_map::base_node* intrusive_map::base_node::next() const {
  base_node const* cur = this;
  base_node* ret;
  if (cur->right_) {
    ret = cur->right_;
    while (ret->left_) {
      ret = ret->left_;
    }
  } else {
    while (cur->is_right()) {
      cur = cur->parent_;
    }
    ret = cur->parent_;
  }
  return ret;
}
intrusive_map::base_node* intrusive_map::base_node::prev() const {
  base_node const* cur = this;
  base_node* ret;
  if (cur->left_) {
    ret = cur->left_;
    while (ret->right_) {
      ret = ret->right_;
    }
  } else {
    while (cur->is_left()) {
      cur = cur->parent_;
    }
    ret = cur->parent_;
  }
  return ret;
}
