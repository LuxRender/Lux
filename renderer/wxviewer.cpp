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

#include "wxviewer.h"

using namespace lux;


// wxViewerSelection Definitions
wxViewerSelection::wxViewerSelection(int x1, int x2, int y1, int y2)
	: m_x1(x1), m_x2(x2), m_y1(y1), m_y2(y2) {}

void wxViewerSelection::GetBounds(int &x1, int &x2, int &y1, int &y2) const {
	x1 = m_x1;
	x2 = m_x2;
	y1 = m_y1;
	y2 = m_y2;
}

void wxViewerSelection::SetBounds(int x1, int x2, int y1, int y2) {
	m_x1 = x1;
	m_x2 = x2;
	m_y1 = y1;
	m_y2 = y2;
}


// wxViewerEvent Definitions
DEFINE_EVENT_TYPE(lux::wxEVT_LUX_VIEWER_SELECTION)

wxViewerEvent::wxViewerEvent(const boost::shared_ptr<wxViewerSelection> selection, wxEventType eventType, int id): wxEvent(id, eventType), m_selection(selection) { m_propagationLevel = wxEVENT_PROPAGATE_MAX; }

boost::shared_ptr<wxViewerSelection> wxViewerEvent::GetSelection() { return m_selection; }

wxEvent* wxViewerEvent::Clone() const { return new wxViewerEvent(*this); }


// wxViewerBase Definitions
wxViewerBase::wxViewerBase():m_viewerMode(STATIC) {}
wxViewerBase::~wxViewerBase() {}

wxViewerSelection wxViewerBase::GetSelection() { return wxViewerSelection(); }

void wxViewerBase::SetMode(wxViewerMode mode) { m_viewerMode = mode; }
void wxViewerBase::SetRulersEnabled(bool enabled) {}
void wxViewerBase::SetZoom(const wxViewerSelection *selection) {}
void wxViewerBase::SetSelection(const wxViewerSelection *selection) {}
void wxViewerBase::SetHighlight(const wxViewerSelection *selection) {}

void wxViewerBase::Reload() {}
void wxViewerBase::Reset() {}
