#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <limits>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "Initial3D.hpp"
#include "GL.hpp"
#include "Window.hpp"




namespace skadi {

	class Projection {
	public:
		Projection() : m_projectionTransform(initial3d::mat4d(1)) {}

		void setPerspectiveProjection(double fovy, double aspect, double zNear, double zFar) {
			double f = initial3d::math::cot(fovy / 2);

			m_projectionTransform = initial3d::mat4d(0);
			m_projectionTransform(0, 0) = f / aspect;
			m_projectionTransform(1, 1) = f;
			m_projectionTransform(2, 2) = (zFar + zNear) / (zNear - zFar);
			m_projectionTransform(2, 3) = (2 * zFar * zNear) / (zNear - zFar);
			m_projectionTransform(3, 2) = -1;

			m_zfar = zFar;
		}

		void setOrthographicProjection(double left, double right, double bottom, double top, double nearVal, double farVal) {
			m_projectionTransform = initial3d::mat4d(0);
			m_projectionTransform(0, 0) = 2 / (right - left);
			m_projectionTransform(0, 3) = (right + left) / (right - left);
			m_projectionTransform(1, 1) = 2 / (top - bottom);
			m_projectionTransform(1, 3) = (top + bottom) / (top - bottom);
			m_projectionTransform(2, 2) = -2 / (farVal - nearVal);
			m_projectionTransform(2, 3) = (farVal + nearVal) / (farVal - nearVal);
			m_projectionTransform(3, 3) = 1;

			m_zfar = farVal;
		}

		initial3d::mat4d getProjectionTransform() {
			return m_projectionTransform;
		}

		double getFarPlane() {
			return m_zfar;
		}

	private:
		double m_zfar;
		initial3d::mat4d m_projectionTransform;
	};


	class Camera {
	public:
		virtual initial3d::mat4d getViewTransform() = 0;
		virtual void update() = 0;
	private:
	};


	class FPSCamera : public Camera {
	public:
		FPSCamera(gecom::Window *win, const initial3d::vec3d &pos, double rot_h = 0, double rot_v = 0) :
			m_window(win), m_pos(pos), m_ori(), m_rot_v(rot_v), m_mouse_captured(false) {
			m_time_last = std::chrono::steady_clock::now();
			m_speed = 2.5;
		}

		initial3d::mat4d getViewTransform() {
			initial3d::quatd rot = (m_ori * initial3d::quatd::axisangle(initial3d::vec3d::i(), m_rot_v));
			return initial3d::mat4d::translate(m_pos) * initial3d::mat4d::rotate(rot); //TODO check / recently switched this
		}

		void update() {

			using namespace initial3d;
			using namespace std;

			// time since last update
			std::chrono::steady_clock::time_point time_now = std::chrono::steady_clock::now();
			double dt = std::chrono::duration_cast<std::chrono::duration<double>>(time_now - m_time_last).count();
			m_time_last = time_now;

			// pixels per 2*pi
			double rot_speed = 600;

			vec3d up = vec3d::j(); // up is world up
			vec3d forward = -~(m_ori * vec3d::k()).reject(up);
			vec3d side = ~(forward ^ up);

			if (m_mouse_captured) {
				int h = m_window->height();
				int w = m_window->width();
				double x, y;
				glfwGetCursorPos(m_window->handle(), &x, &y);
				x -= w * 0.5;
				y -= h * 0.5;
				double rot_h = -x / rot_speed;
				m_ori = quatd::axisangle(up, rot_h) * m_ori;
				m_rot_v += -y / rot_speed;
				m_rot_v = math::clamp(m_rot_v, -0.499 * math::pi(), 0.499 * math::pi());
				glfwSetCursorPos(m_window->handle(), w * 0.5, h * 0.5);
			}

			vec3d move = vec3d::zero();

			if (m_window->getKey(GLFW_KEY_W)) move += forward;
			if (m_window->getKey(GLFW_KEY_S)) move -= forward;
			if (m_window->getKey(GLFW_KEY_A)) move -= side;
			if (m_window->getKey(GLFW_KEY_D)) move += side;
			if (m_window->getKey(GLFW_KEY_LEFT_SHIFT)) move -= up;
			if (m_window->getKey(GLFW_KEY_SPACE)) move += up;

			//TODO change this
			if (m_window->pollKey(GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS) {
				m_mouse_captured = !m_mouse_captured;
				glfwSetCursorPos(m_window->handle(), m_window->width() * 0.5, m_window->height() * 0.5);
			}

			try {
				vec3d dpos = ~move * m_speed * dt;
				m_pos = m_pos + dpos;

				//cout << m_pos << endl;
			}
			catch (nan_error &e) {
				// no movement, do nothing
			}
		}

	private:
		gecom::Window * m_window;
		initial3d::vec3d m_pos;
		initial3d::quatd m_ori;
		double m_rot_v;
		bool m_mouse_captured;
		std::chrono::steady_clock::time_point m_time_last;
		double m_speed;
	};

}

