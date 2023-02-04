#include <cstdlib>
#include <SDL.h>
#include <GL/glew.h>
#include <gtc/matrix_transform.hpp>
#include <string>
#include <fstream>
#include "array2d.h"
#include <vector>


#undef main

#define dq 1.4142135623730951

#define ENABLE_FLEXION

using namespace glm;

struct v3
{
	float x, y, z;
};
//#define ENABLE_COLLISION

vec3 g = vec3(0.0f, 0.5f, 0.0f);
float spring_const = 2.61285f;
float damping_velocity = -1.0f; // verlet damping
float damping_const = 0.2f;
float particle_mass = 0.1f;
float shearing_const = 0.49f;
#ifdef ENABLE_FLEXION
float flexion_const = 0.35f;
#endif
float equalib = 0.1f;
#ifdef ENABLE_COLLISION
float cp_radius = 0.07f;
#endif

/*
h = timestamp
r = particle pos
f(n) = force

r(n+1) = 2 * r(n) - r(n-1) + h * h * f(n) * m

*/

class CP
{
public:
	CP()
	{}
	CP(float mass)
	{
		v = vec3(0.0f);
		m = mass;
		move = true;
		acc = vec3(0.0f);
	}

	void impulse(vec3 a)
	{
		if (move == true)
			acc += a;
	}
	
	void setMass(float m)
	{
		this->m = m;
	}

	void update(float t, bool force, vec3* pos)
	{
		if (move == true)
		{
			if (force == true)
				acc += m * g;
			vec3 nextp = *pos * 2.0f - v + (acc) * t * t;
			v = *pos;
			*pos = nextp;

			nextp = *pos * 2.0f - v + (*pos - v) * damping_velocity * t * t;
			v = *pos;
			*pos = nextp;
			acc = vec3(0.0f);

		}
	}

	bool move;
	vec3 v, acc;
	float m;
};

class Element
{
public:
	Element()
	{}
	Element(vec3 pos, float mass)
	{
		this->pos = pos;
		v = vec3(0.0f);
		m = mass;
		move = true;
	}

	void impulse(vec3 a)
	{
		if (move == true)
			v += a;
	}

	void setMass(float m)
	{
		this->m = m;
	}

	void update(float t, bool force)
	{
		if (move == true)
		{
			v = v * damping_velocity;
			if (force == true)
				v += (g * m);
			pos += v * t;
		}
	}

public:
	bool move;
	vec3 pos;
	vec3 v;
	float m;
};

void spring(Element* e0, Element* e1, float sp_const, float res)
{
	float x = length(e1->pos - e0->pos) - res;
	vec3 dir = normalize(e1->pos - e0->pos);

	float v_dis = length(e1->v - e0->v);

 	float F = (-sp_const * x) + (-damping_const * v_dis);

	e0->impulse(dir * (F * e0->m));
	e1->impulse(dir * (F * -e1->m));
}

void spring(CP* e0, CP* e1, vec3 ep0, vec3 ep1, const float sp_const, const float res)
{
	vec3 mx = ep1 - ep0;
	float x = length(mx);
	x = x - res;
	vec3 dir = normalize(ep1 - ep0);

	float v_dis = length((ep1 - e1->v)-(ep0 - e0->v));

	float F = (-sp_const * x) + (-damping_const * v_dis);
	e0->impulse(dir * (-F / e0->m));
	e1->impulse(dir * (F / e1->m));
}

std::string* GetFileData(char* filename)
{
	std::ifstream stream(filename);
	std::string line;

	std::string* buffer = new std::string;
	while (std::getline(stream, line))
	{
		buffer->append(line);
		buffer->append("\n");
	}
	stream.close();
	return buffer;
}
typedef unsigned int uint;

void generateCloth2d(Array2d<Element>* _array, vec3 separator, vec3 spos)
{
	vec3 p = spos;
	for (int i = 0; i < _array->sizex; i++)
	{
		for (int l = 0; l < _array->sizey; l++)
		{

			p.y = separator.y * l + spos.y;
			p.x = separator.x * i + spos.x;
			_array->add(Element(p, 0.01f), i, l);
		}
	}
}

