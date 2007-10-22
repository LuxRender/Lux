#!BPY
"""Registration info for Blender menus:
Name: 'LuxBlend-v0.1-alpha8...'
Blender: 240
Group: 'Export'
Tooltip: 'Export to LuxRender v0.1 scene format (.lxs)'
"""
#
# ***** BEGIN GPL LICENSE BLOCK *****
#
# --------------------------------------------------------------------------
# LuxBlend v0.1 alpha8 exporter
# --------------------------------------------------------------------------
#
# Authors:
# Radiance (aka Terrence Vergauwen)
#
# Based on:
# * Lux exporter - Nick Chapman, Gregor Quade, Zuegs, Ewout Fernhout, Leope, psor
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
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# ***** END GPL LICENCE BLOCK *****
# --------------------------------------------------------------------------

######################################################
# Importing modules
######################################################

import math
import os
import Blender
from Blender import NMesh, Scene, Object, Material, Texture, Window, sys, Draw, BGL, Mathutils


######################################################
# Functions
######################################################


# New name based on old with a different extension
def newFName(ext):
	return Blender.Get('filename')[: -len(Blender.Get('filename').split('.', -1)[-1]) ] + ext

# zuegs: added color exponent
def colGamma(value):
	return value**ColExponent.val

def luxMatrix(matrix):
	ostr = ""
	ostr += "Transform [%s %s %s %s  %s %s %s %s  %s %s %s %s  %s %s %s %s]\n"\
		%(matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3],\
		  matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3],\
		  matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3],\
		  matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]) 
	return ostr

#################### Export Material Texture ###

# MATERIAL TYPES enum
# 0 = 'glass'
# 1 = 'translucent'
# 2 = 'mirror'
# 3 = 'plastic'
# 4 = 'shinymetal'
# 5 = 'substrate'
# 6 = 'matte' (Oren-Nayar)
# 7 = 'matte' (Lambertian)

### Determine the type of material ###
def getMaterialType(mat):
	# default to matte (Lambertian)
	mat_type = 7

	# test for emissive material
	if (mat.emit > 0):
		### emitter material ###
		mat_type = 100

	# test for Transmissive (transparent) Materials
	elif (mat.mode & Material.Modes.RAYTRANSP):
		if(mat.getTranslucency < 1.0):
			### 'glass' material ###
			mat_type = 0	
		else:
			### 'translucent' material ###
			mat_type = 1

	# test for Mirror Material
	elif(mat.mode & Material.Modes.RAYMIRROR):
		### 'mirror' material ###
		mat_type = 2

	# test for Diffuse / Specular Reflective materials
	else:
		if (mat.getSpec() > 0.0001):
			# Reflective 
			if (mat.getSpecShader() == 2):			# Blinn
				### 'plastic' material ###
				mat_type = 3
			elif (mat.getSpecShader() == 1):		# Phong
				### 'shinymetal' material ###
				mat_type = 4
			else:						# CookTor/other
				### 'substrate' material ###
				mat_type = 5
		else:
			### 'matte' material ### 
			if (mat.getDiffuseShader() == 1):	# Oren-Nayar
				# oren-nayar sigma
				mat_type = 6
			else:					# Lambertian/other
				# lambertian sigma
				mat_type = 7

	return mat_type


# Determine the type of material / returns filename or 'NONE' 
def getTextureChannel(mat, channel):
	for t in mat.getTextures():
		if (t != None) and (t.texco & Blender.Texture.TexCo['UV']) and\
				(t.tex.type == Blender.Texture.Types.IMAGE):
			if t.mapto & Blender.Texture.MapTo[channel]:
				(imagefilepath, imagefilename) = os.path.split(t.tex.getImage().getFilename())
				return imagefilename
	return 'NONE'

def getTextureforChannel(mat, channel):
	for t in mat.getTextures():
		if (t != None) and (t.texco & Blender.Texture.TexCo['UV']) and\
				(t.tex.type == Blender.Texture.Types.IMAGE):
			if t.mapto & Blender.Texture.MapTo[channel]:
				return t
	return 0

# MATERIAL TYPES enum
# 0 = 'glass'
# 1 = 'translucent'
# 2 = 'mirror'
# 3 = 'plastic'
# 4 = 'shinymetal'
# 5 = 'substrate'
# 6 = 'matte' (Oren-Nayar)
# 7 = 'matte' (Lambertian)
#
# 100 = emissive (area light source / trianglemesh)

def exportMaterial(mat):
	str = "# Material '%s'\n" %mat.name

	def write_color_constant( chan, name, r, g, b ):
		return "Texture \"%s-%s\" \"color\" \"constant\" \"color value\" [%.3f %.3f %.3f]\n" %(chan, name, r, g, b)

	def write_color_imagemap( chan, name, filename, scaleR, scaleG, scaleB ):
		om = "Texture \"%s-map-%s\" \"color\" \"imagemap\" \"string filename\" [\"%s\"] \"float vscale\" [-1.0]\n" %(chan, name, filename)
		om += "Texture \"%s-scale-%s\" \"color\" \"constant\" \"color value\" [%f %f %f]\n" %(chan, name, scaleR, scaleG, scaleB)
		om += "Texture \"%s-%s\" \"color\" \"scale\" \"texture tex1\" [\"%s-map-%s\"] \"texture tex2\" [\"%s-scale-%s\"]\n" %(chan, name, chan, name, chan, name)
		return om

	def write_float_constant( chan, name, v ):
		return "Texture \"%s-%s\" \"float\" \"constant\" \"float value\" [%f]\n" %(chan, name, v)

	def write_float_imagemap( chan, name, filename, scale ):
		om = "Texture \"%s-map-%s\" \"float\" \"imagemap\" \"string filename\" [\"%s\"] \"float vscale\" [-1.0]\n" %(chan, name, filename)
		om += "Texture \"%s-scale-%s\" \"float\" \"constant\" \"float value\" [%f]\n" %(chan, name, scale)
		om += "Texture \"%s-%s\" \"float\" \"scale\" \"texture tex1\" [\"%s-map-%s\"] \"texture tex2\" [\"%s-scale-%s\"]\n" %(chan, name, chan, name, chan, name)
		return om

	def write_color_param( m, param, chan, r, g, b ):
		chan_file = getTextureChannel(m, chan)
		if (chan_file == 'NONE'):
			return write_color_constant( param, m.name, r, g, b )
		else:
			tc = getTextureforChannel(m, chan)
			if(tc == 0):
				ss = 1.0
			else:
				ss = tc.colfac
			return write_color_imagemap( param, m.name, chan_file, r * ss, g * ss, b * ss )

	def write_float_param( m, param, chan, v ):
		chan_file = getTextureChannel(m, chan)
		if (chan_file == 'NONE'):
			return write_float_constant( param, m.name, v )
		else:
			tc = getTextureforChannel(m, chan)
			if (chan == 'NOR'):
				ss = tc.norfac * 0.1
			else:
				ss = v
			return write_float_imagemap( param, m.name, chan_file, ss )

	# translates blender mat.hard (range: 1-511) 
	# to lux blinn value (0.0 - 1.0 Reversed)
	def HardtoBlinn(hard):
		blinn = 1.0 / (float(hard) *5)
		if( blinn < 0.00001 ):
			blinn = 0.00001
		if( blinn > 1.0 ):
			blinn = 1.0
		return blinn

	# translates blender mat.hard (range: 1-511) 
	# to lux aniso shinyness value (0.00001 = nearly specular - 1.0 = diffuse)
	def HardtoAniso(hard):
		aniso = 1.0 / (float(hard) *5)
		if( aniso < 0.00001 ):
			aniso = 0.00001
		if( aniso > 1.0 ):
			aniso = 1.0
		return aniso

	mat_type = getMaterialType(mat)

	if (mat_type == 100):
		### emitter material ### COMPLETE
		str += "# Type: emitter\n"
		str += "# See geometry block.\n\n"

	elif (mat_type == 0):
		### 'glass' material ### COMPLETE
		str += "# Type: 'glass'\n"
		str += write_color_param( mat, "Kr", 'SPEC', mat.specR, mat.specG, mat.specB )
		str += write_color_param( mat, "Kt", 'COL', mat.R, mat.G, mat.B )
		str += write_float_param( mat, "index", 'REF', mat.IOR )
		str += write_float_param( mat, "bumpmap", 'NOR', 0.0 )

	elif (mat_type == 1):
		### 'translucent' material ###
		str += "# Type: 'translucent'\n"
		str += write_color_param( mat, "Kd", 'SPEC', mat.specR, mat.specG, mat.specB )
		str += write_color_param( mat, "Ks", 'COL', mat.R, mat.G, mat.B )
		str += "Texture \"reflect-%s\" \"float\" \"constant\" \"float value\" [.5]\n" %(mat.name)
		str += "Texture \"transmit-%s\" \"float\" \"constant\" \"float value\" [.5]\n" %(mat.name)
		str += write_float_param( mat, "roughness", 'HARD', HardtoBlinn(mat.hard) )
		str += write_float_param( mat, "bumpmap", 'NOR', 0.0 )

	elif (mat_type == 2):
		### 'mirror' material ### COMPLETE
		str += "# Type: 'mirror'\n"
		str += write_color_param( mat, "Kr", 'COL', mat.R, mat.G, mat.B )
		str += write_float_param( mat, "bumpmap", 'NOR', 0.0 )

	elif (mat_type == 3):
		### 'plastic' material ### COMPLETE
		str += "# Type: 'plastic'\n"
		str += write_color_param( mat, "Kd", 'COL', mat.R, mat.G, mat.B )
		str += write_color_param( mat, "Ks", 'SPEC', mat.specR * mat.getSpec(), mat.specG * mat.getSpec(), mat.specB * mat.getSpec() )
		str += write_float_param( mat, "roughness", 'HARD', HardtoBlinn(mat.hard) )
		str += write_float_param( mat, "bumpmap", 'NOR', 0.0 )

	elif (mat_type == 4):
		### 'shinymetal' material ### COMPLETE
		str += "# Type: 'shinymetal'\n"
		str += write_color_param( mat, "Ks", 'COL', mat.R, mat.G, mat.B )
		str += write_color_param( mat, "Kr", 'SPEC',mat.specR * mat.getSpec(), mat.specG * mat.getSpec(), mat.specB * mat.getSpec() )
		str += write_float_param( mat, "roughness", 'HARD', HardtoBlinn(mat.hard) )
		str += write_float_param( mat, "bumpmap", 'NOR', 0.0 )

	elif (mat_type == 5):
		### 'substrate' material ### COMPLETE
		str += "# Type: 'substrate'\n"
		str += write_color_param( mat, "Kd", 'COL', mat.R, mat.G, mat.B )
		str += write_color_param( mat, "Ks", 'SPEC', mat.specR * mat.getSpec(), mat.specG * mat.getSpec(), mat.specB * mat.getSpec() )
		# TODO: add different inputs for both u/v roughenss for aniso effect
		str += write_float_param( mat, "uroughness", 'HARD', HardtoAniso(mat.hard) )
		str += write_float_param( mat, "vroughness", 'HARD', HardtoAniso(mat.hard) )
		str += write_float_param( mat, "bumpmap", 'NOR', 0.0 )

	elif (mat_type == 6):
		### 'matte' (Oren-Nayar) material ###
		# TODO
		str += "\n"
	else:
		### 'matte' (Lambertian) material ### COMPLETE
		str += "# Type: 'matte' (Lambertian)\n"
		str += write_color_param( mat, "Kd", 'COL', mat.R, mat.G, mat.B )
		str += write_float_param( mat, "sigma", 'REF', 0 )
		str += write_float_param( mat, "bumpmap", 'NOR', 0.0 )

	str += "#####\n\n"
	return str

