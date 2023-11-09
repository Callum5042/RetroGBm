#pragma once

#include "Window.h"
#include <memory>

//class Render::RenderTarget;
//class Render::RenderTexture;

#include "Render/RenderTarget.h"
#include "Render/RenderTexture.h"

class MainWindow;

class TileWindow : public Window
{
public:
	TileWindow(Application* application);
	virtual ~TileWindow() = default;

	void CreateTilemapWindow();
	void Update();

	inline float GetTileWindowScale() const { return m_TileWindowScale; }

protected:
	virtual void OnClose() override;

private:
	std::unique_ptr<Render::RenderTarget> m_TileRenderTarget = nullptr;
	std::unique_ptr<Render::RenderTexture> m_TileRenderTexture = nullptr;

	float m_TileWindowScale = 4.0f;

	void UpdateTilemapTexture();
};