/**
 * @file    summedareatable.cpp
 * @author  Aldo Zang <aldo.zang@gmail.com>
 * @author  VISGRAF LAB, IMPA
 * @version 1.5.0
 * @date    01/06/2010
 *
 * @brief   HSM Graphical interface: interface controls
 */
//________________________________________________

#include "summedareatable.h"

//--------------------------------------------------------------------------//

SummedAreaTable::SummedAreaTable( int w , int h ) {
	
	if ( w <= 0 || h <= 0 ) {

		width = 0; 
		height = 0; 
		A = 0;
		return;
	}
	width = w;
	height = h;
	A = new float[w * h];
}
//-------------------------------------------------------------------------//

bool SummedAreaTable::setSize( int w , int h ) {

	if ( w <= 0 || h <= 0 ) return false;
	if ( ( width == w ) && ( height == h ) ) return true;
	width = w;
	height = h;

	if ( A ) delete[] A;
		A = new float[width * height];
	return true;
}
//-------------------------------------------------------------------------//

void SummedAreaTable::clear( float c ) {

	for ( int i = 0 ; i < width * height ; i++ ) 

		A[i] = c;
}
//-------------------------------------------------------------------------//

void SummedAreaTable::doSum( void ) {

	int i, j;
	for ( i = 1 ; i < width ; i++ ) 
		pixel( i, 0 ) = pixel( i, 0 ) + pixel( i - 1, 0 );
	
	for ( j = 1 ; j < height ; j++ ) {

		pixel( 0, j ) = pixel( 0, j ) + pixel( 0, j - 1 );
		for ( i = 1 ; i < width ; i++ ) 
			pixel( i, j ) = pixel( i, j ) + pixel( i - 1, j ) + pixel( i, j - 1 ) - pixel( i - 1, j - 1 );
	}
}

//-------------------------------------------------------------------------//

float SummedAreaTable::sumRectangle( int x0, int y0, int x1, int y1 ) {

	if ( x0 > 0 ) {

		if ( y0 > 0 )
			return ( A[ width*(y0-1)+(x0-1) ] + A[ width*y1+x1 ] ) - ( A[ width*(y0-1)+x1 ] + A[ width*y1+(x0-1) ] );
		else 
			return ( A[ width*y1+x1 ] - A[ width*y1+(x0-1) ] );
	}
	else {

		if ( y0 > 0 )
			return ( A[ width*y1+x1 ] - A[ width*(y0-1)+x1 ] );
		else 
			return ( A[ width*y1+x1 ] );
	}
}


float SummedAreaTable::MeanValue( int x0, int y0, int x1, int y1 ) {

    if ( x0 > 0 ) {

	if ( y0 > 0 )
	    return ( ( A[ width*(y0-1)+(x0-1) ] + A[ width*y1+x1 ] ) - ( A[ width*(y0-1)+x1 ] + A[ width*y1+(x0-1) ] ) ) / ( (x1-x0+1)*(y1-y0+1) ) ;
	else
	    return ( A[ width*y1+x1 ] - A[ width*y1+(x0-1) ] ) / ( (x1-x0+1)*(y1-y0+1) ) ;
    }
    else {

	if ( y0 > 0 )
	    return ( A[ width*y1+x1 ] - A[ width*(y0-1)+x1 ] ) / ( (x1-x0+1)*(y1-y0+1) ) ;
	else
	    return ( A[ width*y1+x1 ] ) / ( (x1-x0+1)*(y1-y0+1) ) ;
    }
}
//-------------------------------------------------------------------------//

float SummedAreaTable::getMaxValue( void ) {

	float maxVal = 0.0;
	for ( int i = 0 ; i < width * height ; i++ ) 
		if ( A[i] > maxVal ) 
			maxVal = A[i];
	
	return maxVal;
}
//-------------------------------------------------------------------------//

bool SummedAreaTable::writeToPGM( char *fName, char *headerInfo ) {

	float maxVal = sqrt( getMaxValue() );
	int    i;
	size_t s;
	FILE  *F;
	int    b = 0;

	F = fopen( fName, "wb" );
	if ( !F ) return false;
	s = 1;
	fprintf( F, "P5\n" );
	if (headerInfo != NULL) 
		fprintf( F, "# %s\n", headerInfo );
	else 
		fprintf( F, "# Grayscale Image\n" );

	fprintf(F, "%d %d\n", width, height);

	for ( i = 0 ; i < width * height ; i++ ) {

		b = (int)( 255 * sqrt(A[i]) / maxVal );
		fwrite( (void*)&b, s, 1, F ); 
	}			
	fclose( F );
	return true;
}
//--------------------------------------------------------------------------//

bool SummedAreaTable::write( void ) {

	FILE *F;
	F = fopen( "teste.txt" , "wb" );
	if ( F)	
		for ( int i = 0 ; i < width * height ; i++ ) 
			fprintf( F, "%f\n", A[i] );
		
	fclose(F);  
	return true;
}
