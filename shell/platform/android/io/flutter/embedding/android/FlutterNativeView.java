// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package io.flutter.embedding.android;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import io.flutter.Log;
import io.flutter.embedding.engine.renderer.FlutterRenderer;
import io.flutter.embedding.engine.renderer.FlutterUiDisplayListener;
import io.flutter.embedding.engine.renderer.RenderSurface;

import android.media.ImageReader;
import android.graphics.PixelFormat;
import android.graphics.Canvas;


public class FlutterNativeView extends View implements RenderSurface, ImageReader.OnImageAvailableListener  {
  private static final String TAG = "FlutterNativeView";

  private boolean isSurfaceAvailableForRendering = false;
  private boolean isAttachedToFlutterRenderer = false;
  private boolean isOverlay = false;

  @Nullable private FlutterRenderer flutterRenderer;

  private final FlutterUiDisplayListener flutterUiDisplayListener =
      new FlutterUiDisplayListener() {
        @Override
        public void onFlutterUiDisplayed() {
          Log.v(TAG, "onFlutterUiDisplayed()");
          // Now that a frame is ready to display, take this SurfaceView from transparent to opaque.
          setAlpha(1.0f);
          invalidate();

          if (flutterRenderer != null) {
            flutterRenderer.removeIsDisplayingFlutterUiListener(this);
          }
        }

        @Override
        public void onFlutterUiNoLongerDisplayed() {
          // no-op
        }
      };

  public FlutterNativeView(@NonNull Context context) {
    this(context, null, null);
  }
 
  public FlutterNativeView(@NonNull Context context, @Nullable AttributeSet attrs, @Nullable ImageReader reader) {
    super(context, attrs);
    if (reader != null) {
      this.reader = reader;
      this.isOverlay = true;
    } else {
      setAlpha(0.0f);
    }
  }

  // @Override
  // protected void onSizeChanged(int width, int height, int oldw, int oldh) {
  //   super.onSizeChanged(width, height, oldw, oldh);
  //   changeSurfaceSize(width, height);
  // }

  // @Override
  // protected void onDetachedFromWindow() {
  //   super.onDetachedFromWindow();
  //   Log.v(TAG, "SurfaceHolder.Callback.stopRenderingToSurface()");
  //   isSurfaceAvailableForRendering = false;
  //   if (isAttachedToFlutterRenderer) {
  //     disconnectSurfaceFromRenderer();
  //   }
  // }
  
  private boolean globalListenersAdded = false;
  private boolean hasFrame = false;

  private final android.view.ViewTreeObserver.OnPreDrawListener drawListener =
    new android.view.ViewTreeObserver.OnPreDrawListener() {
        @Override
        public boolean onPreDraw() {
            viewCreated();
            return true;
        }
    };

  @Override
  protected void onAttachedToWindow() {
    super.onAttachedToWindow();

    if (!isOverlay && !globalListenersAdded) {
      android.view.ViewTreeObserver observer = getViewTreeObserver();
      observer.addOnPreDrawListener(drawListener);
      globalListenersAdded = true;
    }
  }

  private boolean viewWasCreated = false;
  private android.graphics.Bitmap bitmap;

  private ImageReader reader;

  private void viewCreated() {
    if (getWidth() == 0 || getHeight() == 0) {
      return;
    }
    if (viewWasCreated) {
      return;
    }
    viewWasCreated = true;

    reader = ImageReader.newInstance(
      getWidth(),
      getHeight(),
      PixelFormat.RGBA_8888,
      2);
    // android.hardware.HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE | android.hardware.HardwareBuffer.USAGE_GPU_COLOR_OUTPUT

    reader.setOnImageAvailableListener(this, null);

    isSurfaceAvailableForRendering = true;

    if (isAttachedToFlutterRenderer) {
      connectSurfaceToRenderer();
    }
  }

  public int numImage = 0;
  @Override
  public void onImageAvailable(ImageReader reader) {
    Log.e("flutter", "==============================> IMAGE available " + numImage);
    numImage++;

    // this.invalidate();
  }

  private android.media.Image image;
  ///
  /// 
  /// 
  int captured = 0;
  @Nullable private android.hardware.HardwareBuffer hardwareBuffer;

  @Override
  public void acquireLatestImage() {
    if (reader == null) {
      Log.d("flutter", " (NO READER) ");
      return;
    }
    image = reader.acquireLatestImage();
    if (image == null) {
      Log.d("flutter", " (NO IMAGE) ");
      return;
    }
    // hardwareBuffer = image.getHardwareBuffer();
    // bitmap = android.graphics.Bitmap.wrapHardwareBuffer(hardwareBuffer, android.graphics.ColorSpace.get(android.graphics.ColorSpace.Named.SRGB));

    Log.d("flutter", " CAPTURED IMAGE " + captured);
    captured++;

    android.media.Image.Plane[] imagePlanes = image.getPlanes();
    android.media.Image.Plane imagePlane = imagePlanes[0];
    java.nio.ByteBuffer byteBuffer = imagePlane.getBuffer();

    int desiredWidth = imagePlane.getRowStride() / imagePlane.getPixelStride();
    int desiredHeight = image.getHeight();

    bitmap = android.graphics.Bitmap.createBitmap(
      desiredWidth,
      desiredHeight,
      android.graphics.Bitmap.Config.ARGB_8888
    );

    bitmap.copyPixelsFromBuffer(byteBuffer);
    
    image.close();
    image = null;
  
    invalidate();
  }