def exportMaterialGeomTag(mat):
	mat_type = getMaterialType(mat)
	if (mat_type == 100):
		str = "AreaLightSource \"area\" \"integer nsamples\" [1] \"color L\" [%f %f %f]" %(mat.R * mat.emit, mat.G * mat.emit, mat.B * mat.emit)
	else:

	 	str = "Material "

		if (mat_type == 0):
			### 'glass' material ### COMPLETE
			str += " \"glass\" \"texture Kr\" \"Kr-%s\"" %mat.name
			str += " \"texture Kt\" \"Kt-%s\"" %mat.name
			str += " \"texture index\" \"index-%s\"" %mat.name
			str += " \"texture bumpmap\" \"bumpmap-%s\"" %mat.name

		elif (mat_type == 1):
			### 'translucent' material ###
			str += " \"translucent\" \"texture Kd\" \"Kd-%s\"" %mat.name
			str += " \"texture Ks\" \"Ks-%s\"" %mat.name
			str += " \"texture reflect\" \"reflect-%s\"" %mat.name
			str += " \"texture transmit\" \"transmit-%s\"" %mat.name
			str += " \"texture roughness\" \"roughness-%s\"" %mat.name
			str += " \"texture index\" \"index-%s\"" %mat.name
			str += " \"texture bumpmap\" \"bumpmap-%s\"" %mat.name

		elif (mat_type == 2):
			### 'mirror' material ### COMPLETE
			str += " \"mirror\" \"texture Kr\" \"Kr-%s\"" %mat.name
			str += " \"texture bumpmap\" \"bumpmap-%s\"" %mat.name

		elif (mat_type == 3):
			### 'plastic' material ### COMPLETE
			str += " \"plastic\" \"texture Kd\" \"Kd-%s\"" %mat.name
			str += " \"texture Ks\" \"Ks-%s\"" %mat.name
			str += " \"texture roughness\" \"roughness-%s\"" %mat.name
			str += " \"texture bumpmap\" \"bumpmap-%s\"" %mat.name

		elif (mat_type == 4):
			### 'shinymetal' material ### COMPLETE
			str += " \"shinymetal\" \"texture Ks\" \"Ks-%s\"" %mat.name
			str += " \"texture Kr\" \"Kr-%s\"" %mat.name
			str += " \"texture roughness\" \"roughness-%s\"" %mat.name
			str += " \"texture bumpmap\" \"bumpmap-%s\"" %mat.name

		elif (mat_type == 5):
			### 'substrate' material ### COMPLETE
			str += " \"substrate\" \"texture Kd\" \"Kd-%s\"" %mat.name
			str += " \"texture Ks\" \"Ks-%s\"" %mat.name
			str += " \"texture uroughness\" \"uroughness-%s\"" %mat.name
			str += " \"texture vroughness\" \"vroughness-%s\"" %mat.name
			str += " \"texture bumpmap\" \"bumpmap-%s\"" %mat.name

		elif (mat_type == 6) or (mat_type == 7):
			### 'matte' (Oren-Nayar or Lambertian) material ###
			str += " \"matte\" \"texture Kd\" \"Kd-%s\"" %mat.name
			str += " \"texture sigma\" \"sigma-%s\"" %mat.name
			str += " \"texture bumpmap\" \"bumpmap-%s\"" %mat.name

	str += "\n"
	return str

# collect Materials
matnames = []
def collectObjectMaterials(obj):
	global matnames
	objectname = obj.getName()
	objecttype = obj.getType()
	if (objecttype == "Mesh") or (objecttype == "Curve") or (objecttype == "Text"):
		print("Collecting materials for object: %s" %objectname)
		materials = obj.getData().getMaterials()
		meshlight = 0
		if len(materials) > 0:
			mat0 = materials[0]
			if mat0.emit > 0:
				meshlight = 1
		if meshlight == 0:
			for mat in materials:
				if mat.name not in matnames:
					matnames.append(mat.name)
	elif (objecttype == "Empty"):
		group = obj.DupGroup
		if group:
			groupname = group.name
			for o, m in obj.DupObjects:
				collectObjectMaterials(o)



