/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if __WINRT__

/* Standard C++11 includes */
#include <functional>
#include <string>
#include <sstream>
using namespace std;


/* Windows includes */
#include "ppltasks.h"
using namespace concurrency;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Devices::Input;
using namespace Windows::Graphics::Display;
using namespace Windows::Foundation;
using namespace Windows::System;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;

#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
using namespace Windows::Phone::UI::Input;
#endif


/* SDL includes */
extern "C" {
#include "../../SDL_internal.h"
#include "SDL_assert.h"
#include "SDL_events.h"
#include "SDL_hints.h"
#include "SDL_log.h"
#include "SDL_main.h"
#include "SDL_stdinc.h"
#include "SDL_render.h"
#include "../../video/SDL_sysvideo.h"
//#include "../../SDL_hints_c.h"
#include "../../events/SDL_events_c.h"
#include "../../events/SDL_keyboard_c.h"
#include "../../events/SDL_mouse_c.h"
#include "../../events/SDL_windowevents_c.h"
#include "../../render/SDL_sysrender.h"
#include "../windows/SDL_windows.h"
}

#include "../../video/winrt/SDL_winrtevents_c.h"
#include "../../video/winrt/SDL_winrtvideo_cpp.h"
#include "SDL_winrtapp_common.h"
#include "SDL_winrtapp_direct3d.h"


// Compile-time debugging options:
// To enable, uncomment; to disable, comment them out.
//#define LOG_POINTER_EVENTS 1
//#define LOG_WINDOW_EVENTS 1
//#define LOG_ORIENTATION_EVENTS 1


// HACK, DLudwig: record a reference to the global, WinRT 'app'/view.
// SDL/WinRT will use this throughout its code.
//
// TODO, WinRT: consider replacing SDL_WinRTGlobalApp with something
// non-global, such as something created inside
// SDL_InitSubSystem(SDL_INIT_VIDEO), or something inside
// SDL_CreateWindow().
SDL_WinRTApp ^ SDL_WinRTGlobalApp = nullptr;

ref class SDLApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};

IFrameworkView^ SDLApplicationSource::CreateView()
{
    // TODO, WinRT: see if this function (CreateView) can ever get called
    // more than once.  For now, just prevent it from ever assigning
    // SDL_WinRTGlobalApp more than once.
    SDL_assert(!SDL_WinRTGlobalApp);
    SDL_WinRTApp ^ app = ref new SDL_WinRTApp();
    if (!SDL_WinRTGlobalApp)
    {
        SDL_WinRTGlobalApp = app;
    }
    return app;
}

int SDL_WinRTInitNonXAMLApp(int (*mainFunction)(int, char **))
{
    WINRT_SDLAppEntryPoint = mainFunction;
    auto direct3DApplicationSource = ref new SDLApplicationSource();
    CoreApplication::Run(direct3DApplicationSource);
    return 0;
}

