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

#include <graphics/Style.h>
#include <lib/FileSystem.h>
#include <core/Logger.h>
#include <lib/XMLReader.h>
#include <libgen.h>
#include <fstream>

namespace ilixi
{

D_DEBUG_DOMAIN( ILX_STYLE, "ilixi/graphics/Style", "Style Parser");

Style::Style()
        : _buttonFont(NULL),
          _defaultFont(NULL),
          _inputFont(NULL),
          _titleFont(NULL),
          _iconPack(NULL),
          _defaultIconSize(48),
          _pack(NULL)
{
}

Style::~Style()
{
    release();
}

Image*
Style::getIcon(std::string name)
{
    ILOG_TRACE(ILX_STYLE);
    IconMap::iterator it = _iconMap.find(name);
    if (it != _iconMap.end())
    {
        ILOG_DEBUG( ILX_STYLE, " -> %s @ (%d, %d, %d, %d)\n", name.c_str(), it->second.x(), it->second.y(), _defaultIconSize, _defaultIconSize);
        return new Image(_iconPack, Rectangle(it->second.x(), it->second.y(), _defaultIconSize, _defaultIconSize));
    }
    ILOG_WARNING(ILX_STYLE, " -> Cannot find icon: %s\n", name.c_str());
    return NULL;
}

void
Style::release()
{
    // fonts
    delete _buttonFont;
    delete _defaultFont;
    delete _inputFont;
    delete _titleFont;

    // images
    delete _iconPack;
    delete _pack;
}

bool
Style::parseStyle(const char* style)
{
    ILOG_TRACE(ILX_STYLE);
    ILOG_DEBUG(ILX_STYLE, " -> file: %s\n", style);

    std::string cacheFile = FileSystem::homeDirectory().append(PrintF("/%u.sxml", createHash(style)));
    if (difftime(FileSystem::getModificationTime(cacheFile), FileSystem::getModificationTime(style)) > 0)
    {
        ILOG_DEBUG(ILX_STYLE, " -> Parsing cached style file.\n");
        std::ifstream ifs(cacheFile.c_str(), std::ios::binary | std::ios::in);
        ifs >> *this;
        ifs.close();
        ILOG_INFO(ILX_STYLE, "Parsed cached style file: %s\n", cacheFile.c_str());
    } else
    {
        XMLReader xml;
        if (xml.loadFile(style) == false)
        {
            ILOG_FATAL(ILX_STYLE, "Could not parse style!\n");
            return false;
        }

        xmlNodePtr group = xml.currentNode();

        while (group != NULL)
        {
            if (xmlStrcmp(group->name, (xmlChar*) "fonts") == 0)
            {
                ILOG_DEBUG(ILX_STYLE, " -> parsing fonts...\n");
                parseFonts(group->children);
            } else if (xmlStrcmp(group->name, (xmlChar*) "icons") == 0)
            {
                ILOG_DEBUG(ILX_STYLE, " -> parsing icons...\n");
                xmlChar* imgFile = xmlGetProp(group, (xmlChar*) "resource");
                xmlChar* imgDefSize = xmlGetProp(group, (xmlChar*) "defaultSize");
                _iconPack = new Image(std::string(ILIXI_DATADIR"" + std::string((char*) imgFile)));
                _defaultIconSize = atoi((char*) imgDefSize);
                parseIcons(group->children);
                xmlFree(imgDefSize);
                xmlFree(imgFile);
            } else if (xmlStrcmp(group->name, (xmlChar*) "pack") == 0)
            {
                ILOG_DEBUG(ILX_STYLE, " -> parsing theme...\n");
                char* path = strdup(style);
                path = dirname(path);
                std::string imgPack = std::string(std::string(path).append("/ui-pack.dfiff"));
                ILOG_DEBUG(ILX_STYLE, " -> pack: %s\n", imgPack.c_str());
                _pack = new Image(imgPack);
                parseTheme(group->children);
                free(path);
            }

            group = group->next;
        } // end while(group)

        ILOG_INFO(ILX_STYLE, "Parsed style file: %s\n", style);

        // create cached file
        std::ofstream ofs(cacheFile.c_str(), std::ios::binary | std::ios::out);
        ofs.seekp(0, std::ios::beg);
        ofs << *this;
        ofs.close();
    }

    return true;
}

void
Style::parseFonts(xmlNodePtr node)
{
    while (node != NULL)
    {
        ILOG_DEBUG(ILX_STYLE, " -> font: %s...\n", node->name);
        xmlChar* fileC = xmlNodeGetContent(node->children);
        xmlChar* sizeC = xmlNodeGetContent(node->children->next);
        xmlChar* styleC = xmlNodeGetContent(node->children->next->next);

        Font::Style fontStyle = Font::Plain;
        if (styleC)
        {
            if (xmlStrcmp(styleC, (xmlChar *) "italic") == 0)
                fontStyle = Font::Italic;
            else if (xmlStrcmp(styleC, (xmlChar *) "bold") == 0)
                fontStyle = Font::Bold;
        }

        if (xmlStrcmp(node->name, (xmlChar*) "defaultFont") == 0)
        {
            _defaultFont = new Font((char*) fileC, atoi((char*) sizeC));
            _defaultFont->setStyle(fontStyle);
        } else if (xmlStrcmp(node->name, (xmlChar*) "buttonFont") == 0)
        {
            _buttonFont = new Font((char*) fileC, atoi((char*) sizeC));
            _buttonFont->setStyle(fontStyle);
        }

        else if (xmlStrcmp(node->name, (xmlChar*) "inputFont") == 0)
        {
            _inputFont = new Font((char*) fileC, atoi((char*) sizeC));
            _inputFont->setStyle(fontStyle);
        }

        else if (xmlStrcmp(node->name, (xmlChar*) "titleFont") == 0)
        {
            _titleFont = new Font((char*) fileC, atoi((char*) sizeC));
            _titleFont->setStyle(fontStyle);
        }

        xmlFree(fileC);
        xmlFree(sizeC);
        node = node->next;
    }
    ILOG_DEBUG(ILX_STYLE, "Parsed fonts.\n");
}

void
Style::parseIcons(xmlNodePtr node)
{
    _iconMap.clear();
    while (node != NULL)
    {
        xmlChar* iconName = xmlGetProp(node, (xmlChar*) "name");
        xmlChar* iconRow = xmlGetProp(node, (xmlChar*) "row");
        xmlChar* iconCol = xmlGetProp(node, (xmlChar*) "col");
        int x = (atoi((char*) iconCol) - 1) * _defaultIconSize;
        int y = (atoi((char*) iconRow) - 1) * _defaultIconSize;
        std::pair<IconMap::iterator, bool> res = _iconMap.insert(std::make_pair((char*) iconName, Point(x, y)));
        if (!res.second)
            ILOG_WARNING(ILX_STYLE, "Icon %s already exists!\n", iconName);
        else
            ILOG_DEBUG(ILX_STYLE, " -> %s - %d, %d\n", iconName, res.first->second.x(), res.first->second.y());

        xmlFree(iconCol);
        xmlFree(iconRow);
        xmlFree(iconName);
        node = node->next;
    }
}

void
Style::parseTheme(xmlNodePtr node)
{
    xmlNodePtr state;
    while (node != NULL)
    {
        ILOG_DEBUG(ILX_STYLE, " Parsing %s...\n", node->name);
        if (xmlStrcmp(node->name, (xmlChar*) "PushButton") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def3") == 0)
                    get3Rectangle(state->children, pb.def);

                else if (xmlStrcmp(state->name, (xmlChar*) "pre3") == 0)
                    get3Rectangle(state->children, pb.pre);

                else if (xmlStrcmp(state->name, (xmlChar*) "exp3") == 0)
                    get3Rectangle(state->children, pb.exp);

                else if (xmlStrcmp(state->name, (xmlChar*) "dis3") == 0)
                    get3Rectangle(state->children, pb.dis);

                else if (xmlStrcmp(state->name, (xmlChar*) "foc3") == 0)
                    get3Rectangle(state->children, pb.foc);

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "PushButtonOK") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def3") == 0)
                    get3Rectangle(state->children, pbOK.def);

                else if (xmlStrcmp(state->name, (xmlChar*) "pre3") == 0)
                    get3Rectangle(state->children, pbOK.pre);

                else if (xmlStrcmp(state->name, (xmlChar*) "exp3") == 0)
                    get3Rectangle(state->children, pbOK.exp);

                else if (xmlStrcmp(state->name, (xmlChar*) "dis3") == 0)
                    get3Rectangle(state->children, pbOK.dis);

                else if (xmlStrcmp(state->name, (xmlChar*) "foc3") == 0)
                    get3Rectangle(state->children, pbOK.foc);

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "PushButtonCANCEL") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def3") == 0)
                    get3Rectangle(state->children, pbCAN.def);

                else if (xmlStrcmp(state->name, (xmlChar*) "pre3") == 0)
                    get3Rectangle(state->children, pbCAN.pre);

                else if (xmlStrcmp(state->name, (xmlChar*) "exp3") == 0)
                    get3Rectangle(state->children, pbCAN.exp);

                else if (xmlStrcmp(state->name, (xmlChar*) "dis3") == 0)
                    get3Rectangle(state->children, pbCAN.dis);

                else if (xmlStrcmp(state->name, (xmlChar*) "foc3") == 0)
                    get3Rectangle(state->children, pbCAN.foc);

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "CheckBox") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def1") == 0)
                    getRectangle(state->children, cb.def);

