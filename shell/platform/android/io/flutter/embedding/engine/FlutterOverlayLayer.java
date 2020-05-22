// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package io.flutter.embedding.engine;

import android.view.Surface;

final public class FlutterOverlayLayer {
  final private Surface surface;
  final private long id;

  public FlutterOverlayLayer(long id, Surface surface) {
    this.id = id;
    this.surface = surface;
  }

  public long getId() {
    return id;
  }

  public Surface getSurface() {
    return surface;
  }
}