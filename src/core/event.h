#pragma once

#include "core/core.h"
#include "core/keycodes.h"

enum class EventType
{
	None = 0,
	WindowResized, WindowClosed,
	KeyPressed, KeyReleased,
	MouseButtonPressed, MouseButtonReleased, MouseScrolled, MouseMoved
};

enum EventCategory
{
	None = 0,
	EventCategoryKeyboard = 1,
	EventCategoryApplication = 2,
	EventCategoryMouse = 4,
	EventCategoryInput = 8,
	EventCategoryMouseButton = 16
};

#define IMPL_EVENT_TYPE(Type, Category) \
	static EventType GetStaticType() { return EventType::##Type; } \
	virtual EventType GetType() const override { return GetStaticType(); } \
	virtual const char* GetName() const override { return #Type; } \
	virtual EventCategory GetCategory() const override { return (EventCategory)(Category); }

class Event
{
	friend class EventDispatcher;
public:
	virtual ~Event() = default;

	virtual EventType GetType() const = 0;
	virtual const char* GetName() const = 0;
	virtual EventCategory GetCategory() const = 0;

	inline bool IsHandled() const { return m_Handled; }
protected:
    bool m_Handled = false;
};

class EventDispatcher
{
public:
	EventDispatcher(Event& event)
		: m_Event(event) {}

	template<typename T, typename F>
	bool Dispatch(const F& fn)
	{
		if (m_Event.GetType() == T::GetStaticType())
		{
			m_Event.m_Handled = fn(static_cast<T&>(m_Event));
			return true;
		}

		return false;
	}

private:
	Event& m_Event;
};

class KeyPressedEvent : public Event
{
public:
	KeyPressedEvent(Key key)
		: m_Key(key) {}
	
	inline Key GetKey() const { return m_Key; }

	IMPL_EVENT_TYPE(KeyPressed, EventCategoryKeyboard | EventCategoryInput)
private:
	Key m_Key;
};

class KeyReleasedEvent : public Event
{
public:
	KeyReleasedEvent(Key key)
		: m_Key(key) {}

	inline Key GetKey() const { return m_Key; }

	IMPL_EVENT_TYPE(KeyReleased, EventCategoryKeyboard | EventCategoryInput)
private:
	Key m_Key;
};

class MouseButtonPressedEvent : public Event
{
public:
	MouseButtonPressedEvent(MouseButton button)
		: m_Button(button) {}

	inline MouseButton GetButton() const { return m_Button; }

	IMPL_EVENT_TYPE(MouseButtonPressed, EventCategoryMouse | EventCategoryInput)
private:
	MouseButton m_Button;
};

class MouseButtonReleasedEvent : public Event
{
public:
	MouseButtonReleasedEvent(MouseButton button)
		: m_Button(button) {}

	inline MouseButton GetButton() const { return m_Button; }

	IMPL_EVENT_TYPE(MouseButtonReleased, EventCategoryMouse | EventCategoryInput)
private:
	MouseButton m_Button;
};

class MouseScrolledEvent : public Event
{
public:
	MouseScrolledEvent(float xOffset, float yOffset)
		: m_XOffset(xOffset), m_YOffset(yOffset) {}

	inline float GetXOffset() const { return m_XOffset; }
	inline float GetYOffset() const { return m_YOffset; }

	IMPL_EVENT_TYPE(MouseScrolled, EventCategoryKeyboard | EventCategoryInput)
private:
	float m_XOffset;
	float m_YOffset;
};

class MouseMovedEvent : public Event
{
public:
	MouseMovedEvent(float xPos, float yPos)
		: m_XPos(xPos), m_YPos(yPos) {}

	inline float GetXPosition() const { return m_XPos; }
	inline float GetYPosition() const { return m_YPos; }

	IMPL_EVENT_TYPE(MouseMoved, EventCategoryKeyboard | EventCategoryInput)
private:
	float m_XPos;
	float m_YPos;
};

class WindowResizedEvent : public Event
{
public:
	WindowResizedEvent(uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height) {}

	inline uint32_t GetWidth() const { return m_Width; }
	inline uint32_t GetHeight() const { return m_Height; }

	IMPL_EVENT_TYPE(WindowResized, EventCategoryApplication)
private:
	uint32_t m_Width;
	uint32_t m_Height;
};

class WindowClosedEvent : public Event
{
public:
	WindowClosedEvent() = default;

	IMPL_EVENT_TYPE(WindowClosed, EventCategoryApplication)
};