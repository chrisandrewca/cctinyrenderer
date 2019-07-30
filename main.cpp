#include "tgaimage.h"
#include "geometry.h"
#include <tinyobjloader/tiny_obj_loader.h>
#include <iostream>
#include <algorithm>
#include <limits>
#include <cmath>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);

bool loadModel(tinyobj::attrib_t &attrib, std::vector<tinyobj::shape_t> &shapes, const char *filename, const char *path)
{
	std::vector<tinyobj::material_t> mats;

	std::string warn;
	std::string err;

	// this filepath depends on the working directory
	// TODO somehow set working directory in some type of single global place...
	auto loaded = tinyobj::LoadObj(&attrib, &shapes, &mats, &warn, &err,
								   filename,
								   path,
								   true);

	if (!warn.empty())
		std::cout << warn << std::endl;
	if (!err.empty())
		std::cout << err << std::endl;

	return loaded;
}

/*
 * Lesson 01
 */
void line(int x0, int y0, int x1, int y1, TGAImage &frame, const TGAColor &color)
{
	/*
	 * https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
	 */

	auto steep = false;
	if (std::abs(x0 - x1) < std::abs(y0 - y1))
	{
		// if the line is steep, transpose it ...
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}

	if (x0 > x1)
	{
		// make the line left to right
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	int dx = x1 - x0;
	int dy = y1 - y0;
	float derror = std::abs(dy) * 2; // whoah ...
	int correction = (y1 > y0 ? 1 : -1);
	float error = 0;
	int y = y0;

	if (steep)
	{
		// if the line is steep, de-transpose
		for (int x = x0; x <= x1; x++)
		{
			frame.set(y, x, color);
			error += derror;
			if (error > .5f)
			{
				y += correction;
				error -= dx * 2; // whoah ...
			}
		}
	}
	else
	{
		for (int x = x0; x <= x1; x++)
		{
			frame.set(x, y, color);
			error += derror;
			if (error > .5f)
			{
				y += correction;
				error -= dx * 2; // whoah ...
			}
		}
	}
}

void line(Vec2i a, Vec2i b, TGAImage &frame, const TGAColor &color)
{
	line(a.x, a.y, b.x, b.y, frame, color);
}

Vec3f barycentric(Vec3f a, Vec3f b, Vec3f c, Vec3f p)
{
	Vec3f s[2];
	for (int i = 2; i--;)
	{
		s[i].x = c[i] - a[i];
		s[i].y = b[i] - a[i];
		s[i].z = a[i] - p[i];
	}

	Vec3f u = cross(s[0], s[1]);

	/*
	 * 'pts' and 'P' have integer values as coordinates
	 * so abs(u[2]) < 1 means u[2] is 0, that means the triangle is degenerate,
	 * in this c ase return something with negative coordinates
	 * ...
	 */
	if (std::abs(u[2]) > 1e-2)
		return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	return Vec3f(-1, 1, 1);
}

void triangle(Vec3f *pts, Vec2f *texCoords, float lightIntensity, float *zbuffer, TGAImage &frame, TGAImage &texture)
{
	/*
	 * Vert bounding box
	 */
	Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	Vec2f clamp(frame.get_width() - 1, frame.get_height() - 1);

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts[i][j]));
			bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
		}
	}

	// you've found point P, what are its texcoords relative to A, B, C?
	// what are P's texcoords?
	Vec3f p;
	for (p.x = bboxmin.x; p.x <= bboxmax.x; p.x++)
	{
		for (p.y = bboxmin.y; p.y <= bboxmax.y; p.y++)
		{
			Vec3f bcScreen = barycentric(pts[0], pts[1], pts[2], p); // TODO assumes pts.len = 3
			if (bcScreen.x < 0 || bcScreen.y < 0 || bcScreen.z < 0)
				continue;

			// We're using bcScreen to weight values of z and texCoords
			p.z = 0;
			for (int i = 0; i < 3; i++) // TODO assumes pts.len = 3
			{
				p.z += pts[i].z * bcScreen[i];
			}

			// zbuffer ...
			int zindex = (int)(p.x + p.y * frame.get_width());
			if (zbuffer[zindex] < p.z)
			{
				zbuffer[zindex] = p.z;

				// float u = (texCoords[0].x * bcScreen.x) + (texCoords[1].x * bcScreen.y) + (texCoords[2].x * bcScreen.z);
				// float v = (texCoords[0].y * bcScreen.x) + (texCoords[1].y * bcScreen.y) + (texCoords[2].y * bcScreen.z);
				float u = 0.f, v = 0.f;
				for (int i = 0; i < 3; i++) u += texCoords[i].x * bcScreen[i];
				for (int i = 0; i < 3; i++) v += texCoords[i].y * bcScreen[i];

				auto color = texture.get(u * texture.get_width(), (1 - v) * texture.get_height());
				color.r *= lightIntensity;
				color.g *= lightIntensity;
				color.b *= lightIntensity;
				frame.set(p.x, p.y, color);
			}
		}
	}
}

