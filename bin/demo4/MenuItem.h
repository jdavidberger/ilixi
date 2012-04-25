/*
 Copyright 2011 Tarik Sekmen.

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

#ifndef ILIXI_MENUITEM_H_
#define ILIXI_MENUITEM_H_

#include "ui/Button.h"
#include "lib/TweenAnimation.h"

namespace ilixi
{

  class MenuItem : public ilixi::Button
  {
  public:
    MenuItem(const std::string& text, Widget* parent = 0);

    virtual
    ~MenuItem();

    virtual ilixi::Size
    preferredSize() const;

    Image*
    image() const;

    void
    setImage(ilixi::Image* image);

    sigc::signal<void> sigPressed;
    sigc::signal<void, Widget*> sigFocused;

  protected:
    void
    tweenSlot();

    virtual void
    focusInEvent();

    virtual void
    focusOutEvent();

    virtual void
    compose();

    void
    updateTextLayoutGeometry();

  private:
    ilixi::Image* _image;
    float _val1;
    ilixi::TweenAnimation* _inAni;
    ilixi::TweenAnimation* _outAni;
  };

} /* namespace ilixi */
#endif /* ILIXI_MENUITEM_H_ */