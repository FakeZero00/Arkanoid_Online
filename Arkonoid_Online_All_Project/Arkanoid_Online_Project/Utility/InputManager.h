#pragma once
#include <map>
#include <gl/glew.h>
#include <gl/glfw3.h>
using namespace std;

class InputManager {
public:
	void SetKey(int Key, bool isPressed) {
		currentKeys[Key] = isPressed;
	}

	void SetMousePosition(float x, float y) {
		mouseX = x;
		mouseY = y;
	}

	float GetMouseX() const {
		return mouseX;
	}

	float GetMouseY() const {
		return mouseY;
	}
	
	void Update() {
		previousKeys = currentKeys;
	}

	bool GetKeyDown(int Key) {
		bool curr = currentKeys[Key];
		bool prev = previousKeys[Key];
		return curr && !prev;
	}

	bool GetKeyPressed(int Key) {
		return currentKeys[Key];
	}

	bool GetKeyUp(int Key) {
		bool curr = currentKeys[Key];
		bool prev = previousKeys[Key];
		return !curr && prev;
	}

private:
	map<int, bool> currentKeys;
	map<int, bool> previousKeys;
	float mouseX = 0.0f;
	float mouseY = 0.0f;
};