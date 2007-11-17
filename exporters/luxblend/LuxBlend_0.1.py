#!BPY
"""Registration info for Blender menus:
Name: 'LuxBlend-v0.1-alpha12...'
Blender: 240
Group: 'Export'
Tooltip: 'Export to LuxRender v0.1 scene format (.lxs)'
"""
#
# ***** BEGIN GPL LICENSE BLOCK *****
#
# --------------------------------------------------------------------------
# LuxBlend v0.1 alpha12 exporter
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


def getMaterials(obj): # retrives materials from object or data dependent of obj.colbits
	mats = [None]*16
	colbits = obj.colbits
	objMats = obj.getMaterials(1)
	data = obj.getData()
	try:
		dataMats = data.materials
	except:
		try:
			dataMats = data.getMaterials(1)
		except:
			dataMats = []
			colbits = 0xffff
	m = max(len(objMats), len(dataMats))
	objMats.extend([None]*16)
	dataMats.extend([None]*16)
	for i in range(m):
		if (colbits & (1<<i) > 0):
			mats[i] = objMats[i]
		else:
			mats[i] = dataMats[i]
	return mats

	
#################### Export Material Texture ###

# MATERIAL TYPES enum
# 0 = 'glass'
# 1 = 'roughglass'
# 2 = 'mirror'
# 3 = 'plastic'
# 4 = 'shinymetal'
# 5 = 'substrate'
# 6 = 'matte' (Oren-Nayar)
# 7 = 'matte' (Lambertian)

# 8 = 'metal'
metal_names = [None]*100
metal_names[0] = "ALUMINIUM"
metal_names[1] = "AMORPHOUS_CARBON"
metal_names[2] = "SILVER"
metal_names[3] = "GOLD"
metal_names[4] = "COBALT"
metal_names[5] = "COPPER"
metal_names[6] = "CHROMIUM"
metal_names[7] = "LITHIUM"
metal_names[8] = "MERCURY"
metal_names[9] = "NICKEL"
metal_names[10] = "POTASSIUM"
metal_names[11] = "PLATIUM"
metal_names[12] = "IRIDIUM"
metal_names[13] = "SILICON"
metal_names[14] = "AMORPHOUS_SILICON"
metal_names[15] = "SODIUM"
metal_names[16] = "RHODIUM"
metal_names[17] = "TUNGSTEN"
metal_names[18] = "VANADIUM"

# 9 = 'carpaint'
carpaint_names = [None]*100
carpaint_names[0] = "CAR_FORD_F8"
carpaint_names[1] = "CAR_POLARIS_SILBER"
carpaint_names[2] = "CAR_OPEL_TITAN"
carpaint_names[3] = "CAR_BMW339"
carpaint_names[4] = "CAR_2K_ACRYLACK"
carpaint_names[5] = "CAR_WHITE"
carpaint_names[6] = "CAR_BLUE"
carpaint_names[7] = "CAR_BLUE_MATTE"


def makeCarpaintName(name):
	if (name == carpaint_names[0]):
		return "ford f8"
	if (name == carpaint_names[1]):
		return "polaris silber"
	if (name == carpaint_names[2]):
		return "opel titan"
	if (name == carpaint_names[3]):
		return "bmw339"
	if (name == carpaint_names[4]):
		return "2k acrylack"
	if (name == carpaint_names[5]):
		return "white"
	if (name == carpaint_names[6]):
		return "blue"
	if (name == carpaint_names[7]):
		return "blue matte"

def makeMetalName(name):
	if (name == metal_names[0]):
		return "aluminum"
	if (name == metal_names[1]):
		return "amorphous carbon"
	if (name == metal_names[2]):
		return "silver"
	if (name == metal_names[3]):
		return "gold"
	if (name == metal_names[4]):
		return "cobalt"
	if (name == metal_names[5]):
		return "copper"
	if (name == metal_names[6]):
		return "chromium"
	if (name == metal_names[7]):
		return "lithium"
	if (name == metal_names[8]):
		return "mercury"
	if (name == metal_names[9]):
		return "nickel"
	if (name == metal_names[10]):
		return "potassium"
	if (name == metal_names[11]):
		return "platium"
	if (name == metal_names[12]):
		return "iridium"
	if (name == metal_names[13]):
		return "silicon"
	if (name == metal_names[14]):
		return "amorphous silicon"
	if (name == metal_names[15]):
		return "sodium"
	if (name == metal_names[16]):
		return "rhodium"
	if (name == metal_names[17]):
		return "tungsten"
	if (name == metal_names[18]):
		return "vanadium"

