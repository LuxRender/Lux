#ifdef __TEST_PNG__
#include <png.h>
#endif
#ifdef __TEST_OPENEXR__
#include <ImfInputFile.h>
#include <ImfFrameBuffer.h>
using namespace Imf;
using namespace Imath;
#endif
int main(int argc, char **argv, char **env)
{
#ifdef __TEST_PNG__
	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info = png_create_info_struct(png);
	png_destroy_write_struct(&png, &info);
#endif
#ifdef __TEST_OPENEXR__
	Header header(1,1);
	Box2i window(V2i(0,0), V2i(0,0));
	header.dataWindow() = window;
#endif
	return 0;
}
