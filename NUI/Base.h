#pragma once

#include <vector>
#include <string>
#include <nanovg/nanovg.h>

#include "Config.h"

namespace nui {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> static T clamp(T value, T min, T max) { return value < min ? min : value > max ? max : value; }
template <typename T> static T minimum(T a, T b) { return a < b ? a : b; }
template <typename T> static T maximum(T a, T b) {   return a > b ? a : b;}
template <typename T> static T clampMinimum(T value, T min) { return (min && value < min) ? min : value; }
template <typename T> static T clampMaximum(T value, T max) { return (max && value > max) ? max : value; }
template <typename T> static T clampMinMax(T value, T min, T max) { return clampMinimum(clampMaximum(value, max), min); }
template <typename T> static void swapValues(T &v1, T &v2) { T temp = v1; v1 = v2; v2 = temp; }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct State
{
  enum Flags
  {
    None = 0,
    Disabled = 1,
    Hot = Disabled << 1,
    Down = Hot << 1,
    Grabbed = Down << 1,
    Focused = Grabbed << 1,
    DeepFocused = Focused << 1,
    Moving = DeepFocused << 1,
    Resizing = Moving << 1,
    Exclusive = Resizing << 1,
    Selected = Exclusive << 1
  };
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum class MouseButton
{
  None = 0,
  Left = 1,
  Middle = Left << 1,
  Right = Middle << 1
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Edge
{
  enum Flags
  {
    None = 0,
    Left = 1,
    Top = Left << 1,
    Right = Top << 1,
    Bottom = Right << 1,
    TopLeft = Top | Left,
    TopRight = Top | Right,
    BottomLeft = Bottom | Left,
    BottomRight = Bottom | Right,
    All = Left | Top | Right | Bottom,
    LeftOrRight = Left | Right,
    TopOrBottom = Top | Bottom
  };
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum class Docking
{
  None = 0,
  Top,
  Right,
  Bottom,
  Left,
  Client,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum class MouseCursor
{
  Default = 0,
  Move,
  ResizeH,
  ResizeV,
  ResizeLR,
  ResizeRL,
  Text
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Vec2
{
  int x, y;

  Vec2()
    : x(0)
    , y(0)
  {
  
  }

  Vec2(int _x, int _y)
    : x(_x)
    , y(_y)
  {
  
  }

  bool set(int _x, int _y)
  {
    if (x != _x || y != _y)
    {
      x = _x;
      y = _y;

      return true;
    }

    return false;
  }

  bool set(const Vec2 &v) { return set(v.x, v.y); }

  Vec2 &operator+=(const Vec2 &v) { x += v.x; y += v.y; return *this; }

  Vec2 operator+(const Vec2 &v) const { return Vec2(x + v.x, y + v.y); }

  Vec2 &operator-=(const Vec2 &v) { x -= v.x; y -= v.y; return *this; }

  Vec2 operator-(const Vec2 &v) const { return Vec2(x - v.x, y - v.y); }

  Vec2 &operator*=(int value) { x *= value; y *= value; return *this; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct RGB
{
  float r, g, b;

  RGB(): r(1.0f), g(1.0f), b(1.0f) { }
  RGB(float _r, float _g, float _b): r(_r), g(_g), b(_b) { }
  RGB(unsigned rgb)
  {
    r = static_cast<float>((rgb >> 16) & 0xFF) / 255.0f;
    g = static_cast<float>((rgb >> 8) & 0xFF) / 255.0f;
    b = static_cast<float>(rgb & 0xFF) / 255.0f;
  }

  void set(float _r, float _g, float _b) { r = _r; g = _g; b = _b; }

  RGB operator+(float coef) const { return RGB(r + coef, g + coef, b + coef); }
  RGB operator-(float coef) const { return RGB(r - coef, g - coef, b - coef); }
  RGB operator*(float coef) const { return RGB(r * coef, g * coef, b * coef); }

  bool operator==(const RGB &rgb) const { return r == rgb.r && g == rgb.g && b == rgb.b; }
  bool operator!=(const RGB &rgb) const { return r != rgb.r || g != rgb.g || b != rgb.b; }

  NVGcolor nvg() const { return nvgRGBf(r, g, b); }
  NVGcolor nvg(float coef) const { return nvgRGBf(r * coef, g * coef, b * coef); }
  NVGcolor nvgA(float a) const { return nvgRGBAf(r, g, b, a); }
  NVGcolor nvgA(float a, float coef) const { return nvgRGBAf(r * coef, g * coef, b * coef, a); }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Borders
{
  int top, right, bottom, left;

  Borders()
    : top(0)
    , right(0)
    , bottom(0)
    , left(0)
  {
  
  }

  explicit Borders(int all)
    : top(all)
    , right(all)
    , bottom(all)
    , left(all)
  {
    
  }

  explicit Borders(int t, int r, int b, int l)
    : top(t)
    , right(r)
    , bottom(b)
    , left(l)
  {
    
  }

  bool set(int t, int l, int b, int r)
  {
    if (top != t || left != l || bottom != b || right != r)
    {
      top = t;
      left = l;
      bottom = b;
      right = r;

      return true;
    }

    return false;
  }

  bool set(const Borders &b) { return set(b.top, b.left, b.bottom, b.right); }

  int getVertical() const { return top + bottom; }

  int getHorizontal() const { return left + right; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Rect
{
  int x, y, width, height;

  Rect()
    : x(0)
    , y(0)
    , width(0)
    , height(0)
  {

  }

  Rect(int _x, int _y, int w, int h)
    : x(_x)
    , y(_y)
    , width(w)
    , height(h)
  {
  
  }

  Vec2 getPosition() const { return Vec2(x, y); }

  Vec2 getSize() const { return Vec2(width, height); }

  bool set(int _x, int _y, int w, int h)
  {
    if (x != _x || y != _y || width != w || height != h)
    {
      x = _x;
      y = _y;
      width = w;
      height = h;

      return true;
    }

    return false;
  }

  bool set(const Rect &r) { return set(r.x, r.y, r.width, r.height); }

  Rect &operator+=(const Borders &borders)
  {
    x += borders.left;
    y += borders.top;
    width -= borders.getHorizontal();
    height -= borders.getVertical();

    return *this;
  }

  Rect operator+(const Borders &borders) const
  {
    Rect result = *this;
    result += borders;
    return result;
  }

  Rect &operator*=(const Rect &rect)
  {
    int x1, y1, x2, y2;

    x1 = maximum(x, rect.x);
    y1 = maximum(y, rect.y);

    x2 = minimum(x + width, rect.x + rect.width);
    y2 = minimum(y + height, rect.y + rect.height);

    if (x1 > x2 || y1 > y2)
    {
      x = y = width = height = 0;
    }
    else
    {
      x = x1;
      y = y1;
      width = x2 - x1;
      height = y2 - y1;
    }

    return *this;
  }

  Rect operator*(const Rect &rect) const
  {
    Rect result = *this;
    result *= rect;
    return result;
  }

  bool operator==(const Rect &rect) const
  {
    return x == rect.x && y == rect.y && width == rect.width && height == rect.height;
  }

  bool operator!=(const Rect &rect) const
  {
    return x != rect.x || y != rect.y || width != rect.width || height != rect.height;
  }

  bool isPointInside(int px, int py) const
  {
    return !(px < x || py < y || px >= (x + width) || py >= (y + height));
  }

  void shrinkHorizontal(int amount)
  {
    x += amount;
    width -= amount;
  }

  void shrinkVertical(int amount)
  {
    y += amount;
    height -= amount;
  }

  bool ensureMinimumSize(int w, int h)
  {
    bool clamped = false;

    if (width < w)
    {
      width = w;
      clamped = true;
    }

    if (height < h)
    {
      height = h;
      clamped = true;
    }

    return clamped;
  }

  bool ensureMinimumSize(const Vec2 &size) { return ensureMinimumSize(size.x, size.y); }

  Vec2 getTopLeft() const { return Vec2(x, y); }

  Vec2 getBottomRight() const { return Vec2(x + width, y + height); }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Simple intrusive smart pointer
template <typename T>
class Ptr
{
  public:
    Ptr() : _ptr(nullptr) { }
    Ptr(T *ptr) : _ptr(ptr) { if (_ptr) _ptr->addRef(); }
    Ptr(const Ptr &rp) : _ptr(rp._ptr) { if (_ptr) _ptr->addRef(); }
    Ptr(Ptr &&rp) : _ptr(rp._ptr) { rp._ptr = nullptr; }
    ~Ptr() { if (_ptr) _ptr->release(); }

    Ptr &operator=(T *ptr) { assign(ptr); return *this; }
    Ptr &operator=(const Ptr &rp) { assign(rp._ptr); return *this; }
    Ptr &operator=(Ptr &&rp)
    {
      if (_ptr != rp._ptr) { if (_ptr) _ptr->release(); _ptr = rp._ptr; rp._ptr = nullptr; }
      return *this;
    }

    operator T *() const { return _ptr; }
    T &operator*() const { return *_ptr; }
    T *operator->() const { return _ptr; }
    T *get() const { return _ptr; }
    bool operator!() const { return _ptr == nullptr; }

    T *release()
    {
      T *ptr = _ptr;
      if (_ptr) { _ptr->releaseNoDelete(); _ptr = nullptr; }
      return ptr;
    }

  private:
    void assign(T *ptr)
    {
      if (_ptr == ptr) return;
      T* tmp_ptr = _ptr; _ptr = ptr;
      if (_ptr) _ptr->addRef();
      if (tmp_ptr) tmp_ptr->release();
    }

    template <class Other> friend class Ptr;
    T *_ptr;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Object
{
  template <typename T> friend class Ptr;

  protected:
    Object() { }
    virtual ~Object() { }

  private:
    void addRef() { ++_refCount; }
    void release() { if (!(--_refCount)) delete this; }
    void releaseNoDelete() { --_refCount; }

    int _refCount = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace nui