### Determine the type of material ###
def getMaterialType(mat):
	# default to matte (Lambertian)
	mat_type = 7

	# test for metal material
	if (mat.name in metal_names):
		mat_type = 8
		return mat_type

	# test for carpaint material
	if (mat.name in carpaint_names):
		mat_type = 9
		return mat_type

	# test for emissive material
	if (mat.emit > 0):
		### emitter material ###
		mat_type = 100
		return mat_type

	# test for Transmissive (transparent) Materials
	elif (mat.mode & Material.Modes.RAYTRANSP):
		if(mat.getTranslucency() < 1.0):
			### 'glass' material ###
			mat_type = 0	
		else:
			### 'roughglass' material ###
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
# 1 = 'roughglass'
# 2 = 'mirror'
# 3 = 'plastic'
# 4 = 'shinymetal'
# 5 = 'substrate'
# 6 = 'matte' (Oren-Nayar)
# 7 = 'matte' (Lambertian)
# 8 = 'metal'
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
		### 'roughglass' material ###
		str += "# Type: 'roughglass'\n"
		str += write_color_param( mat, "Kr", 'SPEC', mat.specR, mat.specG, mat.specB )
		str += write_color_param( mat, "Kt", 'COL', mat.R, mat.G, mat.B )
		str += write_float_param( mat, "uroughness", 'HARD', HardtoMicro(mat.hard) )
		str += write_float_param( mat, "vroughness", 'HARD', HardtoMicro(mat.hard) )
		str += write_float_param( mat, "index", 'REF', mat.IOR )
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

	elif (mat_type == 8):
		### 'metal' material ### COMPLETE
		str += "# Type: 'metal'\n"
		str += write_float_param( mat, "roughness", 'HARD', HardtoMicro(mat.hard) )
		str += write_float_param( mat, "bumpmap", 'NOR', 0.0 )

	elif (mat_type == 9):
		### 'carpaint' material ### COMPLETE
		str += "# Type: 'carpaint'\n"
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
			### 'roughglass' material ###
			str += " \"roughglass\" \"texture Kr\" \"Kr-%s\"" %mat.name
			str += " \"texture Kt\" \"Kt-%s\"" %mat.name
			str += " \"texture uroughness\" \"uroughness-%s\"" %mat.name
			str += " \"texture vroughness\" \"vroughness-%s\"" %mat.name
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

		elif (mat_type == 8):
			### 'metal' material ### COMPLETE
			metalname = makeMetalName(mat.name)
			str += " \"metal\" \"string name\" \"%s\"" %metalname
			str += " \"texture roughness\" \"roughness-%s\"" %mat.name
			str += " \"texture bumpmap\" \"bumpmap-%s\"" %mat.name

		elif (mat_type == 9):
			### 'carpaint' material ### COMPLETE
			carpaintname = makeCarpaintName(mat.name)
			str += " \"carpaint\" \"string name\" \"%s\"" %carpaintname
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


################################################################