                else if (xmlStrcmp(state->name, (xmlChar*) "pre1") == 0)
                    getRectangle(state->children, cb.pre);

                else if (xmlStrcmp(state->name, (xmlChar*) "exp1") == 0)
                    getRectangle(state->children, cb.exp);

                else if (xmlStrcmp(state->name, (xmlChar*) "dis1") == 0)
                    getRectangle(state->children, cb.dis);

                else if (xmlStrcmp(state->name, (xmlChar*) "foc1") == 0)
                    getRectangle(state->children, cb.foc);

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "CheckBoxChecked") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def1") == 0)
                    getRectangle(state->children, cbC.def);

                else if (xmlStrcmp(state->name, (xmlChar*) "pre1") == 0)
                    getRectangle(state->children, cbC.pre);

                else if (xmlStrcmp(state->name, (xmlChar*) "exp1") == 0)
                    getRectangle(state->children, cbC.exp);

                else if (xmlStrcmp(state->name, (xmlChar*) "dis1") == 0)
                    getRectangle(state->children, cbC.dis);

                else if (xmlStrcmp(state->name, (xmlChar*) "foc1") == 0)
                    getRectangle(state->children, cbC.foc);

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "CheckBoxTriChecked") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def1") == 0)
                    getRectangle(state->children, cbT.def);

                else if (xmlStrcmp(state->name, (xmlChar*) "pre1") == 0)
                    getRectangle(state->children, cbT.pre);

                else if (xmlStrcmp(state->name, (xmlChar*) "exp1") == 0)
                    getRectangle(state->children, cbT.exp);

                else if (xmlStrcmp(state->name, (xmlChar*) "dis1") == 0)
                    getRectangle(state->children, cbT.dis);

                else if (xmlStrcmp(state->name, (xmlChar*) "foc1") == 0)
                    getRectangle(state->children, cbT.foc);

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "RadioButtonOn") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def1") == 0)
                    getRectangle(state->children, rbOn.def);

                else if (xmlStrcmp(state->name, (xmlChar*) "pre1") == 0)
                    getRectangle(state->children, rbOn.pre);

                else if (xmlStrcmp(state->name, (xmlChar*) "exp1") == 0)
                    getRectangle(state->children, rbOn.exp);

                else if (xmlStrcmp(state->name, (xmlChar*) "dis1") == 0)
                    getRectangle(state->children, rbOn.dis);

                else if (xmlStrcmp(state->name, (xmlChar*) "foc1") == 0)
                    getRectangle(state->children, rbOn.foc);

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "RadioButtonOff") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def1") == 0)
                    getRectangle(state->children, rbOff.def);

                else if (xmlStrcmp(state->name, (xmlChar*) "pre1") == 0)
                    getRectangle(state->children, rbOff.pre);

                else if (xmlStrcmp(state->name, (xmlChar*) "exp1") == 0)
                    getRectangle(state->children, rbOff.exp);

                else if (xmlStrcmp(state->name, (xmlChar*) "dis1") == 0)
                    getRectangle(state->children, rbOff.dis);

                else if (xmlStrcmp(state->name, (xmlChar*) "foc1") == 0)
                    getRectangle(state->children, rbOff.foc);

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "ProgressBar") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def3") == 0)
                    get3Rectangle(state->children, pr.def);

                else if (xmlStrcmp(state->name, (xmlChar*) "dis3") == 0)
                    get3Rectangle(state->children, pr.dis);

                else if (xmlStrcmp(state->name, (xmlChar*) "pIndicator") == 0)
                {
                    xmlNodePtr iState = state->children;
                    while (iState != NULL)
                    {
                        if (xmlStrcmp(iState->name, (xmlChar*) "def3") == 0)
                            get3Rectangle(iState->children, prI.def);

                        else if (xmlStrcmp(state->name, (xmlChar*) "dis3") == 0)
                            get3Rectangle(iState->children, prI.dis);

                        iState = iState->next;
                    }
                }

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "HSlider") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def3") == 0)
                    get3Rectangle(state->children, hSl.def);

                else if (xmlStrcmp(state->name, (xmlChar*) "dis3") == 0)
                    get3Rectangle(state->children, hSl.dis);

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "VSlider") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def3") == 0)
                    get3Rectangle(state->children, vSl.def);

                else if (xmlStrcmp(state->name, (xmlChar*) "dis3") == 0)
                    get3Rectangle(state->children, vSl.dis);

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "SliderIndicator") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def1") == 0)
                    getRectangle(state->children, slI.def);

                else if (xmlStrcmp(state->name, (xmlChar*) "pre1") == 0)
                    getRectangle(state->children, slI.pre);
                else if (xmlStrcmp(state->name, (xmlChar*) "exp1") == 0)
                    getRectangle(state->children, slI.exp);

                else if (xmlStrcmp(state->name, (xmlChar*) "dis1") == 0)
                    getRectangle(state->children, slI.dis);

                else if (xmlStrcmp(state->name, (xmlChar*) "foc1") == 0)
                    getRectangle(state->children, slI.foc);

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "Frame") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def9") == 0)
                    get9Rectangle(state->children, fr.def);

                else if (xmlStrcmp(state->name, (xmlChar*) "dis9") == 0)
                    get9Rectangle(state->children, fr.dis);

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "Box") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def9") == 0)
                    get9Rectangle(state->children, box.def);

                else if (xmlStrcmp(state->name, (xmlChar*) "dis9") == 0)
                    get9Rectangle(state->children, box.dis);

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "LineInput") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def9") == 0)
                    get9Rectangle(state->children, li.def);

                else if (xmlStrcmp(state->name, (xmlChar*) "pre9") == 0)
                    get9Rectangle(state->children, li.pre);

                else if (xmlStrcmp(state->name, (xmlChar*) "exp9") == 0)
                    get9Rectangle(state->children, li.exp);

                else if (xmlStrcmp(state->name, (xmlChar*) "dis9") == 0)
                    get9Rectangle(state->children, li.dis);

                else if (xmlStrcmp(state->name, (xmlChar*) "foc9") == 0)
                    get9Rectangle(state->children, li.foc);

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "ToolButton") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def9") == 0)
                    get9Rectangle(state->children, tb.def);

                else if (xmlStrcmp(state->name, (xmlChar*) "pre9") == 0)
                    get9Rectangle(state->children, tb.pre);

                else if (xmlStrcmp(state->name, (xmlChar*) "exp9") == 0)
                    get9Rectangle(state->children, tb.exp);

                else if (xmlStrcmp(state->name, (xmlChar*) "dis9") == 0)
                    get9Rectangle(state->children, tb.dis);

                else if (xmlStrcmp(state->name, (xmlChar*) "foc9") == 0)
                    get9Rectangle(state->children, tb.foc);

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "HScrollBar") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def3") == 0)
                    get3Rectangle(state->children, hScr);

                state = state->next;
            }
        } else if (xmlStrcmp(node->name, (xmlChar*) "VScrollBar") == 0)
        {
            state = node->children;
            while (state != NULL)
            {
                ILOG_DEBUG(ILX_STYLE, "  state: %s...\n", state->name);

                if (xmlStrcmp(state->name, (xmlChar*) "def3") == 0)
                    get3Rectangle(state->children, vScr);

                state = state->next;
            }
        }

        node = node->next;
    } // end while(element)
}