static void WINRT_SetDisplayOrientationsPreference(void *userdata, const char *name, const char *oldValue, const char *newValue)
{
    SDL_assert(SDL_strcmp(name, SDL_HINT_ORIENTATIONS) == 0);

    // Start with no orientation flags, then add each in as they're parsed
    // from newValue.
    unsigned int orientationFlags = 0;
    if (newValue) {
        std::istringstream tokenizer(newValue);
        while (!tokenizer.eof()) {
            std::string orientationName;
            std::getline(tokenizer, orientationName, ' ');
            if (orientationName == "LandscapeLeft") {
                orientationFlags |= (unsigned int) DisplayOrientations::LandscapeFlipped;
            } else if (orientationName == "LandscapeRight") {
                orientationFlags |= (unsigned int) DisplayOrientations::Landscape;
            } else if (orientationName == "Portrait") {
                orientationFlags |= (unsigned int) DisplayOrientations::Portrait;
            } else if (orientationName == "PortraitUpsideDown") {
                orientationFlags |= (unsigned int) DisplayOrientations::PortraitFlipped;
            }
        }
    }

    // If no valid orientation flags were specified, use a reasonable set of defaults:
    if (!orientationFlags) {
        // TODO, WinRT: consider seeing if an app's default orientation flags can be found out via some API call(s).
        orientationFlags = (unsigned int) ( \
            DisplayOrientations::Landscape |
            DisplayOrientations::LandscapeFlipped |
            DisplayOrientations::Portrait |
            DisplayOrientations::PortraitFlipped);
    }

    // Set the orientation/rotation preferences.  Please note that this does
    // not constitute a 100%-certain lock of a given set of possible
    // orientations.  According to Microsoft's documentation on WinRT [1]
    // when a device is not capable of being rotated, Windows may ignore
    // the orientation preferences, and stick to what the device is capable of
    // displaying.
    //
    // [1] Documentation on the 'InitialRotationPreference' setting for a
    // Windows app's manifest file describes how some orientation/rotation
    // preferences may be ignored.  See
    // http://msdn.microsoft.com/en-us/library/windows/apps/hh700343.aspx
    // for details.  Microsoft's "Display orientation sample" also gives an
    // outline of how Windows treats device rotation
    // (http://code.msdn.microsoft.com/Display-Orientation-Sample-19a58e93).
#if NTDDI_VERSION > NTDDI_WIN8
    DisplayInformation::AutoRotationPreferences = (DisplayOrientations) orientationFlags;
#else
    DisplayProperties::AutoRotationPreferences = (DisplayOrientations) orientationFlags;
#endif
}

static void
WINRT_ProcessWindowSizeChange()
{
    // Make the new window size be the one true fullscreen mode.
    // This change was initially done, in part, to allow the Direct3D 11.1
    // renderer to receive window-resize events as a device rotates.
    // Before, rotating a device from landscape, to portrait, and then
    // back to landscape would cause the Direct3D 11.1 swap buffer to
    // not get resized appropriately.  SDL would, on the rotation from
    // landscape to portrait, re-resize the SDL window to it's initial
    // size (landscape).  On the subsequent rotation, SDL would drop the
    // window-resize event as it appeared the SDL window didn't change
    // size, and the Direct3D 11.1 renderer wouldn't resize its swap
    // chain.
    SDL_DisplayMode newDisplayMode;
    if (WINRT_CalcDisplayModeUsingNativeWindow(&newDisplayMode) != 0) {
        return;
    }

    // Make note of the old display mode, and it's old driverdata.
    SDL_DisplayMode oldDisplayMode;
    SDL_zero(oldDisplayMode);
    if (WINRT_GlobalSDLVideoDevice) {
        oldDisplayMode = WINRT_GlobalSDLVideoDevice->displays[0].desktop_mode;
    }

    // Setup the new display mode in the appropriate spots.
    if (WINRT_GlobalSDLVideoDevice) {
        // Make a full copy of the display mode for display_modes[0],
        // one with with a separately malloced 'driverdata' field.
        // SDL_VideoQuit(), if called, will attempt to free the driverdata
        // fields in 'desktop_mode' and each entry in the 'display_modes'
        // array.
        if (WINRT_GlobalSDLVideoDevice->displays[0].display_modes[0].driverdata) {
            // Free the previous mode's memory
            SDL_free(WINRT_GlobalSDLVideoDevice->displays[0].display_modes[0].driverdata);
            WINRT_GlobalSDLVideoDevice->displays[0].display_modes[0].driverdata = NULL;
        }
        if (WINRT_DuplicateDisplayMode(&(WINRT_GlobalSDLVideoDevice->displays[0].display_modes[0]), &newDisplayMode) != 0) {
            // Uh oh, something went wrong.  A malloc call probably failed.
            SDL_free(newDisplayMode.driverdata);
            return;
        }

        // Install 'newDisplayMode' into 'current_mode' and 'desktop_mode'.
        WINRT_GlobalSDLVideoDevice->displays[0].current_mode = newDisplayMode;
        WINRT_GlobalSDLVideoDevice->displays[0].desktop_mode = newDisplayMode;
    }

    if (WINRT_GlobalSDLWindow) {
        // Send a window-resize event to the rest of SDL, and to apps:
        SDL_SendWindowEvent(
            WINRT_GlobalSDLWindow,
            SDL_WINDOWEVENT_RESIZED,
            newDisplayMode.w,
            newDisplayMode.h);

#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
        // HACK: On Windows Phone, make sure that orientation changes from
        // Landscape to LandscapeFlipped, Portrait to PortraitFlipped,
        // or vice-versa on either of those two, lead to the Direct3D renderer
        // getting updated.
        const DisplayOrientations oldOrientation = ((SDL_DisplayModeData *)oldDisplayMode.driverdata)->currentOrientation;
        const DisplayOrientations newOrientation = ((SDL_DisplayModeData *)newDisplayMode.driverdata)->currentOrientation;

        if ((oldOrientation == DisplayOrientations::Landscape && newOrientation == DisplayOrientations::LandscapeFlipped) ||
            (oldOrientation == DisplayOrientations::LandscapeFlipped && newOrientation == DisplayOrientations::Landscape) ||
            (oldOrientation == DisplayOrientations::Portrait && newOrientation == DisplayOrientations::PortraitFlipped) ||
            (oldOrientation == DisplayOrientations::PortraitFlipped && newOrientation == DisplayOrientations::Portrait))
        {
            // One of the reasons this event is getting sent out is because SDL
            // will ignore requests to send out SDL_WINDOWEVENT_RESIZED events
            // if and when the event size doesn't change (and the Direct3D 11.1
            // renderer doesn't get the memo).
            //
            // Make sure that the display/window size really didn't change.  If
            // it did, then a SDL_WINDOWEVENT_SIZE_CHANGED event got sent, and
            // the Direct3D 11.1 renderer picked it up, presumably.
            if (oldDisplayMode.w == newDisplayMode.w &&
                oldDisplayMode.h == newDisplayMode.h)
            {
                SDL_SendWindowEvent(
                    WINRT_GlobalSDLWindow,
                    SDL_WINDOWEVENT_SIZE_CHANGED,
                    newDisplayMode.w,
                    newDisplayMode.h);
            }
        }
#endif
    }
    
    // Finally, free the 'driverdata' field of the old 'desktop_mode'.
    if (oldDisplayMode.driverdata) {
        SDL_free(oldDisplayMode.driverdata);
        oldDisplayMode.driverdata = NULL;
    }
}