################################################################
def exportMesh(mesh, obj, isLight, isPortal):

	mestr = ""
	
	#materials = obj.getData().getMaterials()
	materials = mesh.getMaterials()	

	def writeMeshData():
		vdata = []
		vlist = []
		flist = []
		def addVertex(bvindex, coord, normal, uv):
			index = -1
			if (bvindex < len(vdata)):
				for ivindex in vdata[bvindex]:
					v = vlist[ivindex]
					if (abs(v[0][0]-coord[0])<0.0001) and \
					(abs(v[0][1]-coord[1])<0.0001) and \
					(abs(v[0][2]-coord[2])<0.0001) and \
					(abs(v[1][0]-normal[0])<0.0001) and \
					(abs(v[1][1]-normal[1])<0.0001) and \
					(abs(v[1][2]-normal[2])<0.0001):
						if ((v[2]==[]) and (uv==[])) or \
						((abs(v[2][0]-uv[0])<0.0001) and \
						(abs(v[2][1]-uv[1])<0.0001)):
							index = ivindex
			if index < 0:
				index = len(vlist)
				vlist.append([coord, normal, uv])
				while bvindex >= len(vdata):
					vdata.append([])
				vdata[bvindex].append(index)
			return index

		def addFace(mindex, index0, index1, index2):
			#while mindex >= len(flist):
			#	flist.append([])
			#flist[mindex].append([index0, index1, index2])
			flist[0].append([index0, index1, index2])

		def getPoints():
			return "".join(map( \
				lambda v: "\t%f %f %f\n" \
				%(v[0][0], v[0][1], v[0][2]), \
				vlist))

		def getNormals():
			return "".join(map( \
				lambda v: "\t%f %f %f\n" \
				%(v[1][0], v[1][1], v[1][2]), \
				vlist))
		def getUVs():
			def getUVStr(uv):
				if uv != []: return "%f %f" %(uv[0], uv[1])
				else: return "0.000000 1.000000"
			return "".join(map( \
				lambda v: "\t%s\n" \
				%(getUVStr(v[2])), vlist))

		def getMaterialName(mindex):
			if mindex < len(mesh.materials): return mesh.materials[mindex].name
			else:
				print "Warning: faces without material"
				return "Default"

		def getMaterialLine(material):
			#if mindex < len(mesh.materials): 
			return exportMaterialGeomTag(material)
			#else:
			#	print "Warning: faces without material"
			#	return "Material \"matte\" \"texture Kd\" \"Kd-Default\""

		def writeGroup(material):
			print "Nr of materials: %i\n" %len(flist)
			if isPortal:
				return "".join(map( \
					lambda fli: "PortalShape \"trianglemesh\" \"integer indices\" [\n" \
						+ "".join(map(lambda f: "%d %d %d\n"%(f[0], f[1], f[2]), flist[fli])) \
						+ "" , range(len(flist)) )) \
						+ "] \"point P\" [\n" + getPoints() \
						+ "] \"normal N\" [\n" + getNormals() \
						+ "]\n\n"		
			if have_uvs:
				return "".join(map( \
					lambda fli: "%s\nShape \"trianglemesh\" \"integer indices\" [\n"%(getMaterialLine(material)) \
						+ "".join(map(lambda f: "%d %d %d\n"%(f[0], f[1], f[2]), flist[fli])) \
						+ "" , range(len(flist)) )) \
						+ "] \"point P\" [\n" + getPoints() \
						+ "] \"normal N\" [\n" + getNormals() \
						+ "] \"float uv\" [\n" + getUVs() \
						+ "]\n\n"
			else:
				return "".join(map( \
					lambda fli: "%s\nShape \"trianglemesh\" \"integer indices\" [\n"%(getMaterialLine(material)) \
						+ "".join(map(lambda f: "%d %d %d\n"%(f[0], f[1], f[2]), flist[fli])) \
						+ "" , range(len(flist)) )) \
						+ "] \"point P\" [\n" + getPoints() \
						+ "] \"normal N\" [\n" + getNormals() \
						+ "]\n\n"

		vdata = []
		vlist = []
		flist = []
		ostr = ""

		emindex = 0
		have_uvs = 0

		print "obstart\n"
		for material in materials:
		# Construct list of verts/faces
			vdata = []
			vlist = []
			flist = []

			flist.append([])
			have_faces = 0

			for face in mesh.faces:
				iis = [-1, -1, -1, -1]
				if( material.name == materials[face.materialIndex].name ):	
					for vi in range(len(face.v)):
						vert = face.v[vi]
						if face.smooth:
							normal = vert.no
						else:
							normal = face.no
						if len(face.uv) == len(face.v):
							uv = face.uv[vi]
							have_uvs = 1
						else:
							uv = []
						iis[vi] = addVertex(vert.index, vert.co, normal, uv)
					addFace(0, iis[0], iis[1], iis[2])
					have_faces = 1
					if len(face.v)==4:
						addFace(0, iis[2], iis[3], iis[0])
		
			if have_faces:
				ostr += writeGroup(material)
			del vdata[:]
			del vlist[:]
			del flist[:]
		return ostr


	objectname = obj.getName()
	meshname = mesh.name
	str = ""

	# write Object header
	if isLight:
		str += "AttributeBegin\n"
		matrix = obj.getMatrix()
		str += luxMatrix(matrix)
		str += "\n"
		str += writeMeshData()

		str += "AttributeEnd\n\n"
	elif isPortal:
		str += "\n"
		matrix = obj.getMatrix()
		str += luxMatrix(matrix)
		str += "\n"
		str += writeMeshData()
	else:
		#str += "ObjectBegin \"%s\"\n\n" %(meshname.replace("<","-").replace(">","-"))
		#str += writeMeshData()
		#str += "ObjectEnd # %s\n\n\n" %(meshname.replace("<","-").replace(">","-"))
		str += "AttributeBegin\n"
		matrix = obj.getMatrix()
		str += luxMatrix(matrix)
		str += "\n"
		str += writeMeshData()

		str += "AttributeEnd\n\n"
	return str


# export objext
meshlist = []
def exportObject(obj):
	global meshlist
	str = ""
	gstr = ""
	mstr = {}
	objectname = obj.getName()
	objecttype = obj.getType()

	if (objecttype == "Mesh") or\
		(objecttype == "Curve") or\
		 (objecttype == "Text"):
			mesh = Blender.NMesh.GetRawFromObject(objectname)
			meshname = mesh.name

			if (objecttype == "Curve") or (objecttype == "Text"):
				meshname = obj.getData().getName()			  
				mesh.name = meshname
				for f in mesh.faces:
					f.smooth = 1

			if (objecttype == "Curve"):
				mesh.setMaterials(obj.getData().getMaterials())

			isLight = 0
			isPortal = 0
			if len(mesh.materials) > 0:
				mat0 = mesh.materials[0]
				if mat0.name == "PORTAL":
					isPortal = 1
				if mat0.emit > 0:
					isLight = 1

			print "processing Object \"%s\" (Mesh \"%s\")..." %(objectname, meshname)

			try:
				meshlist.index(meshname)
			except ValueError:			
				meshlist.append(meshname)

			# export meshlight geometry
			gstr += exportMesh(mesh, obj, isLight, isPortal)

			# TODO FIX FOR INSTANCING
			#if isLight == 0:
			#	# export instance/object
			#	str += "AttributeBegin\n"
			#	matrix = obj.getMatrix()
			#	str += luxMatrix(matrix)
			#	str += "\n"
			#	str += "\tObjectInstance \"%s\"\n" %(meshname.replace("<","-").replace(">","-"))
			#	str += "AttributeEnd\n\n"
				
	mstr[0] = str
	mstr[1] = gstr
	return mstr

################################################################













######################################################
# EXPORT
######################################################

