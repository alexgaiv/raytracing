#ifndef _MODEL_LOADER_H_
#define _MODEL_LOADER_H_

#include <vector>
#include "mesh.h"
#include "glcontext.h"
using namespace std;

class ModelLoader
{
public:
	ModelLoader(GLRenderingContext *rc) : rc(rc) {  }
	bool LoadObj(const char *filename, Mesh &mesh);
	bool LoadObj(const char *filename, vector<Mesh *> &meshes);
	bool LoadRaw(const char *filename, Mesh &mesh);
	bool LoadRaw(const char *filename, vector<Mesh *> &meshes);
private:
	GLRenderingContext *rc;
	void read_num(const string &line, char &c, int &i, int &n);
	bool loadObj(const char *filename, vector<Mesh *> &meshes, bool separateMeshes);
	bool loadRaw(const char *filename, vector<Mesh *> &meshes, bool separateMeshes);
};

#endif // _MODEL_LOADER_H_