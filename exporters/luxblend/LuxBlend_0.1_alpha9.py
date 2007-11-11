#!BPY
"""Registration info for Blender menus:
Name: 'LuxBlend-v0.1-alpha9...'
Blender: 240
Group: 'Export'
Tooltip: 'Export to LuxRender v0.1 scene format (.lxs)'
"""
#
# ***** BEGIN GPL LICENSE BLOCK *****
#
# --------------------------------------------------------------------------
# LuxBlend v0.1 alpha9 exporter
# --------------------------------------------------------------------------
#
# Authors:
# radiance, zuegs, ideasman42, luxblender
#
# Based on:
# * Indigo exporter 
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
from Blender import Mesh, Scene, Object, Material, Texture, Window, sys, Draw, BGL, Mathutils


######################################################
# Functions
######################################################

# New name based on old with a different extension
def newFName(ext):
	return Blender.Get('filename')[: -len(Blender.Get('filename').split('.', -1)[-1]) ] + ext

def luxMatrix(matrix):
	ostr = ""
	ostr += "Transform [%s %s %s %s  %s %s %s %s  %s %s %s %s  %s %s %s %s]\n"\
		%(matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3],\
		  matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3],\
		  matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3],\
		  matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]) 
	return ostr

# use this because some data dosnt have both .materials and .getMaterials()
def dataMaterials(data):
	try:
		return data.materials
	except:
		try:
			return data.getMaterials()
		except:
			return []
	
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
		if(mat.getTranslucency() < 1.0):
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
	# to lux microfacet shinyness value (0.00001 = nearly specular - 1.0 = diffuse)
	def HardtoMicro(hard):
		micro = 1.0 / (float(hard) *5)
		if( micro < 0.00001 ):
			micro = 0.00001
		if( micro > 1.0 ):
			micro = 1.0
		return micro

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
		str += write_float_param( mat, "roughness", 'HARD', HardtoMicro(mat.hard) )
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
		str += write_float_param( mat, "roughness", 'HARD', HardtoMicro(mat.hard) )
		str += write_float_param( mat, "bumpmap", 'NOR', 0.0 )

	elif (mat_type == 4):
		### 'shinymetal' material ### COMPLETE
		str += "# Type: 'shinymetal'\n"
		str += write_color_param( mat, "Ks", 'COL', mat.R, mat.G, mat.B )
		str += write_color_param( mat, "Kr", 'SPEC',mat.specR * mat.getSpec(), mat.specG * mat.getSpec(), mat.specB * mat.getSpec() )
		str += write_float_param( mat, "roughness", 'HARD', HardtoMicro(mat.hard) )
		str += write_float_param( mat, "bumpmap", 'NOR', 0.0 )

	elif (mat_type == 5):
		### 'substrate' material ### COMPLETE
		str += "# Type: 'substrate'\n"
		str += write_color_param( mat, "Kd", 'COL', mat.R, mat.G, mat.B )
		str += write_color_param( mat, "Ks", 'SPEC', mat.specR * mat.getSpec(), mat.specG * mat.getSpec(), mat.specB * mat.getSpec() )
		# TODO: add different inputs for both u/v roughenss for aniso effect
		str += write_float_param( mat, "uroughness", 'HARD', HardtoMicro(mat.hard) )
		str += write_float_param( mat, "vroughness", 'HARD', HardtoMicro(mat.hard) )
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
		materials = dataMaterials(obj.getData(mesh=1))
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
	materials = mesh.materials

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
		have_uvs = mesh.faceUV
		materials_names = [material.name for material in materials]
		
		print "obstart\n"
		for material in materials:
		# Construct list of verts/faces
			material_name = material.name
			vdata = []
			vlist = []
			flist = []

			flist.append([])
			have_faces = 0

			for face in mesh.faces:
				iis = [-1, -1, -1, -1]
				if( material_name == materials_names[face.mat] ):	
					if have_uvs:
						face_uv = face.uv
					
					if not face.smooth:
						normal = face.no
					
					for vi, vert in enumerate(face):
						if face.smooth:
							normal = vert.no
						
						if have_uvs:
							uv = face_uv[vi]
						else:
							uv = []
						iis[vi] = addVertex(vert.index, vert.co, normal, uv)
					addFace(0, iis[0], iis[1], iis[2])
					have_faces = 1
					if len(face.v)==4:
						addFace(0, iis[2], iis[3], iis[0])
		
			if have_faces:
				ostr += writeGroup(material)
				
			# clear lists
			vdata[:] = []
			vlist[:] = []
			flist[:] = []
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
		str += """# Object: "%s" Mesh: "%s"\n""" % (objectname, obj.getData(name_only=1))
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
			#mesh = Blender.NMesh.GetRawFromObject(objectname)
			mesh = Mesh.New()
			mesh.getFromObject(obj)
			meshname = obj.getData(name_only=1)

			if (objecttype == "Curve") or (objecttype == "Text"):
				meshname = obj.getData(name_only=1)
				mesh.name = meshname
				for f in mesh.faces:
					f.smooth = 1

			# Not needed anymore, Mesh does this, NMesh didnt
			#if (objecttype == "Curve"):
			#	mesh.setMaterials(obj.getData(mesh=1).getMaterials())

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
	file.write("# Exported by LuxBlend_01_alpha9\n")

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
	if(SaveIGI.val == 1):
		file.write("	 \"string igi_filename\" [\"out.igi\"]\n")
		file.write("	 	\"integer igi_writeinterval\" [%i]\n" %(SaveIGIint.val))
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

	if(pathMetropolis.val == 1):
		file.write("\"bool metropolis\" [\"true\"] ")
	else:
		file.write("\"bool metropolis\" [\"false\"] ")

	file.write("\"integer maxconsecrejects\" [%f] " %(pathMetropolisMaxRejects.val))
	file.write("\"float largemutationprob\" [%f] " %(pathMetropolisLMProb.val))

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
	for obj in currentscene.objects:
		if obj.Layers & currentscene.Layers:
			objecttype = obj.getType()
			if (objecttype == "Mesh") and (len(obj.getData(mesh=1).materials) > 0)\
					and (obj.getData(mesh=1).materials[0].name.upper() == "PORTAL"):
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
###	 LAUNCH LuxRender AND RENDER CURRENT SCENE (WINDOWS ONLY)
#########################################################################

