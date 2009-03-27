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
	inline wxViewerSelection(void)  { Clear(); }
	wxViewerSelection(int x1, int x2, int y1, int y2);
	inline void Clear(void)  { m_x1 = m_x2 = m_y1 = m_y2 = 0; }
	void GetBounds(int &x1, int &x2, int &y1, int &y2) const;
	void SetBounds(int x1, int x2, int y1, int y2);
	inline void GetCorner1(int &x, int &y) const  { x = m_x1;  y = m_y1; }
	inline void GetCorner2(int &x, int &y) const  { x = m_x2;  y = m_y2; }
	inline void SetCorner1(int x, int y)  { m_x1 = x;  m_y1 = y; }
	inline void SetCorner2(int x, int y)  { m_x2 = x;  m_y2 = y; }
	inline bool HasSize(void) const  { return (m_x1 != m_x2) && (m_y1 != m_y2); }

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
{	//view control modes:
	STATIC,
	PANZOOM,
	SELECTION,
	//view display modes:
	EMPTY_VIEW,
	LOGO_VIEW,
	RENDER_VIEW
};

// wxViewerBase Declarations
class wxViewerBase {
public:
	wxViewerBase();
	virtual ~wxViewerBase();

	virtual wxWindow* GetWindow() = 0;
	virtual wxViewerSelection GetSelection();

	virtual void SetMode(wxViewerMode mode);
	virtual void SetRulersEnabled(bool enabled = true);
	virtual void SetLogoData(const unsigned char *data, unsigned int length);
	virtual void SetZoom(const wxViewerSelection *selection);
	virtual void SetSelection(const wxViewerSelection *selection);
	virtual void SetHighlight(const wxViewerSelection *selection);

	virtual void Reload();
	virtual void Reset();

	virtual void SetTmExposure(float v) {};
	virtual void SetTmYwa(float v) {};
	virtual void SetTmPreScale(float v) {};
	virtual void SetTmPostScale(float v) {};
	virtual void SetTmBurn(float v) {};
	virtual void SetTmGamma(float v) {};
};

}//namespace lux

#endif // LUX_WXLUXGUI_H