void
Style::getRectangle(xmlNodePtr node, Rectangle& r)
{
    xmlChar* x = xmlGetProp(node, (xmlChar*) "x");
    xmlChar* y = xmlGetProp(node, (xmlChar*) "y");
    xmlChar* w = xmlGetProp(node, (xmlChar*) "w");
    xmlChar* h = xmlGetProp(node, (xmlChar*) "h");

    r.setRectangle(atoi((char*) x), atoi((char*) y), atoi((char*) w), atoi((char*) h));
    ILOG_DEBUG( ILX_STYLE, "   Rectangle(%d, %d, %d, %d)\n", r.x(), r.y(), r.width(), r.height());
    xmlFree(x);
    xmlFree(y);
    xmlFree(w);
    xmlFree(h);
}

void
Style::get3Rectangle(xmlNodePtr node, r3& r)
{
    getRectangle(node, r.l);
    getRectangle(node->next, r.m);
    getRectangle(node->next->next, r.r);
}

void
Style::get9Rectangle(xmlNodePtr node, r9& r)
{
    getRectangle(node, r.tl);
    getRectangle(node->next, r.tm);
    getRectangle(node->next->next, r.tr);
    getRectangle(node->next->next->next, r.l);
    getRectangle(node->next->next->next->next, r.m);
    getRectangle(node->next->next->next->next->next, r.r);
    getRectangle(node->next->next->next->next->next->next, r.bl);
    getRectangle(node->next->next->next->next->next->next->next, r.bm);
    getRectangle(node->next->next->next->next->next->next->next->next, r.br);
}