def save_lux(filename, unindexedname):
	global meshlist, matnames, geom_filename, geom_pfilename, mat_filename, mat_pfilename

	print("Lux Render Export started...\n")
	time1 = Blender.sys.time()

	##### Determine/open files
	print("Exporting scene to '" + filename + "'...\n")
	file = open(filename, 'w')

	filepath = os.path.dirname(filename)
	print filename
	print filepath
	filebase = os.path.basename(filename)

	geom_filename = os.path.join(filepath, "geom-" + filebase)
	geom_pfilename = "geom-" + filebase

	mat_filename = os.path.join(filepath, "mat-" + filebase)
	mat_pfilename = "mat-" + filebase

	print("Exporting materials to '" + mat_filename + "'...\n")
	print("Exporting geometry to '" + geom_filename + "'...\n")
	geom_file = open(geom_filename, 'w')
	mat_file = open(mat_filename, 'w')


	##### Write Header ######
	file.write("# Lux Render v0.1 Scene File\n")
	file.write("# Exported by LuxBlend_01_alpha1\n")

	file.write("\n")

	##### Write camera ######
	currentscene = Scene.GetCurrent()
	camObj = currentscene.getCurrentCamera()
	if camObj:
		print "processing Camera..."
		matrix = camObj.getMatrix()
		pos = matrix[3]
		forwards = -matrix[2]
		target = pos + forwards
		up = matrix[1]
		lens = camObj.data.getLens() 
    		context = Blender.Scene.getCurrent().getRenderingContext()
    		ratio = float(context.imageSizeY())/float(context.imageSizeX())
		fov = 2*math.atan(ratio*16/lens) * (180 / 3.141592653)
		file.write("LookAt %f %f %f   %f %f %f %f %f %f\n" % ( pos[0], pos[1], pos[2], target[0], target[1], target[2], up[0], up[1], up[2] ))
		file.write("Camera \"perspective\" \"float fov\" [%f] \"float lensradius\" [%f] \"float focaldistance\" [%f] \n" % (fov,LensRadius.val,FocalDistance.val))
	file.write("\n")

	##### Write film ######
	file.write("Film \"multiimage\"\n") 
	file.write("     \"integer xresolution\" [%d] \"integer yresolution\" [%d]\n" % (SizeX.val*ScaleSize.val/100, SizeY.val*ScaleSize.val/100) )

	if(SaveEXR.val == 1):
		file.write("	 \"string hdr_filename\" [\"out.exr\"]\n")
		file.write("	 	\"integer hdr_writeinterval\" [%i]\n" %(SaveEXRint.val))
	if(SaveTGA.val == 1):
		file.write("	 \"string ldr_filename\" [\"out.tga\"]\n")
		file.write("		\"integer ldr_writeinterval\" [%i]\n" %(SaveTGAint.val))
	file.write("		\"integer ldr_displayinterval\" [%i]\n" %(Displayint.val))
	file.write("		\"string tonemapper\" [\"reinhard\"]\n")
	file.write("			\"float reinhard_prescale\" [%f]\n" %(ToneMapPreScale.val))
	file.write("			\"float reinhard_postscale\" [%f]\n" %(ToneMapPostScale.val))
	file.write("			\"float reinhard_burn\" [%f]\n" %(ToneMapBurn.val))
	file.write("		\"float gamma\" [%f]\n" %(OutputGamma.val))
	file.write("		\"float dither\" [%f]\n\n" %(OutputDither.val))
	if(Bloom.val == 1):
		file.write("		\"float bloomwidth\" [%f]\n" %(BloomWidth.val))
		file.write("		\"float bloomradius\" [%f]\n" %(BloomRadius.val))

	if(FilterType.val == 1):
		file.write("PixelFilter \"mitchell\" \"float xwidth\" [%f] \"float ywidth\" [%f]\n" %(Filterxwidth.val, Filterywidth.val))
	else:
		file.write("PixelFilter \"gaussian\" \"float xwidth\" [%f] \"float ywidth\" [%f]\n" %(Filterxwidth.val, Filterywidth.val))
	file.write("\n")

	##### Write Pixel Sampler ######
	if(SamplerType.val == 1):
		file.write("Sampler \"random\" ")
	else:
		file.write("Sampler \"lowdiscrepancy\" ")

	if(SamplerProgressive.val == 1):
		file.write("\"bool progressive\" [\"true\"] ")
	else:
		file.write("\"bool progressive\" [\"false\"] ")
	
	file.write("\"integer pixelsamples\" [%i]\n" %(SamplerPixelsamples.val))
	file.write("\n")

	##### Write Integrator ######
	file.write("SurfaceIntegrator \"path\" \"integer maxdepth\" [%i] " %(pathMaxDepth.val))
	if(pathRRforcetransmit.val == 1):
		file.write("\"bool rrforcetransmit\" [\"true\"] ")
	else:
		file.write("\"bool rrforcetransmit\" [\"false\"] ")

	file.write("\"float rrcontinueprob\" [%f]\n" %(pathRRcontinueprob.val))
	file.write("\n")

	##### Write Acceleration ######
	file.write("Accelerator \"kdtree\"\n")
	file.write("\n")

	##### Get all the objects/materials in this scene #####
	portalstr = ""
	activelayers = Window.ViewLayer()
	object_list = []
	matnames= []
	for obj in Blender.Scene.GetCurrent().getChildren():
		for layer in obj.layers:
			if layer in activelayers:
				objecttype = obj.getType()
				if (objecttype == "Mesh") and (len(obj.getData().getMaterials()) > 0)\
						and (obj.getData().getMaterials()[0].name == "PORTAL"):
							portalstr += exportObject(obj)[1]
				else:
					object_list.append(obj)
					collectObjectMaterials(obj)

	########## BEGIN World
	file.write("\n")
	file.write("WorldBegin\n")

	file.write("\n")

	##### Write World Background, Sunsky or Env map ######
	if EnvType.val == 0:
		worldcolor = Blender.World.Get('World').getHor()
		file.write("AttributeBegin\n")
		file.write("LightSource \"infinite\" \"color L\" [%g %g %g] \"integer nsamples\" [1]\n" %(worldcolor[0], worldcolor[1], worldcolor[2]))
		file.write("%s" %portalstr)
		file.write("AttributeEnd\n")
	if EnvType.val == 1:
		for obj in object_list:
			if obj.getType() == "Lamp":
				if obj.data.getType() == 1: # sun object
					invmatrix = Mathutils.Matrix(obj.getInverseMatrix())
					file.write("AttributeBegin\n")
					file.write("LightSource \"sunsky\" \"integer nsamples\" [1]\n")
					file.write("            \"vector sundir\" [%f %f %f]\n" %(invmatrix[0][2], invmatrix[1][2], invmatrix[2][2]))
					file.write("		\"color L\" [%f %f %f]\n" %(SkyGain.val, SkyGain.val, SkyGain.val))
					file.write("		\"float turbidity\" [%f]\n" %(Turbidity.val))
					file.write("%s" %portalstr)
					file.write("AttributeEnd\n")
	if EnvType.val == 2:
		if EnvFile.val != "none" and EnvFile.val != "":
			file.write("AttributeBegin\n")
			file.write("LightSource \"infinite\" \"integer nsamples\" [1]\n")
			file.write("            \"string mapname\" [\"%s\"]\n" %(EnvFile.val) )
			file.write("%s" %portalstr)
			file.write("AttributeEnd\n")
	file.write("\n")


	#### Write material & geometry file includes in scene file
	file.write("Include \"%s\"\n\n" %(mat_pfilename))
	file.write("Include \"%s\"\n\n" %(geom_pfilename))

	##### Write Material file #####
	mat_file.write("")
	mat_file.write("# Dummy Material 'Default'\n")
	mat_file.write("Texture \"Kd-Default\" \"color\" \"constant\" \"color value\" [0.5 0.5 0.5]\n\n")
	materials = Material.Get()
	for mat in materials:
		if mat.name in matnames:
			mat_file.write(exportMaterial(mat))	

	##### Write Geometry file #####
	meshlist = []
	geom_file.write("")
	for obj in object_list:
		global expobj
		expobj = exportObject(obj)
		file.write(expobj[0])
		geom_file.write(expobj[1])

	##### END World & Close files
	geom_file.write("")
	geom_file.close()
	mat_file.write("")
	mat_file.close()
	file.write("WorldEnd\n\n")
	file.close()

	print("Finished.\n")

	time2 = Blender.sys.time()
	print("Processing time: %f\n" %(time2-time1))
	#Draw.Exit()













#########################################################################
###	 LAUNCH LuxRender AND RENDER CURRENT SCENE (WINDOWS ONLY)		 ###
###  psor's first steps to Python(executing Lux per shell script)  ###
#########################################################################

def launchLux(filename):
	# get blenders 'bpydata' directory
	datadir=Blender.Get("datadir")
	
	# open 'LuxWrapper.conf' and read the first line
	f = open(datadir + '/LuxWrapper.conf', 'r+')
	ic=f.readline()
	f.close()
	
	# create 'LuxWrapper.cmd' and write two lines of code into it
	f = open(datadir + "\LuxWrapper.cmd", 'w')
	f.write("cd /d " + ic + "\n")
	f.write("start /b /belownormal Lux.exe %1 -t " + str(Threads.val) + "\n")
	f.close()
	
	# call external shell script to start Lux
	cmd= "\"" + datadir + "\LuxWrapper.cmd " + filename + "\""
	print cmd
	os.system(cmd)


### END OF PSOR ##########################################################




#### SAVE ANIMATION ####	
def save_anim(filename):
	global MatSaved
	
	MatSaved = 0
	startF = Blender.Get('staframe')
	endF = Blender.Get('endframe')

	for i in range (startF, endF):
		Blender.Set('curframe', i)
		Blender.Redraw()
		frameindex = "-" + str(i) + ".lxs"
		indexedname = makename(filename, frameindex)
		unindexedname = filename
		save_lux(indexedname, unindexedname)
		MatSaved = 1

#### SAVE STILL (hackish...) ####
def save_still(filename):
	global MatSaved
	
	MatSaved = 0
	unindexedname = filename
	save_lux(filename, unindexedname)
	#if ExecuteLux.val == 1:
	#	launchLux(filename)







######################################################
# Settings GUI
######################################################


