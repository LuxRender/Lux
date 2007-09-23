/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// lux.cpp*
#include "lux.h"
#include "api.h"
// main program
int main(int argc, char *argv[]) {
	// Print welcome banner
	printf("Lux Renderer version %1.3f of %s at %s\n", LUX_VERSION, __DATE__, __TIME__);     
	printf("This program comes with ABSOLUTELY NO WARRANTY\nThis is free software, covered by the GNU General Public License V3\nYou are welcome to redistribute it under certain conditions;\n see COPYRIGHT.TXT for details.\n");      

	fflush(stdout);
	luxInit();
	// Process scene description
	if (argc == 1) {
		// Parse scene from standard input
		ParseFile("-");
	} else {
		// Parse scene from input files
		for (int i = 1; i < argc; i++)
			if (!ParseFile(argv[i]))
				Error("Couldn't open scene file \"%s\"\n", argv[i]);
	}
	luxCleanup();
	return 0;
}
