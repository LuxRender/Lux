#ifndef MEDIANCUT_H
#define MEDIANCUT_H

#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <math.h>
#include "error.h"
#include "summedareatable.h"

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))
#define sq(s)    (s*s)
#define ABS(s)   ((s)>(0)?(s):(-s))

#define PI       3.141592653589793238462643

using namespace std;

class MC;

typedef vector< MC >     MedCutLight;

//----------------------------------- Box2D class ----------------------------------------//
class Box2D
{
    public:

        Box2D( int x_0 = 0, int y_0 = 0, int x_1 = 0, int y_1 = 0, int u_ = 0, int v_ = 0,  float Avsum_ = 0.f, int coord_ = 0 ) {
            x0 = x_0;
            y0 = y_0;
            x1 = x_1;
            y1 = y_1;
            u = u_;
            v = v_;

            Avsum = Avsum_;
            coord = coord_;
        }
	~Box2D() {}
        void setcoord() { ( x1 - x0 ) > ( y1 - y0 ) ? coord = 1 : coord = 0; }
        float area() { return ( x1 - x0 ) * ( y1 - y0 ); }

        int   x0, y0, x1, y1;
        int   u, v;
        float Avsum;
        int   coord;
};

//---------------------------------- Median Cut class ---------------------------------------//

class MC
{
    public:

        MC( float u_ = 0.f, float v_ = 0.f,float x_ = 0.f, float y_ = 0.f, float z_ = 0.f, float lum_ = 0.f, float r_ = 0.f ) {
            u = u_;
            v = v_;
            x = x_;
            y = y_;
            z = z_;
	    r = r_;
            lum = lum_;
        }
        ~MC() {}

        float u, v;
        float x, y, z;
        float lum, r;

};


//Class Box2D used by Mediancut 

bool split ( int pos, Box2D *MedCut,  Box2D r, SummedAreaTable &AL);

bool center ( Box2D  *MedCut, int k, SummedAreaTable &AL);

int MedCutSample( MedCutLight *MCLight, float *data, float *depth, int k, int w, int h);


#endif // MEDIANCUT_H
