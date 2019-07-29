#include "tgaimage.h"
#include "geometry.h"
#include <tinyobjloader/tiny_obj_loader.h>
#include <iostream>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);

void line(int x0, int y0, int x1, int y1, TGAImage &image, const TGAColor &color)
{
	// for (float t = 0.; t < 1.; t += .01)
	// {
	// 	int x = x0 + (x1 - x0) * t;
	// 	int y = y0 + (y1 - y0) * t;
	// 	image.set(x, y, color);
	// }

	// for (int x = x0; x <= x1; x++)
	// {
	// 	float t = (x - x0) / (float) (x1 - x0);
	// 	int y = y0 * (1.f - t) + y1 * t;
	// 	image.set(x, y, color);
	// }

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
			image.set(y, x, color);
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
			image.set(x, y, color);
			error += derror;
			if (error > .5f)
			{
				y += correction;
				error -= dx * 2; // whoah ...
			}
		}
	}
}

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

void wireframe(int width, int height, TGAImage &image, const TGAColor &color)
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

				line((x1 + 1.f) * width / 2.f,
					 (y1 + 1.f) * height / 2.f,
					 (x2 + 1.f) * width / 2.f,
					 (y2 + 1.f) * height / 2.f, image, color);
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

			line((x1 + 1.f) * width / 2.f,
				 (y1 + 1.f) * height / 2.f,
				 (x2 + 1.f) * width / 2.f,
				 (y2 + 1.f) * height / 2.f, image, color);

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

			line((x1 + 1.f) * width / 2.f,
				 (y1 + 1.f) * height / 2.f,
				 (x2 + 1.f) * width / 2.f,
				 (y2 + 1.f) * height / 2.f, image, color);

			faceOffset += numVerts;
		}
	}
}

int main(int argc, char **argv)
{
	TGAImage image(500, 500, TGAImage::RGB);

	wireframe(500, 500, image, white);

	// line(0, 0, 500, 500, image, white);
	// line(13, 20, 80, 40, image, white);
	// line(0, 0, 500, 0, image, white);
	// line(250, 250, 300, 275, image, white);

	// line(13, 20, 80, 40, image, white);
	// line(20, 13, 40, 80, image, red);
	// line(80, 40, 13, 20, image, red);

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
	return 0;
}