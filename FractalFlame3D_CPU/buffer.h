#ifndef __BUFFER
#define __BUFFER

#include "GenVector.h"
#include <malloc.h>
class ColBuffer
{
public:
	ColBuffer() : w(0), h(0) {
		this->data = NULL;
	}

	ColBuffer(unsigned int _w, unsigned int _h) : w(_w), h(_h)
	{
		alloc();
	}

	Color at(unsigned int x, unsigned int y) const
	{
		return data[x + y*this->w];
	}

	Color & at(unsigned int x, unsigned int y)
	{
		return data[x + y*this->w];
	}

	void dealloc(){
		if (data != NULL)
			_aligned_free(data);
		data = NULL;
	}

private:
	const unsigned int w;
	const unsigned int h;
	__declspec(align(64)) Color __declspec(align(64)) * data;

	void alloc()
	{
		size_t size = this->w * this->h;
		if (size == 0){
			this->data = NULL;
			return;
		}

		data = (Color*) _aligned_malloc(size * sizeof(Color), 64);
		memset(data, 0, this->w * this->h * sizeof(Color));
	}
};

typedef struct {
	float r, g, b;
	float a;
} histogram;

class HistoBuffer
{
public:
	HistoBuffer() : w(0), h(0) {
		this->data = NULL;
	}

	HistoBuffer(unsigned int _w, unsigned int _h) : w(_w), h(_h)
	{
		alloc();
	}

	histogram at(unsigned int x, unsigned int y) const
	{
		return data[x + y*this->w];
	}

	histogram & at(unsigned int x, unsigned int y)
	{
		return data[x + y*this->w];
	}
	void dealloc(){
		if (data != NULL)
			_aligned_free(data);
		data = NULL;
	}

private:
	const unsigned int w;
	const unsigned int h;
	__declspec(align(64)) histogram __declspec(align(64)) * data;

	void alloc()
	{
		size_t size = this->w * this->h;
		if (size == 0){
			this->data = NULL;
			return;
		}

		data = (histogram*)_aligned_malloc(size * sizeof(histogram), 64);
		memset(data, 0, this->w * this->h * sizeof(histogram));
	}
};
#endif