/*
 * GECom GL Header
 *
 * Provides access to the GL API.
 * This should be included instead of GLAER or GLFW,
 * although you probably just want Window.hpp.
 *
 * Main purpose is to resolve an include loop between Window.hpp and Shader.hpp.
 * 
 */

#ifndef GECOM_GL_HPP
#define GECOM_GL_HPP

#include <stdexcept>

#include <GLAER/glaer.h>
#include <GLFW/glfw3.h>

// this is to enable multiple context support, defined in Window.cpp
namespace gecom {
	GlaerContext * getCurrentGlaerContext();
}

// exception thrown by GL debug callback on error if GECOM_GL_NO_EXCEPTIONS is not defined
namespace gecom {
	class gl_error {
	public:
		//gl_error(const std::string &what_ = "GL error") : runtime_error(what_) { }
	};
}

#endif // GECOM_GL_HPP