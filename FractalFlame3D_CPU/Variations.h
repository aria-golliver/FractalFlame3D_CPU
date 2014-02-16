#ifndef VARIATIONS_H
#define VARIATIONS_H

#include "GenVector.h"

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

inline Vector3 v7z(const Vector3 &p) {
	float rsq = p.squaredLength();
	float r = sqrt(rsq);
	float theta = atan2(p[2], p[0]);
	float x = r * sin(theta * r);
	float y = -r * cos(theta * r);
	float z = p[2];

	return Vector3(x, y, z);
}

#endif