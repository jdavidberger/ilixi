/*
 Copyright 2010-2012 Tarik Sekmen.

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

#include "CompositorComponent.h"
#include <core/Logger.h>
#include "Compositor.h"
#include "NotificationManager.h"

namespace ilixi
{

D_DEBUG_DOMAIN( ILX_COMPCOMP, "ilixi/Coma/CompositorComponent",
               "CompositorComponent");

CompositorComponent::CompositorComponent(Compositor* compositor)
        : ComaComponent("CompositorComponent", CompositorNumNotifications),
          _compositor(compositor),
          _notificationMan(NULL)
{
    init();
    createNotification(0, NULL);
    createNotification(1, NULL);
    createNotification(2, NULL);
    createNotification(3, NULL, CNF_NONE);
    createNotification(4, NULL, CNF_NONE);
    createNotification(5, NULL, CNF_NONE);
    createNotification(6, NULL, CNF_NONE);
    _notificationMan = new NotificationManager(compositor);
}

CompositorComponent::~CompositorComponent()
{
    delete _notificationMan;
}

void
CompositorComponent::notifyVisible(pid_t pid)
{
    int* tPid;
    allocate(sizeof(tPid), (void**) &tPid);
    *tPid = pid;
    notify(AppVisible, tPid);
    ILOG_DEBUG(ILX_COMPCOMP, "%d is now visible!\n", pid);
}

void
CompositorComponent::notifyHidden(pid_t pid)
{
    int* tPid;
    allocate(sizeof(tPid), (void**) &tPid);
    *tPid = pid;
    notify(AppHidden, tPid);
    ILOG_DEBUG(ILX_COMPCOMP, "%d is now hidden!\n", pid);
}

void
CompositorComponent::notifyHasFocus(pid_t pid)
{
    int* tPid;
    allocate(sizeof(tPid), (void**) &tPid);
    *tPid = pid;
    notify(AppHasFocus, tPid);
    ILOG_DEBUG(ILX_COMPCOMP, "%d is now focused!\n", pid);
}

void
CompositorComponent::signalHomeShowing()
{
    notify(ShowingHome, NULL);
}

void
CompositorComponent::signalSwitcherShowing()
{
    notify(ShowingSwitcher, NULL);
}

void
CompositorComponent::signalHomeHidden()
{
    notify(HidingHome, NULL);
}

void
CompositorComponent::signalSwitcherHidden()
{
    notify(HidingSwitcher, NULL);
}

DirectResult
CompositorComponent::comaMethod(ComaMethodID method, void *arg)
{
    DirectResult ret = DR_OK;
    switch (method)
    {
    case AddNotification:
        {
            unsigned int surfaceID = *((unsigned int*) arg);
            ILOG_DEBUG(ILX_COMPCOMP, "AddNotification for %u\n", surfaceID);
            _notificationMan->addNotification(surfaceID);
            break;
        }

    case AddOverlay:
        {
            unsigned int surfaceID = *((unsigned int*) arg);
            ILOG_DEBUG(ILX_COMPCOMP, "AddOverlay for %u\n", surfaceID);
            _compositor->addOverlay(surfaceID);
            break;
        }

    case AddDialog:
        {
            unsigned int surfaceID = *((unsigned int*) arg);
            ILOG_DEBUG(ILX_COMPCOMP, "AddDialog for %u\n", surfaceID);
            _compositor->addDialog(surfaceID);
            break;
        }

    case ShowHome:
        _compositor->showLauncher(true);
        break;

    case ShowSwitcher:
        _compositor->showSwitcher(true);
        break;

    case HideHome:
        _compositor->showLauncher(false);
        break;

    case HideSwither:
        _compositor->showSwitcher(false);
        break;

    default:
        ret = DR_NOIMPL;
        break;
    }
    return ret;
}

} /* namespace ilixi */
