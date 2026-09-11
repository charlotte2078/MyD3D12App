#include "Input.h"

// Global variables

// Current state of all keys
EKeyState gKeyStates[NumKeyCodes];

// Current mouse position
int gMouseX;
int gMouseY;

// Initialise the input system
void InitInput()
{
	// Key states defualt to not pressed
	for (int i = 0; i < NumKeyCodes; i++)
	{
		gKeyStates[i] = NotPressed;
	}

	// Initial mouse position defaults to (0, 0)
	gMouseX = 0;
	gMouseY = 0;
}

// Event to indicate a key has been pressed down
void KeyDownEvent(const EKeyCode Key)
{
	// If the key was previously not pressed, it is now pressed.
	// Otherwise the key is still being held down.
	if (gKeyStates[Key] == NotPressed)
	{
		gKeyStates[Key] = Pressed;
	}
	else
	{
		gKeyStates[Key] = Held;
	}
}

// Event to indicate a key has been released
void KeyUpEvent(const EKeyCode Key)
{
	// No matter its previous state, the key is now not pressed.
	gKeyStates[Key] = NotPressed;
}

// Event to indicate the mouse has been moved
void MouseMoveEvent(const int X, const int Y)
{
	gMouseX = X;
	gMouseY = Y;
}

// Returns true when a given key or button is first pressed down
bool KeyHit(const EKeyCode Key)
{
	if (gKeyStates[Key] == Pressed)
	{
		gKeyStates[Key] = Held;
		return true;
	}
	
	return false;
}

// Returns true as long as a given key or button is held down
bool KeyHeld(const EKeyCode Key)
{
	if (gKeyStates[Key] == NotPressed)
	{
		return false;
	}

	gKeyStates[Key] = Held;
	
	return true;
}

// Returns current X position of mouse
int GetMouseX()
{
	return gMouseX;
}

// Returns current Y position of mouse
int GetMouseY()
{
	return gMouseY;
}