std::istream&
operator>>(std::istream& is, Style& obj)
{
    obj.release();
    std::string iconName;
    Point p;
    obj._buttonFont = new Font();
    obj._defaultFont = new Font();
    obj._inputFont = new Font();
    obj._titleFont = new Font();
    obj._iconPack = new Image();
    obj._pack = new Image();
    int mapSize = 0;
    is >> *obj._buttonFont;
    is.ignore(1);
    is >> *obj._defaultFont;
    is.ignore(1);
    is >> *obj._inputFont;
    is.ignore(1);
    is >> *obj._titleFont;
    is.ignore(1);
    is >> obj._defaultIconSize;
    is.ignore(1);
    is >> *obj._iconPack;
    is >> mapSize;
    for (int i = 0; i < mapSize; ++i)
    {
        is >> iconName;
        is.ignore(1);
        is >> p;
        obj._iconMap.insert(std::make_pair(iconName, p));
    }
    is.ignore(1);
    is >> *obj._pack;
    is >> obj.pb >> obj.pbOK >> obj.pbCAN;
    is >> obj.cb >> obj.cbC >> obj.cbT >> obj.rbOn >> obj.rbOff >> obj.slI;

    is >> obj.tb >> obj.li;
    is >> obj.pr >> obj.prI >> obj.hSl >> obj.vSl;
    is >> obj.fr >> obj.box;
    is >> obj.hScr >> obj.vScr;
    return is;
}

