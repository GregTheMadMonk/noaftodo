#ifdef __linux__

#include "fb.hpp"

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <log.hpp>

namespace noaf {

	backend_framebuffer::backend_framebuffer() :
		dev(-1),
		data(nullptr) {
	}

	void backend_framebuffer::init() {
		log << "Initializing \"framebuffer\" backend..." << lend;
		resume();
	}

	void backend_framebuffer::pause() {
		if (dev == -1) return;
		munmap(data, w * h);
		data = nullptr;
		close(dev);
		dev = -1;
	}

	void backend_framebuffer::resume() {
		if (dev >= 0) return;
		dev = open(dev_name.c_str(), O_RDWR);
		if (dev < 0) return;

		struct fb_var_screeninfo vinfo;

		ioctl(dev, FBIOGET_VSCREENINFO, &vinfo);
		w = vinfo.xres;
		h = vinfo.yres;

		data = (uint32_t*)mmap(NULL, w * h * sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED, dev, (off_t)0);
	}

	void backend_framebuffer::kill() {
	}

	void backend_framebuffer::run() {
		while (true) {
			on_paint();
			system("sleep 1");
		}
	}

	int backend_framebuffer::width()	{ return w; }
	int backend_framebuffer::height()	{ return h; }

	void backend_framebuffer::draw_line(const int& x1, const int& y1, const int& x2, const int& y2) {
		if (x1 == x2)
			for (int y = ((y1 < y2) ? y1 : y2); y <= ((y2 > y1) ? y2 : y1); y++)
				data[y * w + x1] = 0xffffff;
		else for (int x = ((x1 < x2) ? x1 : x2); x <= ((x2 > x1) ? x2 : x1); x++) {
			int y = y1 + (x - x1) * (y2 - y1) / (x2 - x1);
			data[y * w + x] = 0xffffff;
		}
	}

	void backend_framebuffer::draw_box(const int& x1, const int& y1, const int& x2, const int& y2) {
		draw_line(x1, y1, x2, y1);
		draw_line(x2, y1, x2, y2);
		draw_line(x2, y2, x1, y2);
		draw_line(x1, y2, x1, y1);
	}

	void backend_framebuffer::draw_text(const int& x, const int& y, const std::string& text) {
	}

}

#endif