void triangleRaster(const char *objFilePath, const char *objBasePath, const char *texturePath, TGAImage &frame)
{
	int frameWidth = frame.get_width(), frameHeight = frame.get_height();

	int zlen = frameWidth * frameHeight;
	float *zbuffer = new float[zlen];
	std::fill(zbuffer, zbuffer + zlen, -std::numeric_limits<float>::max());

	TGAImage texture;
	if (!texture.read_tga_file("obj/african_head_diffuse.tga"))
		std::cout << "Unable to read " << texturePath << std::endl;

	// int texWidth = texture.get_width(), texHeight = texture.get_height();

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	loadModel(attrib, shapes, objFilePath, objBasePath);

	for (auto ishape = 0; ishape < shapes.size(); ishape++)
	{
		auto shape = shapes[ishape];
		size_t faceOffset = 0;
		for (auto iface = 0; iface < shape.mesh.num_face_vertices.size(); iface++)
		{
			// TODO use numVerts
			Vec3f worldCoords[3];
			Vec3f screenCoords[3];
			Vec2f texCoords[3];

			auto numVerts = shape.mesh.num_face_vertices[iface];
			for (auto ivert = 0; ivert < numVerts; ivert++)
			{
				// face has n verts, vert has 3 'points'
				auto face = shape.mesh.indices[faceOffset + ivert];
				auto x = attrib.vertices[3 * face.vertex_index + 0]; // 3 as in obj file format or 3 as in vertices?
				auto y = attrib.vertices[3 * face.vertex_index + 1];
				auto z = attrib.vertices[3 * face.vertex_index + 2];

				worldCoords[ivert] = Vec3f(x, y, z);
				// world to screen coords
				screenCoords[ivert] = Vec3f((int)((x + 1.f) * frameWidth / 2.f + .5f), (int)((y + 1.f) * frameHeight / 2.f + .5f), z);

				auto tx = attrib.texcoords[2 * face.texcoord_index + 0];
				auto ty = attrib.texcoords[2 * face.texcoord_index + 1];

				texCoords[ivert] = Vec2f(tx, ty);
			}

			// illumination
			Vec3f normal = (worldCoords[2] - worldCoords[0]) ^ (worldCoords[1] - worldCoords[0]);
			normal.normalize();
			float intensity = normal * Vec3f(0, 0, -1); // Vec3f light_dir(0,0,-1);

			// back face culling
			if (intensity > 0)
				triangle(screenCoords, texCoords, intensity, zbuffer, frame, texture);

			faceOffset += numVerts;
		}
	}

	delete zbuffer;
}

int main(int argc, char **argv)
{
	TGAImage frame(500, 500, TGAImage::RGB);
	triangleRaster("obj/african_head.obj", "obj/", "obj/african_head_diffuse.tga", frame);

	frame.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	frame.write_tga_file("framebuffer.tga");
	return 0;

	// TODOS
	// - gamma correction, see https://github.com/ssloy/tinyrenderer/wiki/Lesson-2:-Triangle-rasterization-and-back-face-culling
}