std::ostream&
operator<<(std::ostream& os, const Style& obj)
{
    os << *obj._buttonFont << std::endl;
    os << *obj._defaultFont << std::endl;
    os << *obj._inputFont << std::endl;
    os << *obj._titleFont << std::endl;
    os << obj._defaultIconSize << std::endl;
    os << *obj._iconPack << std::endl;
    os << obj._iconMap.size() << std::endl;
    for (Style::IconMap::const_iterator it = obj._iconMap.begin(); it != obj._iconMap.end(); ++it)
        os << it->first << "\t" << it->second << std::endl;
    os << *obj._pack << std::endl;
    os << obj.pb << obj.pbOK << obj.pbCAN;
    os << obj.cb << obj.cbC << obj.cbT << obj.rbOn << obj.rbOff << obj.slI;

    os << obj.tb << obj.li;
    os << obj.pr << obj.prI << obj.hSl << obj.vSl;
    os << obj.fr << obj.box;
    os << obj.hScr << obj.vScr;
    return os;
}

std::istream&
operator>>(std::istream& is, Style::r3& obj)
{
    is >> obj.l;
    is.ignore(1);
    is >> obj.m;
    is.ignore(1);
    is >> obj.r;
    is.ignore(1);
    return is;
}

std::ostream&
operator<<(std::ostream& os, const Style::r3& obj)
{
    return os << obj.l << std::endl << obj.m << std::endl << obj.r << std::endl;
}