void drawCloth2d(Array2d<Element>* _array, uint ML, mat4 ts)
{
	mat4 t;
	for (int i = 0; i < _array->sizex; i++)
	{
		for (int l = 0; l < _array->sizey; l++)
		{
			t = translate(ts, _array->getData(i, l).pos / 0.1f);
			glUniformMatrix4fv(ML, 1, GL_FALSE, &t[0].x);
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}
	}
}

void updateCloth2d(Array2d<Element>* _array)
{
	float struct_stiff = spring_const;
	for (int i = 0; i < _array->sizex - 1; i++)
	{
		for (int l = 0; l < _array->sizey - 1; l++)
		{
			spring(_array->getPData(i, l), _array->getPData(i, l + 1), struct_stiff, equalib);
			spring(_array->getPData(i, l), _array->getPData(i + 1, l), struct_stiff, equalib);
		}
	}

	for (int i = 0; i < _array->sizex; i++)
	{
		for (int l = 0; l < _array->sizey; l++)
		{
			_array->getPData(i, l)->update(0.5f, true);
		}
	}
}

float* meshGeneratorLine(Array2d<Element>* a)
{
	vec3* v = new vec3[a->size() * 4];
	int index = 0;

	for (int i = 0; i < a->sizey - 1; i++)
	{
		for (int l = 0; l < a->sizex - 1; l++)
		{
			*(v + index) = (a->getPData(i, l)->pos);
			index += 1;
			*(v + index) = (a->getPData(i, l + 1)->pos);
			index += 1;
			*(v + index) = (a->getPData(i + 1, l)->pos);
			index += 1;
			*(v + index) = (a->getPData(i, l)->pos);
			index += 1;
		}
	}

	return (float*)v;
}

Array2d<vec3>* createPosClothArray2d(Array2d<CP>* cpa, vec3 sPos, vec3 sep)
{
	vec3 p = sPos;
	Array2d<vec3>* a = new Array2d<vec3>(cpa->sizex, cpa->sizey);
	for (int i = 0; i < cpa->sizex; i++)
	{
		for (int l = 0; l < cpa->sizey; l++)
		{
			p = sPos + vec3(sep.x * i, sep.y * l, sep.z * l);
			//p.y = sep.y * l + sPos.y;
			//p.x = sep.x * i + sPos.x;
			a->add(p, i, l);
			cpa->getPData(i, l)->v = p;
		}
	}
	return a;
}

Array2d<CP>* createCPClothArray2d(int x, int y, float mass)
{
	Array2d<CP>* a = new Array2d<CP>(x, y);
	for (auto i = a->getIter(); i != a->end(); i++)
	{
		*i = CP(mass);
	}
	return a;
}

uint wrapPosClothArray2d(Array2d<vec3>* a)
{
	uint vbo;
	glGenBuffers(1, &vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, a->size() * 12, a->getIter(), GL_STATIC_COPY);

	return vbo;
}

void updatePosClothArray2d(uint bf, Array2d<vec3>* a)
{
	glBindBuffer(GL_ARRAY_BUFFER, bf);
	glBufferSubData(GL_ARRAY_BUFFER, 0, a->size() * 12, a->getIter());
}

uint* generateCloth2dIndex(Array2d<CP>* a)
{
	//(x + y * sizex)
	uint* v = new uint[a->size() * 4];
	int index = 0;

	for (int i = 0; i < a->sizey - 1; i++)
	{
		for (int l = 0; l < a->sizex - 1; l++)
		{
			*(v + index) = (i + l * a->sizex);
			*(v + index + 1) = (i + (l + 1) * a->sizex);
			*(v + index + 2) = (i + l * a->sizex);
			*(v + index + 3) = ((i + 1) + l * a->sizex);
			index += 4;
		}
	}

	return v;
}