def launchLux(filename):
	# get blenders 'bpydata' directory
	#datadir=Blender.Get("datadir")
	
	# open 'LuxWrapper.conf' and read the first line
	#f = open(datadir + '/LuxWrapper.conf', 'r+')
	#ic=f.readline()
	#f.close()
	
	# create 'LuxWrapper.cmd' and write two lines of code into it
	#f = open(datadir + "\LuxWrapper.cmd", 'w')
	#f.write("cd /d " + ic + "\n")
	#f.write("start /b /belownormal Lux.exe %1 -t " + str(Threads.val) + "\n")
	#f.close()
	
	# call external shell script to start Lux
	#cmd= "\"" + datadir + "\LuxWrapper.cmd " + filename + "\""
	cmd= "luxrender \"" + filename + "\""
	print("Running Luxrender:\n"+cmd)
	os.system(cmd)

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
	if ExecuteLux.val == 1:
		launchLux(filename)


######################################################
# Settings GUI
######################################################

ExecuteLux = Draw.Create(0)
DefaultExport = Draw.Create(0)

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
sceneSizeX = Scene.GetCurrent().getRenderingContext().imageSizeX()
sceneSizeY = Scene.GetCurrent().getRenderingContext().imageSizeY()
SizeX = Draw.Create(sceneSizeX)
SizeY = Draw.Create(sceneSizeY)
strScaleSize = "Scale Size %t | 100 % %x100 | 75 % %x75 | 50 % %x50 | 25 % %x25"
ScaleSize = Draw.Create(100)

#ExpotGeom
ExportGeom = Draw.Create(1)
geom_pfilename = ""

# lens radius/focal distance
LensRadius = Draw.Create(0.0)
FocalDistance = Draw.Create(2.0)

