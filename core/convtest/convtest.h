/***************************************************************************
 * Copyright 1998-2017 by authors (see AUTHORS.txt)                        *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
 *                                                                         *
 * Licensed under the Apache License, Version 2.0 (the "License");         *
 * you may not use this file except in compliance with the License.        *
 * You may obtain a copy of the License at                                 *
 *                                                                         *
 *     http://www.apache.org/licenses/LICENSE-2.0                          *
 *                                                                         *
 * Unless required by applicable law or agreed to in writing, software     *
 * distributed under the License is distributed on an "AS IS" BASIS,       *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
 * See the License for the specific language governing permissions and     *
 * limitations under the License.                                          *
 ***************************************************************************/

#ifndef LUX_CONVTEST_H
#define LUX_CONVTEST_H

#include <vector>

#include "luxrays/luxrays.h"
#include "core/convtest/pdiff/metric.h"

namespace lux {

class ConvergenceTest {
public:
	ConvergenceTest(const u_int w, const u_int h);
	virtual ~ConvergenceTest();

	void NeedTVI();
	const float *GetTVI() const { return &tvi[0]; }
	
	void Reset();
	void Reset(const u_int w, const u_int h);
	u_int Test(const float *image);

private:
	u_int width, height;
	
	std::vector<float> reference;
	std::vector<float> tvi;
};

}

#endif