uint* generateClothT2dIndex(Array2d<CP>* a)
{
	//(x + y * sizex)
	uint* v = new uint[a->size() * 6];
	int index = 0;

	for (int i = 0; i < a->sizey - 1; i++)
	{
		for (int l = 0; l < a->sizex - 1; l++)
		{
			*(v + index) = (i + l * a->sizex);
			*(v + index + 1) = (i + (l + 1) * a->sizex);
			*(v + index + 2) = ((i + 1) + l * a->sizex);

			*(v + index + 3) = (i + (l + 1) * a->sizex);
			*(v + index + 4) = ((i + 1) + l * a->sizex);
			*(v + index + 5) = ((i + 1) + ((l + 1) * a->sizex));
			index += 6;
		}
	}

	return v;
}

void updateCpCloth2d(Array2d<CP>* cpa, Array2d<vec3>* pos)
{
	float struct_stiff = spring_const;
	for (int i = 0; i < cpa->sizex - 1; i++)
	{
		for (int l = 0; l < cpa->sizey - 1; l++)
		{
			//structure 
			spring(cpa->getPData(i, l), cpa->getPData(i, l + 1), pos->getData(i, l), pos->getData(i, l + 1), struct_stiff, equalib);
			spring(cpa->getPData(i, l), cpa->getPData(i + 1, l), pos->getData(i, l), pos->getData(i + 1, l), struct_stiff, equalib);

			//shearing
			spring(cpa->getPData(i, l), cpa->getPData(i + 1, l + 1), pos->getData(i, l), pos->getData(i + 1, l + 1), shearing_const, equalib * dq);
			spring(cpa->getPData(i + 1, l), cpa->getPData(i, l + 1), pos->getData(i + 1, l), pos->getData(i, l + 1), shearing_const, equalib * dq);

			//flexion
#ifdef ENABLE_FLEXION
			if (l < cpa->sizex - 2 && i < cpa->sizey - 2)
			{
				spring(cpa->getPData(i, l), cpa->getPData(i, l + 2), pos->getData(i, l), pos->getData(i, l + 2), flexion_const, equalib * 2);
				spring(cpa->getPData(i, l), cpa->getPData(i + 2, l), pos->getData(i, l), pos->getData(i + 2, l), flexion_const, equalib * 2);
			}
#endif
		}
	}
	for (int l = 0; l < cpa->sizey - 1; l++)
	{
		spring(cpa->getPData(cpa->sizex - 1, l), cpa->getPData(cpa->sizex - 1, l + 1),
			pos->getData(cpa->sizex - 1, l), pos->getData(cpa->sizex - 1, l + 1), struct_stiff, equalib);
	}

	for (int i = 0; i < cpa->sizex; i++)
	{
		for (int l = 0; l < cpa->sizey; l++)
		{
			cpa->getPData(i, l)->update(0.1f, true, pos->getPData(i, l));
		}
	}
}