SDL_WinRTApp::SDL_WinRTApp() :
    m_windowClosed(false),
    m_windowVisible(true)
{
}

void SDL_WinRTApp::Initialize(CoreApplicationView^ applicationView)
{
    applicationView->Activated +=
        ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &SDL_WinRTApp::OnActivated);

    CoreApplication::Suspending +=
        ref new EventHandler<SuspendingEventArgs^>(this, &SDL_WinRTApp::OnSuspending);

    CoreApplication::Resuming +=
        ref new EventHandler<Platform::Object^>(this, &SDL_WinRTApp::OnResuming);

    CoreApplication::Exiting +=
        ref new EventHandler<Platform::Object^>(this, &SDL_WinRTApp::OnExiting);
}

#if NTDDI_VERSION > NTDDI_WIN8
void SDL_WinRTApp::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
#else
void SDL_WinRTApp::OnOrientationChanged(Object^ sender)
#endif
{
#if LOG_ORIENTATION_EVENTS==1
    CoreWindow^ window = CoreWindow::GetForCurrentThread();
    if (window) {
        SDL_Log("%s, current orientation=%d, native orientation=%d, auto rot. pref=%d, CoreWindow Size={%f,%f}\n",
            __FUNCTION__,
            (int)DisplayProperties::CurrentOrientation,
            (int)DisplayProperties::NativeOrientation,
            (int)DisplayProperties::AutoRotationPreferences,
            window->Bounds.Width,
            window->Bounds.Height);
    } else {
        SDL_Log("%s, current orientation=%d, native orientation=%d, auto rot. pref=%d\n",
            __FUNCTION__,
            (int)DisplayProperties::CurrentOrientation,
            (int)DisplayProperties::NativeOrientation,
            (int)DisplayProperties::AutoRotationPreferences);
    }
#endif

#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
    // On Windows Phone, treat an orientation change as a change in window size.
    // The native window's size doesn't seem to change, however SDL will simulate
    // a window size change.
    WINRT_ProcessWindowSizeChange();
#endif
}

