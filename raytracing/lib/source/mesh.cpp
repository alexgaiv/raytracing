#include "mesh.h"
#include "datatypes.h"
#include "modelloader.h"

#define TEX_ID_NONE GLuint(-2)

Mesh::Mesh(GLRenderingContext *rc) : rc(rc)
{
	firstIndex = 0;
	indicesCount = -1;

	vertices = indices = normals = texCoords = NULL;
	tangents = binormals = NULL;
	texture = NULL;
	normalMap = specularMap = NULL;
	tangentsComputed = false;
}

Mesh::Mesh(const Mesh &m) {
	clone(m);
}

Mesh::~Mesh() {
	cleanup();
}

Mesh &Mesh::operator=(const Mesh &m) {
	cleanup();
	clone(m);
	return *this;
}

void Mesh::clone(const Mesh &m)
{
	rc = m.rc;

	firstIndex = m.firstIndex;
	indicesCount = m.indicesCount;

	vertices = m.vertices ? new VertexBuffer(*m.vertices) : NULL;
	indices = m.indices ? new VertexBuffer(*m.indices) : NULL;
	normals = m.normals ? new VertexBuffer(*m.normals) : NULL;
	texCoords = m.texCoords  ? new VertexBuffer(*m.texCoords) : NULL;
	tangents = m.tangents ? new VertexBuffer(*m.tangents) : NULL;
	binormals = m.binormals ? new VertexBuffer(*m.binormals) : NULL;

	texture = m.texture ? new BaseTexture(*m.texture) : NULL;
	normalMap = m.normalMap ? new Texture2D(*m.normalMap) : NULL;
	specularMap = m.specularMap ? new Texture2D(*m.specularMap) : NULL;

	tangentsComputed = m.tangentsComputed;
	boundingBox = m.boundingBox;
	boundingSphere = m.boundingSphere;
}

void Mesh::cleanup()
{
	delete vertices;
	delete indices;
	delete normals;
	delete texCoords;
	delete tangents;
	delete binormals;
	delete texture;
	delete normalMap;
	delete specularMap;
}

void Mesh::BindTexture(const BaseTexture &texture) {
	delete this->texture;
	this->texture = new BaseTexture(texture);
}

void Mesh::BindNormalMap(const Texture2D &normalMap) {
	delete this->normalMap;
	this->normalMap = new Texture2D(normalMap);
	if (!tangentsComputed) {
		RecalcTangents();
		tangentsComputed = true;
	}
}

void Mesh::BindSpecularMap(const Texture2D &specularMap) {
	delete this->specularMap;
	this->specularMap = new Texture2D(specularMap);
}

void Mesh::RecalcTangents()
{
	if (!HasNormals() || !HasTexCoords()) return;

	Vector3f *verts = (Vector3f *)vertices->Map(GL_READ_ONLY);
	Vector2f *texs = (Vector2f *)texCoords->Map(GL_READ_ONLY);
	int *inds = (int *)indices->Map(GL_READ_ONLY);
	
	int verticesCount = GetVerticesCount();
	int indicesCount = GetIndicesCount();
	Vector3f *ts = new Vector3f[verticesCount];
	Vector3f *bs = new Vector3f[verticesCount];

	for (int i = 0; i < indicesCount; i += 3)
	{
		int i1 = inds[i], i2 = inds[i+1], i3 = inds[i+2];

		Vector3f &v1 = verts[i1];
		Vector3f &v2 = verts[i2];
		Vector3f &v3 = verts[i3];

		Vector2f &t1 = texs[i1];
		Vector2f &t2 = texs[i2];
		Vector2f &t3 = texs[i3];

		Vector3f edge1 = v2 - v1;
		Vector3f edge2 = v3 - v1;
		Vector2f uv1 = t2 - t1;
		Vector2f uv2 = t3 - t1;

		float f = 1.0f / (uv1.x * uv2.y - uv2.x * uv1.y);
		Vector3f tangent = (uv2.y * edge1 - uv1.y * edge2) * f;
		Vector3f binormal = (uv1.x * edge2 - uv2.x * edge1) * f;
		tangent.Normalize();
		binormal.Normalize();

		ts[i1] = ts[i2] = ts[i3] = tangent;
		bs[i1] = bs[i2] = bs[i3] = binormal;
	}
	
	if (!tangents) tangents = new VertexBuffer(rc, GL_ARRAY_BUFFER);
	if (!binormals) binormals = new VertexBuffer(rc, GL_ARRAY_BUFFER);
	tangents->SetData(verticesCount*sizeof(Vector3f), ts, GL_STATIC_DRAW);
	binormals->SetData(verticesCount*sizeof(Vector3f), bs, GL_STATIC_DRAW);

	delete [] ts;
	delete [] bs;

	vertices->Unmap();
	texCoords->Unmap();
	indices->Unmap();
}

