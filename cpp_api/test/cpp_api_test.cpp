#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "lux_api.h"

typedef boost::shared_ptr<lux_instance> Instance;
typedef boost::shared_ptr<lux_paramset> ParamSet;

// create a new instance with proper destruction
Instance CreateInstance(const std::string name) {
	return Instance(CreateLuxInstance(name.c_str()), DestroyLuxInstance);
}

// create a new paramset with proper destruction
ParamSet CreateParamSet() {
	return ParamSet(CreateLuxParamSet(), DestroyLuxParamSet);
}

void render()
{

	float fov=30;
	std::string filename = "cpp_api_test";

	Instance lux = CreateInstance("cpp_api_test");

	{
		ParamSet cp = CreateParamSet();
		cp->AddFloat("fov", &fov);

		lux->lookAt(0,10,100,0,-1,0,0,1,0);

		lux->camera("perspective", boost::get_pointer(cp));

		//lux->pixelFilter("mitchell","xwidth", &size, "ywidth" , &size,LUX_NULL);
	}

	{
		ParamSet fp = CreateParamSet();
		const int xres = 640;
		const int yres = 480;
		const bool write_png = true;
		const int halttime = 20;
		fp->AddInt("xresolution",&xres);
		fp->AddInt("yresolution",&yres);
		fp->AddBool("write_png",&write_png);
		fp->AddString("filename",filename.c_str());
		fp->AddInt("halttime", &halttime);
		lux->film("fleximage", boost::get_pointer(fp));
	}

	lux->worldBegin();

	{
		ParamSet lp = CreateParamSet();
		float from[3]= {0,5,0};
		float to[3]= {0,0,0};
		lp->AddVector("from", from, 3);
		lp->AddVector("to", to, 3);
		lux->lightSource("distant", boost::get_pointer(lp));
	}

	{
		ParamSet dp = CreateParamSet();
		float r = 1.0f, h = 0.3f;
		dp->AddFloat("radius", &r);
		dp->AddFloat("height", &h);
		lux->shape("disk", boost::get_pointer(dp)); 
	}

	lux->worldEnd();

	// wait for the WorldEnd thread to start running
	// this isn't terribly reliable, cpp_api should be modified
	boost::this_thread::sleep(boost::posix_time::seconds(1));
	lux->wait();

	// saveFLM needs extension
	filename = filename + ".flm";
	lux->saveFLM(filename.c_str());

	std::cout << "Render done" << std::endl;
}

void main()
{
	std::cout << "Starting render" << std::endl;

	try {
		render();
	} catch (std::exception &e) {
		std::cout << e.what() << std::endl;
	}
}