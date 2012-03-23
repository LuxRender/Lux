#include "mediancut.h"
using namespace std;

SummedAreaTable AL = SummedAreaTable();

MedCutList  C_MedCut;
MedCutLight C_MCLight;

bool split ( MedCutList  *MedCut, Box2D r ) {

	Box2D a, b;

	if ( ( r.x0 == r.x1 ) && ( r.y0 == r.y1 ) ) {

		a.x0 = a.x1 = r.x0; 
		a.y0 = a.y1 = r.y0;
		b.x0 = b.x1 = b.y0 = b.y1 = -1;
		a.Avsum = r.Avsum;
		b.Avsum = 0.f;
		(*MedCut).push_back(a); 
		(*MedCut).push_back(b); 
		return 1;
	}

	a.x0 = r.x0; 
	a.y0 = r.y0; 
	b.x1 = r.x1; 
	b.y1 = r.y1; 
	a.Avsum = b.Avsum = r.Avsum * 0.5f;

	if ( r.coord == 1 ) {
      
		a.y1 = r.y1; 
		b.y0 = r.y0;

		for ( int i = r.x0 ; i <= r.x1 ; i++ ) { 

			float areax = AL.sumRectangle(r.x0, r.y0, i, r.y1);

			if ( areax >= a.Avsum ) {

				if ( i > r.x0 ) {

					if( ( areax - a.Avsum ) > ( a.Avsum - AL.sumRectangle( r.x0, r.y0, i, r.y1 ) ) ) {
						
						a.x1 = i-1; 
						b.x0 = i; 
						a.Avsum = AL.sumRectangle(r.x0, r.y0, i-1, r.y1);
						b.Avsum = r.Avsum - a.Avsum;
					} 
					else {	

						a.x1 = i; 
						b.x0 = i+1;  
						a.Avsum = AL.sumRectangle(r.x0, r.y0, i, r.y1);
						b.Avsum = r.Avsum - a.Avsum;
					}
				}
				else {
					
					a.x1 = i; 
					b.x0 = i+1; 
					a.Avsum = AL.sumRectangle(r.x0, r.y0, i, r.y1);
					b.Avsum = r.Avsum - a.Avsum;
				} 
				a.setcoord(); 
				b.setcoord();
				(*MedCut).push_back(a); 
				(*MedCut).push_back(b); 
				return 1;
			}
		}
	}
	else { 

		a.x1 = r.x1; 
		b.x0 = r.x0;

		for ( int j = r.y0 ; j <= r.y1 ; j++ ) {	

			float areay = AL.sumRectangle( r.x0, r.y0, r.x1, j);

			if( areay >= a.Avsum ) {

				if(j > r.y0) {

					if( ( areay - a.Avsum ) > ( a.Avsum - AL.sumRectangle( r.x0, r.y0, r.x1, j ) ) ) {

						a.y1 = j-1; 
						b.y0 = j; 
						a.Avsum = AL.sumRectangle(r.x0, r.y0, r.x1, j-1); 
						b.Avsum = r.Avsum - a.Avsum;
					} 
					else {	

						a.y1 = j; 
						b.y0 = j+1; 
						a.Avsum = AL.sumRectangle(r.x0, r.y0, r.x1, j); 
						b.Avsum = r.Avsum - a.Avsum;
					}
				}
				else {

					a.y1 = j; 
					b.y0 = j+1; 
					a.Avsum = AL.sumRectangle(r.x0, r.y0, r.x1, j); 
					b.Avsum = r.Avsum - a.Avsum;
				} 
				a.setcoord(); b.setcoord();
				(*MedCut).push_back(a); (*MedCut).push_back(b); 
				return 1;
			}
		}
		
	}
	return 0;
}


bool center ( MedCutList *MedCut, int k ) {

	(*MedCut)[k].Avsum = AL.sumRectangle( (*MedCut)[k].x0, (*MedCut)[k].y0, (*MedCut)[k].x1, (*MedCut)[k].y1 ) * 0.5f;

	for ( int i = (*MedCut)[k].x0 ; i <= (*MedCut)[k].x1 ; i++ ) {	

		float area = AL.sumRectangle((*MedCut)[k].x0,(*MedCut)[k].y0, i ,(*MedCut)[k].y1);

		if ( area >= (*MedCut)[k].Avsum ) {

			if ( i > (*MedCut)[k].x0 ) {

				if ( ( area - (*MedCut)[k].Avsum ) > ( (*MedCut)[k].Avsum - AL.sumRectangle( (*MedCut)[k].x0, (*MedCut)[k].y0, i, (*MedCut)[k].y1 ) ) )
					
					(*MedCut)[k].u = i-1;  
					 
				else 	
					
					(*MedCut)[k].u = i;
					
			}
			else
				
				(*MedCut)[k].u = i;  
				 
			break;
		}
	}

	for ( int i = (*MedCut)[k].y0 ; i <= (*MedCut)[k].y1 ; i++ ) {	

		float area = AL.sumRectangle( (*MedCut)[k].x0, (*MedCut)[k].y0, (*MedCut)[k].x1, i );

		if (area >= (*MedCut)[k].Avsum ) {

			if ( i > (*MedCut)[k].y0 ) {
				
				if ( ( area - (*MedCut)[k].Avsum ) > ( (*MedCut)[k].Avsum - AL.sumRectangle( (*MedCut)[k].x0, (*MedCut)[k].y0, (*MedCut)[k].x1, i ) ) )
					
					(*MedCut)[k].v = i-1;  
					 
				else 	
					
					(*MedCut)[k].v = i;
					
			}
			else 

				(*MedCut)[k].v = i; 

			break;
		}
	}


	return 1;
}


void MedCutSample( MedCutList *MedCut, MedCutLight *MCLight, float *data, int k, int w, int h)
{

	AL.setSize(w,h);



	for ( int i = 0 ; i < w * h ; i++ ) 
		AL.A[i] = data[i]; 

	AL.doSum();


	(*MedCut).clear();
	Box2D rec = Box2D( 0, 0, w-1, h-1, 0, 0, AL.A[w*h-1], 1 );
	(*MedCut).push_back( rec );

	for ( int i = 0 ; i < k ; i++ )

		for( int j = (int)( pow( 2.f, i ) - 1 ) ; j < (int)( pow( 2.f, i + 1 ) - 1 ) ; j++ )
			
			split( MedCut, (*MedCut)[j] );

	//Find the center
	(*MCLight).clear();
	float x, y, z, theta, phi;

	for ( int j = (int)( pow( 2.f, k ) - 1 ) ; j < (int)( pow( 2.f, k + 1 ) - 1 ) ; j++ ) {

		if ( (*MedCut)[j].x0 > -1 ) {

			center(MedCut,j);
			theta = ( (float)( (*MedCut)[j].v) + 0.5f ) / (float)(h) * PI;
			phi   = ( (float)( (*MedCut)[j].u) + 0.5f ) / (float)(w) * 2.f * PI;
			x = sin( theta ) * cos( phi ); 
			y = sin( theta ) * sin( phi ); 
			z = cos( theta ); 

			MC mc = MC( ( (float)( (*MedCut)[j].u) + 0.5f ), ( (float)( (*MedCut)[j].v ) + 0.5f ), x, y, z, (*MedCut)[j].Avsum);
			(*MCLight).push_back( mc );
		}
	}

}