std::istream&
operator>>(std::istream& is, Style::r3_Input& obj)
{
    is >> obj.def >> obj.pre >> obj.exp >> obj.dis >> obj.foc;
    return is;
}

std::ostream&
operator<<(std::ostream& os, const Style::r3_Input& obj)
{
    return os << obj.def << obj.pre << obj.exp << obj.dis << obj.foc;
}

std::istream&
operator>>(std::istream& is, Style::r3_View& obj)
{
    is >> obj.def >> obj.dis;
    return is;
}

std::ostream&
operator<<(std::ostream& os, const Style::r3_View& obj)
{
    return os << obj.def << obj.dis;
}

std::istream&
operator>>(std::istream& is, Style::r9& obj)
{
    is >> obj.tl >> obj.tm >> obj.tr >> obj.l >> obj.m >> obj.r >> obj.bl >> obj.bm >> obj.br;
    return is;
}

std::ostream&
operator<<(std::ostream& os, const Style::r9& obj)
{
    return os << obj.tl << std::endl << obj.tm << std::endl << obj.tr << std::endl << obj.l << std::endl << obj.m << std::endl << obj.r << std::endl << obj.bl << std::endl << obj.bm << std::endl << obj.br << std::endl;
}

std::istream&
operator>>(std::istream& is, Style::r9_Input& obj)
{
    is >> obj.def >> obj.pre >> obj.exp >> obj.dis >> obj.foc;
    return is;
}

std::ostream&
operator<<(std::ostream& os, const Style::r9_Input& obj)
{
    return os << obj.def << obj.pre << obj.exp << obj.dis << obj.foc;
}

std::istream&
operator>>(std::istream& is, Style::r9_View& obj)
{
    is >> obj.def >> obj.dis;
    return is;
}

std::ostream&
operator<<(std::ostream& os, const Style::r9_View& obj)
{
    return os << obj.def << obj.dis;
}

std::istream&
operator>>(std::istream& is, Style::r1_Input& obj)
{
    is >> obj.def >> obj.pre >> obj.exp >> obj.dis >> obj.foc;
    return is;
}

std::ostream&
operator<<(std::ostream& os, const Style::r1_Input& obj)
{
    return os << obj.def << std::endl << obj.pre << std::endl << obj.exp << std::endl << obj.dis << std::endl << obj.foc << std::endl;
}

} /* namespace ilixi */