void SDL_WinRTApp::SetWindow(CoreWindow^ window)
{
#if LOG_WINDOW_EVENTS==1
    SDL_Log("%s, current orientation=%d, native orientation=%d, auto rot. pref=%d, window Size={%f,%f}\n",
        __FUNCTION__,
        (int)DisplayProperties::CurrentOrientation,
        (int)DisplayProperties::NativeOrientation,
        (int)DisplayProperties::AutoRotationPreferences,
        window->Bounds.Width,
        window->Bounds.Height);
#endif

    window->SizeChanged += 
        ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &SDL_WinRTApp::OnWindowSizeChanged);

    window->VisibilityChanged +=
        ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &SDL_WinRTApp::OnVisibilityChanged);

    window->Closed += 
        ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &SDL_WinRTApp::OnWindowClosed);

#if WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP
    window->PointerCursor = ref new CoreCursor(CoreCursorType::Arrow, 0);
#endif

    window->PointerPressed +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &SDL_WinRTApp::OnPointerPressed);

    window->PointerMoved +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &SDL_WinRTApp::OnPointerMoved);

    window->PointerReleased +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &SDL_WinRTApp::OnPointerReleased);

    window->PointerWheelChanged +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &SDL_WinRTApp::OnPointerWheelChanged);

#if WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP
    // Retrieves relative-only mouse movements:
    Windows::Devices::Input::MouseDevice::GetForCurrentView()->MouseMoved +=
        ref new TypedEventHandler<MouseDevice^, MouseEventArgs^>(this, &SDL_WinRTApp::OnMouseMoved);
#endif

    window->KeyDown +=
        ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &SDL_WinRTApp::OnKeyDown);

    window->KeyUp +=
        ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &SDL_WinRTApp::OnKeyUp);

#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
    HardwareButtons::BackPressed +=
        ref new EventHandler<BackPressedEventArgs^>(this, &SDL_WinRTApp::OnBackButtonPressed);
#endif

#if NTDDI_VERSION > NTDDI_WIN8
    DisplayInformation::GetForCurrentView()->OrientationChanged +=
        ref new TypedEventHandler<Windows::Graphics::Display::DisplayInformation^, Object^>(this, &SDL_WinRTApp::OnOrientationChanged);
#else
    DisplayProperties::OrientationChanged +=
        ref new DisplayPropertiesEventHandler(this, &SDL_WinRTApp::OnOrientationChanged);
#endif

    // Register the hint, SDL_HINT_ORIENTATIONS, with SDL.
    // TODO, WinRT: see if an app's default orientation can be found out via WinRT API(s), then set the initial value of SDL_HINT_ORIENTATIONS accordingly.
    SDL_AddHintCallback(SDL_HINT_ORIENTATIONS, WINRT_SetDisplayOrientationsPreference, NULL);

#if WINAPI_FAMILY == WINAPI_FAMILY_APP  // for Windows 8/8.1/RT apps... (and not Phone apps)
    // Make sure we know when a user has opened the app's settings pane.
    // This is needed in order to display a privacy policy, which needs
    // to be done for network-enabled apps, as per Windows Store requirements.
    using namespace Windows::UI::ApplicationSettings;
    SettingsPane::GetForCurrentView()->CommandsRequested +=
        ref new TypedEventHandler<SettingsPane^, SettingsPaneCommandsRequestedEventArgs^>
            (this, &SDL_WinRTApp::OnSettingsPaneCommandsRequested);
#endif
}

