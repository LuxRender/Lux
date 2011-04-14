#!/usr/bin/env python3
# -*- coding: utf8 -*-
#
# ***** BEGIN GPL LICENSE BLOCK *****
#
# Authors:
# Doug Hammond
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#
# ***** END GPL LICENCE BLOCK *****
#

# Massive thanks to http://www.dabeaz.com/ply/

import ply.lex as lex, ply.yacc as yacc, sys, json

#------------------------------------------------------------------------------ 
# TOKENIZER
#------------------------------------------------------------------------------ 

tokens = (
	"NL",
	"WS",
	"COMMENT",
	"STRING",
	"NUM",
	"IDENTIFIER",
	"PARAM_ID",
	# "PARAM_VALUE",
	"LBRACK", "RBRACK"
)
t_ignore_NL			= r'\r+|\n+'
t_ignore_WS			= r'\s+'
t_ignore_COMMENT	= r'\#(.*)'
t_STRING			= r'"([^"]+)"'
t_NUM				= r'[+-]?\d+\.?(\d+)?'
v_TYPES				= r'bool|float|color|vector|normal|point|string|texture'
t_PARAM_ID			= r'["|\']('+v_TYPES+')[ ](\w+)["|\']'
#t_PARAM_VALUE		= r'\[(.*)\]'
t_LBRACK			= r'\['
t_RBRACK			= r'\]'

t_IDENTIFIER = (
	
	"MakeNamedVolume|Texture|MakeNamedMaterial"
	
	#"Accelerator|AreaLightSource|AttributeBegin|AttributeEnd|"
	#"Camera|ConcatTransform|CoordinateSystem|CoordSysTransform|Exterior|"
	#"Film|Identity|Interior|LightSource|LookAt|Material|MakeNamedMaterial|"
	#"MakeNamedVolume|NamedMaterial|ObjectBegin|ObjectEnd|ObjectInstance|PortalInstance|"
	#"MotionInstance|LightGroup|PixelFilter|Renderer|ReverseOrientation|Rotate|Sampler|"
	#"Scale|SearchPath|PortalShape|Shape|SurfaceIntegrator|Texture|"
	#"TransformBegin|TransformEnd|Transform|Translate|Volume|VolumeIntegrator|"
	#"WorldBegin|WorldEnd"
)

def t_error(t):
	raise TypeError("Unknown text '%s'" % (t.value,))

#------------------------------------------------------------------------------ 
# PARSER
#------------------------------------------------------------------------------ 

LAST_OBJECT_NAME = ''

class Texture(object):
	def __init__(self, name, variant, type):
		self.name = name
		global LAST_OBJECT_NAME
		LAST_OBJECT_NAME = name
		self.variant = variant
		self.type = type
		self.paramset = []
	def asValue(self):
		return {
			'type': 'Texture',
			'name': self.name,
			'extra_tokens': '"%s" "%s"'% (self.variant, self.type),
			'paramset': [psi.asValue() for psi in self.paramset]
		}
	def __repr__(self):
		return json.dumps(self.asValue())

class Volume(object):
	def __init__(self, name, type):
		self.name = name
		global LAST_OBJECT_NAME
		LAST_OBJECT_NAME = name
		self.type = type
		self.paramset = []
	def asValue(self):
		return {
			'type': 'MakeNamedVolume',
			'name': self.name,
			'extra_tokens': '"%s"'%self.type,
			'paramset': [psi.asValue() for psi in self.paramset]
		}
	def __repr__(self):
		return json.dumps(self.asValue())

class Material(object):
	def __init__(self, name):
		self.name = name
		global LAST_OBJECT_NAME
		LAST_OBJECT_NAME = name
		self.paramset = []
	def asValue(self):
		return {
			'type': 'MakeNamedMaterial',
			'name': self.name,
			'extra_tokens': '',
			'paramset': [psi.asValue() for psi in self.paramset]
		}
	def __repr__(self):
		return json.dumps(self.asValue())

