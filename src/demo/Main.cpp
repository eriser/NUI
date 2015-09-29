#include <iostream>
#include <atomic>
#include <map>

#include <SDL.h>
#include <glew/glew.h>

#include "gl2d.h"

#include "Cursors.h"

#include "NUI/NUI.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg/nanovg.h>
#include <nanovg/nanovg_gl.h>

std::atomic_bool g_ShouldQuit;

SDL_Window *g_MainWindow = nullptr;
SDL_GLContext g_GLContext = nullptr;

SDL_Cursor *g_CursorArrow = nullptr;
SDL_Cursor *g_CursorMove = nullptr;
SDL_Cursor *g_CursorResizeV = nullptr;
SDL_Cursor *g_CursorResizeH = nullptr;
SDL_Cursor *g_CursorResizeLR = nullptr;
SDL_Cursor *g_CursorResizeRL = nullptr;

NVGcontext *g_NVGcontext = nullptr;

gl2d::context *g_Context2D = nullptr;

uint64_t g_TickOffset = 0;
uint64_t g_LastTickCount = 0;
uint64_t g_TickFrequency = 0;

nui::Root::Ptr g_Root = nullptr;
bool g_TextInputActive = false;

//---------------------------------------------------------------------------------------------------------------------
NVGcontext *getNVGcontext()
{
  return g_NVGcontext;
}

//---------------------------------------------------------------------------------------------------------------------
void updateMouseCursor()
{
  switch (g_Root->getMouseState().cursor)
  {
    default:
    case nui::MouseCursor::Default:
      SDL_SetCursor(g_CursorArrow);
      break;

    case nui::MouseCursor::Move:
      SDL_SetCursor(g_CursorMove);
      break;

    case nui::MouseCursor::ResizeV:
      SDL_SetCursor(g_CursorResizeV);
      break;

    case nui::MouseCursor::ResizeH:
      SDL_SetCursor(g_CursorResizeH);
      break;

    case nui::MouseCursor::ResizeLR:
      SDL_SetCursor(g_CursorResizeLR);
      break;

    case nui::MouseCursor::ResizeRL:
      SDL_SetCursor(g_CursorResizeRL);
      break;
  }
}

