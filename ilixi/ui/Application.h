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

#ifndef ILIXI_APPLICATION_H_
#define ILIXI_APPLICATION_H_

#include <core/AppBase.h>
#include <ui/WindowWidget.h>

namespace ilixi
{
//! Windowed application.
/*!
 * This class is used to create a new UI application with its own window.
 */
class Application : public AppBase, public WindowWidget
{
public:
    /*!
     * Constructor.
     */
    Application(int* argc, char*** argv, AppOptions opts = OptNone);

    /*!
     * Destructor.
     */
    virtual
    ~Application();

    /*!
     * Returns background image.
     */
    Image*
    background() const;

    /*!
     * Returns frame's canvas x-coordinate including the left margin.
     */
    virtual int
    canvasX() const;

    /*!
     * Returns frame's canvas y-coordinate including the top margin.
     */
    virtual int
    canvasY() const;

    /*!
     * Returns frame's canvas height excluding margins.
     */
    virtual int
    canvasHeight() const;

    /*!
     * Returns frame's canvas width excluding margins.
     */
    virtual int
    canvasWidth() const;

    /*!
     * Terminates application.
     */
    void
    quit();

    /*!
     * You can reimplement this method to create your own custom main loop.
     */
    virtual void
    exec();

    /*!
     * Sets a background image.
     */
    void
    setBackgroundImage(std::string imagePath);

    /*!
     * This signal is emitted after application window is painted and visible.
     */
    sigc::signal<void> sigVisible;

    /*!
     * This signal is emitted before application becomes hidden.
     */
    sigc::signal<void> sigHidden;

    /*!
     * This signal is emitted before application is terminated.
     */
    sigc::signal<void> sigQuit;

protected:
    /*!
     * Shows application window.
     */
    void
    show();

    /*!
     * Hides application window.
     */
    void
    hide();

    /*!
     * Sets the stylist for application.
     *
     * /warning This method is not implemented.
     */
    static void
    setStylist(StylistBase* stylist);

    /*!
     * Sets the palette file for stylist.
     */
    static void
    setPaletteFromFile(const char* palette);

    /*!
     * Sets the style file for stylist.
     */
    static void
    setStyleFromFile(const char* style);

    void
    postUserEvent(unsigned int type, void* data = NULL);

private:
    //! Background image of application.
    Image* _backgroundImage;

    /*!
     * This method is used if application has backgroundFilled set to true.
     * Default implementation uses a Stylist to tile the background image or
     * fill background with a colour from palette.
     *
     * \sa setBackgroundFilled()
     */
    virtual void
    compose(const PaintEvent& event);
};
}
#endif /* ILIXI_APPLICATION_H_ */
