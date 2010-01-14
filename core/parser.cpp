/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
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
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

// parser.cpp*
#include "lux.h"
#include "error.h"
// Parsing Global Interface
 bool ParseFile(const char *filename) {
	extern FILE *yyin;
	extern int yyparse(void);
	extern string currentFile;
	extern u_int lineNum;

	if (strcmp(filename, "-") == 0)
		yyin = stdin;
	else
		yyin = fopen(filename, "r");
	if (yyin != NULL) {
		currentFile = filename;
		if (yyin == stdin)
			currentFile = "<standard input>";
		lineNum = 1;
		yyparse();
		if (yyin != stdin)
			fclose(yyin);
	} else {
		std::stringstream ss;
		ss << "Unable to read scenefile '" << filename << "'";
		luxError(LUX_NOFILE, LUX_SEVERE, ss.str().c_str());
	}

	currentFile = "";
	lineNum = 0;
	return (yyin != NULL);
}
