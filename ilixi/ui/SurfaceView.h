/*
 Copyright 2012 Tarik Sekmen.

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

#ifndef ILIXI_SURFACEVIEW_H_
#define ILIXI_SURFACEVIEW_H_

#include "ui/Widget.h"
#include "core/SurfaceEventListener.h"

namespace ilixi
{
  //! Renders any DFBSurface onto itself.
  /*!
   * This widget can be used to render the contents of a DFBSurface
   * directly onto itself.
   *
   * \warning Does not support sub-surfaces at the moment.
   */
  class SurfaceView : public SurfaceEventListener, public Widget
  {
  public:
    /*!
     * Constructor
     */
    SurfaceView(Widget* parent = 0);

    /*!
     * Destructor.
     */
    virtual
    ~SurfaceView();

    /*!
     * Returns the size of source surface.
     */
    virtual ilixi::Size
    preferredSize() const;

    /*!
     * Returns source window, if any.
     */
    IDirectFBWindow*
    dfbWindow() const;

    /*!
     * Returns source window id.
     * If there is no window, returns 0.
     */
    DFBWindowID
    dfbWindowID() const;

    /*!
     * Sets source surface using given id.
     */
    void
    setSourceFromSurfaceID(DFBSurfaceID id);

    /*!
     * Sets source surface to given surface.
     */
    void
    setSourceFromSurface(IDirectFBSurface* source);

    /*!
     * Sets source surface to window's surface.
     */
    void
    setSourceFromWindow(IDirectFBWindow* window);

    void
    paint(const PaintEvent& event);

    /*!
     * This signal is emitted after the first flip operation
     * on source surface.
     */
    sigc::signal<void> sigSourceReady;
    /*!
     * This signal is emitted each time the source surface is updated.
     */
    sigc::signal<void> sigSourceUpdated;
    /*!
     * This signal is emitted once the source surface is destroyed.
     */
    sigc::signal<void> sigSourceDestroyed;

  protected:
    /*!
     * Scale ratio in horizontal direction.
     */
    float
    hScale() const;

    /*!
     * Scale ratio in vertical direction.
     */
    float
    vScale() const;

    /*!
     * Reimplement for decorations.
     */
    virtual void
    compose(const PaintEvent& event);

    /*!
     * Blits source surface on itself.
     */
    virtual void
    renderSource(const PaintEvent& event);

  private:
    //! This property stores the scale ratio in horizontal direction.
    double _hScale;
    //! This property stores the scale ratio in vertical direction.
    double _vScale;
    //! This property stores the window interface of source surface, if any.
    IDirectFBWindow* _sourceWindow;
    //! This property stores the id of window, if any.
    DFBWindowID _windowID;
    //! This property stores the flip counter for last update.
    unsigned int _flipCount;
#ifdef ILIXI_STEREO_OUTPUT
    bool _sourceStereo;
#endif

    void
    onSourceUpdate(const DFBSurfaceEvent& event);

    void
    onSourceDestroyed(const DFBSurfaceEvent& event);

    void
    onSVGeomUpdate();

    // handlers
    virtual void
    keyDownEvent(const KeyEvent& keyEvent);

    virtual void
    keyUpEvent(const KeyEvent& keyEvent);

    virtual void
    pointerButtonDownEvent(const PointerEvent& pointerEvent);

    virtual void
    pointerButtonUpEvent(const PointerEvent& pointerEvent);

    virtual void
    pointerMotionEvent(const PointerEvent& pointerEvent);

    virtual void
    pointerWheelEvent(const PointerEvent& pointerEvent);

    virtual void
    focusInEvent();

    virtual void
    focusOutEvent();

    virtual void
    enterEvent(const PointerEvent& pointerEvent);

    virtual void
    leaveEvent(const PointerEvent& pointerEvent);

  };

} /* namespace ilixi */
#endif /* ILIXI_SURFACEVIEW_H_ */