## Environment Type
strEnvType = "Env Type %t | Background Color %x0 | Physical Sky %x1 | Texture Map %x2 | None %x3"
EnvType = Draw.Create(0)

##  <turbidity>2.0</turbidity>
Turbidity = Draw.Create(2.0)

##  <sky_gain>2.0</sky_gain>
SkyGain = Draw.Create(0.005)

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
pathMetropolis = Draw.Create(1)
pathMetropolisMaxRejects = Draw.Create(128)
pathMetropolisLMProb = Draw.Create(0.25)
pathRRforcetransmit = Draw.Create(0)
pathRRcontinueprob = Draw.Create(0.5)
pathMaxDepth = Draw.Create(512)

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
SaveIGI = Draw.Create(0)
SaveIGIint = Draw.Create(120)
SaveTGA = Draw.Create(1)
SaveTGAint = Draw.Create(120)
Displayint = Draw.Create(12)

# bloom
Bloom = Draw.Create(0)
BloomWidth = Draw.Create(0.1)
BloomRadius = Draw.Create(0.1)

# text color fix
textcol = [0, 0, 0]
geom_pfilename = ""

## Registry
def update_Registry():
	d = {}
	d['sizex'] = SizeX#.val
	d['sizey'] = SizeY#.val
	d['scalesize'] = ScaleSize.val
	d['apertureradius'] = LensRadius.val
	d['focusdistance'] = FocalDistance.val
	d['saveexr'] = SaveEXR.val
	d['saveexrint'] = SaveEXRint.val
	d['saveigi'] = SaveIGI.val
	d['saveigiint'] = SaveIGIint.val
	d['savetga'] = SaveTGA.val
	d['savetgaint'] = SaveTGAint.val
	d['savedisplayint'] = Displayint.val
	d['tonemaptype'] = ToneMapType.val
	d['tonemapprescale'] = ToneMapPreScale.val
	d['tonemappostscale'] = ToneMapPostScale.val
	d['outputgamma'] = OutputGamma.val
	d['outputdither'] = OutputDither.val
	d['tonemapburn'] = ToneMapBurn.val
	d['envtype'] = EnvType.val
	d['envfile'] = EnvFile.val
	d['envmaptype'] = EnvMapType.val
	d['envgain'] = EnvGain.val
	d['turbidity'] = Turbidity.val
	d['skygain'] = SkyGain.val
	d['ExecuteLux'] = ExecuteLux.val

#TODO save/retrieve to registry for samplers, filters and engine (renderer tab contents)

	Blender.Registry.SetKey('BlenderLux', d, True)
rdict = Blender.Registry.GetKey('BlenderLux', True)

if rdict:
	try:
		SizeX.val = rdict['sizex']
		SizeY.val = rdict['sizey']
		ScaleSize.val = rdict['scalesize']
		FilmWidth.val = rdict['filmwidth']
		LensRadius.val = rdict['apertureradius']
		FocalDistance.val = rdict['focusdistance']
		SaveEXR.val = rdict['saveexr']
		SaveEXRint.val = rdict['saveexrint']
		SaveIGI.val = rdict['saveigi']
		SaveIGIint.val = rdict['saveigiint']
		SaveTGA.val = rdict['savetga']
		SaveTGAint.val = rdict['savetgaint']
		Displayint.val = rdict['savedisplayint']
		ToneMapType.val = rdict['tonemaptype']
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
		ExecuteLux.val = rdict['ExecuteLux']
		
	except: update_Registry()   
		
