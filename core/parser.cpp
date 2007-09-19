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

// parser.cpp*
#include "pbrt.h"
// Parsing Global Interface
COREDLL bool ParseFile(const char *filename) {
	extern FILE *yyin;
	extern int yyparse(void);
	extern string current_file;
	extern int line_num;
	extern int yydebug;

	if (getenv("PBRT_YYDEBUG") != NULL)
		yydebug = 1;

	if (strcmp(filename, "-") == 0)
		yyin = stdin;
	else
		yyin = fopen(filename, "r");
	if (yyin != NULL) {
		current_file = filename;
		if (yyin == stdin) current_file = "<standard input>";
		line_num = 1;
		yyparse();
		if (yyin != stdin) fclose(yyin);
	}
	current_file = "";
	line_num = 0;
	return (yyin != NULL);
}
