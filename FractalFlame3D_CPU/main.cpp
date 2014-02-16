#include <utility>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <iterator>
#include <ctime>
#include <chrono>



#include <omp.h>

#include <glm/glm.hpp>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4, glm::ivec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr

#include <cmath>

#include "rdrand.h"
#include "constants.h"
#include "buffer.h"
#include "simplePPM.h"
#include "Variations.h"
#include "AffineTransform.h"

using namespace std;

float map(float n, float x1, float y1, float x2, float y2){
	return (n - x1) / (y1 - x1) * (y2 - x2) + (x2);
}

#define PI 3.141592f

#define n_affs 3
#define n_frames 128
#define n_threads 8
//#define max(a,b) (a > b ? a : b)
//#define min(a,b) (a < b ? a : b)

using namespace std::chrono;

int main(int argc, char **argv){
	high_resolution_clock::time_point program_start = high_resolution_clock::now();
	omp_set_num_threads(n_threads);
	//const u64 seed = time(NULL);
	//const u64 seed = 0x52ffb454;
	const u64 seed = 0;
	//srand(seed);
	// generate affine transforms
	affinetransform affs[n_affs];

	// generate image buffers
	HistoBuffer hbufs[n_frames];
	for (int i = 0; i < n_frames; i++){
		hbufs[i] = HistoBuffer(wid, hei);
	}

	uint64_t bytes_plotted = 0;

	glm::mat4 cams[n_frames];
	glm::mat4 persp = glm::perspective(45.0f, aspect, 0.01f, 100.0f);

	for (int frame = 0; frame < n_frames; frame++){
		float theta = map(frame, 0, n_frames, 0, 2 * PI);

		glm::vec3 eye(4 * cos(theta), 0, 4 * sin(theta));
		glm::vec3 lookAt(0, 0, 0);
		glm::vec3 upvec(0, 1, 0);
		cams[frame] = persp * glm::lookAt(eye, lookAt, upvec);
	}

	int frame_count = 0;
	while (1) {
		high_resolution_clock::time_point time_now = high_resolution_clock::now();
		duration<double> time_since_start = duration_cast<duration<double>>(time_now - program_start);
		printf("time since program start %.2f\n", time_since_start.count());

		++frame_count;
#pragma omp parallel for schedule(dynamic)
		for (int j = 0; j < n_threads * 5; j++){
			Vector3 p(0, 0, 0);
			Vector3 c(0, 0, 0);
			for (int i = -20; i < 100000 * fmin(frame_count, 100); i++){
//#pragma omp critical
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
					for (int frame = 0; frame < n_frames; frame++){
						glm::vec4 p_glm(p[0], p[1], p[2], 1);
						glm::vec4 p_trans = cams[frame] * p_glm;
						p_trans /= p_trans.w;

						if (p_trans.z > -1.0f){
							int x = p_trans.x * (wid / 4.0) + (wid / 2.0f);
							int y = p_trans.y * (hei / 4.0) + (hei / 2.0f);

							if (x >= 0 && x < wid){
								if (y >= 0 && y < hei){
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
		}
		cout << "done genning " << (bytes_plotted/1000000000.f) << " GB plotted" << endl;

#pragma omp parallel for schedule(dynamic)
		for (int frame = 0; frame < n_frames; frame++){
			float max_a = 1;
			for (int y = 0; y < hei; y++){
				for (int x = 0; x < wid; x++){
					max_a = fmax(max_a, hbufs[frame].at(x, y).a);
				}
			}

			float inv_log_max_a = 1.0f / log((float)max_a);

			ColBuffer buf(wid, hei);

			for (int y = 0; y < hei; y++){
				for (int x = 0; x < wid; x++){
					if (hbufs[frame].at(x, y).a > 0){
						float alpha = log(hbufs[frame].at(x, y).a) * inv_log_max_a;

						float r = hbufs[frame].at(x, y).r;
						float g = hbufs[frame].at(x, y).g;
						float b = hbufs[frame].at(x, y).b;

						float max_c = fmax(r, fmax(g, b));

						r /= max_c;
						g /= max_c;
						b /= max_c;

						unsigned char new_r = alpha * r * 0xFF;
						unsigned char new_g = alpha * g * 0xFF;
						unsigned char new_b = alpha * b * 0xFF;
						buf.at(x, y) = Color(new_r, new_g, new_b);
					}
				}
			}
			char filename[80];
			sprintf(filename, "images/frame_%04d_%x.ppm", frame, seed);
			simplePPM_write_ppm(filename, wid, hei, &buf.at(0, 0)[0]);
			buf.dealloc();
		}
		cout << "done saving" << endl;
	}

	getchar();

	// save image buffers to disk
}