// Python dosctrings for pylux Module
// Try to keep within 80 columns
// ref: http://epydoc.sourceforge.net/manual-epytext.html
// ref: https://renderman.pixar.com/products/rispec/rispec_pdf/RISpec3_2.pdf
// ref: http://www.luxrender.net/wiki/index.php?title=Scene_file_format
// -----------------------------------------------------------------------------

const char * ds_pylux =
"LuxRender Python Bindings\n\n"
"Provides access to the LuxRender API in python\n\n"
"TODO: Docstrings marked (+) need verification.";

const char * ds_pylux_version =
"Returns the pylux/LuxRender version";

const char * ds_pylux_ThreadSignals =
"Valid states for rendering threads";

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
"Initialise geometry acceleration structure type. Valid types are\n"
"- bruteforce\n"
"- bvh\n"
"- grid\n"
"- qbvh\n"
"- tabreckdtree (or kdtree)";

const char * ds_pylux_Context_addServer =
"Add a (remote) rendering server to the context";

const char * ds_pylux_Context_addThread =
"Add a local rendering thread to the context";

const char * ds_pylux_Context_areaLightSource =
"Attach a light source to the current geometry definition. (See: RiSpec 3.2 p.43)";

const char * ds_pylux_Context_attributeBegin =
"Begin a new Attribute scope.";

const char * ds_pylux_Context_attributeEnd =
"End an Attribute scope.";

const char * ds_pylux_Context_camera =
"Add a camera to the scene. Valid types are:\n"
"- environment\n"
"- orthographic\n"
"- perspective\n"
"- realistic";

const char * ds_pylux_Context_cleanup =
"Clean up and reset the renderer state after rendering.";

const char * ds_pylux_Context_concatTransform =
"Concatenate the given transformation onto the current transformation. The\n"
"transformation is applied before all previously applied transformations, that\n"
"is, before the current transformation. (Ref: RiSpec 3.2 p.56)";

const char * ds_pylux_Context_coordSysTransform =
"Replaces the current transformation matrix with the matrix that forms the named\n"
"coordinate system. This permits objects to be placed directly into special or\n"
"user-defined coordinate systems by their names. (Ref: RiSpec 3.2 p.57)";

const char * ds_pylux_Context_coordinateSystem =
"This function marks the coordinate system defined by the current transformation\n"
"with the name space and saves it. (Ref: RiSpec 3.2 p.58)";

const char * ds_pylux_Context_disableRandomMode =
"Disables random mode in the renderer core.";

const char * ds_pylux_Context_enableDebugMode =
"Puts the renderer core into Debug mode.";

const char * ds_pylux_Context_exit =
"Stop the current rendering.";

const char * ds_pylux_Context_exterior =
"Sets the current Exterior volume shader. (Ref: RiSpec 3.2 p.48)";

const char * ds_pylux_Context_film =
"Initialise the Film for rendering. Valid types are:\n"
"- fleximage.";

const char * ds_pylux_Context_framebuffer =
"Returns the current post-processed LDR framebuffer in RGB888 format as a list.\n"
"It is advisable to call updateFramebuffer() before calling this function.";

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
