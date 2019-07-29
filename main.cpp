#include "tgaimage.h"
#include "geometry.h"
#include <tinyobjloader/tiny_obj_loader.h>
#include <iostream>
#include <algorithm>

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

void wireframeLines(int width, int height, TGAImage &frame, const TGAColor &color)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	loadModel(attrib, shapes, "obj/african_head.obj", "obj/");

	for (auto ishape = 0; ishape < shapes.size(); ishape++)
	{
		auto shape = shapes[ishape];
		size_t faceOffset = 0;
		for (auto iface = 0; iface < shape.mesh.num_face_vertices.size(); iface++)
		{
			Vec2i screenCoords[3];

			auto numVerts = shape.mesh.num_face_vertices[iface];
			for (auto ivert = 0; ivert < numVerts - 1; ivert++)
			{
				// face has n verts, vert has 3 'points'
				auto face = shape.mesh.indices[faceOffset + ivert];
				auto x1 = attrib.vertices[3 * face.vertex_index + 0]; // 3 as in obj file format or 3 as in vertices?
				auto y1 = attrib.vertices[3 * face.vertex_index + 1];
				//auto z1 = attrib.vertices[3 * face.vertex_index + 2];

				face = shape.mesh.indices[faceOffset + ivert + 1];
				auto x2 = attrib.vertices[3 * face.vertex_index + 0];
				auto y2 = attrib.vertices[3 * face.vertex_index + 1];
				//auto z2 = attrib.vertices[3 * face.vertex_index + 2];

				// world to screen coords
				line((x1 + 1.f) * width / 2.f,
					 (y1 + 1.f) * height / 2.f,
					 (x2 + 1.f) * width / 2.f,
					 (y2 + 1.f) * height / 2.f, frame, color);
			}

			/*
			 * For 2nd last and last vertices
			 */
			auto face = shape.mesh.indices[faceOffset + numVerts - 2];
			auto x1 = attrib.vertices[3 * face.vertex_index + 0];
			auto y1 = attrib.vertices[3 * face.vertex_index + 1];
			//auto z1 = attrib.vertices[3 * face.vertex_index + 2];

			face = shape.mesh.indices[faceOffset + numVerts - 1];
			auto x2 = attrib.vertices[3 * face.vertex_index + 0];
			auto y2 = attrib.vertices[3 * face.vertex_index + 1];
			//auto z2 = attrib.vertices[3 * face.vertex_index + 2];

			// world to screen coords
			line((x1 + 1.f) * width / 2.f,
				 (y1 + 1.f) * height / 2.f,
				 (x2 + 1.f) * width / 2.f,
				 (y2 + 1.f) * height / 2.f, frame, color);

			/*
			* For last and first vertices
			*/
			face = shape.mesh.indices[faceOffset + numVerts - 1];
			x1 = attrib.vertices[3 * face.vertex_index + 0];
			y1 = attrib.vertices[3 * face.vertex_index + 1];
			//auto z1 = attrib.vertices[3 * face.vertex_index + 2];

			face = shape.mesh.indices[faceOffset];
			x2 = attrib.vertices[3 * face.vertex_index + 0];
			y2 = attrib.vertices[3 * face.vertex_index + 1];
			//auto z2 = attrib.vertices[3 * face.vertex_index + 2];

			// world to screen coords
			line((x1 + 1.f) * width / 2.f,
				 (y1 + 1.f) * height / 2.f,
				 (x2 + 1.f) * width / 2.f,
				 (y2 + 1.f) * height / 2.f, frame, color);

			faceOffset += numVerts;
		}
	}
}

void lesson01(int width, int height, TGAImage &frame)
{
	// line(0, 0, 500, 500, frame, white);
	// line(13, 20, 80, 40, frame, white);
	// line(0, 0, 500, 0, frame, white);
	// line(250, 250, 300, 275, frame, white);

	// line(13, 20, 80, 40, frame, white);
	// line(20, 13, 40, 80, frame, red);
	// line(80, 40, 13, 20, frame, red);

	wireframeLines(width, height, frame, white);
}

/*
 * Lesson 02
 */

Vec3f barycentric(Vec2i *pts, Vec2i P)
{
	// TODO use a.x, a.y notation
	Vec3f u = cross(
		Vec3f(pts[2][0] - pts[0][0], pts[1][0] - pts[0][0], pts[0][0] - P[0]),
		Vec3f(pts[2][1] - pts[0][1], pts[1][1] - pts[0][1], pts[0][1] - P[1]));

	/*
	 * 'pts' and 'P' have integer values as coordinates
	 * so abs(u[2]) < 1 means u[2] is 0, that means the triangle is degenerate,
	 * in this c ase return something with negative coordinates
	 * ...
	 */
	if (std::abs(u[2]) < 1)
		return Vec3f(-1, 1, 1);
	return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
}

void triangle(Vec2i *pts, TGAImage &frame, const TGAColor &color)
{
	Vec2i bboxmin(frame.get_width() - 1, frame.get_height() - 1);
	Vec2i bboxmax(0, 0);
	Vec2i clamp(frame.get_width() - 1, frame.get_height() - 1);

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			bboxmin[j] = std::max(0, std::min(bboxmin[j], pts[i][j]));
			bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
		}
	}

	Vec2i P;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++)
	{
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++)
		{
			Vec3f bcScreen = barycentric(pts, P);
			if (bcScreen.x < 0 || bcScreen.y < 0 || bcScreen.z < 0)
				continue;
			frame.set(P.x, P.y, color);
		}
	}
}

void triangleRaster(const char *objFilePath, const char *objBasePath, TGAImage &frame)
{
	int width = frame.get_width(), height = frame.get_height();
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
			Vec2i screenCoords[3];

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
				screenCoords[ivert] = Vec2i((x + 1.f) * width / 2.f, (y + 1.f) * height / 2.f);
			}

			// illumination
			Vec3f normal = (worldCoords[2] - worldCoords[0]) ^ (worldCoords[1] - worldCoords[0]);
			normal.normalize();
			float intensity = normal * Vec3f(0, 0, -1); // Vec3f light_dir(0,0,-1);

			// back face culling
			if (intensity > 0)
				triangle(screenCoords, frame, TGAColor(intensity*255, intensity*255, intensity*255, 255)); // TGAColor(rand() % 255, rand() % 255, rand() % 255, 255));

			faceOffset += numVerts;
		}
	}
}

void lesson02(TGAImage &frame)
{
	triangleRaster("obj/african_head.obj", "obj/", frame);
}

int main(int argc, char **argv)
{
	TGAImage frame(500, 500, TGAImage::RGB);

	//lesson01(frame.get_width(), height.get_height(), frame);
	lesson02(frame);

	frame.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	frame.write_tga_file("framebuffer.tga");
	return 0;

	// TODOS
	// - gamma correction, see https://github.com/ssloy/tinyrenderer/wiki/Lesson-2:-Triangle-rasterization-and-back-face-culling
}