void SDL_WinRTApp::Load(Platform::String^ entryPoint)
{
}

void SDL_WinRTApp::Run()
{
    SDL_SetMainReady();
    if (WINRT_SDLAppEntryPoint)
    {
        // TODO, WinRT: pass the C-style main() a reasonably realistic
        // representation of command line arguments.
        int argc = 0;
        char **argv = NULL;
        WINRT_SDLAppEntryPoint(argc, argv);
    }
}

void SDL_WinRTApp::PumpEvents()
{
    if (!m_windowClosed)
    {
        if (m_windowVisible)
        {
            CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
        }
        else
        {
            CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
        }
    }
}

void SDL_WinRTApp::Uninitialize()
{
}

#if WINAPI_FAMILY == WINAPI_FAMILY_APP
void SDL_WinRTApp::OnSettingsPaneCommandsRequested(
    Windows::UI::ApplicationSettings::SettingsPane ^p,
    Windows::UI::ApplicationSettings::SettingsPaneCommandsRequestedEventArgs ^args)
{
    using namespace Platform;
    using namespace Windows::UI::ApplicationSettings;
    using namespace Windows::UI::Popups;

    String ^privacyPolicyURL = nullptr;     // a URL to an app's Privacy Policy
    String ^privacyPolicyLabel = nullptr;   // label/link text
    const char *tmpHintValue = NULL;        // SDL_GetHint-retrieved value, used immediately
    wchar_t *tmpStr = NULL;                 // used for UTF8 to UCS2 conversion

    // Setup a 'Privacy Policy' link, if one is available (via SDL_GetHint):
    tmpHintValue = SDL_GetHint(SDL_HINT_WINRT_PRIVACY_POLICY_URL);
    if (tmpHintValue && tmpHintValue[0] != '\0') {
        // Convert the privacy policy's URL to UCS2:
        tmpStr = WIN_UTF8ToString(tmpHintValue);
        privacyPolicyURL = ref new String(tmpStr);
        SDL_free(tmpStr);

        // Optionally retrieve custom label-text for the link.  If this isn't
        // available, a default value will be used instead.
        tmpHintValue = SDL_GetHint(SDL_HINT_WINRT_PRIVACY_POLICY_LABEL);
        if (tmpHintValue && tmpHintValue[0] != '\0') {
            tmpStr = WIN_UTF8ToString(tmpHintValue);
            privacyPolicyLabel = ref new String(tmpStr);
            SDL_free(tmpStr);
        } else {
            privacyPolicyLabel = ref new String(L"Privacy Policy");
        }

        // Register the link, along with a handler to be called if and when it is
        // clicked:
        auto cmd = ref new SettingsCommand(L"privacyPolicy", privacyPolicyLabel,
            ref new UICommandInvokedHandler([=](IUICommand ^) {
                Windows::System::Launcher::LaunchUriAsync(ref new Uri(privacyPolicyURL));
        }));
        args->Request->ApplicationCommands->Append(cmd);
    }
}
#endif // if WINAPI_FAMILY == WINAPI_FAMILY_APP

void SDL_WinRTApp::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
#if LOG_WINDOW_EVENTS==1
    SDL_Log("%s, size={%f,%f}, current orientation=%d, native orientation=%d, auto rot. pref=%d, WINRT_GlobalSDLWindow?=%s\n",
        __FUNCTION__,
        args->Size.Width, args->Size.Height,
        (int)DisplayProperties::CurrentOrientation,
        (int)DisplayProperties::NativeOrientation,
        (int)DisplayProperties::AutoRotationPreferences,
        (WINRT_GlobalSDLWindow ? "yes" : "no"));
#endif

    WINRT_ProcessWindowSizeChange();
}

void SDL_WinRTApp::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
#if LOG_WINDOW_EVENTS==1
    SDL_Log("%s, visible?=%s, WINRT_GlobalSDLWindow?=%s\n",
        __FUNCTION__,
        (args->Visible ? "yes" : "no"),
        (WINRT_GlobalSDLWindow ? "yes" : "no"));
