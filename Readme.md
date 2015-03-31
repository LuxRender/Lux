#LuxRender v. 1.5 Release Notes

This version includes vast enhancements to the LuxCore architecture. When using LuxCore LuxRender can render a scene six times faster than the previous version, when using the CPU. With the addition of an OpenCL GPU furhter speed improvements can be achieved. 

LuxCore needs to be enabled by the exporter that you use so check with the exporter if it provides support for this feature.

##Installation

###OS X
- Drag the LuxRender folder into the Application folder, as instructed in the DMG file.
- If you want to use the command-line tools, included with LuxRender, execute the script `create_systemwide_links` in the LuxRender/interactive_script folder. That will give you access to all the tools, like luxconsole, from the Terminal
- There are some pre-configured scripts that can be used to start useful services, like the node renderer, by simply double-clicking on them from Finder.

####Blender plugin.
LuxRender includes support for Blender, the Open Source 3D modelling program. The following instructions describe how to install the LuxRender plugin for Blender.

To use the Blender plugin you will need Blender v.2.71 or higher, and  OS X 10.6 or higher, since the plugin only works as a 64-bit extension.

	1.	Start Blender and from the menu select **File | User Preferences**
	2. Click on the **Add-ons** tab
	3. Click on the **Install from File...** button.
	2.	Navigate to the opened LuxRender DMG file, which should be /Volumes/LuxRender. From there select the LuxBlend.zip file. 
	3.	You should now see the LuxRender add-on listed in the add-ons menu. Click the box on the right to activate it.
	4.	To make the installation permanent, click on the **Save User Settings** button or press the *Ctrl-u* key combination.
	5.	For further instructions about starting a rendering, please visit http://www.luxrender.net/wiki/LuxBlend_2.5_installation_instructions

###Windows
TBD

###Linux
TBD
##Known Issues
TBD

###OS X
On Yosemite, OS X 10.10, the vertical divider and a few other widgets might look different from the native look of the OS.

###Windows
TBD

###Linux
TBD

Enjoy LuxRender!


