#pragma once

#include "Window.h"
#include <string>
#include <vector>

class NetworkConnectWindow;

class BaseComponent
{
public:
	void SetParent(BaseComponent* parent)
	{
		this->parent = parent;
	}

	inline BaseComponent* GetParent() const
	{
		return parent;
	}

	BaseComponent* AddChild(BaseComponent* child)
	{
		children.push_back(child);
		return child;
	}

protected:
	BaseComponent* parent = nullptr;
	std::vector<BaseComponent*> children;
};

class ContainerComponent : public BaseComponent
{
public:

};

class TextComponent : public BaseComponent
{
public:
	TextComponent(const NetworkConnectWindow& window, const std::string& text);

private:
	HWND m_Hwnd = NULL;
};

class ButtonComponent : public BaseComponent
{
public:
	ButtonComponent(const NetworkConnectWindow& window, const std::string& text);

private:
	HWND m_Hwnd = NULL;
	const int m_ControlId = 2001;
};

class NetworkConnectWindow
{
public:
	NetworkConnectWindow();
	virtual ~NetworkConnectWindow();

	void Create();
	void Destroy();

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	inline HWND GetHwnd() const
	{
		return m_Hwnd;
	}

private:


	void WindowCreate(const std::string& title, int width, int height);
	HWND m_Hwnd = NULL;
	std::wstring m_RegisterClassName;

	HWND m_LabelName;

	HWND m_ButtonAdd;
	const int m_ControlAddButtonId = 2001;


	// Components

	ContainerComponent* Container(std::vector<BaseComponent*> components);
	TextComponent* Label(const std::string& text);
	ButtonComponent* Button(const std::string& text);

	BaseComponent* RootComponent = nullptr;
};