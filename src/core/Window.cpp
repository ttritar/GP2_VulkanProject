#pragma once

#include "Window.h"


namespace cat
{

    void Window::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        auto ptr = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        ptr->SetFrameBufferResized(true);
		ptr->m_Width = width;
		ptr->m_Height = height;
    }
    
    Window::Window(int width, int height, const char* title)
    	:m_Width{ width }, m_Height(height)
    {
        InitializeWindow(width, height, title);
    }
    
    Window::~Window()
    {
        glfwDestroyWindow(m_pWindow);
    	glfwTerminate();
    }
    
    void Window::InitializeWindow(int width, int height, const char* title)
    {
        // 1. Initialize the GLFW library.
        //-----
        glfwInit();
    
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    
        // 2. Create the window.
        //-----
        m_pWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
        glfwSetWindowUserPointer(m_pWindow, this);
        glfwSetFramebufferSizeCallback(m_pWindow, FramebufferResizeCallback);
    }

}
