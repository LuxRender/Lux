/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
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

#ifndef LUX_RENDERER_H
#define LUX_RENDERER_H

#include <vector>
#include "queryable.h"
#include "lux.h"
#include "rendererstatistics.h"

namespace lux
{

/*! \brief The interface for accessing to a device information.
 */
class RendererDeviceDescription {
public:
	virtual ~RendererDeviceDescription() { }

	/*! \brief return the name of the device.
	*/
	virtual const string &GetName() const = 0;

	virtual unsigned int GetAvailableUnitsCount() const = 0;
	virtual unsigned int GetUsedUnitsCount() const = 0;
	virtual void SetUsedUnitsCount(const unsigned int units) = 0;
};

/*! \brief The interface for accessing to an host information.
 */
class RendererHostDescription {
public:
	virtual ~RendererHostDescription() { }

	/*! \brief return the name of the host.
	 */
	virtual const string &GetName() const = 0;

	virtual vector<RendererDeviceDescription *> &GetDeviceDescs() = 0;
};

/*! \brief The interface for rendering a scene.
 */
class Renderer : public Queryable {
public:
	/*! Valid states of a Renderer.
	 */
	typedef enum {
		INIT, RUN, PAUSE, TERMINATE
	} RendererState;

	/*! The type of known renderers.
	 */
	typedef enum {
		SAMPLER_TYPE, HYBRIDSAMPLER_TYPE, SPPM_TYPE, SLG_TYPE
	} RendererType;

	Renderer() : Queryable("renderer") { }
    virtual ~Renderer() { }

	/*! \brief Return the type of the Renderer.
	 *
	 * Must be thread-safe. Can be called in any state.
	 */
	virtual RendererType GetType() const = 0;

	/*! \brief Return the current state of the Renderer.
	 *
	 * Must be thread-safe. Can be called in any state.
	 */
	virtual RendererState GetState() const = 0;

	/*! \brief Return the list of available renderer hosts.
	 *
	 * Must be thread-safe. Can be called in any state.
	 */
	virtual vector<RendererHostDescription *> &GetHostDescs() = 0;

	/*! \brief Tell the Renderer what to when an halt condition has been met.
	 *
	 * Must be thread-safe. Can be called in any state.
	 */
	virtual void SuspendWhenDone(bool v) = 0;

	/*! \brief Starts the rendering of a Scene.
	 *
	 * Must be thread-safe. Change the state from INIT to RUN. Can be called only
	 * if in INIT or TERMINATE state. This method is synchronous: it waits the
	 * end of the rendering.
	 *
	 * \param scene is the pointer to the scene to render
	 */
    virtual void Render(Scene *scene) = 0;

	/*! \brief Wait for theend of the rendering of a Scene.
	 *
	 * Must be thread-safe. Can be called only if in RUN or PAUSE state.
	 *
	 * \param scene is the pointer to the scene to render
	 */
    //virtual void WaitRender() const = 0;

	/*! \brief Pause the rendering of the scene.
	 *
	 * Must be thread-safe. Can be called if in one of the following
	 * states: PAUSE, RUN. Change the state to PAUSE.
	 */
	virtual void Pause() = 0;

	/*! \brief Resume the rendering of the scene.
	 *
	 * Must be thread-safe. Can be called if in one of the following
	 * states: PAUSE, RUN. Change the state to RUN.
	 */
	virtual void Resume() = 0;

	/*! \brief Terminate the rendering of the scene.
	 *
	 * Must be thread-safe. Can be called if in one of the following
	 * states: PAUSE, RUN, TERMINATE. Change the state to TERMINATE.
	 */
	virtual void Terminate() = 0;

	RendererStatistics* rendererStatistics;
};

}//namespace lux

#endif //LUX_RENDERER_H