int main(int a, char** c)
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	SDL_Window* window = SDL_CreateWindow("title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 600, SDL_WINDOW_OPENGL);
	SDL_Event e;
	SDL_GL_CreateContext(window);
	glewInit();

	const char* vertexShaderSrc = GetFileData((char*)"vertexShader.glsl")->c_str();
	const char* fragmentShaderSrc = GetFileData((char*)"fragmentShader.glsl")->c_str();
	const char* geometryShaderSrc = GetFileData((char*)"geometry.glsl")->c_str();

	uint program, shaderVertex, shaderFragment, shaderGeom;
	shaderVertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shaderVertex, 1, &vertexShaderSrc, NULL);
	glCompileShader(shaderVertex);

	shaderGeom = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(shaderGeom, 1, &geometryShaderSrc, NULL);
	glCompileShader(shaderGeom);

	shaderFragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shaderFragment, 1, &fragmentShaderSrc, NULL);
	glCompileShader(shaderFragment);

	program = glCreateProgram();
	glAttachShader(program, shaderVertex);
	glAttachShader(program, shaderFragment);
	glAttachShader(program, shaderGeom);


	glLinkProgram(program);

	glUseProgram(program);
	uint modelL, projL, camL, colorL, lightLocL;
	colorL = glGetUniformLocation(program, "color");
	modelL = glGetUniformLocation(program, "model");
	projL = glGetUniformLocation(program, "proj");
	camL = glGetUniformLocation(program, "camera");
	//lightLocL = glGetUniformLocation(program, "lightLocation");

	mat4 ts = scale(mat4(1.0f), vec3(0.5f, 0.5f, 0.5f));
	float camRangle = 0.0f;

	mat4 modelM = mat4(1.0f);
	mat4 camM = lookAt(
		vec3(0.1f, 0.0f, 0.5f),
		vec3(0.0f, 0.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f)
	);
	mat4 projM = perspective(60.0f, 1.0f, 0.1f, 100.0f);

	glUniformMatrix4fv(modelL, 1, GL_FALSE, &modelM[0].x);
	glUniformMatrix4fv(camL, 1, GL_FALSE, &camM[0].x);
	glUniformMatrix4fv(projL, 1, GL_FALSE, &projM[0].x);
	//glUniform3f(lightLocL, 0.0f, 0.0f, 0.0f);

	float vtx[] = {
		0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f
	};

	uint vbo, vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 9 * 4, vtx, GL_STATIC_COPY);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 12, (void*)0);
	glEnableVertexAttribArray(0);

	Array2d<Element>* cloth = new Array2d<Element>(12, 12);
	generateCloth2d(cloth, vec3(-0.2f, 0.2f, 0.0f), vec3(0.0f, -2.5f, 0.0f));

	uint meshVao, meshVbo;
	glGenVertexArrays(1, &meshVao);
	glGenBuffers(1, &meshVbo);
	glBindVertexArray(meshVao);
	glBindBuffer(GL_ARRAY_BUFFER, meshVbo);

	float* tMesh = meshGeneratorLine(cloth);
	glBufferData(GL_ARRAY_BUFFER, cloth->size() * 12 * 4, tMesh, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 12, (void*)0);
	glEnableVertexAttribArray(0);

	cloth->getPData(0, 0)->move = false;
	cloth->getPData(24, 0)->move = false;
	// -- new cloth system

	auto clothCP = createCPClothArray2d(30, 30, particle_mass);
	auto clothPos = createPosClothArray2d(clothCP, vec3(0.0f, -2.5f, -0.2f), vec3(-0.1f, 0.1f, 0.0f));
	uint* indices = generateClothT2dIndex(clothCP);
	uint cpClothVbo = wrapPosClothArray2d(clothPos);

	clothCP->getPData(0, 0)->move = false;
	//clothCP->getPData(5, 0)->move = false;
	//clothCP->getPData(10, 0)->move = false;
	clothCP->getPData(15, 0)->move = false;
	//clothCP->getPData(20, 0)->move = false;
	clothCP->getPData(25, 0)->move = false;
	//clothCP->getPData(12, 0)->move = false;


	uint cpClothVao, cpClothInd;
	glGenBuffers(1, &cpClothInd);
	glGenVertexArrays(1, &cpClothVao);
	glBindVertexArray(cpClothVao);

	glBindBuffer(GL_ARRAY_BUFFER, cpClothVbo);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 12, (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cpClothInd);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * 6 * clothCP->size(), indices, GL_STATIC_DRAW);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	float r = 0.4f;
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	while (true)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(program);
		glBindVertexArray(cpClothVao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cpClothInd);


		updateCpCloth2d(clothCP, clothPos);
		updatePosClothArray2d(cpClothVbo, clothPos);
		//glUniformMatrix4fv(modelL, 1, GL_FALSE, &ts[0].x);
		glUniform3f(colorL, 0.3f, 0.1f, 0.0f);
		glDrawElements(GL_TRIANGLES, clothCP->size() * 6, GL_UNSIGNED_INT, NULL); // clothCP->size() * 4
	



		SDL_GL_SwapWindow(window);
		SDL_Delay(20);
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
				exit(1);
			else if (e.type == SDL_KEYDOWN)
			{
				if (e.key.keysym.sym == SDLK_SPACE)
				{
					clothCP->getPData(5, 5)->impulse(vec3(0.0f, 0.0f, 0.5f));
				}
			}

		}
	}
}