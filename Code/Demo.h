/*
Copyright 2019
Original authors: Niko Procopi
Written under the supervision of David I. Schwartz, Ph.D., and
supported by a professional development seed grant from the B. Thomas
Golisano College of Computing & Information Sciences
(https://www.rit.edu/gccis) at the Rochester Institute of Technology.
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.
<http://www.gnu.org/licenses/>.

Special Thanks to Exzap from Team Cemu,
he gave me advice on how to optimize Vulkan
graphics, he is working on a Wii U emulator
that utilizes Vulkan, see more at http://cemu.info
*/

#pragma once

// This is a simple helper that gives
// us the size of an array
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

// This is a simple helper function
// that allows us to print errors when
// they happen. We give it a message,
// and then it makes a box appear that gives
// the message to the user. We will do this
// to display all errors
#define ERR_EXIT(err_msg, err_class)                                             \
    do {                                                                         \
        MessageBox(NULL, err_msg, err_class, MB_OK); \
        exit(1);                                                                 \
    } while (0)

// Next thing we do is include all of the Vulkan headers

#define APP_NAME_STR_LEN 80
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>

// Allow a maximum of two outstanding presentation operations.
#define FRAME_LAG 2

class Demo
{
public:
	char name[APP_NAME_STR_LEN];  // Name to put on the window/icon
	HWND window;                  // hWnd - window handle
	POINT minsize;                // minimum window size

	VkSurfaceKHR surface;
	bool prepared;
	bool is_minimized;

	bool syncd_with_actual_presents;
	uint64_t refresh_duration;
	uint64_t refresh_duration_multiplier;
	uint64_t target_IPD;  // image present duration (inverse of frame rate)
	uint64_t prev_desired_present_time;
	uint32_t next_present_id;
	uint32_t last_early_id;  // 0 if no early images
	uint32_t last_late_id;   // 0 if no late images

	VkInstance inst;
	VkPhysicalDevice gpu;

	uint32_t enabled_extension_count;
	uint32_t enabled_layer_count;
	char *extension_names[64];
	char *enabled_layers[64];

	int width, height;
	VkFormat format;
	VkColorSpaceKHR color_space;

	VkCommandPool cmd_pool;
	VkRenderPass render_pass;
	
	bool validate;
	uint32_t current_buffer;

	void prepare_console();
	void prepare_window();
	void prepare_instance();
	void prepare_physical_device();
	void prepare_instance_functionPointers();
	void prepare_surface();
	void prepare_device_queue();
	void prepare_device_functionPointers();
	void prepare();


	void delete_resolution_dependencies();
	void run();

	Demo();
	~Demo();
};

