#include "../NCLGL/window.h"
#include "Renderer.h"

int main()	{
	Window w("Coursework", 1280, 720, false);

	if(!w.HasInitialised()) {
		return -1;
	}
	
	Renderer renderer(w);
	if(!renderer.HasInitialised()) {
		return -1;
	}

	w.LockMouseToWindow(true);

	while(w.UpdateWindow()  && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)){
		
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_B)) {
			renderer.moveCamera();
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_V)) {
			renderer.stopCamera();
		}

		renderer.UpdateScene(w.GetTimer()->GetTimeDeltaSeconds());
		renderer.RenderScene();
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_T)) {
			renderer.DrawPostProcess();
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_U)) {
			renderer.DrawTwoScene();
			renderer.PresentTwoScene();
		}
		else
			renderer.PresentScene();
		
		renderer.SwapBuffers();

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
			Shader::ReloadAllShaders();
		}

	}
	return 0;
}