######  Draw Camera  ###############################################
def drawCamera():
	global evtNoEvt, evtExport, evtExportAnim, evtFocusS, evtFocusC, openCamera, openEnv ,evtchangesize, Screen
	global SizeX, SizeY, strScaleSize, ScaleSize, LensRadius, FocalDistance
	global ToneMapType, ToneMapPreScale, ToneMapPostScale, ToneMapBurn, OutputGamma
	global textcol, strEnvType, EnvType, EnvFile, strEnvMapType, EnvMapType, EnvGain, Turbidity, SkyGain
	global ExecuteLux
   
	drawButtons()
	
	BGL.glColor3f(1.0,0.5,0.4)
	BGL.glRectf(10,182,90,183)
	BGL.glColor3f(0.9,0.9,0.9)

	LensRadius = Draw.Number("Lens Radius: ", evtNoEvt, 10, 130, 200, 18, LensRadius.val, 0.0, 3.0, "Defines the lens radius. Values higher than 0. enable DOF and control the amount")
	FocalDistance = Draw.Number("Focal Distance: ", evtNoEvt, 10, 90, 200, 18, FocalDistance.val, 0.0, 100, "Distance from the camera at which objects will be in focus. Has no effect if Lens Radius is 0.")
	Draw.Button("S", evtFocusS, 215, 90, 20, 18, "Get the distance from the selected object")
	Draw.Button("C", evtFocusC, 235, 90, 20, 18, "Get the distance from the 3d cursor")

	BGL.glColor3f(0.9,0.9,0.9) ; BGL.glRasterPos2i(10,65) ; Draw.Text("Size:")
	SizeX = Draw.Number("X: ", evtchangesize, 45, 60, 75, 18, SizeX.val, 1, 4096, "Width of the render")
	SizeY = Draw.Number("Y: ", evtchangesize, 130, 60, 75, 18, SizeY.val, 1, 3072, "Height of the render")
	ScaleSize = Draw.Menu(strScaleSize, evtNoEvt, 210, 60, 65, 18, ScaleSize.val, "Scale Image Size of ...")

##############  Draw Environment  #######################################
def drawEnv():
	global evtNoEvt, evtExport, evtExportAnim, evtFocusS, evtFocusC, openCamera, openEnv, evtloadimg
	global SizeX, SizeY, strScaleSize, ScaleSize, LensRadius, FocalDistance
	global ToneMapType, ToneMapPreScale, ToneMapPostScale, ToneMapBurn, OutputGamma
	global textcol, strEnvType, EnvType, EnvFile, strEnvMapType, EnvMapType, EnvGain, Turbidity, SkyGain
	global Screen, ExecuteLux
	
	drawButtons()
	
	BGL.glColor3f(1.0,0.5,0.4)
	BGL.glRectf(90,182,170,183)
	BGL.glColor3f(0.9,0.9,0.9)
	
	EnvType = Draw.Menu(strEnvType, evtNoEvt, 10, 150, 150, 18, EnvType.val, "Set the Enviroment type")
	if EnvType.val == 2:
		EnvFile = Draw.String("Probe: ", evtNoEvt, 10, 130, 255, 18, EnvFile.val, 50, "the file name of the EXR latlong map")
		EnvMapType = Draw.Menu(strEnvMapType, evtNoEvt, 10, 110, 100, 18, EnvMapType.val, "Set the map type of the probe")
		Draw.Button("Load", evtloadimg, 235, 110, 30,18,"Load Env Map")
	if EnvType.val == 1:
		Turbidity = Draw.Number("Sky Turbidity", evtNoEvt, 10, 130, 150, 18, Turbidity.val, 1.5, 5.0, "Sky Turbidity")
		SkyGain = Draw.Number("Sky Gain", evtNoEvt, 10, 110, 150, 18, SkyGain.val, 0.0, 5.0, "Sky Gain")
	
