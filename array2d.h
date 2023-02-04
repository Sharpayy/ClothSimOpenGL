#pragma once
#include <cstdlib>

typedef unsigned char percent;

struct Texture
{
	char tex;
};

template<typename T>
class Array2d
{
public:
	Array2d(int sizeX, int sizeY);
	Array2d();
	~Array2d();
	void add(T __x, int x, int y);
	T getData(int x, int y);
	T* getPData(int x, int y);
	void fill(T sample);
	int size();

	T* getIter(int offset = 0);
	T* end();

	int sizex, sizey;

private:
	T* data;
	int allSize;
};

template<typename T>
Array2d<T>::Array2d(int sizeX, int sizeY)
{
	sizex = sizeX; sizey = sizeY;
	//data = new T[sizex * sizey];
	allSize = sizex * sizey;
	data = (T*)malloc(sizeof(T) * (sizex * sizey));
}

template<typename T>
Array2d<T>::Array2d()
{
}

template<typename T>
Array2d<T>::~Array2d()
{
}

template<typename T>
void Array2d<T>::add(T __x, int x, int y)
{
	*(data + (x + y * sizex)) = __x;
}

template<typename T>
T Array2d<T>::getData(int x, int y)
{
	return *(data + (x + y * sizex));
}

template<typename T>
T* Array2d<T>::getPData(int x, int y)
{
	return (data + (x + y * sizex));
}

template<typename T>
void Array2d<T>::fill(T sample)
{
	for (int i = 0; i < allSize; i++)
	{
		(data + i) = sample;
	}
}

template<typename T>
int Array2d<T>::size()
{
	return allSize;
}

template<typename T>
T* Array2d<T>::getIter(int offset)
{
	return data + offset;
}

template<typename T>
T* Array2d<T>::end()
{
	return data + size();
}