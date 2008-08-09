/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
 *                                                                         *
 *   Lux Renderer is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Lux Renderer is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   This project is based on PBRT ; see http://www.pbrt.org               *
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

#ifndef LUX_WXVIEWER_H
#define LUX_WXVIEWER_H

#include <boost/shared_ptr.hpp>
#include <wx/event.h>
#include <wx/window.h>

namespace lux
{

// wxViewerSelection Declarations - Used for zoom and other tools
class wxViewerSelection {
public:
	wxViewerSelection();
	wxViewerSelection(int x1, int x2, int y1, int y2);
	void GetBounds(int &x1, int &x2, int &y1, int &y2);

protected:
	int m_x1, m_x2, m_y1, m_y2;
};

// wxViewerEvent Declarations
class wxViewerEvent : public wxEvent {
public:
    wxViewerEvent(const boost::shared_ptr<wxViewerSelection> selection, wxEventType eventType = wxEVT_NULL, int id = 0);

		boost::shared_ptr<wxViewerSelection> GetSelection();

    // required for sending with wxPostEvent()
    wxEvent* Clone() const;

protected:
    boost::shared_ptr<wxViewerSelection> m_selection;
};

DECLARE_EVENT_TYPE(wxEVT_LUX_VIEWER_SELECTION, -1)

typedef void (wxEvtHandler::*wxViewerEventFunction)(wxViewerEvent&);

#define EVT_LUX_VIEWER_SELECTION(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( wxEVT_LUX_VIEWER_SELECTION, id, -1, \
    (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) (wxNotifyEventFunction) \
    wxStaticCastEvent( wxViewerEventFunction, & fn ), (wxObject *) NULL ),

// wxViewerMode Declarations
enum wxViewerMode
{
	STATIC,
	PANZOOM,
	SELECTION
};

// wxViewerBase Declarations
class wxViewerBase {
public:
	wxViewerBase();
	virtual ~wxViewerBase();

	virtual wxWindow* GetWindow() = 0;
	virtual wxViewerSelection GetSelection();

	virtual void SetMode(wxViewerMode mode);
	virtual void SetZoom(wxViewerSelection *selection);
	virtual void SetSelection(wxViewerSelection *selection);
	virtual void SetHighlight(wxViewerSelection *selection);

	virtual void Reload();
	virtual void Reset();

protected:
	wxViewerMode m_viewerMode;
};

}//namespace lux

#endif // LUX_WXLUXGUI_H