class ParamSetItem(object):
	def __init__(self, pt, name, value):
		self.type = pt
		self.name = name
		self.value = value
	def asValue(self):
		return {
			'name': self.name,
			'type': self.type,
			'value': self.value
		}
	def __repr__(self):
		return json.dumps(self.asValue())

def p_object_list(p):
	"""
	object_list		: object_list texture
					| object_list volume
					| object_list material
					|
	"""
	if len(p) == 3:
		if p[1] != None:
			p[0] = p[1] + [p[2]]
		else:
			p[0] = [p[2]]

def p_texture(p):
	"""texture : IDENTIFIER STRING STRING STRING paramset"""
	t = Texture(p[2].replace('"',''), p[3].replace('"',''), p[4].replace('"',''))
	if p[5] != None:
		t.paramset.extend( p[5] )
	p[0] = t

def p_volume(p):
	"""volume : IDENTIFIER STRING STRING paramset"""
	o = Volume(p[2].replace('"',''), p[3].replace('"',''))
	if p[4] != None:
		o.paramset.extend( p[4] )
	p[0] = o

def p_material(p):
	"""material : IDENTIFIER STRING paramset"""
	o = Material(p[2].replace('"',''))
	if p[3] != None:
		o.paramset.extend( p[3] )
	p[0] = o

def p_paramset(p):
	"""
	paramset	: paramset numparam
				| paramset colorparam
				| paramset stringparam
				|
	"""
	if len(p) == 3:
		if p[1] != None:
			p[0] = p[1] + [p[2]]
		else:
			p[0] = [p[2]]

def p_numparam(p):
	"""numparam : PARAM_ID LBRACK NUM RBRACK"""
	p_type, p_name = p[1].replace('"','').split(' ')
	p[0] = ParamSetItem(p_type, p_name, float(p[3]))

def p_colorparam(p):
	"""colorparam : PARAM_ID LBRACK NUM NUM NUM RBRACK"""
	p_type, p_name = p[1].replace('"','').split(' ')
	p[0] = ParamSetItem(p_type, p_name, [float(p[3]), float(p[4]), float(p[5])])

def p_stringparam(p):
	"""stringparam : PARAM_ID LBRACK STRING RBRACK"""
	p_type, p_name = p[1].replace('"','').split(' ')
	if p_type == 'bool':
		p[0] = ParamSetItem(p_type, p_name, p[3].replace('"','').lower()=='true')
	else:
		p[0] = ParamSetItem(p_type, p_name, p[3].replace('"',''))

if __name__ == "__main__":
	
	if len(sys.argv) < 3:
		print("""
Usage:	lxm_to_lbm2 <input file> <output file> [debug]
		<input file> should be a well-formed LXM or LXS file, containing
			only MakeNamedVolume, MakeNamedMaterial or Texture blocks
		<output file> is the name of the .lbm2 file to write
		[debug] enables debugging output
""")
		sys.exit()
	
	dbg = len(sys.argv) > 3 and sys.argv[3] == 'debug'
	
	try:
		lex.lex(debug=dbg)
		yacc.yacc(debug=dbg,write_tables=0,errorlog=yacc.NullLogger() if not dbg else yacc.PlyLogger(sys.stdout))
		
		lxm = ''
		with open(sys.argv[1], 'r') as infile:
			lxm = infile.read()
		
		objs = yacc.parse(lxm)
		lbm2 = {
			'name': LAST_OBJECT_NAME,
			'version': '0.8',
			'objects': [o.asValue() for o in objs],
			'metadata': {
				'comment': 'Converted from LXM data'
			}
		}
		
		lbm2_str = json.dumps(lbm2,indent=1)
		
		with open(sys.argv[2], 'w') as outfile:
			outfile.write(lbm2_str)
		
		print('Completed, converted %d objects:' % len(objs))
		for o in objs:
			print('\t%s : %s' % (o.__class__.__name__, o.name))
	except Exception as err:
		print("ERROR: %s" % err)
