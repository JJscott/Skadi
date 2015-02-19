/*
*
* Skiro Heightmap Object
*
* Handels creation, storage and rendering of heightmap
*
*/
#pragma once

#include <vector>

#include "Image.hpp"
#include "Initial3D.hpp"
#include "SimpleShader.hpp"
#include "GL.hpp"

namespace skadi {



	class Heightmap {
	public:
		Heightmap(int width, int height) : m_width(width), m_height(height), vao(0), vbo_pos(0), vbo_uv(0), m_position(), m_scale() {

			for (int y = 0; y <= height; y++) {
				for (int x = 0; x <= width; x++) {
					pos.push_back(2 * x / float(width) - 1);
					pos.push_back(0);
					pos.push_back(2 * y / float(height) - 1);
					uv.push_back(x / float(width));
					uv.push_back(y / float(height));
				}
			}

			auto get_index = [&](int x, int y) -> unsigned {
				// reserve index 0 for 'not a vertex'
				if (x < 0 || x > width) return 0;
				if (y < 0 || y > height) return 0;
				return unsigned(width + 1) * unsigned(y) + unsigned(x);
			};

			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {

					// 1---3 //
					// | /   //
					// 2     //
					idx.push_back(get_index(  x  ,   y  ));
					idx.push_back(get_index(  x  , y + 1));
					idx.push_back(get_index(x + 1,   y  ));

					//     2 //
					//   / | //
					// 3---1 //
					idx.push_back(get_index(x + 1, y + 1));
					idx.push_back(get_index(x + 1,   y  ));
					idx.push_back(get_index(  x  , y + 1));
				}
			}


			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);
			glGenBuffers(1, &ibo);
			glGenBuffers(1, &vbo_pos);
			glGenBuffers(1, &vbo_uv);

			// upload indices
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo); // this sticks to the vao
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(GLuint), &idx[0], GL_STATIC_DRAW);

			// upload positions
			glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
			glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(float), &pos[0], GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

			// upload texture coord
			glBindBuffer(GL_ARRAY_BUFFER, vbo_uv);
			glBufferData(GL_ARRAY_BUFFER, uv.size() * sizeof(float), &uv[0], GL_STATIC_DRAW);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

			// cleanup
			glBindBuffer(GL_ARRAY_BUFFER, 0);


		}

		void setHeights(std::vector<float> heights, int width, int height) {


			glGenTextures(1, &tex_height);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex_height);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, &heights[0]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}

		void setHeights(std::string filename) {

			image heightImage(image::type_png(), filename);

			glGenTextures(1, &tex_height);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex_height);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, heightImage.width(), heightImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, heightImage.data());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		}

		void setPosition(const initial3d::vec3d &position) {
			m_position = position;
		}

		void setScale(const initial3d::vec3d &scale) {
			m_scale = scale;
		}

		initial3d::mat4f getModelWorldMatrix() {
			return initial3d::mat4f::translate(m_position) * initial3d::mat4f::scale(m_scale);
		}

		void draw(initial3d::mat4f worldViewMat, initial3d::mat4f projMat) {

			static GLuint prog = 0;
			static const char *shader_prog_src = R"delim(

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform sampler2D sampler_heightmap;

#ifdef _VERTEX_

// input: model-space position and uv
layout(location = 0) in vec3 m_pos;
layout(location = 1) in vec2 m_uv;

// output: view-space position
out VertexData {
	vec3 col_v;
} vertex_out;

void main() {
	vec3 pos_v = (modelViewMatrix * vec4(m_pos + vec3(0, texture(sampler_heightmap, m_uv).r, 0), 1.0)).xyz;
	gl_Position = projectionMatrix * vec4(pos_v, 1.0);
	vertex_out.col_v = vec3(texture(sampler_heightmap, m_uv).r);
}

#endif


#ifdef _FRAGMENT_

in VertexData {
	vec3 col_v;
} vertex_in;

out vec3 fragment_color;

void main(){
    fragment_color = vertex_in.col_v;
    // fragment_color = vec3(1,1,1);
}

#endif

)delim";

			if (prog == 0) {
				prog = makeShaderProgram("330 core", { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER }, shader_prog_src);
			}


			glUseProgram(prog);

			glUniformMatrix4fv(glGetUniformLocation(prog, "projectionMatrix"), 1, true, initial3d::mat4f(projMat));
			glUniformMatrix4fv(glGetUniformLocation(prog, "modelViewMatrix"), 1, true, worldViewMat * getModelWorldMatrix());

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex_height);

			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES, idx.size(), GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);
		}

	private:
		int m_width;
		int m_height;
		std::vector<GLuint> idx;
		std::vector<float> pos;
		std::vector<float> uv;

		GLuint vao;
		GLuint ibo;
		GLuint vbo_pos;
		GLuint vbo_uv;

		GLuint tex_height;

		initial3d::vec3d m_position;
		initial3d::vec3d m_scale;
	};
}
