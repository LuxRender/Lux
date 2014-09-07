/* This small program converts the ArHosekData.h for use in LuxRender:
 *  - add more structure to the data array
 *  - swap the interleaving of coefficients and their parameters
 *  - add radiance data into the array instead of a separate array
 */
#include <stdlib.h>
#include <stdio.h>
#include "ArHosekSkyModelData_Spectral.h"
#include "ArHosekSkyModelData_RGB.h"
int main(int argc, char **argv)
{
	float wavelengths[] = {320, 360, 400, 440, 480, 520, 560, 600, 640, 680, 720};
	printf("const float datasets[11][2][10][10][6] = {\n");
	for (int wl = 0; wl < 11; ++wl) {
		printf("{ // %fnm\n", wavelengths[wl]);
		for (int a = 0; a < 2; ++a) {
			printf("{ // albedo %d\n", a);
			for (int t = 0; t < 10; ++t) {
				printf("{ // turbidity %d\n", t + 1);
				for (int c = 0; c < 9; ++c) {
					printf("{ // coefficient %c\n", 'A' + c);
					for (int p = 0; p < 6; ++p) {
						if (p == 0)
							printf("%.7gf", datasets[wl][((10*a+t)*6+p)*9+c]);
						else
							printf(", %.7gf", datasets[wl][((10*a+t)*6+p)*9+c]);
					}
					printf("\n},\n");
				}
				printf("{ // radiance\n");
				for (int p = 0; p < 6; ++p) {
					if (p == 0)
						printf("%.7gf", datasetsRad[wl][(10*a+t)*6+p]);
					else
						printf(", %.7gf", datasetsRad[wl][(10*a+t)*6+p]);
				}
				printf("\n}\n");
				if (t < 9)
					printf("},\n");
				else
					printf("}\n");
			}
			if (a < 1)
				printf("},\n");
			else
				printf("}\n");
		}
		if (wl < 10)
			printf("},\n");
		else
			printf("}\n");
	}
	printf("};\n\n");
	printf("const float datasetsRGB[3][2][10][10][6] = {\n");
	for (int wl = 0; wl < 3; ++wl) {
		printf("{ // %s\n", wl == 0 ? "Red" : wl == 1 ? "Green" : "Blue");
		for (int a = 0; a < 2; ++a) {
			printf("{ // albedo %d\n", a);
			for (int t = 0; t < 10; ++t) {
				printf("{ // turbidity %d\n", t + 1);
				for (int c = 0; c < 9; ++c) {
					printf("{ // coefficient %c\n", 'A' + c);
					for (int p = 0; p < 6; ++p) {
						if (p == 0)
							printf("%.7gf", datasetsRGB[wl][((10*a+t)*6+p)*9+c]);
						else
							printf(", %.7gf", datasetsRGB[wl][((10*a+t)*6+p)*9+c]);
					}
					printf("\n},\n");
				}
				printf("{ // radiance\n");
				for (int p = 0; p < 6; ++p) {
					if (p == 0)
						printf("%.7gf", datasetsRGBRad[wl][(10*a+t)*6+p]);
					else
						printf(", %.7gf", datasetsRGBRad[wl][(10*a+t)*6+p]);
				}
				printf("\n}\n");
				if (t < 9)
					printf("},\n");
				else
					printf("}\n");
			}
			if (a < 1)
				printf("},\n");
			else
				printf("}\n");
		}
		if (wl < 2)
			printf("},\n");
		else
			printf("}\n");
	}
	printf("};\n\n");
	return 0;
}

