#ifndef MEDIANCUT_H
#define MEDIANCUT_H

#include <stdlib.h>
#include <stdio.h>
#include "defs.h"


//Class Box2D used by Mediancut 

bool split (MedCutList  *MedCut,  Box2D r);

bool center (MedCutList  *MedCut, int k);

void MedCutSample(MedCutList  *MedCut, MedCutLight *MCLight, 
	float *data, int k, int w, int h);

#endif // MEDIANCUT_H