######################################################
# luxExport class
######################################################
class luxExport:
	#-------------------------------------------------
	# __init__
	# initializes the exporter object
	#-------------------------------------------------
	def __init__(self, scene):
		self.scene = scene
		self.camera = scene.objects.camera
		self.objects = []
		self.portals = []
		self.meshes = {}
		self.materials = []

	#-------------------------------------------------
	# analyseObject(self, obj, matrix, name)
	# called by analyseScene to build the lists before export
	#-------------------------------------------------
	def analyseObject(self, obj, matrix, name):
		if (obj.users > 0):
			obj_type = obj.getType()
			if (obj.enableDupGroup or obj.enableDupVerts):
				for o, m in obj.DupObjects:
					self.analyseObject(o, m, "%s.%s"%(name, o.getName()))	
			elif (obj_type == "Mesh") or (obj_type == "Surf") or (obj_type == "Curve") or (obj_type == "Text"):
				mesh_name = obj.getData(name_only=True)
				try:
					self.meshes[mesh_name] += [obj]
				except KeyError:
					self.meshes[mesh_name] = [obj]				
				mats = getMaterials(obj)
				if (len(mats)>0) and (mats[0]!=None):
					if (mats[0].name == "PORTAL"):
						self.portals.append([obj, matrix])
					else:
						self.objects.append([obj, matrix])
						for mat in mats:
							if (mat!=None) and (mat not in self.materials):
								self.materials.append(mat)
				else:
					print "WARNING: object \"%s\" has no materials assigned"%(obj.getName())

	#-------------------------------------------------
	# analyseScene(self)
	# this function builds the lists of object, lights, meshes and materials before export
	#-------------------------------------------------
	def analyseScene(self):
		for obj in self.scene.objects:
			if ((obj.Layers & self.scene.Layers) > 0):
				self.analyseObject(obj, obj.getMatrix(), obj.getName())

	#-------------------------------------------------
	# exportMaterialLink(self, file, mat)
	# exports material link. LuxRender "Material" 
	#-------------------------------------------------
	def exportMaterialLink(self, file, mat):
		file.write("\t%s"%exportMaterialGeomTag(mat)) # use original methode

	#-------------------------------------------------
	# exportMaterial(self, file, mat)
	# exports material. LuxRender "Texture" 
	#-------------------------------------------------
	def exportMaterial(self, file, mat):
		file.write("\t%s"%exportMaterial(mat)) # use original methode		
	
	#-------------------------------------------------
	# exportMaterials(self, file)
	# exports materials to the file
	#-------------------------------------------------
	def exportMaterials(self, file):
		for mat in self.materials:
			print "material %s"%(mat.getName())
			self.exportMaterial(file, mat)

	#-------------------------------------------------
	# exportMesh(self, file, mesh, mats, name, portal)
	# exports mesh to the file without any optimization
	#-------------------------------------------------
	def exportMesh(self, file, mesh, mats, name, portal=False):
		for matIndex in range(len(mats)):
			if (mats[matIndex] != None):
				if (portal):
					file.write("\tShape \"trianglemesh\" \"integer indices\" [\n")
				else:
					self.exportMaterialLink(file, mats[matIndex])
					file.write("\tPortalShape \"trianglemesh\" \"integer indices\" [\n")
				index = 0
				for face in mesh.faces:
					if (face.mat == matIndex):
						file.write("%d %d %d\n"%(index, index+1, index+2))
						if (len(face.verts)==4):
							file.write("%d %d %d\n"%(index, index+2, index+3))
						index += len(face.verts)
				file.write("\t] \"point P\" [\n");
				for face in mesh.faces:
					if (face.mat == matIndex):
						for vertex in face.verts:
							file.write("%f %f %f\n"%(vertex.co[0], vertex.co[1], vertex.co[2]))
				file.write("\t] \"normal N\" [\n")
				for face in mesh.faces:
					if (face.mat == matIndex):
						normal = face.no
						for vertex in face.verts:
							if (face.smooth):
								normal = vertex.no
							file.write("%f %f %f\n"%(normal[0], normal[1], normal[2]))
				if (mesh.faceUV):
					file.write("\t] \"float uv\" [\n")
					for face in mesh.faces:
						if (face.mat == matIndex):
							for uv in face.uv:
								file.write("%f %f\n"%(uv[0], uv[1]))
				file.write("\t]\n")

	#-------------------------------------------------
	# exportMeshOpt(self, file, mesh, mats, name, portal, optNormals)
	# exports mesh to the file with optimization.
	# portal: export without normals and UVs
	# optNormals: speed and filesize optimization, flat faces get exported without normals
	#-------------------------------------------------
	def exportMeshOpt(self, file, mesh, mats, name, portal=False, optNormals=True):
		shapeList, smoothFltr, shapeText = [0], [[0,1]], [""]
		if portal:
			normalFltr, uvFltr, shapeText = [0], [0], ["portal"] # portal, no normals, no UVs
		else:
			uvFltr, normalFltr, shapeText = [1], [1], ["mixed with normals"] # normals and UVs
			if optNormals: # one pass for flat faces without normals and another pass for smoothed faces with normals, all with UVs
				shapeList, smoothFltr, normalFltr, uvFltr, shapeText = [0,1], [[0],[1]], [0,1], [1,1], ["flat w/o normals", "smoothed with normals"]
		for matIndex in range(len(mats)):
			if (mats[matIndex] != None):
				if not(portal):
					self.exportMaterialLink(file, mats[matIndex])
				for shape in shapeList:
					blenderExportVertexMap = []
					exportVerts = []
					exportFaces = []
					for face in mesh.faces:
						if (face.mat == matIndex) and (face.smooth in smoothFltr[shape]):
							exportVIndices = []
							index = 0
							for vertex in face.verts:
								v = [vertex.co[0], vertex.co[1], vertex.co[2]]
								if normalFltr[shape]:
									if (face.smooth):
										v.extend(vertex.no)
									else:
										v.extend(face.no)
								if (uvFltr[shape]) and (mesh.faceUV):
									v.extend((face.uv[index])[0:1])
								blenderVIndex = vertex.index
								newExportVIndex = -1
								length = len(v)
								if (blenderVIndex < len(blenderExportVertexMap)):
									for exportVIndex in blenderExportVertexMap[blenderVIndex]:
										v2 = exportVerts[exportVIndex]
										if (length==len(v2)) and (v == v2):
											newExportVIndex = exportVIndex
											break
								if (newExportVIndex < 0):
									newExportVIndex = len(exportVerts)
									exportVerts.append(v)
									while blenderVIndex >= len(blenderExportVertexMap):
										blenderExportVertexMap.append([])
									blenderExportVertexMap[blenderVIndex].append(newExportVIndex)
								exportVIndices.append(newExportVIndex)
								index += 1
							exportFaces.append(exportVIndices)
					if (len(exportVerts)>0):
						if (portal):
							file.write("\tPortalShape \"trianglemesh\" \"integer indices\" [\n")
						else:
							file.write("\tShape \"trianglemesh\" \"integer indices\" [\n")
						for face in exportFaces:
							file.write("%d %d %d\n"%(face[0], face[1], face[2]))
							if (len(face)==4):
								file.write("%d %d %d\n"%(face[0], face[2], face[3]))
						file.write("\t] \"point P\" [\n");
						for vertex in exportVerts:
							file.write("%f %f %f\n"%(vertex[0], vertex[1], vertex[2]))
						if normalFltr[shape]:
							file.write("\t] \"normal N\" [\n")
							for vertex in exportVerts:
								file.write("%f %f %f\n"%(vertex[3], vertex[4], vertex[5]))
						if (uvFltr[shape]) and (mesh.faceUV):
							file.write("\t] \"float uv\" [\n")
							for vertex in exportVerts:
								file.write("%f %f\n"%(vertex[-2], vertex[-1]))				
						file.write("\t]\n")
						print "  shape(%s): %d vertices, %d faces"%(shapeText[shape], len(exportVerts), len(exportFaces))
	
	#-------------------------------------------------
	# exportMeshes(self, file)
	# exports meshes that uses instancing (meshes that are used by at least "instancing_threshold" objects)
	#-------------------------------------------------
	def exportMeshes(self, file):
		instancing_threshold = 2 # getLuxProp(self.scene, "instancing_threshold", 2)
		mesh_optimizing = True # getLuxProp(self.scene, "mesh_optimizing", False)
		mesh = Mesh.New('')
		for (mesh_name, objs) in self.meshes.items():
			if (len(objs) >= instancing_threshold):
				del self.meshes[mesh_name]
				obj = objs[0]
				mesh.getFromObject(obj, 0, 1)
				mats = getMaterials(obj) # mats = obj.getData().getMaterials()
				print "blender-mesh: %s (%d vertices, %d faces)"%(mesh_name, len(mesh.verts), len(mesh.faces))
				file.write("ObjectBegin \"%s\"\n"%mesh_name)
				if (mesh_optimizing):
					self.exportMeshOpt(file, mesh, mats, mesh_name)
				else:
					self.exportMesh(file, mesh, mats, mesh_name)
				file.write("ObjectEnd # %s\n\n"%mesh_name)

	#-------------------------------------------------
	# exportPortals(self, file)
	# exports objects to the file
	#-------------------------------------------------
	def exportPortals(self, file):
		mesh_optimizing = True # getLuxProp(self.scene, "mesh_optimizing", False)
		mesh = Mesh.New('')
		for [obj, matrix] in self.portals:
			print "portal: %s"%(obj.getName())
			file.write("\tTransform [%s %s %s %s  %s %s %s %s  %s %s %s %s  %s %s %s %s]\n"\
				%(matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3],\
				  matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3],\
				  matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3],\
		  		  matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]))
			mesh_name = obj.getData(name_only=True)
			mesh.getFromObject(obj, 0, 1)
			mats = getMaterials(obj) # mats = obj.getData().getMaterials()
			if (mesh_optimizing):
				self.exportMeshOpt(file, mesh, mats, mesh_name, True)
			else:
				self.exportMesh(file, mesh, mats, mesh_name, True)

	#-------------------------------------------------
	# exportObjects(self, file)
	# exports objects to the file
	#-------------------------------------------------
	def exportObjects(self, file):
		mesh_optimizing = True # getLuxProp(self.scene, "mesh_optimizing", False)
		mesh = Mesh.New('')
		for [obj, matrix] in self.objects:
			print "object: %s"%(obj.getName())
			file.write("AttributeBegin # %s\n"%obj.getName())
			file.write("\tTransform [%s %s %s %s  %s %s %s %s  %s %s %s %s  %s %s %s %s]\n"\
				%(matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3],\
				  matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3],\
				  matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3],\
		  		  matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]))
			mesh_name = obj.getData(name_only=True)
			if mesh_name in self.meshes:
				mesh.getFromObject(obj, 0, 1)
				mats = getMaterials(obj)
				print "  blender-mesh: %s (%d vertices, %d faces)"%(mesh_name, len(mesh.verts), len(mesh.faces))
				if (mesh_optimizing):
					self.exportMeshOpt(file, mesh, mats, mesh_name)
				else:
					self.exportMesh(file, mesh, mats, mesh_name)
			else:
				print "  instance %s"%(mesh_name)
				file.write("\tObjectInstance \"%s\"\n"%mesh_name)
			file.write("AttributeEnd\n\n")



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
	filebase = os.path.splitext(os.path.basename(filename))[0]

	geom_filename = os.path.join(filepath, filebase + "-geom.lxo")
	geom_pfilename = filebase + "-geom.lxo"

	mat_filename = os.path.join(filepath, filebase + "-mat.lxm")
	mat_pfilename = filebase + "-mat.lxm"

	print("Exporting materials to '" + mat_filename + "'...\n")
	print("Exporting geometry to '" + geom_filename + "'...\n")
	geom_file = open(geom_filename, 'w')
	mat_file = open(mat_filename, 'w')


	##### Write Header ######
	file.write("# Lux Render v0.1 Scene File\n")
	file.write("# Exported by LuxBlend_01_alpha12\n")

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
		if (ratio < 1.0):
			fov = 2*math.atan(16/lens*ratio) * (180 / 3.141592653)
		else:
			fov = 2*math.atan(16/lens/ratio) * (180 / 3.141592653)
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


	########## BEGIN World
	file.write("\n")
	file.write("WorldBegin\n")

	file.write("\n")


