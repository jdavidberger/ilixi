/*
 Copyright 2010-2015 Tarik Sekmen.

 All Rights Reserved.

 Written by Tarik Sekmen <tarik@ilixi.org>.

 This file is part of ilixi.

 ilixi is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 ilixi is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with ilixi.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <core/Application.h>

#include <core/Engine.h>
#include <core/Logger.h>
#include <core/PlatformManager.h>

#include <graphics/Stylist.h>

#include <directfb_util.h>
#include <algorithm>
#include <string.h>

namespace ilixi
{

D_DEBUG_DOMAIN( ILX_APPLICATION, "ilixi/core/Application", "Application");
D_DEBUG_DOMAIN( ILX_APPLICATION_TIMER, "ilixi/core/Application/Timers", "Application Timers");
D_DEBUG_DOMAIN( ILX_APPLICATION_UPDATES, "ilixi/core/Application/Updates", "Application Updates");
D_DEBUG_DOMAIN( ILX_APPLICATION_EVENTS, "ilixi/core/Application/Events", "Application Events");

Application* Application::__instance = NULL;

Application::Application(int* argc, char*** argv, AppOptions opts)
        : _dragging(false),
          _appWindow(NULL),
          __flags(APS_HIDDEN),
          __activeWindow(NULL),
          _frameTime(0)
{
    ILOG_TRACE_F(ILX_APPLICATION);

    if (__instance)
        ILOG_THROW(ILX_APPLICATION, "Cannot allow more than one instance!\n");

    __instance = this;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&__windowMutex, &attr);
    pthread_mutexattr_destroy(&attr);

    setenv("XML_CATALOG_FILES", ILIXI_DATADIR"ilixi_catalog.xml", 0);

    __cursorNew.x = 0;
    __cursorNew.y = 0;
    __cursorOld.x = 0;
    __cursorOld.y = 0;

    PlatformManager::instance().initialize(argc, argv, opts);
    Engine::instance().initialise();

    Size s = PlatformManager::instance().getScreenSize();
    __screenSize.w = s.width() - 1;
    __screenSize.h = s.height() - 1;

    setStylist(new Stylist());

    _appWindow = new AppWindow(this);
    _appWindow->sigAbort.connect(sigc::ptr_fun(&Application::quit));
}

Application::~Application()
{
    ILOG_TRACE_F(ILX_APPLICATION);

    delete _appWindow;
    delete Widget::_stylist;

    Engine::instance().release();
    PlatformManager::instance().release();

    pthread_mutex_destroy(&__windowMutex);

    __activeWindow = NULL;
    __instance = NULL;
}

Application*
Application::instance()
{
    return __instance;
}

int
Application::width() const
{
    if (_appWindow)
        return _appWindow->width();
    return 0;
}

int
Application::height() const
{
    if (_appWindow)
        return _appWindow->height();
    return 0;
}

bool
Application::addWidget(Widget* widget)
{
    if (_appWindow)
        return _appWindow->addWidget(widget);
    return false;
}

bool
Application::removeWidget(Widget* widget)
{
    if (_appWindow)
        return _appWindow->removeWidget(widget);
    return false;
}

void
Application::update()
{
    if (_appWindow)
        _appWindow->update();
}

void
Application::quit()
{
    if (__instance)
    {
        __instance->_appWindow->setVisible(false);
        Engine::instance().stop();
    }
}

void
Application::exec()
{
    ILOG_INFO(ILX_APPLICATION, "Starting...\n");

    show();

    while (true)
    {
        if (Engine::instance().stopped())
            break;
        else
        {
            __instance->handleEvents(Engine::instance().cycle());
            updateWindows();
        }
    }

    hide();

    ILOG_INFO(ILX_APPLICATION, "Stopping...\n");

    sigQuit();
}

void
Application::setBackgroundImage(const std::string& imagePath, bool tile)
{
    if (_appWindow)
        _appWindow->setBackgroundImage(imagePath, tile);
}

void
Application::setLayout(LayoutBase* layout)
{
    if (_appWindow)
        _appWindow->setLayout(layout);
}

void
Application::setMargins(int top, int bottom, int left, int right)
{
    if (_appWindow)
        _appWindow->setMargins(top, bottom, left, right);
}

void
Application::setMargin(const Margin& margin)
{
    if (_appWindow)
        _appWindow->setMargin(margin);
}

bool
Application::setToolbar(ToolBar* toolbar, bool positionNorth)
{
    if (_appWindow)
        return _appWindow->setToolbar(toolbar, positionNorth);
    return false;
}

void
Application::postKeyEvent(DFBInputDeviceKeySymbol symbol, DFBInputDeviceModifierMask modifierMask, DFBInputDeviceLockState lockState, bool down)
{
    if (PlatformManager::instance().appOptions() & OptExclusive)
    {
        DFBInputEvent event;
        event.clazz = DFEC_INPUT;
        event.type = down ? DIET_KEYPRESS : DIET_KEYRELEASE;
        event.flags = (DFBInputEventFlags) (DIEF_KEYSYMBOL | DIEF_MODIFIERS | DIEF_LOCKS);
        event.key_symbol = symbol;
        event.modifiers = modifierMask;
        event.locks = lockState;
        Engine::instance().postEvent(DFB_EVENT(&event) );
    } else
    {
        DFBWindowEvent event;
        event.clazz = DFEC_WINDOW;
        event.window_id = activeWindow()->windowID();
        event.type = down ? DWET_KEYDOWN : DWET_KEYUP;
        event.flags = DWEF_NONE;
        event.key_symbol = symbol;
        event.modifiers = modifierMask;
        event.locks = lockState;
        Engine::instance().postEvent(DFB_EVENT(&event) );
    }
}

void
Application::postPointerEvent(PointerEventType type, PointerButton button, PointerButtonMask buttonMask, int x, int y, int cx, int cy, int step)
{
    if (PlatformManager::instance().appOptions() & OptExclusive)
    {
        DFBInputEvent event_1;
        event_1.clazz = DFEC_INPUT;
        event_1.type = DIET_AXISMOTION;
        event_1.flags = (DFBInputEventFlags) (DIEF_AXISABS | DIEF_FOLLOW);
        event_1.axis = DIAI_X;
        event_1.axisabs = x;
        event_1.max = __screenSize.w;
        event_1.min = 0;
        Engine::instance().postEvent(DFB_EVENT(&event_1) );

        DFBInputEvent event_2;
        event_2.clazz = DFEC_INPUT;
        event_2.type = DIET_AXISMOTION;
        event_2.flags = DIEF_AXISABS;
        event_2.axis = DIAI_Y;
        event_2.axisabs = y;
        event_2.max = __screenSize.h;
        event_2.min = 0;
        event_2.button = (DFBInputDeviceButtonIdentifier) button;
        event_2.buttons = (DFBInputDeviceButtonMask) buttonMask;
        Engine::instance().postEvent(DFB_EVENT(&event_2) );

        if (type == PointerButtonDown)
        {
            DFBInputEvent event_3;
            event_3.clazz = DFEC_INPUT;
            event_3.type = DIET_BUTTONPRESS;
            event_3.flags = DIEF_NONE;
            event_3.button = (DFBInputDeviceButtonIdentifier) button;
            event_3.buttons = (DFBInputDeviceButtonMask) buttonMask;
            Engine::instance().postEvent(DFB_EVENT(&event_3) );

        } else if (type == PointerButtonUp)
        {
            DFBInputEvent event_3;
            event_3.clazz = DFEC_INPUT;
            event_3.type = DIET_BUTTONRELEASE;
            event_3.flags = DIEF_NONE;
            event_3.button = (DFBInputDeviceButtonIdentifier) button;
            event_3.buttons = (DFBInputDeviceButtonMask) buttonMask;
            Engine::instance().postEvent(DFB_EVENT(&event_3) );
        } else if (type == PointerWheel)
        {
            DFBInputEvent event_3;
            event_3.clazz = DFEC_INPUT;
            event_3.type = DIET_AXISMOTION;
            event_3.flags = DIEF_AXISABS;
            event_3.axis = DIAI_Z;
            event_3.axisabs = step;
            event_3.max = __screenSize.h;
            event_3.min = 0;
            event_3.button = (DFBInputDeviceButtonIdentifier) button;
            event_3.buttons = (DFBInputDeviceButtonMask) buttonMask;
            Engine::instance().postEvent(DFB_EVENT(&event_3) );
        }
    } else
    {
        DFBWindowEvent event;
        event.clazz = DFEC_WINDOW;
        event.type = (DFBWindowEventType) type;
        event.window_id = activeWindow()->windowID();
        event.flags = DWEF_NONE;
        event.x = x;
        event.y = y;
        event.cx = cx;
        event.cy = cy;
        event.step = step;
        event.button = (DFBInputDeviceButtonIdentifier) button;
        event.buttons = (DFBInputDeviceButtonMask) buttonMask;
        Engine::instance().postEvent(DFB_EVENT(&event) );
    }
}

#if ILIXI_HAS_GETFRAMETIME
long long
Application::getFrameTime()
{
    if (__instance)
        return __instance->_frameTime ? __instance->_frameTime : direct_clock_get_time(DIRECT_CLOCK_MONOTONIC);
    return 0;
}

void
Application::setFrameTime(long long micros)
{
    if (__instance)
        __instance->_frameTime = micros;
}
#endif

AppWindow*
Application::appWindow() const
{
    return _appWindow;
}

void
Application::show()
{
    if (__flags & APS_HIDDEN)
    {
        if (_appWindow)
            _appWindow->showWindow();
        __flags = APS_VISIBLE;
        sigVisible();
    }
}

void
Application::hide()
{
    if (__flags & APS_VISIBLE)
    {
        if (_appWindow)
            _appWindow->closeWindow();
        __flags = APS_HIDDEN;
        sigHidden();
    }
}

void
Application::handleUserEvent(const DFBUserEvent& event)
{
}

bool
Application::windowPreEventFilter(const DFBWindowEvent& event)
{
    return false;
}

void
Application::handleEvents(int32_t timeout, bool forceWait)
{
    ILOG_TRACE_F(ILX_APPLICATION_EVENTS);

    bool wait = true;
    if (!forceWait)
    {
        for (WindowList::iterator it = __windowList.begin(); it != __windowList.end(); ++it)
        {
            if (((WindowWidget*) *it)->_updates._updateQueue.valid)
            {
                wait = false;
                break;
            }
        }
    }

    if (wait)
        Engine::instance().waitForEvents(timeout);

    DFBEvent event;
#if ILIXI_HAVE_MOTION_COMPRESSION
    DFBWindowEvent lastMotion; // Used for compressing motion events.

    lastMotion.type = DWET_NONE;
#endif
    while (Engine::instance().getNextEvent(&event) == DFB_OK)
    {
        switch (event.clazz)
        {
        // If layer is exclusively used by application, we get DFEC_INPUT events.
        case DFEC_INPUT:
            switch (event.input.type)
            {
            case DIET_KEYPRESS:
                handleKeyInputEvent((const DFBInputEvent&) event, DWET_KEYDOWN);
                break;

            case DIET_KEYRELEASE:
                handleKeyInputEvent((const DFBInputEvent&) event, DWET_KEYUP);
                break;

            case DIET_BUTTONPRESS:
                handleButtonInputEvent((const DFBInputEvent&) event, DWET_BUTTONDOWN);
                break;

            case DIET_BUTTONRELEASE:
                handleButtonInputEvent((const DFBInputEvent&) event, DWET_BUTTONUP);
                break;

            case DIET_AXISMOTION:
                handleAxisMotion((const DFBInputEvent&) event);
                break;

            default:
                ILOG_WARNING(ILX_APPLICATION, "Unknown input event type\n");
                break;
            }
            break;

        case DFEC_WINDOW:
            if (!(PlatformManager::instance().appOptions() & OptExclusive) && event.window.type != DWET_UPDATE)
            {
#if ILIXI_HAVE_MOTION_COMPRESSION
                if (event.window.type == DWET_MOTION && event.window.buttons == 0)
                    lastMotion = event.window;
                else if (lastMotion.type == DWET_NONE)
                {
                    if (!windowPreEventFilter((const DFBWindowEvent&) lastMotion))
                        handleWindowEvents((const DFBWindowEvent&) lastMotion);
                    lastMotion.type = DWET_NONE;
                    if (!windowPreEventFilter((const DFBWindowEvent&) event.window))
                        handleWindowEvents((const DFBWindowEvent&) event.window);
                }
#else
                if (!windowPreEventFilter((const DFBWindowEvent&) event.window))
                    handleWindowEvents((const DFBWindowEvent&) event.window);
#endif
            }
            break;

        case DFEC_USER:
            handleUserEvent((const DFBUserEvent&) event);
            break;

        case DFEC_UNIVERSAL:
            {
                UniversalEvent* uEvent = (UniversalEvent*) &event;
                ILOG_DEBUG(ILX_APPLICATION_EVENTS, " -> target: %p\n", uEvent->target);
                if (uEvent->target)
                    uEvent->target->universalEvent(uEvent);
            }
            break;

#if ILIXI_HAS_SURFACEEVENTS
        case DFEC_SURFACE:
            Engine::instance().consumeSurfaceEvent((const DFBSurfaceEvent&) event);
            break;
#endif
        default:
            break;
        }
    }

#if ILIXI_HAVE_MOTION_COMPRESSION
    if (lastMotion.type != 0)
        if (!windowPreEventFilter((const DFBWindowEvent&) lastMotion))
            handleWindowEvents((const DFBWindowEvent&) lastMotion);
#endif
    ILOG_DEBUG(ILX_APPLICATION_EVENTS, " -> end handle events \n");
}

void
Application::updateWindows()
{
    ILOG_TRACE_F(ILX_APPLICATION_UPDATES);
    if (!(PlatformManager::instance().appOptions() & OptNoUpdates))
    {
        pthread_mutex_lock(&__windowMutex);

        for (WindowList::iterator it = __windowList.begin(); it != __windowList.end(); ++it)
            ((WindowWidget*) *it)->updateWindow();

        pthread_mutex_unlock(&__windowMutex);
    }

    if ((PlatformManager::instance().appOptions() & OptExclusive)) {
        static DFBPoint preCursor = __cursorNew;
        if ((preCursor.x != __cursorNew.x) || (preCursor.y != __cursorNew.y))
        {
            PlatformManager::instance().renderCursor(Application::cursorPosition(), _dragging);
            preCursor = __cursorNew;
        }
    }
    ILOG_DEBUG(ILX_APPLICATION_UPDATES, " -> finished updating windows.\n");
}

void
Application::setStylist(StylistBase* stylist)
{
    // TODO we will allow setting custom stylist in the future.
    if (__instance->_appWindow->_stylist)
    {
        ILOG_WARNING(ILX_APPLICATION, "setStylist()  -  Not implemented!");
        return;
    }

    if (!stylist)
        return;

    __instance->_appWindow->_stylist = stylist;

    if (!__instance->_appWindow->_stylist->setFontPack(PlatformManager::instance().getFontPack().c_str()))
        ILOG_THROW(ILX_APPLICATION, "Please fix your configuration file!\n");

    if (!__instance->_appWindow->_stylist->setIconPack(PlatformManager::instance().getIconPack().c_str()))
        ILOG_THROW(ILX_APPLICATION, "Please fix your configuration file!\n");

    if (!__instance->_appWindow->_stylist->setPaletteFromFile(PlatformManager::instance().getPalette().c_str()))
        ILOG_THROW(ILX_APPLICATION, "Please fix your configuration file!\n");

    if (!__instance->_appWindow->_stylist->setStyleFromFile(PlatformManager::instance().getStyle().c_str()))
        ILOG_THROW(ILX_APPLICATION, "Please fix your configuration file!\n");
}

bool
Application::setFontPack(const char* fontPack)
{
    if (__instance->_appWindow->_stylist)
        return __instance->_appWindow->_stylist->setFontPack(fontPack);
    return false;
}

bool
Application::setIconPack(const char* iconPack)
{
    if (__instance->_appWindow->_stylist)
        return __instance->_appWindow->_stylist->setIconPack(iconPack);
    return false;
}

bool
Application::setPaletteFromFile(const char* palette)
{
    if (__instance->_appWindow->_stylist)
        return __instance->_appWindow->_stylist->setPaletteFromFile(palette);
    return false;
}

bool
Application::setStyleFromFile(const char* style)
{
    if (__instance->_appWindow->_stylist)
        return __instance->_appWindow->_stylist->setStyleFromFile(style);
    return false;
}

void
Application::compose(const PaintEvent& event)
{
}

WindowWidget *
Application::activeWindow()
{
    if (__instance)
    {
        pthread_mutex_lock(&__instance->__windowMutex);
        WindowWidget* w = __instance->__activeWindow;
        pthread_mutex_unlock(&__instance->__windowMutex);
        return w;
    }
    return NULL;
}

Size
Application::appSize()
{
    return __instance->__windowList.front()->size();
}

DFBPoint
Application::cursorPosition()
{
    return __instance->__cursorNew;
}

void
Application::handleWindowEvents(const DFBWindowEvent& event)
{
    ILOG_TRACE_F(ILX_APPLICATION);
    pthread_mutex_lock(&__windowMutex);
    if (__activeWindow && (__activeWindow->_modality & WindowWidget::WindowModal))
    {
        ILOG_DEBUG(ILX_APPLICATION, " -> Modal active window: %p\n", __activeWindow);
        __activeWindow->handleWindowEvent(event);
    }
    else
    {
        ILOG_DEBUG(ILX_APPLICATION, " -> Non modal active window: %p\n", __activeWindow);
        WindowList::reverse_iterator it = __windowList.rbegin();
        while (it != __windowList.rend())
        {
            if(((WindowWidget*) *it)->handleWindowEvent(event, _dragging))
                break;
            ++it;
        }
        ILOG_DEBUG(ILX_APPLICATION, " -> Non modal loop ends\n");
    }
    pthread_mutex_unlock(&__windowMutex);
}

void
Application::handleDragEvents(const DFBWindowEvent& event)
{
    ILOG_TRACE_F(ILX_APPLICATION);
    pthread_mutex_lock(&__instance->__windowMutex);
    WindowWidget* w;
    WindowList::reverse_iterator it = __instance->__windowList.rbegin();
    while (it != __instance->__windowList.rend())
    {
        w = ((WindowWidget*) *it);
        if (w->_dragWindow)
        {
            ++it;
            continue;
        }
        if (w->handleWindowEvent(event, __instance->_dragging))
            break;
        ++it;
    }
    pthread_mutex_unlock(&__instance->__windowMutex);
}

void
Application::handleKeyInputEvent(const DFBInputEvent& event, DFBWindowEventType type)
{
    DFBEvent we;
    we.clazz = DFEC_WINDOW;
    we.window.type = type;

    we.window.flags = (event.flags & DIEF_REPEAT) ? DWEF_REPEAT : DWEF_NONE;
    we.window.key_code = event.key_code;
    we.window.key_id = event.key_id;
    we.window.key_symbol = event.key_symbol;

    we.window.locks = event.locks;
    we.window.modifiers = event.modifiers;
    we.window.button = event.button;
    we.window.buttons = event.buttons;

    we.window.x = __cursorNew.x;
    we.window.y = __cursorNew.y;

    we.window.cx = __cursorNew.x;
    we.window.cy = __cursorNew.y;

    if (!windowPreEventFilter((const DFBWindowEvent&) we))
        handleWindowEvents((const DFBWindowEvent&) we);
}

void
Application::handleButtonInputEvent(const DFBInputEvent& event, DFBWindowEventType type)
{
    DFBEvent we;
    we.clazz = DFEC_WINDOW;

    we.window.type = type;
    we.window.x = __cursorNew.x;
    we.window.y = __cursorNew.y;

    we.window.cx = __cursorNew.x;
    we.window.cy = __cursorNew.y;

    we.window.button = event.button;
    we.window.buttons = event.buttons;

    if (!windowPreEventFilter((const DFBWindowEvent&) we))
        handleWindowEvents((const DFBWindowEvent&) we);
}

void
Application::handleAxisMotion(const DFBInputEvent& event)
{
    DFBEvent we;
    we.clazz = DFEC_WINDOW;
    we.window.type = DWET_MOTION;

    we.window.step = 0;

    __cursorOld = __cursorNew;

    if (event.flags & DIEF_AXISREL)
    {
        if (event.axis == DIAI_X)
            __cursorNew.x += event.axisrel;
        else if (event.axis == DIAI_Y)
            __cursorNew.y += event.axisrel;
        else
        {
            we.window.type = DWET_WHEEL;
            we.window.step = -event.axisrel;
        }
    } else if (event.flags & DIEF_AXISABS)
    {
        int newPos;
        if ((event.flags & DIEF_MIN) && (event.flags & DIEF_MAX))
            newPos = (event.axisabs - event.min) * __screenSize.w / (event.max - event.min);
        else
            newPos = event.axisabs;

        if (event.axis == DIAI_X)
            __cursorNew.x = newPos;
        else if (event.axis == DIAI_Y)
            __cursorNew.y = newPos;
        else
        {
            we.window.type = DWET_WHEEL;
            we.window.step = -event.axisrel;
        }
    }

    if (__cursorNew.x < 0)
        __cursorNew.x = 0;
    else if (__cursorNew.x > __screenSize.w - 1)
        __cursorNew.x = __screenSize.w - 1;

    if (__cursorNew.y < 0)
        __cursorNew.y = 0;
    else if (__cursorNew.y > __screenSize.h - 1)
        __cursorNew.y = __screenSize.h - 1;

    if (we.window.type == DWET_MOTION && PlatformManager::instance().cursorVisible())
    {
        Rectangle cold(__instance->__cursorOld.x, __instance->__cursorOld.y, 32, 32);
        Rectangle cnew(__instance->__cursorNew.x, __instance->__cursorNew.y, 32, 32);

        activeWindow()->update(PaintEvent(cnew.united(cold), 10));
    }

    we.window.x = 0;
    we.window.y = 0;

    we.window.x = __cursorNew.x;
    we.window.y = __cursorNew.y;

    we.window.cx = __cursorNew.x;
    we.window.cy = __cursorNew.y;

    we.window.button = event.button;
    we.window.buttons = event.buttons;

    if (!windowPreEventFilter((const DFBWindowEvent&) we))
        handleWindowEvents((const DFBWindowEvent&) we);
}

void
Application::setActiveWindow(WindowWidget* window)
{
    if (__instance)
    {
        ILOG_TRACE_F(ILX_APPLICATION);
        pthread_mutex_lock(&__instance->__windowMutex);

        if (__instance->__activeWindow)
        {
            if (window->_modality & WindowWidget::WindowModal)
                detachDFBWindow(__instance->__activeWindow->_window);
            __instance->__activeWindow->_eventManager->setGrabbedWidget(NULL);
            __instance->__activeWindow->_eventManager->setExposedWidget(NULL);
        }

        __instance->__activeWindow = window;
        attachDFBWindow(__instance->__activeWindow->_window);

        pthread_mutex_unlock(&__instance->__windowMutex);
        ILOG_DEBUG(ILX_APPLICATION, "WindowWidget %p is now active.\n", __instance->__activeWindow);
    }
}

bool
Application::addWindow(WindowWidget* window)
{
    if (__instance && window)
    {
        ILOG_TRACE_F(ILX_APPLICATION);
        pthread_mutex_lock(&__instance->__windowMutex);

        WindowList::iterator it = std::find(__instance->__windowList.begin(), __instance->__windowList.end(), window);
        if (window == *it)
        {
            pthread_mutex_unlock(&__instance->__windowMutex);
            ILOG_ERROR(ILX_APPLICATION, "WindowWidget %p already added!\n", window);
            return false;
        }
        __instance->__windowList.push_back(window);

        pthread_mutex_unlock(&__instance->__windowMutex);
        ILOG_DEBUG(ILX_APPLICATION, "WindowWidget %p is added.\n", window);
        return true;
    }
    return false;
}

bool
Application::removeWindow(WindowWidget* window)
{
    if (__instance && window)
    {
        ILOG_TRACE_F(ILX_APPLICATION);
        pthread_mutex_lock(&__instance->__windowMutex);

        for (WindowList::iterator it = __instance->__windowList.begin(); it != __instance->__windowList.end(); ++it)
        {
            if (window == *it)
            {
                it = __instance->__windowList.erase(it);

                if (__instance->__activeWindow == window)
                {
                    if (__instance->__windowList.size())
                        setActiveWindow(__instance->__windowList.back());
                    else
                        __instance->__activeWindow = NULL;
                    ILOG_DEBUG(ILX_APPLICATION, " -> WindowWidget %p is now active.\n", __instance->__activeWindow);
                }
                ILOG_DEBUG(ILX_APPLICATION, " -> WindowWidget %p is removed.\n", window);
                pthread_mutex_unlock(&__instance->__windowMutex);
                return true;
            }
        }
        pthread_mutex_unlock(&__instance->__windowMutex);
        ILOG_WARNING( ILX_APPLICATION, "Cannot remove WindowWidget, %p not found!\n", window);
    }
    return false;
}

void
Application::attachDFBWindow(Window* window)
{
    if (window && !(PlatformManager::instance().appOptions() & OptExclusive))
    {
        ILOG_TRACE_F(ILX_APPLICATION);
        DFBResult ret;

        if (!window->_dfbWindow)
        {
            ILOG_WARNING( ILX_APPLICATION, " -> Window::_dfbWindow is NULL\n");
            return;
        }

        ret = Engine::instance().attachWindow(window->_dfbWindow);

        ret = window->_dfbWindow->RequestFocus(window->_dfbWindow);
        if (ret != DFB_OK)
            ILOG_ERROR( ILX_APPLICATION, "RequestFocus error: %s! \n", DirectFBErrorString(ret));

        ret = Engine::instance().resetBuffer();

        ILOG_DEBUG(ILX_APPLICATION, " -> Window %p is attached.\n", window);
    }
}

void
Application::detachDFBWindow(Window* window)
{
    if (window && !(PlatformManager::instance().appOptions() & OptExclusive))
    {
        ILOG_TRACE_F(ILX_APPLICATION);
        DFBResult ret;

        if (!window->_dfbWindow)
        {
            ILOG_WARNING( ILX_APPLICATION, "Window::_dfbWindow is NULL\n");
            return;
        }

        ret = Engine::instance().detachWindow(window->_dfbWindow);

        ret = Engine::instance().resetBuffer();

        ILOG_DEBUG(ILX_APPLICATION, " -> Window %p is detached.\n", window);
    }
}

void
Application::setDragging(bool dragging)
{
    __instance->_dragging = dragging;
}

} /* namespace ilixi */
