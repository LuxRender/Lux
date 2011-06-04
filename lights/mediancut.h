#ifndef MEDIANCUT_H_
#define MEDIANCUT_H_

#include <stdlib.h>
#include <stdio.h>
#include "defs.h"


//class Rectangle used by Mediancut 

bool
split (MedCutList  *MedCut,  Rectangle r);

bool
center (MedCutList  *MedCut, int k);

void
MedCutSample(MedCutList  *MedCut, MedCutLight *MCLight, float *data, int k, int w, int h);

#endif
