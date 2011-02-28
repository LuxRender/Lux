from string import Template

plugins = {
	'Shapes': 'Shape',
	'Materials': 'Material',
	'FloatTextures': 'FloatTexture',
	'SWCSpectrumTextures': 'SWCSpectrumTexture',
	'FresnelTextures': 'FresnelTexture',
	'Lights': 'Light',
	'AreaLights': 'AreaLight',
	'VolumeRegions': 'VolumeRegion',
	'Volumes': 'Volume',
	'SurfaceIntegrators': 'SurfaceIntegrator',
	'VolumeIntegrators': 'VolumeIntegrator',
	'Accelerators': 'Accelerator',
	'Cameras': 'Camera',
	'Samplers': 'Sampler',
	'Filters': 'Filter',
	'ToneMaps': 'ToneMap',
	'Films': 'Film',
	'PixelSamplers': 'PixelSampler',
	'Renderer': 'Renderer',
}

func_template = Template("""
boost::python::list py_getRegistered$f1()
{
	boost::python::list names;
	const map<string, DynamicLoader::Create$f2> &pluginMap = DynamicLoader::registered$f1();
	map<string, DynamicLoader::Create$f2>::const_iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};
""")

def_template = Template("""
	def("registered$f1",
		py_getRegistered$f1,
		"Return a list of registered $f2 names"
	);
""")

funcs = []
defs = []
for k, v in plugins.items():
	funcs.append( func_template.substitute(f1=k, f2=v) )
	defs.append( def_template.substitute(f1=k, f2=v) )

for func in funcs:
	print(func)
for deff in defs:
	print(deff)
