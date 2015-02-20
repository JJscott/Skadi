#pragma once

#include <vector>

#include "Initial3D.hpp"

#include "GECom.hpp"
#include "Window.hpp"
#include "SimpleShader.hpp"

namespace skadi {

	class Brush {
	private:
		initial3d::vec3f m_pos;
		double m_radius = 30.0;

		initial3d::mat4f m_proj;

	public:
		Brush() : m_proj(1) {

		}

		void position(const initial3d::vec3f &p) {
			m_pos = p;
		}

		initial3d::vec3f position() const {
			return m_pos;
		}

		void radius(double r) {
			m_radius = r;
		}

		double radius() const {
			return m_radius;
		}

		void projection(const initial3d::mat4f proj) {
			m_proj = proj;
		}

		const initial3d::mat4f & projection() const {
			return m_proj;
		}

		void draw() {

			static GLuint vao = 0;
			if (!vao) {
				glGenVertexArrays(1, &vao);
				glBindVertexArray(vao);

				std::vector<float> angles;

				for (int i = 0; i < 1000; i++) {
					float a = 2.0 * initial3d::math::pi() * i / (1000.0);
					angles.push_back(a);
				}

				GLuint vbo;
				glGenBuffers(1, &vbo);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, sizeof(float) * angles.size(), &angles[0], GL_STATIC_DRAW);
				glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
				glEnableVertexAttribArray(0);

				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}

			static const char *prog_src = R"delim(

			uniform vec2 pos;
			uniform float radius;
			uniform mat4 proj_matrix;
			uniform float time;

			#ifdef _VERTEX_

			layout(location=0) in float angle;

			out VertexData {
				float angle;
			} vertex_out;

			void main() {
				vertex_out.angle = angle;
				gl_Position = proj_matrix * vec4(pos + radius * vec2(cos(angle), sin(angle)), 0.0, 1.0);
			}

			#endif

			#ifdef _FRAGMENT_

			in VertexData {
				float angle;
			} vertex_in;

			out vec4 frag_color;

			void main() {
				const float a = 7.0;
				float f = mix(0.0, 1.0, mod((time * 0.25 + vertex_in.angle) * radius, a) > 0.5 * a);
				frag_color = vec4(vec3(f), 1.0);
			}

			#endif

			)delim";

			static GLuint prog = 0;

			if (!prog) prog = makeShaderProgram("330 core", { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER }, prog_src);

			glBindVertexArray(vao);

			glUseProgram(prog);

			glUniform2f(glGetUniformLocation(prog, "pos"), m_pos.x(), m_pos.y());
			glUniform1f(glGetUniformLocation(prog, "radius"), m_radius);
			glUniformMatrix4fv(glGetUniformLocation(prog, "proj_matrix"), 1, true, m_proj);
			glUniform1f(glGetUniformLocation(prog, "time"), fmod(glfwGetTime(), initial3d::math::pi() * 32.0));

			glDrawArrays(GL_LINE_LOOP, 0, 1000);

			glUseProgram(0);

			glBindVertexArray(0);
		}
	};

}
