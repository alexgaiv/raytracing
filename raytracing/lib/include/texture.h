#ifndef _TEXTURE2D_H_
#define _TEXTURE2D_H_

#include "common.h"
#include "datatypes.h"
#include "glcontext.h"
#include "sharedptr.h"
#include "image.h"

#define TEX_GENERATE_ID GLuint(-1)

class BaseTexture
{
protected:
	struct Shared;
	my_shared_ptr<Shared> ptr;
public:
	BaseTexture(GLenum target, GLenum textureUnit = GL_TEXTURE0, GLuint id = TEX_GENERATE_ID);

	void Bind();
	GLuint GetId() const { return ptr->id; }
	GLenum GetTarget() const { return target; }
	GLenum GetTextureUnit() const { return textureUnit; }
	void SetTextureUnit(GLenum unit) { textureUnit = unit; }

	void SetFilters(GLint minFilter, GLint magFilter);
	void SetWrapMode(GLint wrapS, GLint wrapT, GLint wrapR = -1);
	void SetBorderColor(Color4f color);

	void BuildMipmaps();
protected:
	GLenum target, textureUnit;

	bool loadFromTGA(const char *filename, Image &img);
	void texImage2D(GLenum target, const Image &img);

	struct Shared
	{
		bool needDelete;
		GLuint id;
		int width, height;
		bool loaded;

		Shared() : needDelete(false), id(0), loaded(false) {
			width = height = 0;
		}
		~Shared() {
			if (needDelete) glDeleteTextures(1, &id);
		}
	};

	void read(HANDLE hFile, LPVOID lpBuffer, DWORD nNumBytes);
};

class Texture2D : public BaseTexture
{
public:
	Texture2D(GLenum textureUnit = GL_TEXTURE0, GLuint id = TEX_GENERATE_ID);
	Texture2D(const char *name, GLenum textureUnit = GL_TEXTURE0, GLuint id = TEX_GENERATE_ID);

	int GetWidth() const { return ptr->width; }
	int GetHeight() const { return ptr->height; }

	bool IsLoaded() const { return ptr->loaded; }
	bool LoadFromTGA(const char *filename);
	void SetTexImage(GLenum level, GLint internalFormat, GLsizei width, GLsizei height,
		GLint border, GLenum format, GLenum type, const GLvoid *data);
};

class CubeTexture : public BaseTexture
{
public:
	CubeTexture(GLenum textureUnit = GL_TEXTURE0);
	CubeTexture(const char **sides, GLenum textureUnit = GL_TEXTURE0);

	bool IsLoaded() const { return ptr->loaded; }
	bool LoadFromTGA(const char **sides);
};

#endif // _TEXTURE2D_H_