#version 130

in vec2 fTexCoord;

struct Ray
{
	vec3 origin;
	vec3 dir;
};

struct Object
{
	vec3 color;
	vec3 normal;

	// 0 = diffuse
	// 1 = diffuse + specular
	// 2 = diffuse + mirror
	// 3 = diffuse + mirror + specular
	// 4 = glass
	int material;

	float specPower;
	float refractIndex;
};

struct Sphere
{
	Object base;
	vec3 center;
	float radius;
};

struct Plane
{
	Object base;
	float D;
};

struct StackEntry
{
	Object obj;
	float fresnel;
	vec3 color;
	vec3 lightDir;
	vec3 viewDir;
};

const int TRACE_DEPTH = 3;
const int numSpheres = 3;
const int numPlanes = 6;

uniform mat4 ModelView;
uniform int ImageWidth;
uniform int ImageHeight;
uniform float TanHalfFov;

uniform vec3 lightSource;
uniform vec3 lightAmbient;
uniform vec3 backColor;
uniform Sphere spheres[numSpheres];
uniform Plane planes[numPlanes];

Ray GetCameraRay()
{
	vec2 imageWH = vec2(float(ImageWidth), float(ImageHeight));
	float acpectRatio = imageWH.x / imageWH.y;

	vec2 pxl = fTexCoord * imageWH;
	pxl.x = int(pxl.x);
	pxl.y = int(pxl.y);

	vec3 pixelCamera;
	pixelCamera.x = (2 * ((pxl.x + 0.5) / imageWH.x) - 1.0) * TanHalfFov * acpectRatio;
	pixelCamera.y = (1 - 2 * ((pxl.y + 0.5) / imageWH.y)) * TanHalfFov;
	pixelCamera.z = -1;

	vec3 rayOrigin = vec3(ModelView * vec4(0, 0, 0, 1));
	pixelCamera = vec3(ModelView * vec4(pixelCamera, 1));

	vec3 rayDirection = normalize(pixelCamera - rayOrigin);
	return Ray(rayOrigin, rayDirection);
}

bool intersect(Ray ray, Sphere sphere, out float t)
{
	float r2 = sphere.radius * sphere.radius;
	vec3 u = sphere.center - ray.origin;
	float d = dot(u, ray.dir);

	if (d < 0.0) return false;
	float d2 = dot(u, u) - d*d;
	if (d2 > r2) return false;

	float h = sqrt(r2 - d2);
	t = min(d - h, d + h);
	if (t < 0.0) return false;
	
	return true;
}

bool intersect(Ray ray, Plane plane, out float t)
{
	float d = dot(ray.dir, plane.base.normal);
	if (d == 0.0) return false;
	t = -(plane.D + dot(ray.origin, plane.base.normal)) / d;
	return t >= 0;
}

void TestObjects(Ray ray, int objFrom, out int hitObject, out float tmin)
{
	tmin = 0;
	hitObject = -1;
	bool first = true;
	float t = 0;

	for (int i = 0; i < numSpheres; i++) {
		if (i != objFrom && intersect(ray, spheres[i], t)) {
			if (t < tmin || first) {
				tmin = t;
				hitObject = i;
				first = false;
			}
		}
	}

	for (int i = 0; i < numPlanes; i++) {
		if (i + numSpheres != objFrom && intersect(ray, planes[i], t)) {
			if (t < tmin || first) {
				tmin = t;
				hitObject = i + numSpheres;
				first = false;
			}
		}
	}
}

bool TestShadow(Ray ray, int objFrom)
{
	float t = 0;
	float tLight = (lightSource - ray.origin).x / ray.dir.x;

	for (int i = 0; i < numSpheres; i++) {
		if (i != objFrom && intersect(ray, spheres[i], t)) {
			if (t < tLight) return true;
		}
	}

	for (int i = 0; i < numPlanes; i++) {
		if (i + numSpheres != objFrom && intersect(ray, planes[i], t)) {
			if (t < tLight) return true;
		}
	}
	return false;
}

Object GetObject(vec3 hitPoint, int object)
{
	Object obj;
	if (object < numSpheres)
	{
		obj = spheres[object].base;
		obj.normal = normalize(hitPoint - spheres[object].center);
	}
	else if ((object -= numSpheres) < numPlanes)
	{
		obj = planes[object].base;
	}
	return obj;
}

