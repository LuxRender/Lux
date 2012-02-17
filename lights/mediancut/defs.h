#ifndef DEFS_H
#define DEFS_H

#include <vector>
#include <math.h>
#include <stdio.h>
#include "summedareatable.h"

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))
#define sq(s)    (s*s)
#define ABS(s)   ((s)>(0)?(s):(-s)) 

#define PI       3.141592653589793238462643  

using namespace std;

//----------------------------------- Rectangle class ----------------------------------------//

class Rectangle
{
	public:
		int   x0, y0, x1, y1;
		int   u, v;
		float Avsum;
		int   coord;

		Rectangle() {
			x0 = y0 = x1 = y1 = u = v = coord = 0; 
			Avsum = 0.f; 
		}
		Rectangle( int x_0, int y_0, int x_1, int y_1, int u_, int v_,  float Avsum_, int coord_ ) {
			x0 = x_0; 
			y0 = y_0; 
			x1 = x_1; 
			y1 = y_1; 
			u = u_; 
			v = v_; 

			Avsum = Avsum_;
			coord = coord_; 
		}
		~Rectangle() {}
		void setcoord() { ( x1 - x0 ) > ( y1 - y0 ) ? coord = 1 : coord = 0; };
};

//---------------------------------- Median Cut class ---------------------------------------//

class MC
{
	public:
		float u, v;
		float x, y, z;
		float lum;

		MC() { u = v = x = y = z = lum = 0.f; }
		MC( float u_, float v_,float x_, float y_, float z_, float lum_ ) {
			u = u_; 
			v = v_; 
			x = x_; 
			y = y_; 
			z = z_; 
			lum = lum_; 
		}
		~MC() {};
};

typedef vector<Rectangle>  MedCutList;
typedef vector<MC>         MedCutLight;


//Median Cut Data 

extern SummedAreaTable 	AL;
extern MedCutList  	C_MedCut;
extern MedCutLight 	C_MCLight;


#endif // DEFS_H