void Mesh::Draw()
{
	if (texture) texture->Bind();
	if (specularMap) specularMap->Bind();
	if (normalMap) {
		normalMap->Bind();
		glEnableVertexAttribArray(AttribsLocations.Tangent);
		glEnableVertexAttribArray(AttribsLocations.Binormal);
		tangents->AttribPointer(AttribsLocations.Tangent, 3, GL_FLOAT);
		binormals->AttribPointer(AttribsLocations.Binormal, 3, GL_FLOAT);
	}

	glEnableVertexAttribArray(AttribsLocations.Vertex);
	vertices->AttribPointer(AttribsLocations.Vertex, 3, GL_FLOAT);

	if (HasNormals()) {
		glEnableVertexAttribArray(AttribsLocations.Normal);
		normals->AttribPointer(AttribsLocations.Normal, 3, GL_FLOAT);
	}

	if (HasTexCoords()) {
		glEnableVertexAttribArray(AttribsLocations.TexCoord);
		texCoords->AttribPointer(AttribsLocations.TexCoord, 2, GL_FLOAT);
	}

	indices->DrawElements(GL_TRIANGLES, GetIndicesCount(), GL_UNSIGNED_INT, firstIndex * sizeof(int));

	glDisableVertexAttribArray(AttribsLocations.Vertex);
	glDisableVertexAttribArray(AttribsLocations.Normal);
	glDisableVertexAttribArray(AttribsLocations.TexCoord);
	if (normalMap) {
		glDisableVertexAttribArray(AttribsLocations.Tangent);
		glDisableVertexAttribArray(AttribsLocations.Binormal);
	}
}

void Mesh::DrawInstanced(int instanceCount)
{
	if (texture) texture->Bind();
	if (specularMap) specularMap->Bind();
	if (normalMap) {
		normalMap->Bind();
		glEnableVertexAttribArray(AttribsLocations.Tangent);
		glEnableVertexAttribArray(AttribsLocations.Binormal);
		tangents->AttribPointer(AttribsLocations.Tangent, 3, GL_FLOAT);
		binormals->AttribPointer(AttribsLocations.Binormal, 3, GL_FLOAT);
	}

	glEnableVertexAttribArray(AttribsLocations.Vertex);
	vertices->AttribPointer(AttribsLocations.Vertex, 3, GL_FLOAT);

	if (HasNormals()) {
		glEnableVertexAttribArray(AttribsLocations.Normal);
		normals->AttribPointer(AttribsLocations.Normal, 3, GL_FLOAT);
	}

	if (HasTexCoords()) {
		glEnableVertexAttribArray(AttribsLocations.TexCoord);
		texCoords->AttribPointer(AttribsLocations.TexCoord, 2, GL_FLOAT);
	}

	indices->DrawElementsInstanced(GL_TRIANGLES, GetIndicesCount(),
		GL_UNSIGNED_INT, instanceCount, firstIndex * sizeof(int));

	glDisableVertexAttribArray(AttribsLocations.Vertex);
	glDisableVertexAttribArray(AttribsLocations.Normal);
	glDisableVertexAttribArray(AttribsLocations.TexCoord);
	if (normalMap) {
		glDisableVertexAttribArray(AttribsLocations.Tangent);
		glDisableVertexAttribArray(AttribsLocations.Binormal);
	}
}

void Mesh::DrawFixed()
{
	if (texture) texture->Bind();

	glEnableClientState(GL_VERTEX_ARRAY);
	vertices->VertexPointer(3, GL_FLOAT, 0);

	if (HasNormals()) {
		glEnableClientState(GL_NORMAL_ARRAY);
		normals->NormalPointer(GL_FLOAT, 0);
	}

	if (HasTexCoords()) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		texCoords->TexCoordPointer(2, GL_FLOAT, 0);
	}

	indices->DrawElements(GL_TRIANGLES, GetIndicesCount(), GL_UNSIGNED_INT, firstIndex);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

bool Mesh::LoadObj(const char *filename)
{
	ModelLoader loader(rc);
	return loader.LoadObj(filename, *this);
}

bool Mesh::LoadRaw(const char *filename)
{
	ModelLoader loader(rc);
	return loader.LoadRaw(filename, *this);
}