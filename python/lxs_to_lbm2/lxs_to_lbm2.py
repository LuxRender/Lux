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

import ply.lex as lex, ply.yacc as yacc, sys, json, hashlib

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
	"LBRACK", "RBRACK"
)
t_ignore_NL			= r'\r+|\n+'
t_ignore_WS			= r'\s+'
t_ignore_COMMENT	= r'\#(.*)'
t_STRING			= r'"([^"]+)"'
t_NUM				= r'[+-]?\d+\.?(\d+)?'
v_TYPES				= r'bool|float|color|vector|normal|point|string|texture|integer'
t_PARAM_ID			= r'["|\']('+v_TYPES+')[ ](\w+)["|\']'
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

# Simple graph based on http://www.python.org/doc/essays/graphs.html
class DataGraph(object):
	def __init__(self):
		self.structure = {}
	
	def add_arc(self, frm, to):
		if frm not in self.structure.keys():
			self.structure[frm] = set()
		self.structure[frm].add(to)
	
	def find_path(self, start, end, path=[]):
		path = path + [start]
		if start == end:
			return path
		if not start in self.structure.keys():
			return None
		for node in self.structure[start]:
			if node not in path:
				newpath = self.find_path(node, end, path)
				if newpath: return newpath
		return None
	
	def find_shortest_path(self, start, end, path=[]):
		path = path + [start]
		if start == end:
			return path
		if not start in self.structure.keys():
			return None
		trg = None
		for node in self.structure[start]:
			if node not in path:
				newpath = self.find_shortest_path(node, end, path)
				if newpath:
					if not trg or len(newpath) < len(trg):
						trg = newpath
		return trg
	
	def all_paths(self):
		nodes = self.structure.keys()
		out = []
		for i in nodes:
			for j in nodes:
				if i==j: continue
				pth = self.find_shortest_path(i,j)
				if pth==None: continue
				if len(pth)<2: continue
				if pth in out: continue
				
				out.append(pth)
		
		for pth in out:
			for opth in out:
				if pth[-1] == opth[0]:
					for k in pth[:-1]:
						opth.insert(0,k)
					out.remove(pth)
#		
		for i,pth in enumerate(out):
			for j,opth in enumerate(out):
				if i!=j and pth==opth:
					out.remove(pth)
		return out

MaterialGraph = DataGraph()

LAST_OBJECT_NAME = ''
OBJECT_NAME_MAP = {}

def shorten_name(n):
	return hashlib.md5(n.encode()).hexdigest()[:21] if len(n) > 21 else n

class LBMObjectBase(object):
	def update_graph(self):
		global MaterialGraph, OBJECT_NAME_MAP
		for psi in self.paramset:
			if psi.type.lower()=='texture':
				MaterialGraph.add_arc(OBJECT_NAME_MAP[psi.value], self)

class Texture(LBMObjectBase):
	def __init__(self, name, variant, type):
		self.name = shorten_name(name)
		global LAST_OBJECT_NAME, OBJECT_NAME_MAP
		LAST_OBJECT_NAME = self.name
		OBJECT_NAME_MAP[self.name] = self
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
		return "<Texture %s>" % self.name

class Volume(LBMObjectBase):
	def __init__(self, name, type):
		self.name = shorten_name(name)
		global LAST_OBJECT_NAME, OBJECT_NAME_MAP
		LAST_OBJECT_NAME = self.name
		OBJECT_NAME_MAP[self.name] = self
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
		return "<Volume %s>" % self.name

class Material(LBMObjectBase):
	def __init__(self, name):
		self.name = shorten_name(name)
		global LAST_OBJECT_NAME, OBJECT_NAME_MAP
		LAST_OBJECT_NAME = self.name
		OBJECT_NAME_MAP[self.name] = self
		self.paramset = []
	def asValue(self):
		return {
			'type': 'MakeNamedMaterial',
			'name': self.name,
			'extra_tokens': '',
			'paramset': [psi.asValue() for psi in self.paramset]
		}
	def __repr__(self):
		return "<Material %s>" % self.name

class ParamSetItem(object):
	def __init__(self, pt, name, value):
		self.type = pt
		self.name = name
		self.value = value
		
		if pt.lower() == 'texture':
			self.value = shorten_name(value)
		
	def asValue(self):
		return {
			'name': self.name,
			'type': self.type,
			'value': self.value
		}
	def __repr__(self):
		return "<ParamSetItem %s:%s>" % (self.type, self.name)

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
	o = Texture(p[2].replace('"',''), p[3].replace('"',''), p[4].replace('"',''))
	if p[5] != None:
		o.paramset.extend( p[5] )
	p[0] = o
	o.update_graph()

def p_volume(p):
	"""volume : IDENTIFIER STRING STRING paramset"""
	o = Volume(p[2].replace('"',''), p[3].replace('"',''))
	if p[4] != None:
		o.paramset.extend( p[4] )
	p[0] = o
	o.update_graph()

def p_material(p):
	"""material : IDENTIFIER STRING paramset"""
	o = Material(p[2].replace('"',''))
	if p[3] != None:
		o.paramset.extend( p[3] )
	p[0] = o
	o.update_graph()

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
Usage:	lxs_to_lbm2 <input file> <output file> [debug]
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
				'comment': 'Converted from LXS data'
			}
		}
		
		with open(sys.argv[2], 'w') as outfile:
			json.dump(lbm2, outfile, indent=1)
		
		print('Completed, converted %d objects:' % len(objs))
		for o in objs:
			print('\t%s' % o)
	except Exception as err:
		print("ERROR: %s" % err)
