#ifndef AFFINETRANSFORM_H
#define AFFINETRANSFORM_H

#include "GenVector.h"
#include "rdrand.h"

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

#endif