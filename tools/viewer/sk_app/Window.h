/*
* Copyright 2016 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#ifndef Window_DEFINED
#define Window_DEFINED

#include "DisplayParams.h"
#include "SkRect.h"
#include "SkTouchGesture.h"
#include "SkTypes.h"

class SkCanvas;

namespace sk_app {

class WindowContext;

class Window {
public:
    static Window* CreateNativeWindow(void* platformData);

    virtual ~Window() {};

    virtual void setTitle(const char*) = 0;
    virtual void show() = 0;
    virtual void inval() = 0;

    virtual bool scaleContentToFit() const { return false; }
    virtual bool supportsContentRect() const { return false; }
    virtual SkRect getContentRect() { return SkRect::MakeEmpty(); }

    enum BackEndType {
        kNativeGL_BackendType,
        kVulkan_BackendType
    };

    virtual bool attach(BackEndType attachType,  const DisplayParams& params) = 0;
    void detach();

    // input handling
    enum class Key {
        kNONE,    //corresponds to android's UNKNOWN

        kLeftSoftKey,
        kRightSoftKey,

        kHome,    //!< the home key - added to match android
        kBack,    //!< (CLR)
        kSend,    //!< the green (talk) key
        kEnd,     //!< the red key

        k0,
        k1,
        k2,
        k3,
        k4,
        k5,
        k6,
        k7,
        k8,
        k9,
        kStar,    //!< the * key
        kHash,    //!< the # key

        kUp,
        kDown,
        kLeft,
        kRight,

        kOK,      //!< the center key

        kVolUp,   //!< volume up    - match android
        kVolDown, //!< volume down  - same
        kPower,   //!< power button - same
        kCamera,  //!< camera       - same

        kLast = kCamera
    };
    static const int kKeyCount = static_cast<int>(Key::kLast) + 1;

    enum ModifierKeys {
        kShift_ModifierKey = 1 << 0,
        kControl_ModifierKey = 1 << 1,
        kOption_ModifierKey = 1 << 2,   // same as ALT
        kCommand_ModifierKey = 1 << 3,
        kFirstPress_ModifierKey = 1 << 4,
    };

    enum InputState {
        kDown_InputState,
        kUp_InputState,
        kMove_InputState   // only valid for mouse
    };

    // return value of 'true' means 'I have handled this event'
    typedef bool(*OnCharFunc)(SkUnichar c, uint32_t modifiers, void* userData);
    typedef bool(*OnKeyFunc)(Key key, InputState state, uint32_t modifiers, void* userData);
    typedef bool(*OnMouseFunc)(int x, int y, InputState state, uint32_t modifiers, void* userData);
    typedef bool(*OnTouchFunc)(int owner, InputState state, float x, float y, void* userData);
    typedef void(*OnPaintFunc)(SkCanvas*, void* userData);

    void registerCharFunc(OnCharFunc func, void* userData) {
        fCharFunc = func;
        fCharUserData = userData;
    }

    void registerKeyFunc(OnKeyFunc func, void* userData) {
        fKeyFunc = func;
        fKeyUserData = userData;
    }

    void registerMouseFunc(OnMouseFunc func, void* userData) {
        fMouseFunc = func;
        fMouseUserData = userData;
    }

    void registerPaintFunc(OnPaintFunc func, void* userData) {
        fPaintFunc = func;
        fPaintUserData = userData;
    }

    void registerTouchFunc(OnTouchFunc func, void* userData) {
        fTouchFunc = func;
        fTouchUserData = userData;
    }

    bool onChar(SkUnichar c, uint32_t modifiers);
    bool onKey(Key key, InputState state, uint32_t modifiers);
    bool onMouse(int x, int y, InputState state, uint32_t modifiers);
    bool onTouch(int owner, InputState state, float x, float y);  // multi-owner = multi-touch
    void onPaint();
    void onResize(uint32_t width, uint32_t height);

    uint32_t width() { return fWidth; }
    uint32_t height() { return fHeight;  }

    virtual const DisplayParams& getDisplayParams();
    void setDisplayParams(const DisplayParams& params);

protected:
    Window();

    uint32_t     fWidth;
    uint32_t     fHeight;

    OnCharFunc   fCharFunc;
    void*        fCharUserData;
    OnKeyFunc    fKeyFunc;
    void*        fKeyUserData;
    OnMouseFunc  fMouseFunc;
    void*        fMouseUserData;
    OnTouchFunc  fTouchFunc;
    void*        fTouchUserData;
    OnPaintFunc  fPaintFunc;
    void*        fPaintUserData;

    WindowContext* fWindowContext = nullptr;
};

}   // namespace sk_app
#endif
