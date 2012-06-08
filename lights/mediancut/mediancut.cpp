#include "mediancut.h"
using namespace std;


bool split ( int pos, Box2D  *MedCut, Box2D r, SummedAreaTable &AL ) {

	Box2D *a = new Box2D;
	Box2D *b = new Box2D;

	if ( ( r.x0 == r.x1 ) && ( r.y0 == r.y1 ) ) {

		a->x0 = a->x1 = r.x0;
		a->y0 = a->y1 = r.y0;
		b->x0 = b->x1 = b->y0 = b->y1 = -1;
		a->Avsum = r.Avsum;
		b->Avsum = 0.f;
		MedCut[pos] = *a;
		MedCut[pos+1] = *b;

		delete a,b;
		return true;
	}

	a->x0 = r.x0;
	a->y0 = r.y0;
	b->x1 = r.x1;
	b->y1 = r.y1;
	a->Avsum = b->Avsum = r.Avsum * 0.5f;

	if ( r.coord == 1 ) {

		a->y1 = r.y1;
		b->y0 = r.y0;

		for ( int i = r.x0 ; i <= r.x1 ; i++ ) {

			float areax = AL.sumRectangle(r.x0, r.y0, i, r.y1);

			if ( areax >= a->Avsum ) {

				if ( i > r.x0 ) {

					if( ( areax - a->Avsum ) > ( a->Avsum - AL.sumRectangle( r.x0, r.y0, i, r.y1 ) ) ) {
						
						a->x1 = i-1;
						b->x0 = i;
						a->Avsum = AL.sumRectangle(r.x0, r.y0, i-1, r.y1);
						b->Avsum = r.Avsum - a->Avsum;
					}
					else {

						a->x1 = i;
						b->x0 = i+1;
						a->Avsum = AL.sumRectangle(r.x0, r.y0, i, r.y1);
						b->Avsum = r.Avsum - a->Avsum;
					}
				}
				else {
					
					a->x1 = i;
					b->x0 = i+1;
					a->Avsum = AL.sumRectangle(r.x0, r.y0, i, r.y1);
					b->Avsum = r.Avsum - a->Avsum;
				}
				a->setcoord();
				b->setcoord();
				MedCut[pos] = *a;
				MedCut[pos+1] = *b;

				delete a, b;
				return true;
			}
		}
	}
	else {

		a->x1 = r.x1;
		b->x0 = r.x0;

		for ( int j = r.y0 ; j <= r.y1 ; j++ ) {

			float areay = AL.sumRectangle( r.x0, r.y0, r.x1, j);

			if( areay >= a->Avsum ) {

				if(j > r.y0) {

					if( ( areay - a->Avsum ) > ( a->Avsum - AL.sumRectangle( r.x0, r.y0, r.x1, j ) ) ) {

						a->y1 = j-1;
						b->y0 = j;
						a->Avsum = AL.sumRectangle(r.x0, r.y0, r.x1, j-1);
						b->Avsum = r.Avsum - a->Avsum;
					}
					else {

						a->y1 = j;
						b->y0 = j+1;
						a->Avsum = AL.sumRectangle(r.x0, r.y0, r.x1, j);
						b->Avsum = r.Avsum - a->Avsum;
					}
				}
				else {

					a->y1 = j;
					b->y0 = j+1;
					a->Avsum = AL.sumRectangle(r.x0, r.y0, r.x1, j);
					b->Avsum = r.Avsum - a->Avsum;
				}
				a->setcoord(); b->setcoord();
				MedCut[pos] = *a;
				MedCut[pos+1] = *b;
				delete a,b;
				return true;
			}
		}
	}
	delete a,b;
	return false;
}