###psor's add
ExecuteLux = Draw.Create(1)
DefaultExport = Draw.Create(1)
###END


# Assign event numbers to buttons
evtNoEvt	= 0
evtExport   = 1
evtExportAnim   = 2
openCamera = 6
openEnv = 7
openRSet = 8
openSSet = 9
openTmap = 10
evtloadimg = 11
evtchangesize = 12
evtIsNumber = 3
evtFocusS = 4
evtFocusC = 5

# Set initial values of buttons

##  <size>800 600</size>

sceneSizeX = Scene.GetCurrent().getRenderingContext().imageSizeX()
sceneSizeY = Scene.GetCurrent().getRenderingContext().imageSizeY()

SizeX = Draw.Create(sceneSizeX)
SizeY = Draw.Create(sceneSizeY)

strScaleSize = "Scale Size %t | 100 % %x100 | 75 % %x75 | 50 % %x50 | 25 % %x25"
ScaleSize = Draw.Create(100)

ColExponent = Draw.Create(2.2)

##  <metropolis>1</metropolis>
MLT = Draw.Create(1)

##  <large_mutation_prob>0.1</large_mutation_prob>
LMP = Draw.Create(0.1)

##  <max_change>0.02</max_change>
MaxChange = Draw.Create(0.02)

##  <max_num_consec_rejections>100</max_num_consec_rejections>
MaxNumConsRej = Draw.Create(100)

##  <russian_roulette_live_prob>0.7</russian_roulette_live_prob>
RRLP = Draw.Create(0.7)

##  <max_depth>1000</max_depth>
MaxDepth = Draw.Create(1000)

##  <bidirectional>true</bidirectional>
Bidirectional = Draw.Create(1)

##  <strata_width>14</strata_width>
StrataWidth = Draw.Create(14)

##Threads
Threads = Draw.Create(1)

##  <logging>0</logging>
Logging = Draw.Create(0)

##  <save_untonemapped_exr>false</save_untonemapped_exr>
SaveUTMExr = Draw.Create(0)

##  <save_tonemapped_exr>false</save_tonemapped_exr>
SaveTMExr = Draw.Create(0)

#HaltTime
HaltTime = Draw.Create(-1)

#Frameup
FrameUp = Draw.Create(20)

#ImageSave
ImageSave = Draw.Create(20)

#ExpotGeom
ExportGeom = Draw.Create(1)
geom_pfilename = ""
##  <sensor_width>0.035</sensor_width>
FilmWidth = Draw.Create(35.0)

##  <lens_radius>0.0</lens_radius>
LensRadius = Draw.Create(0.0)

##  <focus_distance>2.0</focus_distance>
FocalDistance = Draw.Create(2.0)

##  <white_balance>D65</white_balance>
#strWhiteBalance = "White Balance %t | D50 %x50 | D55 %x55 | D65 %x65 | D75 %x75"
strWhiteBalance = "White Balance %t | E %x0 | D50 %x1 | D55 %x2 | D65 %x3 | D75 %x4 "
strWhiteBalance += "| A %x5 | B %x6 | C %x7 | 9300 %x8 | F2 %x9 | F7 %x10 | F11 %x11"
WhiteBalance = Draw.Create(0)
whiteBalanceV = {}
whiteBalanceV[0] = "E"
whiteBalanceV[1] = "D50"
whiteBalanceV[2] = "D55"
whiteBalanceV[3] = "D65"
whiteBalanceV[4] = "D75"
whiteBalanceV[5] = "A"
whiteBalanceV[6] = "B"
whiteBalanceV[7] = "C"
whiteBalanceV[8] = "9300"
whiteBalanceV[9] = "F2"
whiteBalanceV[10] = "F7"
whiteBalanceV[11] = "F11"

##FilmIso
FilmIso = Draw.Create(100)

## Environment Type
strEnvType = "Env Type %t | Background Color %x0 | Physical Sky %x1 | Texture Map %x2 | None %x3"
EnvType = Draw.Create(0)

##  <turbidity>2.0</turbidity>
Turbidity = Draw.Create(2.0)

##  <sky_gain>2.0</sky_gain>
SkyGain = Draw.Create(0.005)

GroundPlane = Draw.Create(0)

## Separate materials
MatFile = Draw.Create(0)

## Environment map
EnvFile = Draw.Create("none")
strEnvMapType = "Map type %t | LatLong %x0"
EnvMapType = Draw.Create(0)
EnvMapTypeV = {}
EnvMapTypeV[0] = "latlong"
EnvGain = Draw.Create(1.0)
EnvWidth = Draw.Create(640)

# filters
strFilterType = "Filter %t | gaussian %x0 | mitchell %x1"
FilterType = Draw.Create(0)
FilterTypeV = {}
FilterTypeV[0] = "gaussian"
FilterTypeV[1] = "mitchell"
Filterxwidth = Draw.Create(2.0)
Filterywidth = Draw.Create(2.0)

# samplers
strSamplerType = "Sampler %t | lowdiscrepancy %x0 | random %x1"
SamplerType = Draw.Create(0)
SamplerTypeV = {}
SamplerTypeV[0] = "lowdiscrepancy"
SamplerTypeV[1] = "random"
SamplerProgressive = Draw.Create(1)
SamplerPixelsamples = Draw.Create(4)

# engine
strIntegratorType = "Integrator %t | path %x0"
IntegratorType = Draw.Create(0)
IntegratorTypeV = {}
IntegratorTypeV[0] = "path"
pathRRforcetransmit = Draw.Create(1)
pathRRcontinueprob = Draw.Create(0.5)
pathMaxDepth = Draw.Create(512)

## Exposure
ToneMapScale = Draw.Create(125)
Autoexp = Draw.Create(0)

## Tonemapping
strToneMapType = "ToneMap type %t | None %x0 | Reinhard %x1"
ToneMapType = Draw.Create(1)
ToneMapPreScale = Draw.Create(1.0)
ToneMapPostScale = Draw.Create(1.0)
ToneMapBurn = Draw.Create(6.0)

# output gamma
OutputGamma = Draw.Create(2.2)

# output dither
OutputDither = Draw.Create(0.0)

# file output
SaveEXR = Draw.Create(0)
SaveEXRint = Draw.Create(120)
SaveTGA = Draw.Create(1)
SaveTGAint = Draw.Create(120)
Displayint = Draw.Create(12)

# bloom
Bloom = Draw.Create(0)
BloomWidth = Draw.Create(0.1)
BloomRadius = Draw.Create(0.1)

## Overall color gain
DiffuseGain = Draw.Create(1.00)
SpecularGain = Draw.Create(1.00)

# text color fix
textcol = [0, 0, 0]
geom_pfilename = ""

## Registry
def update_Registry():
	#global EnvFile, EnvMapType
	d = {}
	d['sizex'] = SizeX#.val
	d['sizey'] = SizeY#.val
	d['scalesize'] = ScaleSize.val
	d['colexponent'] = ColExponent.val
	d['filmwidth'] = FilmWidth.val
	d['apertureradius'] = LensRadius.val
	d['focusdistance'] = FocalDistance.val
	d['whitebalance'] = WhiteBalance.val
	d['filmiso'] = FilmIso.val
	d['saveexr'] = SaveEXR.val
	d['saveexrint'] = SaveEXRint.val
	d['savetga'] = SaveTGA.val
	d['savetgaint'] = SaveTGAint.val
	d['savedisplayint'] = Displayint.val
	d['mlt'] = MLT.val
	d['lmp'] = LMP.val
	d['maxchange'] = MaxChange.val
	d['maxnumconsrej'] = MaxNumConsRej.val
	d['bidirectional'] = Bidirectional.val
	d['rrlp'] = RRLP.val
	d['maxdepth'] = MaxDepth.val
	d['stratawidth'] = StrataWidth.val
	d['threads'] = Threads.val
	d['logging'] = Logging.val
	d['groundplane'] = GroundPlane.val
	d['saveutmexr'] = SaveUTMExr.val
	d['savetmexr'] = SaveTMExr.val
	d['tonemaptype'] = ToneMapType.val
	d['tonemapscale'] = ToneMapScale.val
	d['autoexp'] = Autoexp.val
	d['tonemapprescale'] = ToneMapPreScale.val
	d['tonemappostscale'] = ToneMapPostScale.val
	d['outputgamma'] = OutputGamma.val
	d['outputdither'] = OutputDither.val
	d['tonemapburn'] = ToneMapBurn.val
	d['envtype'] = EnvType.val
	d['envfile'] = EnvFile.val
	d['envmaptype'] = EnvMapType.val
	d['envgain'] = EnvGain.val
	d['envwidth'] = EnvWidth.val
	d['turbidity'] = Turbidity.val
	d['skygain'] = SkyGain.val
	d['matfile'] = MatFile.val
	d['ExecuteLux'] = ExecuteLux.val
	d['HaltTime'] = HaltTime.val
	d['frameup'] = FrameUp.val
	d['imagesave'] = ImageSave.val
	d['pfile'] = geom_pfilename