###############  Draw Rendersettings   ###################################
def drawSettings():
	global SizeX, SizeY, strScaleSize, ScaleSize, LensRadius, FocalDistance
	global ToneMapType, ToneMapPreScale, ToneMapPostScale, ToneMapBurn, OutputGamma
	global textcol, strEnvType, EnvType, EnvFile, strEnvMapType, EnvMapType, EnvGain, Turbidity, SkyGain
	global Screen, ExecuteLux
	global IntegratorType, SamplerType, FilterType, SamplerProgressive, SamplerPixelsamples, Filterxwidth, Filterywidth
	global pathMaxDepth, pathRRcontinueprob, pathRRforcetransmit, pathMetropolis, pathMetropolisMaxRejects, pathMetropolisLMProb

	drawButtons()
	
	BGL.glColor3f(1.0,0.5,0.4)
	BGL.glRectf(170,182,250,183)
	BGL.glColor3f(0.9,0.9,0.9)

	IntegratorType = Draw.Menu(strIntegratorType, evtNoEvt, 10, 150, 100, 18, IntegratorType.val, "Engine Integrator type")
	pathMetropolis =  Draw.Toggle("MLT", evtNoEvt, 120, 150, 60, 18, pathMetropolis.val, "use Metropolis Light Transport")
	pathMetropolisMaxRejects = Draw.Number("MaxRejects:", evtNoEvt,180,150,120,18, pathMetropolisMaxRejects.val,1,1024, "Maximum nr of consecutive rejections for Metropolis accept")
	pathMetropolisLMProb = Draw.Number("LMprob:", evtNoEvt,300,150,120,18, pathMetropolisLMProb.val,0,1, "Probability of using a large mutation for Metropolis")
	pathMaxDepth = Draw.Number("Maxdepth:", evtNoEvt,120,130,120,18, pathMaxDepth.val,1,1024, "Maximum path depth (bounces)")
	pathRRcontinueprob = Draw.Number("RRprob:", evtNoEvt,240,130,120,18, pathRRcontinueprob.val,0.01,1.0, "Russian Roulette continue probability")
	pathRRforcetransmit = Draw.Toggle("RRfTrans", evtNoEvt, 360, 130, 70, 18, pathRRforcetransmit.val, "Russian Roulette force transmission")

	SamplerType = Draw.Menu(strSamplerType, evtNoEvt, 10, 100, 120, 18, SamplerType.val, "Engine Sampler type")
	SamplerProgressive = Draw.Toggle("Progressive", evtNoEvt, 140, 100, 120, 18, SamplerProgressive.val, "Sample film progressively or linearly")
	SamplerPixelsamples = Draw.Number("Pixelsamples:", evtNoEvt,260,100,150,18, SamplerPixelsamples.val,1,512, "Number of samples per pixel")

	FilterType = Draw.Menu(strFilterType, evtNoEvt, 10, 70, 120, 18, FilterType.val, "Engine pixel reconstruction filter type")
	Filterxwidth = Draw.Number("X width:", evtNoEvt,140,70,120,18, Filterxwidth.val,1,4, "Horizontal filter width")
	Filterywidth = Draw.Number("Y width:", evtNoEvt,260,70,120,18, Filterywidth.val,1,4, "Vertical filter width")

##################  Draw RSettings  #########################	
def drawSystem():
	global OutputGamma, SaveEXR, SaveEXRint, SaveIGI, SaveIGIint, SaveTGA, SaveTGAint, Displayint, OutputDither
	
	drawButtons()
	
	BGL.glColor3f(1.0,0.5,0.4)
	BGL.glRectf(330,182,410,183)
	BGL.glColor3f(0.9,0.9,0.9)
	
	SaveIGI = Draw.Toggle("Save IGI", evtNoEvt, 10, 160, 80, 18, SaveIGI.val, "Save untonemapped IGI file")
	SaveIGIint = Draw.Number("Interval", evtNoEvt,132,160,150,18, SaveIGIint.val,20,10000, "Set Interval for IGI file write (seconds)")
	SaveEXR = Draw.Toggle("Save EXR", evtNoEvt, 10, 140, 80, 18, SaveEXR.val, "Save untonemapped EXR file")
	SaveEXRint = Draw.Number("Interval", evtNoEvt,132,140,150,18, SaveEXRint.val,20,10000, "Set Interval for EXR file write (seconds)")
	SaveTGA = Draw.Toggle("Save TGA", evtNoEvt, 10, 120, 80, 18, SaveTGA.val, "Save tonemapped TGA file")
	SaveTGAint = Draw.Number("Interval", evtNoEvt,132,120,150,18, SaveTGAint.val,20,10000, "Set Interval for TGA file write (seconds)")

	Displayint = Draw.Number("Display Interval", evtNoEvt,10,90,150,18, Displayint.val,5,10000, "Set Interval for Display (seconds)")

	OutputGamma = Draw.Number("Output Gamma: ", evtNoEvt, 10, 60, 200, 18, OutputGamma.val, 0.0, 6.0, "Output Image Gamma")
	OutputDither = Draw.Number("Output Dither: ", evtNoEvt, 210, 60, 150, 18, OutputDither.val, 0.0, 1.0, "Output Image Dither")

