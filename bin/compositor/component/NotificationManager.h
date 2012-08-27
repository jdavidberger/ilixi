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

#ifndef NOTIFICATIONMANAGER_H_
#define NOTIFICATIONMANAGER_H_

#include <vector>
#include <directfb.h>
#include <lib/Timer.h>
#include <lib/TweenAnimation.h>
#include <lib/Notify.h>

namespace ilixi
{
class Compositor;
class Notification;

class NotificationManager
{
public:
    NotificationManager(Compositor* compositor);

    virtual
    ~NotificationManager();

    void
    addNotification(const Notify::NotifyData& data);

private:
    int _deltaY;
    Compositor* _compositor;
    typedef std::vector<Notification*> NotificationVector;
    NotificationVector _notifications;
    Timer _timer;
    TweenAnimation _anim;
    Tween* _tween;

    void
    removeNotifications();

    void
    arrangeNotifications(int deltaY);

    void
    tweenSlot();
};

} /* namespace ilixi */
#endif /* NOTIFICATIONMANAGER_H_ */