  @Override
  protected void onDraw(Canvas canvas) {
    super.onDraw(canvas);
    if (bitmap == null) {
      return;
    }

    Log.d("flutter", " onDraw(Canvas canvas) ");

  
    canvas.drawBitmap(bitmap, 0, 0, null);
    bitmap = null;

    if (isOverlay) {
      android.graphics.Paint paint = new android.graphics.Paint();
      paint.setStyle(android.graphics.Paint.Style.STROKE);
      paint.setColor(android.graphics.Color.BLACK);
      canvas.drawRect(new android.graphics.Rect(0, 0, getWidth()-1, getHeight()-1), paint);
    }


    // if (imagePlanes.length != 1) {
    //   Log.e("flutter", "==============================> PLANES.LENGTH != 1 ");
    //   return;
    // }

    // android.media.Image.Plane imagePlane = imagePlanes[0];
    // java.nio.ByteBuffer byteBuffer = imagePlane.getBuffer();

    // int desiredWidth = imagePlane.getRowStride() / imagePlane.getPixelStride();
    // int desiredHeight = image.getHeight();

    // if (bitmap == null) {
    //   bitmap = android.graphics.Bitmap.createBitmap(
    //     desiredWidth,
    //     desiredHeight,
    //     android.graphics.Bitmap.Config.ARGB_8888
    //   );
    // }

    // bitmap.copyPixelsFromBuffer(byteBuffer);
  }

  @Nullable
  @Override
  public FlutterRenderer getAttachedRenderer() {
    return flutterRenderer;
  }

  /**
   * Invoked by the owner of this {@code FlutterSurfaceView} when it wants to begin rendering a
   * Flutter UI to this {@code FlutterSurfaceView}.
   *
   * <p>If an Android {@link android.view.Surface} is available, this method will give that {@link
   * android.view.Surface} to the given {@link FlutterRenderer} to begin rendering Flutter's UI to
   * this {@code FlutterSurfaceView}.
   *
   * <p>If no Android {@link android.view.Surface} is available yet, this {@code FlutterSurfaceView}
   * will wait until a {@link android.view.Surface} becomes available and then give that {@link
   * android.view.Surface} to the given {@link FlutterRenderer} to begin rendering Flutter's UI to
   * this {@code FlutterSurfaceView}.
   */
  public void attachToRenderer(@NonNull FlutterRenderer flutterRenderer) {
    Log.v(TAG, "Attaching to FlutterRenderer.");
    if (this.flutterRenderer != null) {
      Log.v(
          TAG,
          "Already connected to a FlutterRenderer. Detaching from old one and attaching to new one.");
      this.flutterRenderer.stopRenderingToSurface();
      this.flutterRenderer.removeIsDisplayingFlutterUiListener(flutterUiDisplayListener);
    }

    this.flutterRenderer = flutterRenderer;
    isAttachedToFlutterRenderer = true;

    this.flutterRenderer.addIsDisplayingFlutterUiListener(flutterUiDisplayListener);

    // If we're already attached to an Android window then we're now attached to both a renderer
    // and the Android window. We can begin rendering now.
    if (isSurfaceAvailableForRendering) {
      Log.v(
          TAG,
          "Surface is available for rendering. Connecting FlutterRenderer to Android surface.");
      connectSurfaceToRenderer();
    }
  }

  /**
   * Invoked by the owner of this {@code FlutterSurfaceView} when it no longer wants to render a
   * Flutter UI to this {@code FlutterSurfaceView}.
   *
   * <p>This method will cease any on-going rendering from Flutter to this {@code
   * FlutterSurfaceView}.
   */
  public void detachFromRenderer() {
    if (flutterRenderer != null) {
      // If we're attached to an Android window then we were rendering a Flutter UI. Now that
      // this FlutterSurfaceView is detached from the FlutterRenderer, we need to stop rendering.
      // TODO(mattcarroll): introduce a isRendererConnectedToSurface() to wrap "getWindowToken() !=
      // null"
      if (getWindowToken() != null) {
        Log.v(TAG, "Disconnecting FlutterRenderer from Android surface.");
        disconnectSurfaceFromRenderer();
      }

      // Make the SurfaceView invisible to avoid showing a black rectangle.
      setAlpha(0.0f);

      this.flutterRenderer.removeIsDisplayingFlutterUiListener(flutterUiDisplayListener);

      flutterRenderer = null;
      isAttachedToFlutterRenderer = false;
    } else {
      Log.w(TAG, "detachFromRenderer() invoked when no FlutterRenderer was attached.");
    }
  }

  // FlutterRenderer and getSurfaceTexture() must both be non-null.
  private void connectSurfaceToRenderer() {
    if (flutterRenderer == null) {
      throw new IllegalStateException(
          "connectSurfaceToRenderer() should only be called when flutterRenderer is non-null.");
    }
    flutterRenderer.startRenderingToSurface(reader.getSurface());
  }

  // FlutterRenderer must be non-null.
  private void changeSurfaceSize(int width, int height) {
    if (flutterRenderer == null) {
      throw new IllegalStateException(
          "changeSurfaceSize() should only be called when flutterRenderer is non-null.");
    }

    Log.v(
        TAG,
        "Notifying FlutterRenderer that Android surface size has changed to "
            + width
            + " x "
            + height);
    flutterRenderer.surfaceChanged(width, height);
  }

  // FlutterRenderer must be non-null.
  private void disconnectSurfaceFromRenderer() {
    if (flutterRenderer == null) {
      throw new IllegalStateException(
          "disconnectSurfaceFromRenderer() should only be called when flutterRenderer is non-null.");
    }
    flutterRenderer.stopRenderingToSurface();
  }
}