#TODO save/retrieve to registry for samplers, filters and engine (renderer tab contents)

	Blender.Registry.SetKey('BlenderLux', d, True)

rdict = Blender.Registry.GetKey('BlenderLux', True)

if rdict:
	try:
		SizeX.val = rdict['sizex']
		SizeY.val = rdict['sizey']
		ScaleSize.val = rdict['scalesize']
		ColExponent.val = rdict['colexponent']
		FilmWidth.val = rdict['filmwidth']
		LensRadius.val = rdict['apertureradius']
		FocalDistance.val = rdict['focusdistance']
		WhiteBalance.val = rdict['whitebalance']
		FilmIso.val = rdict['filmiso']
		SaveEXR.val = rdict['saveexr']
		SaveEXRint.val = rdict['saveexrint']
		SaveTGA.val = rdict['savetga']
		SaveTGAint.val = rdict['savetgaint']
		Displayint.val = rdict['savedisplayint']
		MLT.val = rdict['mlt'] 
		LMP.val = rdict['lmp']
		MaxChange.val = rdict['maxchange']
		MaxNumConsRej.val = rdict['maxnumconsrej']
		Bidirectional.val = rdict['bidirectional']
		RRLP.val = rdict['rrlp']
		MaxDepth.val = rdict['maxdepth']
		StrataWidth.val = rdict['stratawidth']
		Threads.val = rdict['threads']
		Logging.val = rdict['logging']
		GroundPlane.val = rdict['groundplane']
		SaveUTMExr.val = rdict['saveutmexr']
		SaveTMExr.val = rdict['savetmexr']
		ToneMapType.val = rdict['tonemaptype']
		ToneMapScale.val = rdict['tonemapscale']
		Autoexp.val = rdict['autoexp']
		ToneMapPreScale.val = rdict['tonemapprescale']
		ToneMapPostScale.val = rdict['tonemappostscale']
		OutputGamma.val = rdict['outputgamma']
		OutputDither.val = rdict['outputdither']
		ToneMapBurn.val = rdict['tonemapburn']
		EnvType.val = rdict['envtype']
		EnvFile.val = rdict['envfile']
		EnvMapType.val = rdict['envmaptype']
		EnvGain.val = rdict['envgain']
		EnvWidth.val = rdict['envwidth']
		Turbidity.val = rdict['turbidity']
		SkyGain.val = rdict['skygain']
		MatFile.val = rdict['matfile']
		ExecuteLux.val = rdict['ExecuteLux']
		LuxDir.val = rdict['LuxDir']
		HaltTime.val = rdict['HaltTime']
		FrameUp.val = rdicr['frameup']
		ImageSave.val = rdict['imagesave']
		geom_pfilename = rdict['pfile']
		
	except: update_Registry()   
		
######  Draw Camera  ###############################################
def drawCamera():
####################################################################
	global evtNoEvt, evtExport, evtExportAnim, evtFocusS, evtFocusC, openCamera, openEnv ,evtchangesize
	global SizeX, SizeY, strScaleSize, ScaleSize, ColExponent, RRLP, MaxDepth, Bidirectional, StrataWidth, Logging, LensRadius, FocalDistance, GroundPlane, MatFile, FilmWidth
	global SaveUTMExr, SaveTMExr, ToneMapType, ToneMapScale, Autoexp, ToneMapPreScale, ToneMapPostScale, ToneMapBurn, OutputGamma
	global MLT, LMP, MaxChange, MaxNumConsRej
	global textcol, strEnvType, EnvType, EnvFile, strEnvMapType, EnvMapType, EnvGain, EnvWidth, Turbidity, SkyGain, DiffuseGain, SpecularGain
	global strWhiteBalance, WhiteBalance, whiteBalanceV, Screen, HaltTime, ExecuteLux, FilmIso
   
	drawButtons()
	
	BGL.glColor3f(1.0,0.5,0.4)
	BGL.glRectf(10,182,90,183)
	BGL.glColor3f(0.9,0.9,0.9)


	#ToneMapScale = Draw.Number("Shutter: 1/", evtNoEvt,10, 150, 200, 18, ToneMapScale.val, 1, 8000, "Exposure")
	#Autoexp = Draw.Toggle("AUTO", evtNoEvt, 215, 150, 40, 18, Autoexp.val, "Enable Autoexposure")
	LensRadius = Draw.Number("Lens Radius: ", evtNoEvt, 10, 130, 200, 18, LensRadius.val, 0.0, 3.0, "Defines the lens radius. Values higher than 0. enable DOF and control the amount")
	#FilmIso = Draw.Number("FilmIso", evtNoEvt,10,110,200,18,FilmIso.val,1,100000, "Set FilmIso")
	FocalDistance = Draw.Number("Focal Distance: ", evtNoEvt, 10, 90, 200, 18, FocalDistance.val, 0.0, 100, "Distance from the camera at which objects will be in focus. Has no effect if Lens Radius is 0.")
	Draw.Button("S", evtFocusS, 215, 90, 20, 18, "Get the distance from the selected object")
	Draw.Button("C", evtFocusC, 235, 90, 20, 18, "Get the distance from the 3d cursor")
	#BGL.glRasterPos2i(260,135) ; Draw.Text("White Balance:")
	#WhiteBalance = Draw.Menu(strWhiteBalance, evtNoEvt, 345, 130, 65, 18, WhiteBalance.val, "Set the white_balance (def=D65)")
	
	#FilmWidth = Draw.Number("Film Width: ", evtNoEvt, 260, 150, 150, 18, FilmWidth.val, 0.0, 100.0, "Width of the \"Film.\" in mm.")
	
	BGL.glColor3f(0.9,0.9,0.9) ; BGL.glRasterPos2i(10,65) ; Draw.Text("Size:")
	SizeX = Draw.Number("X: ", evtchangesize, 45, 60, 75, 18, SizeX.val, 1, 4096, "Width of the render")
	SizeY = Draw.Number("Y: ", evtchangesize, 130, 60, 75, 18, SizeY.val, 1, 3072, "Height of the render")
	ScaleSize = Draw.Menu(strScaleSize, evtNoEvt, 210, 60, 65, 18, ScaleSize.val, "Scale Image Size of ...")
	#
##############  Draw Environment  #######################################
def drawEnv():
####################################################
	global evtNoEvt, evtExport, evtExportAnim, evtFocusS, evtFocusC, openCamera, openEnv, evtloadimg
	global SizeX, SizeY, strScaleSize, ScaleSize, ColExponent, RRLP, MaxDepth, Bidirectional, StrataWidth, Logging, LensRadius, FocalDistance, GroundPlane, MatFile, FilmWidth
	global SaveUTMExr, SaveTMExr, ToneMapType, ToneMapScale, Autoexp, ToneMapPreScale, ToneMapPostScale, ToneMapBurn, OutputGamma
	global MLT, LMP, MaxChange, MaxNumConsRej
	global textcol, strEnvType, EnvType, EnvFile, strEnvMapType, EnvMapType, EnvGain, EnvWidth, Turbidity, SkyGain, DiffuseGain, SpecularGain
	global strWhiteBalance, WhiteBalance, whiteBalanceV, Screen, HaltTime, ExecuteLux
	
	drawButtons()
	
	BGL.glColor3f(1.0,0.5,0.4)
	BGL.glRectf(90,182,170,183)
	BGL.glColor3f(0.9,0.9,0.9)
	
	EnvType = Draw.Menu(strEnvType, evtNoEvt, 10, 150, 150, 18, EnvType.val, "Set the Enviroment type")
	if EnvType.val == 2:
		EnvFile = Draw.String("Probe: ", evtNoEvt, 10, 130, 255, 18, EnvFile.val, 50, "the file name of the raw/exr probe")
		EnvMapType = Draw.Menu(strEnvMapType, evtNoEvt, 10, 110, 100, 18, EnvMapType.val, "Set the map type of the probe")
		#EnvGain = Draw.Number("Gain: ", evtNoEvt, 165, 150, 100, 18, EnvGain.val, 0.001, 1000.00, "Gain")
		Draw.Button("Load", evtloadimg, 235, 110, 30,18,"Load Env Map")
		#if EnvMapType.val == 0:
		#	EnvWidth = Draw.Number("Width: ", evtNoEvt, 110, 110, 100, 18, EnvWidth.val, 1, 10000, "Width")
	if EnvType.val == 1:
		Turbidity = Draw.Number("Sky Turbidity", evtNoEvt, 10, 130, 150, 18, Turbidity.val, 1.5, 5.0, "Sky Turbidity")
		SkyGain = Draw.Number("Sky Gain", evtNoEvt, 10, 110, 150, 18, SkyGain.val, 0.0, 5.0, "Sky Gain")
	
	#GroundPlane = Draw.Toggle("Ground Plane", evtNoEvt, 10, 60, 150, 18, GroundPlane.val, "Place infinite large ground plane at 0,0,0")
	
