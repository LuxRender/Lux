// Python dosctrings for pylux Module
// Try to keep within 80 columns
// ref: http://epydoc.sourceforge.net/manual-epytext.html
// -----------------------------------------------------------------------------

const char * ds_pylux =
"LuxRender Python Bindings\n\n"
"Provides access to the LuxRender API in python\n\n"
"TODO: Docstrings marked (+) need verification.";

const char * ds_pylux_version =
"Returns the pylux/LuxRender version";

const char * ds_pylux_ThreadSignals =
"(+) Valid signals to send to rendering threads";

const char * ds_pylux_RenderingThreadInfo =
"Container class for information about rendering threads";

const char * ds_pylux_luxComponent =
"(+) LuxRender Components available to modify at render-time";

const char * ds_pylux_luxComponentParameters =
"(+) Parameters of luxComponents available to modify at render time";

const char * ds_pylux_RenderingServerInfo =
"(+) Container class for information about rendering servers";

const char * ds_pylux_errorHandler =
"Specify an alternate error handler (logging) function for LuxRender engine\n"
"output. By default the render engine will print to stdout";

const char * ds_pylux_Context =
"An instance of a LuxRender rendering context";

const char * ds_pylux_Context_init =
"Create a new rendering Context object with the given name";

const char * ds_pylux_Context_accelerator =
"Initialise geometry acceleration structure type";

const char * ds_pylux_Context_addServer =
"Add a (remote) rendering server to the context";

const char * ds_pylux_Context_addThread =
"Add a local rendering thread to the context";

const char * ds_pylux_Context_areaLightSource =
"";

const char * ds_pylux_Context_attributeBegin =
"";

const char * ds_pylux_Context_attributeEnd =
"";

const char * ds_pylux_Context_camera =
"";

const char * ds_pylux_Context_cleanup =
"";

const char * ds_pylux_Context_concatTransform =
"";

const char * ds_pylux_Context_coordSysTransform =
"";

const char * ds_pylux_Context_coordinateSystem =
"";

const char * ds_pylux_Context_disableRandomMode =
"";

const char * ds_pylux_Context_enableDebugMode =
"";

const char * ds_pylux_Context_exit =
"";

const char * ds_pylux_Context_exterior =
"";

const char * ds_pylux_Context_film =
"";

const char * ds_pylux_Context_framebuffer =
"";

const char * ds_pylux_Context_getDefaultParameterValue =
"";

const char * ds_pylux_Context_getDefaultStringParameterValue =
"";

const char * ds_pylux_Context_getHistogramImage =
"";

const char * ds_pylux_Context_getNetworkServerUpdateInterval =
"";

const char * ds_pylux_Context_getOption =
"";

const char * ds_pylux_Context_getOptions =
"";

const char * ds_pylux_Context_getParameterValue =
"";

const char * ds_pylux_Context_getRenderingServersStatus =
"";

const char * ds_pylux_Context_getRenderingThreadsStatus =
"";

const char * ds_pylux_Context_getServerCount =
"";

const char * ds_pylux_Context_getStringParameterValue =
"";

const char * ds_pylux_Context_identity =
"";

const char * ds_pylux_Context_interior =
"";

const char * ds_pylux_Context_lightGroup =
"";

const char * ds_pylux_Context_lightSource =
"";

const char * ds_pylux_Context_loadFLM =
"";

const char * ds_pylux_Context_lookAt =
"";

const char * ds_pylux_Context_makeNamedMaterial =
"";

const char * ds_pylux_Context_makeNamedVolume =
"";

const char * ds_pylux_Context_material =
"";

const char * ds_pylux_Context_motionInstance =
"";

const char * ds_pylux_Context_namedMaterial =
"";

const char * ds_pylux_Context_objectBegin =
"";

const char * ds_pylux_Context_objectEnd =
"";

const char * ds_pylux_Context_objectInstance =
"";

const char * ds_pylux_Context_overrideResumeFLM =
"";

const char * ds_pylux_Context_parse =
"Parse the given filename. If done asynchronously, control will pass\n"
"immediately back to the python interpreter, otherwise this function blocks.";

const char * ds_pylux_Context_pause =
"";

const char * ds_pylux_Context_pixelFilter =
"";

const char * ds_pylux_Context_portalShape =
"";

const char * ds_pylux_Context_removeServer =
"";

const char * ds_pylux_Context_removeThread =
"";

const char * ds_pylux_Context_reverseOrientation =
"";

const char * ds_pylux_Context_rotate =
"";

const char * ds_pylux_Context_sampler =
"";

const char * ds_pylux_Context_saveFLM =
"";

const char * ds_pylux_Context_scale =
"";

const char * ds_pylux_Context_setEpsilon =
"";

const char * ds_pylux_Context_setHaltSamplePerPixel =
"";

const char * ds_pylux_Context_setNetworkServerUpdateInterval =
"";

const char * ds_pylux_Context_setOption =
"";

const char * ds_pylux_Context_setParameterValue =
"";

const char * ds_pylux_Context_setStringParameterValue =
"";

const char * ds_pylux_Context_shape =
"";

const char * ds_pylux_Context_start =
"";

const char * ds_pylux_Context_statistics =
"";

const char * ds_pylux_Context_surfaceIntegrator =
"";

const char * ds_pylux_Context_texture =
"";

const char * ds_pylux_Context_transform =
"";

const char * ds_pylux_Context_transformBegin =
"";

const char * ds_pylux_Context_transformEnd =
"";

const char * ds_pylux_Context_translate =
"";

const char * ds_pylux_Context_updateFilmFromNetwork =
"";

const char * ds_pylux_Context_updateFramebuffer =
"";

const char * ds_pylux_Context_volume =
"";

const char * ds_pylux_Context_volumeIntegrator =
"";

const char * ds_pylux_Context_wait =
"";

const char * ds_pylux_Context_worldBegin =
"";

const char * ds_pylux_Context_worldEnd =
"";