#endif

    m_windowVisible = args->Visible;
    if (WINRT_GlobalSDLWindow) {
        SDL_bool wasSDLWindowSurfaceValid = WINRT_GlobalSDLWindow->surface_valid;

        if (args->Visible) {
            SDL_SendWindowEvent(WINRT_GlobalSDLWindow, SDL_WINDOWEVENT_SHOWN, 0, 0);
        } else {
            SDL_SendWindowEvent(WINRT_GlobalSDLWindow, SDL_WINDOWEVENT_HIDDEN, 0, 0);
        }

        // HACK: Prevent SDL's window-hide handling code, which currently
        // triggers a fake window resize (possibly erronously), from
        // marking the SDL window's surface as invalid.
        //
        // A better solution to this probably involves figuring out if the
        // fake window resize can be prevented.
        WINRT_GlobalSDLWindow->surface_valid = wasSDLWindowSurfaceValid;
    }
}

void SDL_WinRTApp::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
#if LOG_WINDOW_EVENTS==1
    SDL_Log("%s\n", __FUNCTION__);
#endif
    m_windowClosed = true;
}

void SDL_WinRTApp::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
    CoreWindow::GetForCurrentThread()->Activate();
}

static int SDLCALL RemoveAppSuspendAndResumeEvents(void * userdata, SDL_Event * event)
{
    if (event->type == SDL_WINDOWEVENT)
    {
        switch (event->window.event)
        {
            case SDL_WINDOWEVENT_MINIMIZED:
            case SDL_WINDOWEVENT_RESTORED:
                // Return 0 to indicate that the event should be removed from the
                // event queue:
                return 0;
            default:
                break;
        }
    }

    // Return 1 to indicate that the event should stay in the event queue:
    return 1;
}

void SDL_WinRTApp::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
    // Save app state asynchronously after requesting a deferral. Holding a deferral
    // indicates that the application is busy performing suspending operations. Be
    // aware that a deferral may not be held indefinitely. After about five seconds,
    // the app will be forced to exit.
    SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();
    create_task([this, deferral]()
    {
        // Send a window-minimized event immediately to observers.
        // CoreDispatcher::ProcessEvents, which is the backbone on which
        // SDL_WinRTApp::PumpEvents is built, will not return to its caller
        // once it sends out a suspend event.  Any events posted to SDL's
        // event queue won't get received until the WinRT app is resumed.
        // SDL_AddEventWatch() may be used to receive app-suspend events on
        // WinRT.
        //
        // In order to prevent app-suspend events from being received twice:
        // first via a callback passed to SDL_AddEventWatch, and second via
        // SDL's event queue, the event will be sent to SDL, then immediately
        // removed from the queue.
        if (WINRT_GlobalSDLWindow)
        {
            SDL_SendWindowEvent(WINRT_GlobalSDLWindow, SDL_WINDOWEVENT_MINIMIZED, 0, 0);   // TODO: see if SDL_WINDOWEVENT_SIZE_CHANGED should be getting triggered here (it is, currently)
            SDL_FilterEvents(RemoveAppSuspendAndResumeEvents, 0);
        }

        SDL_SendAppEvent(SDL_APP_WILLENTERBACKGROUND);
        SDL_SendAppEvent(SDL_APP_DIDENTERBACKGROUND);

        deferral->Complete();
    });
}

void SDL_WinRTApp::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
    SDL_SendAppEvent(SDL_APP_WILLENTERFOREGROUND);
    SDL_SendAppEvent(SDL_APP_DIDENTERFOREGROUND);

    // Restore any data or state that was unloaded on suspend. By default, data
    // and state are persisted when resuming from suspend. Note that this event
    // does not occur if the app was previously terminated.
    if (WINRT_GlobalSDLWindow)
    {
        SDL_SendWindowEvent(WINRT_GlobalSDLWindow, SDL_WINDOWEVENT_RESTORED, 0, 0);    // TODO: see if SDL_WINDOWEVENT_SIZE_CHANGED should be getting triggered here (it is, currently)

        // Remove the app-resume event from the queue, as is done with the
        // app-suspend event.
        //
        // TODO, WinRT: consider posting this event to the queue even though
        // its counterpart, the app-suspend event, effectively has to be
        // processed immediately.
        SDL_FilterEvents(RemoveAppSuspendAndResumeEvents, 0);
    }
}

