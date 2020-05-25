// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package io.flutter.view;

import android.view.Choreographer;
import android.view.WindowManager;
import androidx.annotation.NonNull;
import io.flutter.embedding.engine.FlutterJNI;
import io.flutter.Log;

// TODO(mattcarroll): add javadoc.
public class VsyncWaiter {
  private static VsyncWaiter instance;

  @NonNull
  public static VsyncWaiter getInstance(@NonNull WindowManager windowManager) {
    if (instance == null) {
      instance = new VsyncWaiter(windowManager);
    }
    return instance;
  }

  @NonNull private final WindowManager windowManager;

  private final FlutterJNI.AsyncWaitForVsyncDelegate asyncWaitForVsyncDelegate =
      new FlutterJNI.AsyncWaitForVsyncDelegate() {
        private int frame = 0;
        @Override
        public void asyncWaitForVsync(long cookie) {
          Log.e("flutter", " ============= asyncWaitForVsync ============= ");
          // io.flutter.embedding.android.FlutterView.globalView.adquireLatestSurfaceImage();

          Choreographer.getInstance()
              .postFrameCallback(
                  new Choreographer.FrameCallback() {
                    @Override
                    public void doFrame(long frameTimeNanos) {
                      Log.e("flutter", " ============= doFrame "+frame+" ============= ");
                      float fps = windowManager.getDefaultDisplay().getRefreshRate();
                      long refreshPeriodNanos = (long) (1000000000.0 / fps);
                      FlutterJNI.nativeOnVsync(
                          frameTimeNanos, frameTimeNanos + refreshPeriodNanos, cookie);
                     Log.e("flutter", " =============/ end doFrame  "+frame+" ============= ");

                        Choreographer.getInstance()
              .postFrameCallback(
                  new Choreographer.FrameCallback() {
                    @Override
                    public void doFrame(long frameTimeNanos) {
                     Log.e("flutter", " ============= FINISHED FRAME  "+frame+" ============= ");
                     frame++;

                    }
                  });

                    }
                  });
        }
      };

  private VsyncWaiter(@NonNull WindowManager windowManager) {
    this.windowManager = windowManager;
  }

  public void init() {
    FlutterJNI.setAsyncWaitForVsyncDelegate(asyncWaitForVsyncDelegate);

    // TODO(mattcarroll): look into moving FPS reporting to a plugin
    float fps = windowManager.getDefaultDisplay().getRefreshRate();
    FlutterJNI.setRefreshRateFPS(fps);
  }
}