###############  Draw Rendersettings   ###################################
def drawSettings():
#####################################################
	global SizeX, SizeY, strScaleSize, ScaleSize, ColExponent, RRLP, MaxDepth, Bidirectional, StrataWidth, Logging, LensRadius, FocalDistance, GroundPlane, MatFile, FilmWidth
	global SaveUTMExr, SaveTMExr, ToneMapType, ToneMapScale, Autoexp, ToneMapPreScale, ToneMapPostScale, ToneMapBurn, OutputGamma
	global MLT, LMP, MaxChange, MaxNumConsRej, Threads
	global textcol, strEnvType, EnvType, EnvFile, strEnvMapType, EnvMapType, EnvGain, EnvWidth, Turbidity, SkyGain, DiffuseGain, SpecularGain
	global strWhiteBalance, WhiteBalance, whiteBalanceV, Screen, HaltTime, ExecuteLux
	global IntegratorType, SamplerType, FilterType, SamplerProgressive, SamplerPixelsamples, Filterxwidth, Filterywidth
	global pathMaxDepth, pathRRcontinueprob, pathRRforcetransmit


	drawButtons()
	
	BGL.glColor3f(1.0,0.5,0.4)
	BGL.glRectf(170,182,250,183)
	BGL.glColor3f(0.9,0.9,0.9)


	IntegratorType = Draw.Menu(strIntegratorType, evtNoEvt, 10, 150, 120, 18, IntegratorType.val, "Engine Integrator type")
	pathMaxDepth = Draw.Number("Maxdepth:", evtNoEvt,140,150,120,18, pathMaxDepth.val,1,1024, "Maximum path depth (bounces)")
	pathRRcontinueprob = Draw.Number("RRcProb:", evtNoEvt,140,130,140,18, pathRRcontinueprob.val,0.01,1.0, "Russian Roulette continue probability")
	pathRRforcetransmit = Draw.Toggle("RRfTransmit", evtNoEvt, 290, 130, 100, 18, pathRRforcetransmit.val, "Russian Roulette force transmission")

	SamplerType = Draw.Menu(strSamplerType, evtNoEvt, 10, 100, 120, 18, SamplerType.val, "Engine Sampler type")
	SamplerProgressive = Draw.Toggle("Progressive", evtNoEvt, 140, 100, 120, 18, SamplerProgressive.val, "Sample film progressively or linearly")
	SamplerPixelsamples = Draw.Number("Pixelsamples:", evtNoEvt,260,100,150,18, SamplerPixelsamples.val,1,512, "Number of samples per pixel")

	FilterType = Draw.Menu(strFilterType, evtNoEvt, 10, 70, 120, 18, FilterType.val, "Engine pixel reconstruction filter type")
	Filterxwidth = Draw.Number("X width:", evtNoEvt,140,70,120,18, Filterxwidth.val,1,4, "Horizontal filter width")
	Filterywidth = Draw.Number("Y width:", evtNoEvt,260,70,120,18, Filterywidth.val,1,4, "Vertical filter width")


	#BGL.glRasterPos2i(10,165) ; Draw.Text("Metropolis light transport settings")
	#MLT = Draw.Toggle("Metropolis", evtNoEvt, 10, 80, 80, 18, MLT.val, "If pressed, use MLT otherwise use pathtracer")
	#LMP = Draw.Number("LM probability: ", evtNoEvt, 10, 140, 200, 18, LMP.val, 0.0, 1.0, "Probability of using fresh random numbers")
	#MaxChange = Draw.Number("Max Change: ", evtNoEvt, 10, 120, 200, 18, MaxChange.val, 0.0, 1.0, "Maximum mutation size") 
	#MaxNumConsRej = Draw.Number("Max num consec reject:", evtNoEvt, 10, 100, 200, 18, MaxNumConsRej.val, 0, 10000, "The lower the value the more biased the calculation")
	#Threads = Draw.Number("Threads", evtNoEvt, 210,60,200,18, Threads.val,1,4, "Set Threadnumber")
	
	#BGL.glRasterPos2i(210,165) ; Draw.Text("General tracing parameters")
	#Bidirectional = Draw.Toggle("Bidirectional", evtNoEvt, 210, 80, 80, 18, Bidirectional.val, "If pressed, use bidirectional tracing")
	#RRLP = Draw.Number("RR live probability: ", evtNoEvt, 210, 140, 200, 18, RRLP.val, 0.0, 1.0, "Russian roulette live probability")
	#MaxDepth = Draw.Number("Max depth:", evtNoEvt, 210, 120, 200, 18, MaxDepth.val, 1, 10000, "Maximum ray bounce depth")
	#
	#BGL.glRasterPos2i(10,150) ; Draw.Text("Path tracer settings")
	#StrataWidth = Draw.Number("Strata width:", evtNoEvt, 210, 100, 200, 18, StrataWidth.val, 1, 50, "Number of samples per pixel = strata width*strata width")
	#SaveEXR = Draw.Toggle("Save EXR", evtNoEvt, 10, 150, 80, 18, SaveEXR.val, "Save untonemapped EXR file")#

##################  Draw RSettings  #########################	
def drawSystem():
########################################
	global Logging, MatFile
	global SaveUTMExr, SaveTMExr, ColExponent, OutputGamma, SaveEXR, SaveEXRint, SaveTGA, SaveTGAint, Displayint, OutputDither
	global HaltTime, FrameUp, ImageSave
	
	drawButtons()
	
	BGL.glColor3f(1.0,0.5,0.4)
	BGL.glRectf(330,182,410,183)
	BGL.glColor3f(0.9,0.9,0.9)
	
	#Logging = Draw.Toggle("Logging", evtNoEvt, 210, 150, 80, 18, Logging.val, "Write to log.txt if pressed")
	#SaveUTMExr = Draw.Toggle("Save utm EXR", evtNoEvt, 210, 130, 80, 18, SaveUTMExr.val, "Save untonemapped EXR file")
	#SaveTMExr = Draw.Toggle("Save tm EXR", evtNoEvt, 210, 110, 80, 18, SaveTMExr.val, "Save tonemapped EXR file")
	#MatFile = Draw.Toggle("Sep. Materials", evtNoEvt, 210, 90, 80, 18, MatFile.val, "Save all the material settings to a separate file with a \"-materials\" extension")
	

	SaveEXR = Draw.Toggle("Save EXR", evtNoEvt, 10, 150, 80, 18, SaveEXR.val, "Save untonemapped EXR file")
	SaveEXRint = Draw.Number("Interval", evtNoEvt,132,150,150,18, SaveEXRint.val,20,10000, "Set Interval for EXR file write (seconds)")
	SaveTGA = Draw.Toggle("Save TGA", evtNoEvt, 10, 130, 80, 18, SaveTGA.val, "Save tonemapped TGA file")
	SaveTGAint = Draw.Number("Interval", evtNoEvt,132,130,150,18, SaveTGAint.val,20,10000, "Set Interval for TGA file write (seconds)")

	Displayint = Draw.Number("Display Interval", evtNoEvt,10,100,150,18, Displayint.val,5,10000, "Set Interval for Display (seconds)")


	OutputGamma = Draw.Number("Output Gamma: ", evtNoEvt, 10, 60, 200, 18, OutputGamma.val, 0.0, 6.0, "Output Image Gamma")
	OutputDither = Draw.Number("Output Dither: ", evtNoEvt, 210, 60, 150, 18, OutputDither.val, 0.0, 1.0, "Output Image Dither")

	#ColExponent = Draw.Number("Gamma : ", evtNoEvt, 10, 150, 150, 18, ColExponent.val, 0.01, 5.0, "Gamma exponent")
	#HaltTime = Draw.Number("Halt Time :", evtNoEvt, 10, 130, 150, 18, HaltTime.val, -1, 10000, "Set the Halttime in min")  
	#FrameUp = Draw.Number("Frame Upl :", evtNoEvt,10,110,150,18, FrameUp.val,20,10000, "Set Frame Upload Time")	
	#ImageSave = Draw.Number("Image Up", evtNoEvt,10,90,150,18, ImageSave.val,20,10000, "Set Image Uplaod Time")
	