float Fresnel(vec3 normal, vec3 viewDir, float eta)
{
	float R0 = (1.0 - eta) / (1 + eta);
	R0 *= R0;
	return R0 + (1.0 - R0) * pow(1.0 - dot(normal, viewDir), 5);
}

vec3 BlinnPhong(Object obj, vec3 lightDir, vec3 viewDir)
{
	float diffuseCoeff = max(0.0, dot(obj.normal, lightDir));
	vec3 diffuse = obj.color * (lightAmbient + diffuseCoeff);

	if (obj.material == 0 || obj.material == 2) return diffuse;
	
	vec3 halfDir = normalize(lightDir + viewDir);
	float specAngle = max(0.0, dot(obj.normal, halfDir));
	float specCoeff = pow(specAngle, obj.specPower);

	return diffuse + vec3(1.0) * specCoeff;
}

vec3 CastRay2(int object, Ray ray)
{
	float t;
	int hitObject;
	TestObjects(ray, object, hitObject, t);
	
	if (hitObject != -1) {
		vec3 hitPoint = ray.origin + t * ray.dir;
		vec3 lightDir = normalize(lightSource - hitPoint);
		Object obj = GetObject(hitPoint, hitObject);

		Ray shadowRay = Ray(hitPoint, lightDir);
		if (TestShadow(shadowRay, hitObject)) {
			return vec3(0.2) * obj.color;
		}
		else {
			return BlinnPhong(obj, lightDir, -ray.dir);
		}
	}
	else return backColor;
}

vec3 CastRay(int object, Ray ray)
{
	float t;
	int hitObject;
	TestObjects(ray, object, hitObject, t);

	if (hitObject != -1) {
		vec3 hitPoint = ray.origin + t * ray.dir;
		vec3 lightDir = normalize(lightSource - hitPoint);
		Object obj = GetObject(hitPoint, hitObject);

		Ray shadowRay = Ray(hitPoint, lightDir);
		if (TestShadow(shadowRay, hitObject)) {
			return vec3(0.1) * obj.color;
		}
		else {
			vec3 reflectColor;
			if (obj.material >= 2) {
				Ray reflectionRay = Ray(hitPoint, normalize(reflect(ray.dir, obj.normal)));
				reflectColor = CastRay2(hitObject, reflectionRay);
			}
			else reflectColor = obj.color;

			vec3 refractColor;
			if (obj.material == 4) {
				Ray refractionRay = Ray(hitPoint, normalize(refract(ray.dir, obj.normal, obj.refractIndex)));
				refractColor = CastRay2(hitObject, refractionRay);
			}
			else refractColor = obj.color;

			float k = Fresnel(obj.normal, -ray.dir, obj.refractIndex);
			obj.color = mix(refractColor, reflectColor, k);
			return BlinnPhong(obj, lightDir, -ray.dir);
		}
	}
	else return backColor;
}

void main()
{
	float t = 0;
	int hitObject;
	Ray cameraRay = GetCameraRay();

	TestObjects(cameraRay, -1, hitObject, t);

	if (hitObject != -1)
	{
		gl_FragColor.xyz = vec3(0);
		vec3 hitPoint = cameraRay.origin + t * cameraRay.dir;
		vec3 lightDir = normalize(lightSource - hitPoint);
		Object obj = GetObject(hitPoint, hitObject);

		Ray shadowRay = Ray(hitPoint, lightDir);
		if (TestShadow(shadowRay, hitObject)) {
			gl_FragColor.xyz = vec3(0.1) * obj.color;
		}
		else {
			vec3 reflectColor;
			if (obj.material >= 2) {
				Ray reflectionRay = Ray(hitPoint, normalize(reflect(cameraRay.dir, obj.normal)));
				reflectColor = CastRay(hitObject, reflectionRay);
			}
			else reflectColor = obj.color;

			vec3 refractColor;
			if (obj.material == 4) {
				Ray refractionRay = Ray(hitPoint, normalize(refract(cameraRay.dir, obj.normal, obj.refractIndex)));
				refractColor = CastRay(hitObject, refractionRay);
			}
			else refractColor = obj.color;

			float k = Fresnel(obj.normal, -cameraRay.dir, obj.refractIndex);
			obj.color = mix(refractColor, reflectColor, k);

			gl_FragColor.xyz = BlinnPhong(obj, lightDir, -cameraRay.dir);
		}
	}
	else gl_FragColor = vec4(backColor, 1);
}