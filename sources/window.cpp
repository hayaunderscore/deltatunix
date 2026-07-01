#ifdef __linux__
#include <X11/Xlib.h>
#define GLFW_EXPOSE_NATIVE_X11
#endif

#include <cstring>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

void WindowUnfocus()
{
	// TODO windows
	GLFWwindow *window = glfwGetCurrentContext();
#ifdef __linux__
	Display *display = glfwGetX11Display();
	XSetInputFocus(display, DefaultRootWindow(display), RevertToParent, CurrentTime);
	XFlush(display);
#endif
}

void WindowHideFromTaskbar()
{
	// TODO windows
	GLFWwindow *window = glfwGetCurrentContext();
#ifdef __linux__
	Window win = glfwGetX11Window(window);
	Display *display = glfwGetX11Display();
	XEvent xev;
	Atom _NET_WM_STATE = XInternAtom(display, "_NET_WM_STATE", False);
	Atom _NET_WM_STATE_SKIP_TASKBAR = XInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", False);
	memset(&xev, 0, sizeof(xev));
	xev.type = ClientMessage;
	xev.xclient.window = win;
	xev.xclient.message_type = _NET_WM_STATE;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 1;
	xev.xclient.data.l[1] = _NET_WM_STATE_SKIP_TASKBAR;
	xev.xclient.data.l[2] = 0;
	XSendEvent(display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
#endif
}
