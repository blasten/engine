// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_LIB_UI_PAINTING_PATCHED_R_TREE_H_
#define FLUTTER_LIB_UI_PAINTING_PATCHED_R_TREE_H_

#include "third_party/skia/include/core/SkTypes.h"
#include "third_party/skia/include/core/SkBBHFactory.h"

namespace flutter {

class PatchedRTree : public SkBBoxHierarchy {
public:
  PatchedRTree() = default;

  void insert(const SkRect[], int N) override;
  void search(const SkRect& query, std::vector<int>* results) const override;
  size_t bytesUsed() const override;

  // Methods and constants below here are only public for tests.

  // Return the depth of the tree structure.
  int getDepth() const { return fCount ? fRoot.fSubtree->fLevel + 1 : 0; }
  // Insertion count (not overall node count, which may be greater).
  int getCount() const { return fCount; }

  // These values were empirically determined to produce reasonable performance in most cases.
  static const int kMinChildren = 6,
                   kMaxChildren = 11;

private:
  struct Node;

  struct Branch {
      union {
          Node* fSubtree;
          int fOpIndex;
      };
      SkRect fBounds;
  };

  struct Node {
      uint16_t fNumChildren;
      uint16_t fLevel;
      Branch fChildren[kMaxChildren];
  };

  void search(Node* root, const SkRect& query, std::vector<int>* results) const;

  // Consumes the input array.
  Branch bulkLoad(std::vector<Branch>* branches, int level = 0);

  // How many times will bulkLoad() call allocateNodeAtLevel()?
  static int CountNodes(int branches);

  Node* allocateNodeAtLevel(uint16_t level);

  // This is the count of data elements (rather than total nodes in the tree)
  int fCount;
  Branch fRoot;
  std::vector<Node> fNodes;
};

class PatchedRTreeFactory : public SkBBHFactory {
public:
  sk_sp<SkBBoxHierarchy> operator()() const override;
};

}  // namespace flutter

#endif  // FLUTTER_LIB_UI_PAINTING_PATCHED_R_TREE_H_