### Zuegs: initialization for export class
	export = luxExport(Blender.Scene.GetCurrent())
	export.analyseScene()



	##### Write World Background, Sunsky or Env map ######
	if EnvType.val == 0:
		worldcolor = Blender.World.Get('World').getHor()
		file.write("AttributeBegin\n")
		file.write("LightSource \"infinite\" \"color L\" [%g %g %g] \"integer nsamples\" [1]\n" %(worldcolor[0], worldcolor[1], worldcolor[2]))

		# file.write("%s" %portalstr)
		export.exportPortals(file)


		file.write("AttributeEnd\n")
	if EnvType.val == 1:
		for obj in currentscene.objects:
			if obj.getType() == "Lamp":
				if obj.data.getType() == 1: # sun object
					invmatrix = Mathutils.Matrix(obj.getInverseMatrix())
					file.write("AttributeBegin\n")
					file.write("LightSource \"sunsky\" \"integer nsamples\" [1]\n")
					file.write("            \"vector sundir\" [%f %f %f]\n" %(invmatrix[0][2], invmatrix[1][2], invmatrix[2][2]))
					file.write("		\"color L\" [%f %f %f]\n" %(SkyGain.val, SkyGain.val, SkyGain.val))
					file.write("		\"float turbidity\" [%f]\n" %(Turbidity.val))

					# file.write("%s" %portalstr)
					export.exportPortals(file)


					file.write("AttributeEnd\n")
	if EnvType.val == 2:
		if EnvFile.val != "none" and EnvFile.val != "":
			file.write("AttributeBegin\n")
			file.write("LightSource \"infinite\" \"integer nsamples\" [1]\n")
			file.write("            \"string mapname\" [\"%s\"]\n" %(EnvFile.val) )

			# file.write("%s" %portalstr)
			export.exportPortals(file)

			file.write("AttributeEnd\n")
	file.write("\n")


	#### Write material & geometry file includes in scene file
	file.write("Include \"%s\"\n\n" %(mat_pfilename))
	file.write("Include \"%s\"\n\n" %(geom_pfilename))

	##### Write Material file #####
	mat_file.write("")
	mat_file.write("# Dummy Material 'Default'\n")
	mat_file.write("Texture \"Kd-Default\" \"color\" \"constant\" \"color value\" [0.5 0.5 0.5]\n\n")

	export.exportMaterials(mat_file)


	##### Write Geometry file #####
	meshlist = []
	geom_file.write("")


	export.exportMeshes(geom_file)
	export.exportObjects(geom_file)
	del export


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
	cmd= "lux \"" + filename + "\""
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
SamplerType = Draw.Create(1)
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
	BGL.glColor3f(0.9,0.9,0.9);BGL.glRasterPos2i(10,205) ; Draw.Text("LuxBlend v0.1alpha12", "small")
	
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