void SDL_WinRTApp::OnExiting(Platform::Object^ sender, Platform::Object^ args)
{
    SDL_SendAppEvent(SDL_APP_TERMINATING);
}

static void
WINRT_LogPointerEvent(const char * header, Windows::UI::Core::PointerEventArgs ^ args, Windows::Foundation::Point transformedPoint)
{
    Windows::UI::Input::PointerPoint ^ pt = args->CurrentPoint;
    SDL_Log("%s: Position={%f,%f}, Transformed Pos={%f, %f}, MouseWheelDelta=%d, FrameId=%d, PointerId=%d, SDL button=%d\n",
        header,
        pt->Position.X, pt->Position.Y,
        transformedPoint.X, transformedPoint.Y,
        pt->Properties->MouseWheelDelta,
        pt->FrameId,
        pt->PointerId,
        WINRT_GetSDLButtonForPointerPoint(pt));
}

void SDL_WinRTApp::OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args)
{
#if LOG_POINTER_EVENTS
    WINRT_LogPointerEvent("pointer pressed", args, WINRT_TransformCursorPosition(WINRT_GlobalSDLWindow, args->CurrentPoint->Position, TransformToSDLWindowSize));
#endif

    WINRT_ProcessPointerPressedEvent(WINRT_GlobalSDLWindow, args->CurrentPoint);
}

void SDL_WinRTApp::OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args)
{
#if LOG_POINTER_EVENTS
    WINRT_LogPointerEvent("pointer moved", args, WINRT_TransformCursorPosition(WINRT_GlobalSDLWindow, args->CurrentPoint->Position, TransformToSDLWindowSize));
#endif

    WINRT_ProcessPointerMovedEvent(WINRT_GlobalSDLWindow, args->CurrentPoint);
}

void SDL_WinRTApp::OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
{
#if LOG_POINTER_EVENTS
    WINRT_LogPointerEvent("pointer released", args, WINRT_TransformCursorPosition(WINRT_GlobalSDLWindow, args->CurrentPoint->Position, TransformToSDLWindowSize));
#endif

    WINRT_ProcessPointerReleasedEvent(WINRT_GlobalSDLWindow, args->CurrentPoint);
}

void SDL_WinRTApp::OnPointerWheelChanged(CoreWindow^ sender, PointerEventArgs^ args)
{
#if LOG_POINTER_EVENTS
    WINRT_LogPointerEvent("pointer wheel changed", args, WINRT_TransformCursorPosition(WINRT_GlobalSDLWindow, args->CurrentPoint->Position, TransformToSDLWindowSize));
#endif

    WINRT_ProcessPointerWheelChangedEvent(WINRT_GlobalSDLWindow, args->CurrentPoint);
}

void SDL_WinRTApp::OnMouseMoved(MouseDevice^ mouseDevice, MouseEventArgs^ args)
{
    WINRT_ProcessMouseMovedEvent(WINRT_GlobalSDLWindow, args);
}

void SDL_WinRTApp::OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
    WINRT_ProcessKeyDownEvent(args);
}

void SDL_WinRTApp::OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
    WINRT_ProcessKeyUpEvent(args);
}

#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
void SDL_WinRTApp::OnBackButtonPressed(Platform::Object^ sender, Windows::Phone::UI::Input::BackPressedEventArgs^ args)
{
    SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_AC_BACK);
    SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_AC_BACK);

    const char *hint = SDL_GetHint(SDL_HINT_WINRT_HANDLE_BACK_BUTTON);
    if (hint) {
        if (*hint == '1') {
            args->Handled = true;
        }
    }
}
#endif

#endif
