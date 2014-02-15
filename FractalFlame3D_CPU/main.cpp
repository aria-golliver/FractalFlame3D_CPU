#include <utility>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <iterator>
#include <ctime>

#include "rdrand.h"
#include "constants.h"
#include "buffer.h"
#include "simplePPM.h"

using namespace std;

float map(float n, float x1, float y1, float x2, float y2){
	return (n - x1) / (y1 - x1) * (y2 - x2) + (x2);
}

#define PI 3.141592f

class affinetransform {
public:
	float a, b, c, d, e, f, g, h, i, j, k, l;

	Vector3 color;

	affinetransform(){
		rdrand_f32(&a);
		rdrand_f32(&b);
		rdrand_f32(&c);
		rdrand_f32(&d);
		rdrand_f32(&e);
		rdrand_f32(&f);
		rdrand_f32(&g);
		rdrand_f32(&h);
		rdrand_f32(&i);
		rdrand_f32(&j);
		rdrand_f32(&k);
		rdrand_f32(&l);
		rdrand_f32(&color[0]);
		rdrand_f32(&color[1]);
		rdrand_f32(&color[2]);

		color[0] = abs(color[0]);
		color[1] = abs(color[1]);
		color[2] = abs(color[2]);
	}

	Vector3 apply(Vector3 p) const {
		float x = p[0] * a + p[1] * b + p[2] * c + d;
		float y = p[0] * e + p[1] * f + p[2] * g + h;
		float z = p[0] * i + p[1] * j + p[2] * k + l;

		return Vector3(x, y, z);
	}

	Vector3 apply_col(Vector3 c) const {
		return (c + this->color) * 0.5f;
	}
};
inline Vector3 v1(const Vector3 &p) {
	float x = sin(p[0]);
	float y = sin(p[1]);
	float z = sin(p[2]);

	return Vector3(x, y, z);
}
inline Vector3 v3z(const Vector3 &p) {
	float rsq = p.squaredLength();
	float x = p[0] * sin(rsq) - p[1] * cos(rsq);
	float y = p[0] * cos(rsq) + p[1] * sin(rsq);
	float z = p[2];

	return Vector3(x, y, z);
}

#define n_affs 3
#define n_frames 128
#define n_threads 12
#define max(a,b) (a > b ? a : b)
#define min(a,b) (a < b ? a : b)

#include <omp.h>

int main(int argc, char **argv){
	omp_set_num_threads(n_threads);
	//const u64 seed = time(NULL);
	const u64 seed = 0x52fee234;
	srand(seed);
	// generate affine transforms
	affinetransform affs[n_affs];

	// generate image buffers
	HistoBuffer hbufs[n_frames];
	for (int i = 0; i < n_frames; i++){
		hbufs[i] = HistoBuffer(wid, hei);
	}

	uint64_t bytes_plotted = 0;

	int frame_count = 0;
	while (1) {
		++frame_count;
#pragma omp parallel for schedule(dynamic)
		for (int j = 0; j < n_threads * 5; j++){
			Vector3 p(0, 0, 0);
			Vector3 c(0, 0, 0);
			for (int i = -20; i < 100000 * min(frame_count, 100); i++){
#pragma omp critical
				bytes_plotted += 6 * sizeof(float);
				unsigned int rand_idx;
				rdrand_u32(&rand_idx);
				// apply random affine
				p = affs[rand_idx % n_affs].apply(p);
				c = affs[rand_idx % n_affs].apply_col(c);
				// apply variation
				p = v3z(p);
				// plot to each camera
				if (i >= 0){ // ignore the first 20 iterations
					float p_theta = atan2(p[2], p[0]);
					float p_rad = sqrt(p[2] * p[2] + p[0] * p[0]);
					float py = p[1] * (hei / 4.0f) * aspect + (hei / 2.0f);

					int y = (int)py;
					if (y >= 0 && y < hei){
						__declspec(align(64)) float new_x[n_frames];
						#pragma vector aligned
						for (int frame = 0; frame < n_frames; frame++){
							float f_theta = map(frame, 0, n_frames, 0, 2 * PI);

							float new_theta = p_theta + f_theta;
							new_x[frame] = cos(new_theta) * p_rad * (wid / 4.0f) + (wid / 2.0f);
						}



						for (int frame = 0; frame < n_frames; frame++){
							int x = (int)new_x[frame];

							if (x >= 0 && x < wid){
								hbufs[frame].at(x, y).r += c[0];
								hbufs[frame].at(x, y).g += c[1];
								hbufs[frame].at(x, y).b += c[2];
								hbufs[frame].at(x, y).a++;
							}
						}
					}
				}
			}
		}
		cout << "done genning " << (bytes_plotted/1000000000.f) << " GB plotted" << endl;

#pragma omp parallel for schedule(dynamic)
		for (int frame = 0; frame < n_frames; frame++){
			float max_a = 1;
			for (int y = 0; y < hei; y++){
				for (int x = 0; x < wid; x++){
					max_a = max(max_a, hbufs[frame].at(x, y).a);
				}
			}

			float inv_log_max_a = 1.0f / log((float)max_a);

			ColBuffer buf(wid, hei);

			for (int y = 0; y < hei; y++){
				for (int x = 0; x < wid; x++){
					if (hbufs[frame].at(x, y).a > 0){
						float r = hbufs[frame].at(x, y).r;
						float g = hbufs[frame].at(x, y).g;
						float b = hbufs[frame].at(x, y).b;

						float max_c = max(r, max(g, b));

						r /= max_c;
						g /= max_c;
						b /= max_c;

						float alpha = log(hbufs[frame].at(x, y).a) * inv_log_max_a;

						unsigned char new_r = alpha * r * 0xFF;
						unsigned char new_g = alpha * g * 0xFF;
						unsigned char new_b = alpha * b * 0xFF;

						buf.at(x, y) = Color(new_r, new_g, new_b);
					}
				}
			}
			char filename[80];
			sprintf(filename, "frame_%04d_%x.ppm", frame, seed);
			simplePPM_write_ppm(filename, wid, hei, &buf.at(0, 0)[0]);
			buf.dealloc();
		}
		cout << "done saving" << endl;
	}

	getchar();

	// save image buffers to disk
}