#################  Draw Tonemapping  #########################
def drawTonemap():
	global SizeX, SizeY, strScaleSize, ScaleSize, LensRadius, FocalDistance
	global ToneMapType, ToneMapPreScale, ToneMapPostScale, ToneMapBurn, OutputGamma
	global textcol, strEnvType, EnvType, EnvFile, strEnvMapType, EnvMapType, EnvGain, Turbidity, SkyGain
	global Screen, ExecuteLux, Bloom, BloomWidth, BloomRadius
	
	drawButtons()
	
	BGL.glColor3f(1.0,0.5,0.4)
	BGL.glRectf(250,182,330,183)
	BGL.glColor3f(0.9,0.9,0.9)
	
	ToneMapType = Draw.Menu(strToneMapType, evtNoEvt, 10, 150, 85, 18, ToneMapType.val, "Set the type of the tonemapping")
	
	if ToneMapType.val == 1:
		ToneMapBurn = Draw.Number("Burn: ", evtNoEvt, 95, 150, 135, 18, ToneMapBurn.val, 0.1, 12.0, "12.0: no burn out, 0.1 lot of burn out")
		ToneMapPreScale = Draw.Number("PreS: ", evtNoEvt, 10, 130, 110, 18, ToneMapPreScale.val, 0.01,100.0, "Pre Scale: See Lux Manual ;)")
		ToneMapPostScale = Draw.Number("PostS: ", evtNoEvt, 120, 130, 110, 18, ToneMapPostScale.val, 0.01,100.0, "Post Scale: See Lux Manual ;)")
	
	Bloom = Draw.Toggle("Bloom", evtNoEvt, 10, 80, 80, 18, Bloom.val, "Enable HDR Bloom")
	BloomWidth = Draw.Number("Width: ", evtNoEvt, 90, 80, 120, 18, BloomWidth.val, 0.1, 2.0, "Amount of bloom")
	BloomRadius = Draw.Number("Radius: ", evtNoEvt, 210, 80, 120, 18, BloomRadius.val, 0.1, 2.0, "Radius of the bloom filter")
	
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
	BGL.glColor3f(0.9,0.9,0.9);BGL.glRasterPos2i(10,205) ; Draw.Text("LuxBlend v0.1alpha9", "small")
	
	Draw.Button("Render", evtExport, 10, 19, 100, 30, "Open file dialog and export")
	Draw.Button("Export Anim", evtExportAnim, 112, 19, 100, 30, "Open file dialog and export animation (careful: takes a lot of diskspace!!!)")
	Draw.Button("Camera", openCamera, 10, 185, 80, 13, "Open Camera Dialog")
	Draw.Button("Environment", openEnv, 90, 185, 80, 13, "Open Environment Dialog")
	Draw.Button("Renderer", openRSet, 170, 185, 80, 13, "Open Rendersettings Dialog")
	Draw.Button("Tonemap", openTmap, 250, 185, 80, 13, "open Tonemap Settings")
	Draw.Button("System", openSSet, 330, 185, 80, 13, "open System Settings")
	
	ExecuteLux = Draw.Toggle("Run", evtNoEvt, 10, 5, 30, 10, ExecuteLux.val, "Execute Lux after saving")
	DefaultExport = Draw.Toggle("def",evtNoEvt,40,5,30,10, DefaultExport.val, "Save as default.lxs to a temporary directory")
	
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
		if DefaultExport == 0:
			Blender.Window.FileSelector(save_still, "Export", newFName('lxs'))
		else:
			datadir=Blender.Get("datadir")
		#	f = open(datadir + '/LuxWrapper.conf', 'r+')
		#	ic=f.readline()
		#	f.close()
		#	filename = ic + "\default.lxs"
			filename = datadir + "/default.lxs"
			save_still(filename)
	
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