//---------------------------------------------------------------------------------------------------------------------
void tick()
{
  uint64_t tickCount = SDL_GetPerformanceCounter() - g_TickOffset;
  uint64_t deltaTicks = tickCount - g_LastTickCount;
  g_LastTickCount = tickCount;

  double time = static_cast<double>(tickCount) / static_cast<double>(g_TickFrequency);
  double deltaTime = static_cast<double>(deltaTicks) / static_cast<double>(g_TickFrequency);

  nui::Vec2 windowSize;
  SDL_GetWindowSize(g_MainWindow, &windowSize.x, &windowSize.y);

  // Clear everything
  glViewport(0, 0, windowSize.x, windowSize.y);
  glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
  glClearDepth(1.0f);
  glClearStencil(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  nvgBeginFrame(g_NVGcontext, windowSize.x, windowSize.y, 1);

  g_Root->setSize(windowSize.x, windowSize.y);
  g_Root->tick(time, deltaTime);
  g_Root->draw();

  nvgEndFrame(g_NVGcontext);

  g_Context2D->frame_begin();
  g_Context2D->move_to({0, 0});
  g_Context2D->line_to({400, 300});
  g_Context2D->frame_end();

  SDL_GL_SwapWindow(g_MainWindow);

  updateMouseCursor();

  bool textInputRequired = g_Root->isTextInputRequired();

  if (textInputRequired != g_TextInputActive)
  {
    g_TextInputActive = textInputRequired;

    if (g_TextInputActive)
      SDL_StartTextInput();
    else
      SDL_StopTextInput();
  }
}

//---------------------------------------------------------------------------------------------------------------------
int eventFilter(void *userData, SDL_Event *e)
{
  if (e->type == SDL_WINDOWEVENT && e->window.event == SDL_WINDOWEVENT_RESIZED)
    tick();

  return 1;
}

//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  // Initialize SDL
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    return -1;

  SDL_SetEventFilter(&eventFilter, nullptr);

  // Initialize globals
  g_ShouldQuit = false;
  g_CursorArrow = createCursorFromImage(g_CursorArrowImage);
  g_CursorMove = createCursorFromImage(g_CursorMoveImage);
  g_CursorResizeV = createCursorFromImage(g_CursorResizeVImage);
  g_CursorResizeH = createCursorFromImage(g_CursorResizeHImage);
  g_CursorResizeLR = createCursorFromImage(g_CursorResizeLRImage);
  g_CursorResizeRL = createCursorFromImage(g_CursorResizeRLImage);

  // Setup OpenGL attributes
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

  // Create main window
  unsigned windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

  int width = 800;
  int height = 480;

  g_MainWindow = SDL_CreateWindow("NUI Demo", SDL_WINDOWPOS_CENTERED_DISPLAY(2), SDL_WINDOWPOS_CENTERED_DISPLAY(2), width, height, windowFlags);
  if (!g_MainWindow)
    return -1;

  // Create OpenGL context
  g_GLContext = SDL_GL_CreateContext(g_MainWindow);
  if (!g_GLContext)
    return -1;

  // Set new OpenGL context as current (just to be safe)
  SDL_GL_MakeCurrent(g_MainWindow, g_GLContext);

  // Initialize GLEW
  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK)
    return -1;

  // Initialize NanoVG
  g_NVGcontext = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);

  g_Context2D = new gl2d::context();

  // Initialize UI
  g_Root = new nui::Root(g_NVGcontext);
  g_Root->setSize(width, height);

  // Set new cursor
  SDL_SetCursor(g_CursorArrow);

  // Setup timer variables
  g_TickOffset = SDL_GetPerformanceCounter();
  g_TickFrequency = SDL_GetPerformanceFrequency();
  g_LastTickCount = 0;

  std::map<int, nui::MouseButton> SDL2UIButton
  {
    { SDL_BUTTON_LEFT, nui::MouseButton::Left },
    { SDL_BUTTON_MIDDLE, nui::MouseButton::Middle },
    { SDL_BUTTON_RIGHT, nui::MouseButton::Right }
  };

  std::map<int, nui::Key> SDL2UIKey
  {
    { SDLK_ESCAPE, nui::Key::Escape },
    { SDLK_F1, nui::Key::F1 },
    { SDLK_F2, nui::Key::F2 },
    { SDLK_F3, nui::Key::F3 },
    { SDLK_F4, nui::Key::F4 },
    { SDLK_F5, nui::Key::F5 },
    { SDLK_F6, nui::Key::F6 },
    { SDLK_F7, nui::Key::F7 },
    { SDLK_F8, nui::Key::F8 },
    { SDLK_F9, nui::Key::F9 },
    { SDLK_F10, nui::Key::F10 },
    { SDLK_F11, nui::Key::F11 },
    { SDLK_F12, nui::Key::F12 },
    { SDLK_UP, nui::Key::Up },
    { SDLK_DOWN, nui::Key::Down },
    { SDLK_LEFT, nui::Key::Left },
    { SDLK_RIGHT, nui::Key::Right },
    { SDLK_TAB, nui::Key::Tab },
    { SDLK_BACKSPACE, nui::Key::Backspace },
    { SDLK_RETURN, nui::Key::Enter },
    { SDLK_INSERT, nui::Key::Insert },
    { SDLK_DELETE, nui::Key::Delete },
    { SDLK_HOME, nui::Key::Home },
    { SDLK_END, nui::Key::End },
    { SDLK_PAGEUP, nui::Key::PageUp },
    { SDLK_PAGEDOWN, nui::Key::PageDown },
    { SDLK_LSHIFT, nui::Key::ShiftLeft },
    { SDLK_RSHIFT, nui::Key::ShiftRight },
    { SDLK_LCTRL, nui::Key::ControlLeft },
    { SDLK_RCTRL, nui::Key::ControlRight },
    { SDLK_LALT, nui::Key::AltLeft },
    { SDLK_RALT, nui::Key::AltRight }
  };

  // Docking test
  {
    nui::Window::Ptr window = new nui::Window(g_Root, "Docking test");
    window->setStyle(new nui::Graphics::Style(0x404040, 0x309040));

    nui::Button::Ptr b;

    b = new nui::Button(window, "Button", nui::Docking::Top);
    b = new nui::Button(window, "Button", nui::Docking::Left);
    b = new nui::Button(window, "Button", nui::Docking::Right);
    b = new nui::Button(window, "Button", nui::Docking::Bottom);
    b = new nui::Button(window, "Button", nui::Docking::Left);
    b = new nui::Button(window, "Button", nui::Docking::Bottom);

    for (int i = 0; i < 1; ++i)
    {
      b = new nui::Button(window, "Move me!");
      b->addFlags(nui::Control::CanMove);
    }
  }

  // Text editor
  {
    nui::Window::Ptr window = new nui::Window(g_Root, "Text editor");

    nui::TextBox::Ptr textBox = new nui::TextBox(window, "", nui::Docking::Client);
    textBox->setMonospace();
    textBox->setMultiline();

    textBox->loadTextFromFile("C:\\Temp\\Forward.cpp");
  }

  for (size_t i = 0; i < 0; ++i)
  {
    // Setup example GUI
    nui::Window::Ptr mainWindow = new nui::Window(g_Root, "My window");
    mainWindow->setPosition(40 + i * 5, 40 + i * 5);
    mainWindow->setIcon(NUI_MAKE_ICON(0, 2));
    mainWindow->setStyle(new nui::Graphics::Style(0x404040, 0xB04020));

    {
      nui::Box::Ptr box = new nui::Box(mainWindow, nui::Docking::Bottom);

      {
        nui::Button::Ptr button = new nui::Button(box, "Cancel", nui::Docking::Right, NUI_ICON_CANCEL);
      }

      {
        nui::Button::Ptr button = new nui::Button(box, "OK", nui::Docking::Right, NUI_ICON_OK);
      }

      box->autoSize();
    }

    {
      nui::ScrollBar::Ptr scrollBar = new nui::ScrollBar(mainWindow, nui::Docking::Right);
      scrollBar->setLimits(0, 10);
    }

    {
      nui::MenuBar::Ptr menuBar = new nui::MenuBar(mainWindow);

      nui::Menu::Ptr menu;
      
      menu = menuBar->addMenu("File");
      menu->addItem("New...", NUI_ICON_FILE_NEW);
      menu->addItem("Open...", NUI_ICON_FILE_OPEN);
      menu->addItem("-");
      menu->addItem("Save", NUI_ICON_FILE_SAVE);
      menu->addItem("Save as...");
      menu->addItem("-");
      menu->addItem("Exit", NUI_ICON_QUIT);

      menu = menuBar->addMenu("Edit");
      menu->addItem("Undo");
      menu->addItem("Redo");
      menu->addItem("-");
      menu->addItem("Preferences");

      menu = menuBar->addMenu("Help");
      menu->addItem("Check for updates");
      menu->addItem("-");
      menu->addItem("About...", NUI_ICON_HELP);
    }

    {
      nui::CheckBox::Ptr checkBox;
      checkBox = new nui::CheckBox(mainWindow, "Some option to be checked 1", nui::Docking::Top);
      checkBox = new nui::CheckBox(mainWindow, "Some option to be checked 2", nui::Docking::Top);
      checkBox = new nui::CheckBox(mainWindow, "Some option to be checked 3", nui::Docking::Top);
      checkBox = new nui::CheckBox(mainWindow, "Some option to be checked 4", nui::Docking::Top);
    }

    {
      nui::TextBox::Ptr textBox;
      textBox = new nui::TextBox(mainWindow, "Hello, world!", nui::Docking::Top);
    }

    {
      nui::TextBox::Ptr textBox;
      textBox = new nui::TextBox(mainWindow, "This text\nshould be\non multiple\nlines!\n\nAyy?!", nui::Docking::Top);
      textBox->setSize(textBox->getWidth(), 100);
      textBox->setMultiline();
      textBox->setMonospace();
    }
  }

  // Main loop
  while (!g_ShouldQuit)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_QUIT:
          g_ShouldQuit = true;
          break;

        case SDL_MOUSEMOTION:
        {
          g_Root->eventMouseMotion(event.motion.x, event.motion.y);
        }
        break;

        case SDL_MOUSEBUTTONDOWN:
        {
          g_Root->eventMouseButtonDown(SDL2UIButton[event.button.button]);
        }
        break;

        case SDL_MOUSEBUTTONUP:
        {
          g_Root->eventMouseButtonUp(SDL2UIButton[event.button.button]);
        }
        break;

        case SDL_KEYDOWN:
        {
          auto iter = SDL2UIKey.find(event.key.keysym.sym);

          if (iter != SDL2UIKey.end())
            g_Root->eventKeyDown(iter->second);
        }
        break;

        case SDL_KEYUP:
        {
          auto iter = SDL2UIKey.find(event.key.keysym.sym);

          if (iter != SDL2UIKey.end())
            g_Root->eventKeyUp(iter->second);
        }
        break;

        case SDL_TEXTINPUT:
        {
          g_Root->eventKeyDown(nui::Key::Character, event.text.text[0]);
          g_Root->eventKeyUp(nui::Key::Character, event.text.text[0]);
        }
        break;
      }
    }

    tick();
  }

  // Deinitialize UI
  g_Root = nullptr;

  // Destroy NanoVG context
  nvgDeleteGL3(g_NVGcontext);

  // Destroy OpenGL context
  SDL_GL_DeleteContext(g_GLContext);

  // Destroy main window
  SDL_DestroyWindow(g_MainWindow);

  // Deinitialize SDL and quit
  SDL_Quit();

  gl2d::context ctx;
  ctx.color({ 1, 1, 1 });
  ctx.move_to({ 0, 0 });
  ctx.line_to({ 100, 100 });

  return 0;
}