bool center ( Box2D *MedCut, int k, SummedAreaTable &AL ) {

	MedCut[k].Avsum = AL.sumRectangle( MedCut[k].x0, MedCut[k].y0, MedCut[k].x1, MedCut[k].y1 ) * 0.5f;

	for ( int i = MedCut[k].x0 ; i <= MedCut[k].x1 ; i++ ) {

		float area = AL.sumRectangle(MedCut[k].x0,MedCut[k].y0, i , MedCut[k].y1);

		if ( area >= MedCut[k].Avsum ) {

			if ( i > MedCut[k].x0 ) {

				if ( ( area - MedCut[k].Avsum ) > ( MedCut[k].Avsum - AL.sumRectangle( MedCut[k].x0, MedCut[k].y0, i, MedCut[k].y1 ) ) )
					
					MedCut[k].u = i-1;
				else
					MedCut[k].u = i;

			}
			else
				MedCut[k].u = i;
			break;
		}
	}

	for ( int i = MedCut[k].y0 ; i <= MedCut[k].y1 ; i++ ) {

		float area = AL.sumRectangle( MedCut[k].x0, MedCut[k].y0, MedCut[k].x1, i );

		if (area >= MedCut[k].Avsum ) {

			if ( i > MedCut[k].y0 ) {
				
				if ( ( area - MedCut[k].Avsum ) > ( MedCut[k].Avsum - AL.sumRectangle( MedCut[k].x0, MedCut[k].y0, MedCut[k].x1, i ) ) )
					
					MedCut[k].v = i-1;
				else
					MedCut[k].v = i;

			}
			else
				MedCut[k].v = i;
			break;
		}
	}

	return true;
}


int MedCutSample( MedCutLight *MCLight, float *data, float *depth, int k, int w, int h)
{
	float x, y, z, theta, phi, radius;
	bool flag;
	int counter, auxcont;
	SummedAreaTable *AL = new SummedAreaTable();
	SummedAreaTable *BL = new SummedAreaTable();
	if (depth != NULL) {
		BL->setSize(w,h);
		for ( int i = 0 ; i < w * h ; i++ )
			BL->A[i] = depth[i];

		BL->doSum();
	}

	int val =(int)(pow( 2.f, k )+1);

	Box2D *MedCut0 = new Box2D[val];
	Box2D *MedCut1 = new Box2D[val];

	AL->setSize(w,h);

	for ( int i = 0 ; i < w * h ; i++ )
		AL->A[i] = data[i];

	AL->doSum();

	float min_area = (float)(w*w)/32400.f; //(w/360)^2

	MedCut0[0]= Box2D( 0, 0, w-1, h-1, 0, 0, AL->A[w*h-1], 1 );
	counter = 1;

	for ( int i = 0 ; i < k ; i++ ) {

		auxcont = 0;
		for( int j = 0 ; j < counter ; j++ ) {

			if( MedCut0[j].area() > min_area ) {
				flag = split( auxcont, MedCut1, MedCut0[j], *AL );
				auxcont += 2;
			} else {
				MedCut1[auxcont] = MedCut0[j];
				auxcont++;
			}
		}

		counter = auxcont;

		for( int k = 0 ; k < auxcont ; k++ )
			MedCut0[k] = MedCut1[k];

	}

	radius = 1.f;
	//Find the center
	for ( int j = 0 ; j < counter ; j++ ) {

		if ( MedCut0[j].area() > 0.f ) {

			center( MedCut0, j, *AL );
			theta = ( (float)( MedCut0[j].v) + 0.5f ) / (float)(h) * PI;
			phi   = ( (float)( MedCut0[j].u) + 0.5f ) / (float)(w) * 2.f * PI;
			x = sin( theta ) * cos( phi );
			y = sin( theta ) * sin( phi );
			z = cos( theta );
			if(depth !=NULL)
				radius = BL->MeanValue( MedCut0[j].x0, MedCut0[j].y0, MedCut0[j].x1, MedCut0[j].y1 );
			MC mc = MC( ( (float)( MedCut0[j].u) + 0.5f ), ( (float)( MedCut0[j].v ) + 0.5f ), x, y, z, MedCut0[j].Avsum, radius);
			(*MCLight).push_back( mc );
		}
	}

	delete [] MedCut0;
	delete [] MedCut1;
	return counter;
}