#################  Draw Tonemapping  #########################
def drawTonemap():
#######################################
	global SizeX, SizeY, strScaleSize, ScaleSize, ColExponent, RRLP, MaxDepth, Bidirectional, StrataWidth, Logging, LensRadius, FocalDistance, GroundPlane, MatFile, FilmWidth
	global SaveUTMExr, SaveTMExr, ToneMapType, ToneMapScale, Autoexp, ToneMapPreScale, ToneMapPostScale, ToneMapBurn, OutputGamma
	global MLT, LMP, MaxChange, MaxNumConsRej
	global textcol, strEnvType, EnvType, EnvFile, strEnvMapType, EnvMapType, EnvGain, EnvWidth, Turbidity, SkyGain, DiffuseGain, SpecularGain
	global strWhiteBalance, WhiteBalance, whiteBalanceV, Screen, HaltTime, ExecuteLux, Bloom, BloomWidth, BloomRadius
	
	drawButtons()
	
	BGL.glColor3f(1.0,0.5,0.4)
	BGL.glRectf(250,182,330,183)
	BGL.glColor3f(0.9,0.9,0.9)
	
	ToneMapType = Draw.Menu(strToneMapType, evtNoEvt, 10, 150, 85, 18, ToneMapType.val, "Set the type of the tonemapping")
	
	if ToneMapType.val == 1:
		ToneMapBurn = Draw.Number("Burn: ", evtNoEvt, 95, 150, 135, 18, ToneMapBurn.val, 0.1, 12.0, "12.0: no burn out, 0.1 lot of burn out")
		ToneMapPreScale = Draw.Number("PreS: ", evtNoEvt, 10, 130, 110, 18, ToneMapPreScale.val, 0.01,100.0, "Pre Scale: See Lux Manual ;)")
		ToneMapPostScale = Draw.Number("PostS: ", evtNoEvt, 120, 130, 110, 18, ToneMapPostScale.val, 0.01,100.0, "Post Scale: See Lux Manual ;)")
	
	#

	Bloom = Draw.Toggle("Bloom", evtNoEvt, 10, 80, 80, 18, Bloom.val, "Enable HDR Bloom")
	BloomWidth = Draw.Number("Width: ", evtNoEvt, 90, 80, 120, 18, BloomWidth.val, 0.1, 2.0, "Amount of bloom")
	BloomRadius = Draw.Number("Radius: ", evtNoEvt, 210, 80, 120, 18, BloomRadius.val, 0.1, 2.0, "Radius of the bloom filter")

#	DiffuseGain = Draw.Number("Diffuse gain: ", evtNoEvt, 130, 40, 150, 18, DiffuseGain.val, 0.0, 1.0, "Overall diffuse color gain")
#	SpecularGain = Draw.Number("Specular gain: ", evtNoEvt, 300, 40, 150, 18, SpecularGain.val, 0.0, 1.0, "Overall specular color gain")
	
##############
def drawGUI():
##############

	global Screen
	
	if Screen==0:
		drawCamera()
 	if Screen==1:
 		drawEnv()
	if Screen==2:
		drawSettings()
 	if Screen==3:
   		drawSystem()
 	if Screen==4:
 		drawTonemap()

##################
def drawButtons():
##################

      ## buttons
	global evtExport, evtExportAnim, openCamera, openEnv, openRSet, openSSet, openTmap, evtNoEvt, ExecuteLux, DefaultExport, ExportGeom
	
	BGL.glColor3f(0.2,0.2,0.2)
	BGL.glRectf(0,0,440,230)
	BGL.glColor3f(0.9,0.9,0.9);BGL.glRasterPos2i(10,205) ; Draw.Text("LuxBlend v0.1alpha8", "small")
	
	Draw.Button("Render", evtExport, 10, 19, 100, 30, "Open file dialog and export")
	Draw.Button("Export Anim", evtExportAnim, 112, 19, 100, 30, "Open file dialog and export animation (careful: takes a lot of diskspace!!!)")
	Draw.Button("Camera", openCamera, 10, 185, 80, 13, "Open Camera Dialog")
	Draw.Button("Environment", openEnv, 90, 185, 80, 13, "Open Environment Dialog")
	Draw.Button("Renderer", openRSet, 170, 185, 80, 13, "Open Rendersettings Dialog")
	Draw.Button("Tonemap", openTmap, 250, 185, 80, 13, "open Tonemap Settings")
	Draw.Button("System", openSSet, 330, 185, 80, 13, "open System Settings")
	
	#ExecuteLux = Draw.Toggle("Run", evtNoEvt, 10, 5, 30, 10, ExecuteLux.val, "Execute Lux and render the saved .lxs file")
	#DefaultExport = Draw.Toggle("def",evtNoEvt,40,5,30,10, DefaultExport.val, "Use default.lxs as filename") 
	#ExportGeom = Draw.Toggle("geo",evtNoEvt,70,5,30,10, ExportGeom.val, "Export Geometry")
	
	BGL.glColor3f(0.9, 0.9, 0.9) ; BGL.glRasterPos2i(340,7) ; Draw.Text("Press Q or ESC to quit.", "tiny")
	
		
def event(evt, val):  # function that handles keyboard and mouse events
	if evt == Draw.ESCKEY or evt == Draw.QKEY:
		stop = Draw.PupMenu("OK?%t|Cancel export %x1")
		if stop == 1:
			Draw.Exit()
			return

def setEnvMap(efilename):
	EnvFile.val = os.path.basename(efilename)
	
def buttonEvt(evt):  # function that handles button events
	
	global Screen, EnFile
	
	if evt == evtExport:
		#if DefaultExport == 0:
			Blender.Window.FileSelector(save_still, "Export", newFName('lxs'))
		#else:
		#	datadir=Blender.Get("datadir")
		#	f = open(datadir + '/LuxWrapper.conf', 'r+')
		#	ic=f.readline()
		#	f.close()
		#	filename = ic + "\default.lxs"
		#	save_still(filename)
	
	if evt == evtExportAnim:
		Blender.Window.FileSelector(save_anim, "Export Animation", newFName('lxs'))
	#if there was an event, redraw the window   
	if evt:
		Draw.Redraw()
		
	if evt == evtNoEvt:
		Draw.Redraw()
		update_Registry()
		
	if evt == evtchangesize:
		Scene.GetCurrent().getRenderingContext().imageSizeX(SizeX.val);
		Scene.GetCurrent().getRenderingContext().imageSizeY(SizeY.val);
		Draw.Redraw()
		
	if evt == openCamera:
		Screen = 0
		drawGUI()
		Draw.Redraw()
	
	if evt == openEnv:
		Screen = 1
		drawGUI()
		Draw.Redraw()
		
	if evt == openRSet:
		Screen = 2
		drawGUI()
		Draw.Redraw()
	
	if evt == openSSet:
		Screen = 3
		drawGUI()
		Draw.Redraw()
	
	if evt == openTmap:
		Screen = 4
		drawGUI()
		Draw.Redraw()
	
	if evt == evtloadimg:
		Blender.Window.FileSelector(setEnvMap, "Load Environment Map", ('.exr'))
		Draw.Redraw()
		update_Registry()
	
	if evt == evtFocusS:
		setFocus("S")
		Draw.Redraw()
		update_Registry()
	if evt == evtFocusC:
		setFocus("C")
		Draw.Redraw()
		update_Registry()

def setFocus(target):
	global FocalDistance
	currentscene = Scene.GetCurrent()
	camObj = currentscene.getCurrentCamera()
	loc1 = camObj.getLocation()
	if target == "S":
		selObj = Object.GetSelected()[0]
		try:	
			loc2 = selObj.getLocation()
		except:
			print "select an object to focus\n"
	if target == "C":
		loc2 = Window.GetCursorPos()
	FocalDistance.val = (((loc1[0]-loc2[0])**2)+((loc1[1]-loc2[1])**2)+((loc1[2]-loc2[2])**2))**0.5

Screen = 0
Draw.Register(drawGUI, event, buttonEvt)
