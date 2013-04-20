#include <iostream>
#include <vector>
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

		float t[2] = {0.f, 1.f};

		lux->motionBegin(2, t);
		lux->lookAt(
			3.f, 3.f, 3.f,
			0.f, -1.f, 0.f,
			0.f, 1.f, 0.f);
		lux->lookAt(
			3.f, 3.f, 3.f,
			0.f, -0.9f, 0.f,
			0.f, 1.f, 0.f);
		lux->motionEnd();

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
		fp->AddString("filename",&filename);
		fp->AddInt("halttime", &halttime);
		lux->film("fleximage", boost::get_pointer(fp));
	}

	lux->worldBegin();

	{
		ParamSet lp = CreateParamSet();
		float from[3]= {0,5,0};
		float to[3]= {0,0,0};
		float L[3] = {1,0,1};
		float theta = 0.1f;
		lp->AddPoint("from", from);
		lp->AddPoint("to", to);
		lp->AddRGBColor("L", L);
		lp->AddFloat("theta", &theta);
		lux->lightSource("distant", boost::get_pointer(lp));
	}

	//{
	//	ParamSet lp = CreateParamSet();
	//	lp->AddPoint("from", from, 3);
	//	lp->AddPoint("to", to, 3);
	//	lp->AddRGBColor("L", L, 3);
	//	lp->AddFloat("theta", &theta, 1);

	//	lux->lightSource("distant", boost::get_pointer(lp));	
	//}

	{
		ParamSet dp = CreateParamSet();
		float r = 1.0f, h = 0.3f;
		dp->AddFloat("radius", &r);
		dp->AddFloat("height", &h);
		lux->shape("disk", boost::get_pointer(dp)); 
	}

	{
		ParamSet mp = CreateParamSet();

		std::vector<float> vert_p;
		std::vector<float> vert_n;
		std::vector<float> vert_uv;
		std::vector<int> tri_idx;

		const int num_triangles = 2;

		// bottom left
		vert_p.push_back(-1.f);
		vert_p.push_back(-1.f);
		vert_p.push_back(0.f);
		vert_n.push_back(0.f);
		vert_n.push_back(0.f);
		vert_n.push_back(1.f);
		vert_uv.push_back(0.f);
		vert_uv.push_back(0.f);

		// bottom right
		vert_p.push_back(-1.f);
		vert_p.push_back(1.f);
		vert_p.push_back(0.f);
		vert_n.push_back(0.f);
		vert_n.push_back(0.f);
		vert_n.push_back(1.f);
		vert_uv.push_back(1.f);
		vert_uv.push_back(0.f);

		// top right
		vert_p.push_back(1.f);
		vert_p.push_back(1.f);
		vert_p.push_back(0.f);
		vert_n.push_back(0.f);
		vert_n.push_back(0.f);
		vert_n.push_back(1.f);
		vert_uv.push_back(1.f);
		vert_uv.push_back(1.f);

		// top left
		vert_p.push_back(-1.f);
		vert_p.push_back(1.f);
		vert_p.push_back(0.f);
		vert_n.push_back(0.f);
		vert_n.push_back(0.f);
		vert_n.push_back(1.f);
		vert_uv.push_back(0.f);
		vert_uv.push_back(1.f);

		// first face
		tri_idx.push_back(0);
		tri_idx.push_back(1);
		tri_idx.push_back(2);

		// second face
		tri_idx.push_back(0);
		tri_idx.push_back(2);
		tri_idx.push_back(3);

		mp->AddPoint("P", &vert_p[0], vert_p.size()/3);
		mp->AddNormal("N", &vert_n[0], vert_n.size()/3);
		mp->AddFloat("uv", &vert_uv[0], vert_uv.size());
		mp->AddInt("triindices", &tri_idx[0], tri_idx.size());

		lux->shape("trianglemesh", boost::get_pointer(mp));
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