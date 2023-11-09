#pragma once

#include "Window.h"

class TileWindow : public Window
{
public:
	TileWindow(Application* application);
	virtual ~TileWindow() = default;
};