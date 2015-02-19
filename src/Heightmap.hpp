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
		Heightmap(int width, int height) : m_width(width), m_height(height) {

			for (int y = 0; y <= height; y++) {
				for (int x = 0; x <= width; x++) {
					pos.push_back(2 * x / float(width) - 1);
					pos.push_back(0);
					pos.push_back(2 * y / float(height) - 1);
					// 
					uv.push_back((x + 0.5) / float(width + 1));
					uv.push_back((y + 0.5) / float(height + 1));
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

		void updateNormals() {

			if (!tex_height) throw 3456;

			if (!fbo) glGenFramebuffers(1, &fbo);
			
			if (tex_norm) glDeleteTextures(1, &tex_norm);

			glGenTextures(1, &tex_norm);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex_height);

			GLint w, h;

			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

			glBindTexture(GL_TEXTURE_2D, tex_norm);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

			glBindTexture(GL_TEXTURE_2D, tex_height);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_norm, 0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);

			static const char *prog_src = R"delim(
			
			uniform sampler2D sampler_heightmap;

			#ifdef _VERTEX_

			void main() { }

			#endif

			#ifdef _GEOMETRY_

			layout(points) in;
			layout(triangle_strip, max_vertices = 3) out;

			void main() {
				gl_Position = vec4(3.0, 1.0, 0.0, 1.0);
				EmitVertex();
				gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);
				EmitVertex();
				gl_Position = vec4(-1.0, -3.0, 0.0, 1.0);
				EmitVertex();
				EndPrimitive();
			}

			#endif

			#ifdef _FRAGMENT_
			
			vec3 positionFromTexel(ivec2 tx) {
				ivec2 ts = textureSize(sampler_heightmap, 0);
				// texelFetch() is undefined when out of bounds - clamp to edges
				tx = clamp(tx, ivec2(0), ts - 1);
				float h = texelFetch(sampler_heightmap, tx, 0).r;
				return vec3(vec2(tx) / (vec2(ts) - 1.0) * 2.0 - 1.0, h).xzy;
			}

			vec3 normalFromTexel(ivec2 tx) {
				vec3 p0 = positionFromTexel(tx);
				vec3 n = vec3(0.0);
				const ivec2[] dp = ivec2[](ivec2(1, 0), ivec2(0, -1), ivec2(-1, 0), ivec2(0, 1));
				for (int i = 0; i < 4; i++) {
					n += normalize(cross(normalize(positionFromTexel(tx + dp[i]) - p0), normalize(positionFromTexel(tx + dp[(i + 1) % 4]) - p0)));
				}
				return normalize(n);
			}
			
			out vec4 frag_color;
			
			void main() {
				frag_color = vec4(normalFromTexel(ivec2(gl_FragCoord.xy)), 0.0);
			}

			#endif

			)delim";

			static GLuint prog = 0;
			
			if (!prog) prog = makeShaderProgram("330 core", { GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER }, prog_src);

			glViewport(0, 0, w, h);

			glUseProgram(prog);

			glUniform1i(glGetUniformLocation(prog, "sampler_heightmap"), 0);

			gecom::draw_dummy();

			glUseProgram(0);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		}

		void setHeights(float *heights, int width, int height) {

			if (tex_height) glDeleteTextures(1, &tex_height);

			glGenTextures(1, &tex_height);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex_height);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, heights);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

			updateNormals();
		}

		void setHeights(std::string filename) {

			image heightImage(image::type_png(), filename);

			if (tex_height) glDeleteTextures(1, &tex_height);

			glGenTextures(1, &tex_height);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex_height);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, heightImage.width(), heightImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, heightImage.data());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

			updateNormals();
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
			uniform sampler2D sampler_normalmap;

			#ifdef _VERTEX_

			// input: model-space position and uv
			layout(location = 0) in vec3 pos_m;
			layout(location = 1) in vec2 uv;

			out VertexData {
				vec3 pos_w;
				vec3 norm_w;
			} vertex_out;

			void main() {
				vec3 pos_w = pos_m + vec3(0, texture(sampler_heightmap, uv).r, 0);
				vec3 pos_v = (modelViewMatrix * vec4(pos_w, 1.0)).xyz;
				gl_Position = projectionMatrix * vec4(pos_v, 1.0);
				vertex_out.pos_w = pos_w;
				vertex_out.norm_w = texture(sampler_normalmap, uv).xyz;
			}

			#endif


			#ifdef _FRAGMENT_

			in VertexData {
				vec3 pos_w;
				vec3 norm_w;
			} vertex_in;

			out vec4 frag_color;

			void main() {
				float d = normalize(vertex_in.norm_w).y;
				frag_color = vec4(vec3(d * 0.8), 1.0);
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
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, tex_norm);
			glUniform1i(glGetUniformLocation(prog, "sampler_heightmap"), 0);
			glUniform1i(glGetUniformLocation(prog, "sampler_normalmap"), 1);

			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES, idx.size(), GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);

			glUseProgram(0);
		}

	private:
		int m_width;
		int m_height;
		std::vector<GLuint> idx;
		std::vector<float> pos;
		std::vector<float> uv;

		GLuint vao = 0;
		GLuint ibo = 0;
		GLuint vbo_pos = 0;
		GLuint vbo_uv = 0;

		GLuint tex_height = 0;
		GLuint tex_norm = 0;
		GLuint fbo = 0;

		initial3d::vec3d m_position;
		initial3d::vec3d m_scale;
	};
}
