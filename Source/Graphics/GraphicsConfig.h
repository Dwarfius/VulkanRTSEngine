#pragma once

struct GraphicsConfig
{
	// Dictates how many frames can Graphics schedule
	// for rendering before it runs out of space/ability
	constexpr static uint8_t kMaxFramesScheduled = 2;
};