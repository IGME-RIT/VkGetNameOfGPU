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

#include "Demo.h"

#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>
#include <vector>
#include "Main.h"

// This boolean keeps track of how many times we have executed the
// "prepare()" function. If we have never used the function before
// then we know we are initializing the program for the first time,
// if the boolean is false, then we know the program has already
// been initialized
bool firstInit = true;

void Demo::prepare_console()
{
	// This line is commented out,
	// if you uncomment this "freopen" line,
	// it will redirect all text from the console
	// window into a text file. Then, nothing will
	// print to the console window, but it will all
	// be saved to a file

	// freopen("output.txt", "a+", stdout);

	// If you leave that line commented out ^^
	// then everything will print to the console
	// window, just like normal

	// Create Console Window
	
	// Allocate memory for the console
	AllocConsole();
	
	// Attatch the console to this process,
	// so that printf statements from this 
	// process go into the console
	AttachConsole(GetCurrentProcessId());

	// looks confusing, but we never need to change it,
	// basically this redirects all printf, scanf,
	// cout, and cin to go to the console, rather than
	// going where it goes by default, which is nowhere
	FILE *stream;
	freopen_s(&stream, "conin$", "r", stdin);
	freopen_s(&stream, "conout$", "w", stdout);
	freopen_s(&stream, "conout$", "w", stderr);
	SetConsoleTitle(TEXT("Console window"));

	// Adjust Console Window
	// This is completely optional

	// Get the console window
	HWND console = GetConsoleWindow();

	// Move the window to position (0, 0),
	// resize the window to be 640 x 360 (+40).
	// The +40 is to account for the title bar
	MoveWindow(console, 0, 0, 640, 360 + 40, TRUE);
}

void Demo::prepare_window()
{
	// Make the title of the screen "Loading"
	// while the program loads
	strncpy(name, "Loading...", APP_NAME_STR_LEN);

	WNDCLASSEX win_class = {};

	// Initialize the window class structure:
	// The most important detail here is that we
	// give it the WndProc function from main.cpp,
	// so that the function can handle our window.
	// Everything else here sets the icon, the title,
	// the cursor, thing like that
	win_class.cbSize = sizeof(WNDCLASSEX);
	win_class.style = CS_HREDRAW | CS_VREDRAW;
	win_class.lpfnWndProc = WndProc;
	win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	win_class.lpszMenuName = NULL;
	win_class.lpszClassName = name;
	win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	// RegisterClassEx will do waht it sounds like,
	// it registers the WNDCLASSEX structure, so that
	// we can use win_class to make a window

	// If the function fails to register win_class
	// then give an error
	if (!RegisterClassEx(&win_class))
	{
		// It didn't work, so try to give a useful error:
		printf("Unexpected error trying to start the application!\n");
		fflush(stdout);
		exit(1);
	}

	// Create window with the registered class:
	RECT wr = { 0, 0, width, height };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	// We will now create our window
	// with the function "CreateWindowEx"
	// Win32 and DirectX 11 have some functions
	// with a huge ton of parameters. What 
	// Vulkan does for all functions, and what
	// DirectX 11 sometimes does, is organize
	// all parameters into a structure, and then
	// pass the structure as one parameter
	window = CreateWindowEx(0,
		name,            // class name
		name,            // app name
		WS_OVERLAPPEDWINDOW |  // window style
		WS_VISIBLE | WS_SYSMENU,

		// The position (0,0) is the top-left
		// corner of the screen, which is where
		// the command prompt is, and the command 
		// prompt is 640 pixels wide, so lets put
		// the Vulkan window right next to the console window
		640, 0,				 // (x,y) position
		wr.right - wr.left,  // width
		wr.bottom - wr.top,  // height
		NULL,                // handle to parent, no parent exists

		// This would give you "File" "Edit" "Help", etc
		NULL,                // handle to menu, no menu exists
		(HINSTANCE)0,		 // no hInstance
		NULL);               // no extra parameters

	// if we failed to make the window
	// then give an error that the window failed
	if (!window)
	{
		// It didn't work, so try to give a useful error:
		printf("Cannot create a window in which to draw!\n");
		fflush(stdout);
		exit(1);
	}

	// Window client area size must be at least 1 pixel high, to prevent crash.
	minsize.x = GetSystemMetrics(SM_CXMINTRACK);
	minsize.y = GetSystemMetrics(SM_CYMINTRACK) + 1;
}

void Demo::prepare_instance()
{
	// The first thing we need to do, is choose the 
	// "layers" and "extensions" that we want to use in Vulkan
	// "Extensions" give us additional features, like ray tracing.
	// By default, Vulkan does not have support for any type of 
	// window: not iOS, Android, Windows, Linux, Mac, GLFW, 
	// or any type of Window, those are all available in different
	// extensions, which we will be using

	// "Layers" are what they sound like, they sit between
	// the driver and our code.
	// if we have no layers, then our code will speak
	// directly to the Vulkan driver, if we put a layer
	// between our code and the driver, the code will not be
	// as fast, but layers can still be very helpful, or even
	// simplify the programming process

	// In this case, we want the "Khronos Validation Layer".
	// Just like an HTML validation (which you may have used if you
	// made a website), the Vulkan validator will check our code for us,
	// and tell us if it thinks we've made any mistakes. This layer
	// can be disabled when development of a project is finished
	char *instance_validation_layer = "VK_LAYER_KHRONOS_validation";

	// our window is not minimized
	is_minimized = false;

	// We have one validation layer that we want to enable,
	// but we do not know if the layer is supported on the
	// computer that is running the software, so lets quickly
	// find out if it is supported

	// we create a boolean to see if we found the layer
	VkBool32 validation_found = 0;

	// only enable validation if the validate bool
	// is true. This boolean was set to true in init().
	// This is set to true or false by the programmer, it
	// does not depend on anything. It should be set to true
	// during development, and set to false when it is time 
	// to release the software
	if (validate)
	{
		// set the number of instance layers to zero
		// This is the number of total layers supported by 
		// our vulkan device, we set it to zero by default
		uint32_t total_instance_layers = 0;

		// What you are about to see, is something that Vulkan
		// does a lot, so please make sure you understand this 
		// before continuing. The function we are about to use is
		// vkEnumerateInstanceLayerProperties, it takes two parameters,
		// the first parameter is a pointer to a uint32_t number, and
		// the other parameter is a pointer to an array

		// if the array is set to null, 
		// then vkEnumerateInstanceLayerProperties will set the
		// uint32_t number equal to the total number of layers
		// that are available.

		// if the array is not set to null,
		// then vkEnumerateInstanceLayerProperties will fill the
		// array with elements, the number of elements will be equal
		// to the uint32_t number given in the first parameter

		// That means we will use vkEnumerateInstanceLayerProperties
		// two times, the first time will give us the number of
		// layers, and the second time will give us the array

		// call vkEnumerateInstanceLayerProperties for the 1st time,
		// get number of instance layers supported by the GPU
		vkEnumerateInstanceLayerProperties(&total_instance_layers, NULL);

		if (total_instance_layers > 0)
		{
			// Create an array that can hold the properties of every layer available
			// The properties of each layer (VkLayerProperties) will tell us 
			// the name of the layer, the version, and even a description of the layer
			VkLayerProperties* instance_layers = new VkLayerProperties[total_instance_layers];

			// call vkEnumerateInstanceLayerProperties for the 2nd time,
			// get the properties of all instance layers that are available
			vkEnumerateInstanceLayerProperties(&total_instance_layers, instance_layers);

			// If you did not understand the last three lines, please read the comments again.
			// This pattern WILL be used several times, maybe over 10 times, throughout the
			// course of the program

			// out of all the available layers,
			// check all the layers to see if the Khronos Validation Layer is supported.
			// We are checking each layer to see if the name of each layer is equal
			// to the name of the layer we want. If the layer we want is on the list
			// of supported layers, then we can use the validation layer, which we 
			// set at the beginning of the function
			for (uint32_t j = 0; j < total_instance_layers; j++)
			{
				// check to see if this layer has the same name as the one we want
				if (!strcmp(instance_validation_layer, instance_layers[j].layerName))
				{
					// we found the validation layer, woo!
					validation_found = 1;
					break;
				}
			}

			// we set the number of enabled layers to zero.
			// regardless if we found the validation layer in the 'for'
			// loop or not, we still have not enabled any layers yet
			enabled_layer_count = 0;

			// If we found the validation layer
			if (validation_found)
			{
				// then we have one layer that we can enable
				enabled_layer_count = 1;

				// add our validation layer to the list
				// of enabled layers. Technically this layer
				// is still not enabled yet, but it is guarranteed
				// that it can be enabled successfully, because we
				// just confirmed that it is supported. Don't worry
				// though, it will be enabled by the end of this function
				enabled_layers[0] = instance_validation_layer;
			}

			// we dont need the full list of instance layers
			// we can remove it now that we're done with it
			free(instance_layers);
		}

		// if we did not find a validation layer,
		// or, if we did not find any layers at all
		if (!validation_found || total_instance_layers <= 0)
		{
			// Give the user an error that we failed to find the validation layer.
			// If this error is found, you can fix it by disabling validate
			// in init()
			ERR_EXIT(
				"vkEnumerateInstanceLayerProperties failed to find required validation layer.\n\n"
				"Please look at the Getting Started guide for additional information.\n",
				"vkCreateInstance Failure");
		}
	}

	// some tutorials will have VkApplicationInfo,
	// and then that will be put inside the 
	// structure of VkInstanceCreateInfo. However,
	// I personally believe that it is a waste of time.
	// All it does is hold the name of the app, the version,
	// the name of the engine, and the version, it literally
	// does not do or change anything in the program, so I 
	// just ignore it

	// It is time to talk about another pattern that 
	// will be used in all over the Vulkan program.

	// When we create the structure VkInstanceCreateInfo,
	// we will give it an "sType" that is ALWAYS equal 
	// to VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO.

	// Some people wonder "what is the point of having the
	// ability to change sType if it should always be the 
	// same anyways?"

	// Here is why: Eventually, we will take our object
	// of VkInstanceCreateInfo, which in this case is 
	// called "inst_info", and we will pass it to the
	// function "vkCreateInstance". When we do that,
	// we will give it a pointer to inst_info.

	// vkCreateInstance will check to see if the
	// first four bytes of the pointer is equal to 
	// VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO. If it
	// is equal, then that means we passed the correct
	// pointer to the function, which is "inst_info".
	// If vkCreateInstance does not see "VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO",
	// then that means the programmer might have made
	// a mistake, by giving the wrong poniter to the
	// vkCreateInstance function, and then the validation
	// layer will print an error in the command prompt window
	// that we made a mistake.

	// Therefore, the reason why every Vk---CreateInfo
	// structure has sType, is to help us make sure that
	// we are not making mistakes. If you do not understand this
	// then please read the commments again, this will
	// be a pattern that is seen throughout the entire tutorial

	// Also, the reason why Vulkan has structures for "create Info",
	// is so that we can pass the structure as one parameter,
	// rather than passing all of these variables as seperate
	// parameters. Some DirectX 11 functions like D3DCreateDeviceAndSwapchain,
	// have 10 parameters, and Vulkan puts the parameters into a structure,
	// to make the code look more organized

	// We create an "instance" of Vulkan, so that we can start
	// using vulkan features, this allows us to use Vulkan commands
	// that the driver provides

	// create our InstanceCreateInfo, with the necessary sType,
	// the number of layers we want to enable, the array of layers
	// that we want to enable, the number of extensions we want to enable,
	// and the array of extensions that we want to enable.
	VkInstanceCreateInfo inst_info = {};
	inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.enabledLayerCount = enabled_layer_count;
	inst_info.ppEnabledLayerNames = (const char *const *)enabled_layers;

	// Attempt to create a Vulkan Instance with the information provided.
	// We take the value that this returns, so that we can see if the
	// instance was created correctly
	VkResult err = vkCreateInstance(&inst_info, NULL, &inst);

	// If the function returns a value of -9,
	// then the driver is not compatible with Vulkan
	if (err == VK_ERROR_INCOMPATIBLE_DRIVER)
	{
		ERR_EXIT(
			"Cannot find a compatible Vulkan installable client driver (ICD).\n\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkCreateInstance Failure");
	}

	// if the function returns a value of -7,
	// then there is an extension that is not supported
	// that we tried to enable. This will probably never
	// happen since we wnt through the necessary checks
	// earlier in the function
	else if (err == VK_ERROR_EXTENSION_NOT_PRESENT)
	{
		ERR_EXIT(
			"Cannot find a specified extension library.\n"
			"Make sure your layers path is set appropriately.\n",
			"vkCreateInstance Failure");
	}

	// if any other value is returned that is not zero,
	// then we got an unknown error
	else if (err)
	{
		ERR_EXIT(
			"vkCreateInstance failed.\n\n"
			"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkCreateInstance Failure");
	}
}

void Demo::prepare_physical_device()
{
	// Now that we have an instance of Vulkan, we need
	// to determine how many GPUs are available that
	// can use Vulkan. We will be following the same
	// pattern as before: get the number of elements,
	// make an array, give the array to a function, and
	// the function will fill it with data

	// set the number of GPUs available to zero
	uint32_t gpu_count = 0;

	// In Vulkan, a PhysicalDevice holds all the information
	// about a GPU that is in a computer: how much memory it has,
	// what features it supports, the name of the GPU, the
	// company that made the GPU, the amount of processors,
	// literally everything.

	// HOWEVER, the PhysicalDevice only holds information
	// about the GPU, we cannot use PhysicalDevice to 
	// give commands to the GPU. For that, we need to 
	// create a "Device" from the same GPU. The first time
	// we do this, it will be confusing, but it will 
	// eventually make sense.

	// use the function vkEnumeratePhysicalDevices to get
	// the number of GPUs that are present in the computer.
	// we set the array of VkPhysicalDevice to NULL, so that
	// the function knows that it is looking for the number 
	// of GPUs
	vkEnumeratePhysicalDevices(inst, &gpu_count, NULL);

	// if we found a GPU
	if (gpu_count > 0)
	{
		// Create an array that will hold all PhysicalDevices in the computer
		VkPhysicalDevice* physical_devices = new VkPhysicalDevice[gpu_count];

		// Pass the array to this function, and then the array
		// will be filled with data that tells us about every GPU
		// in the computer that supports Vulkan
		vkEnumeratePhysicalDevices(inst, &gpu_count, physical_devices);

		// Each PhsyicalDevice be a dedicated graphics card, 
		// or an integraded graphics chip in a CPU, some devices
		// support graphics, some only support compute, there are
		// a lot of different types of GPUs out there that support Vulkan.

		// If someone wanted to, they could do a search to find which GPU
		// in the list most powerwful, or they could check for different
		// properties, but for this simple example, just take 
		// the first on the list, because that is usually best one

		// If you have multiple GPUs in your computer, maybe you have
		// both a dedicated GPU and a GPU integrated in the CPU,
		// set your dedicated GPU as your default graphics processor
		// in the Nvidia Control Panel, or AMD Catalayst, or Intel
		// Graphics Settings, and then that one will be the first
		// that shows up in the array
		gpu = physical_devices[0];

		// we do not need all of the devices anymore, we have
		// the one that we want
		free(physical_devices);

		// We want to get properteis from the physical device.
		// This is completely option, I did it because I feel like it
		VkPhysicalDeviceProperties features;

		// This gives us the name of the GPU, the number of processors
		// on the GPU, the amount of memory, the company that made the
		// GPU, everything there is to know
		vkGetPhysicalDeviceProperties(gpu, &features);

		// set the title of the window to the name of the GPU,
		// so that we know we are using the GPU that we want to use
		SetWindowText(window, features.deviceName);

		printf("We found a GPU, the name of the GPU is:\n");
		printf("%s\n\n", features.deviceName);
	}

	// If no GPUs were found, then 
	// give an error to the user
	else
	{
		ERR_EXIT(
			"vkEnumeratePhysicalDevices reported zero accessible devices.\n\n"
			"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkEnumeratePhysicalDevices Failure");
	}

	// When we created the instance, we chose to use some extensions of the instance,
	// such as the SURFACE and WIN32 extensions. Now, we will enable extensions from
	// the PhysicalDevice. These extensions will help us with creating the "swapchain".

	// If a student does not know what a swapchain is, don't worry, the "swapchain" will
	// be fully explained in the prepare_swapchain() function, explaining what it is 
	// and why we need it. For now, here is how we enable the swapchain extension.

	// by default, we have not found the swapchain extension (yet)
	VkBool32 swapchainExtFound = 0;

	// Set the number of enabled_extensions to zero,
	// and clear the list of extension names. These
	// are the same variables we used for the instance,
	// but the instance doesn't need these variables now,
	// so we will re-use them for a similar purpose
	enabled_extension_count = 0;
	memset(extension_names, 0, sizeof(extension_names));

	// By default, there are zero device extensions that exist (that we know of)
	uint32_t device_extension_count = 0;

	// call this function to find out how many device extensions are available
	vkEnumerateDeviceExtensionProperties(gpu, NULL, &device_extension_count, NULL);

	// if there are more than zero extensions available,
	// then look for the extension that we want
	if (device_extension_count > 0)
	{
		// first we make a list that can hold all of the device's extensions
		VkExtensionProperties* device_extensions = new VkExtensionProperties[device_extension_count];

		// then we call the same function again to get the list of all extensions that are available
		vkEnumerateDeviceExtensionProperties(gpu, NULL, &device_extension_count, device_extensions);

		// we search through the list to look for the extension that we want,
		// which in this case, is the swapchain extension
		for (uint32_t i = 0; i < device_extension_count; i++)
		{
			// if we find the swapchain extension on the list of supported extensions
			if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, device_extensions[i].extensionName))
			{
				// set this boolean so we know that we found it
				swapchainExtFound = 1;

				// add the swapchain to our list of extensions,
				// and increment the counter for the number of extensions
				extension_names[enabled_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
			}
		}

		// we do not need the list of extensions anymore,
		// now that we have already searched through it,
		// so we can delete it now
		free(device_extensions);
	}

	// if the swapchain was not found, then give an error and let the
	// user know that the swapchain could not be found
	if (!swapchainExtFound)
	{
		ERR_EXIT("vkEnumerateDeviceExtensionProperties failed to find the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
			" extension.\n\nDo you have a compatible Vulkan installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkCreateInstance Failure");
	}
}

void Demo::prepare()
{
	// We will be calling prepare() multiple times.
	// Some Vulkan assets only need to be created once, like
	// the instance, the device, and the queue (i'll explain those soon),
	// while some things need to be destroyed and rebuilt, like the 
	// swapchain images (I'll explain those soon).

	// We keep track of a variable called firstInit, to determine
	// if we have run the prepare() function before. "firstInit"
	// is initialized as true at the top of Demo.cpp, and it is
	// set to false after the first initialization is done

	if (firstInit)
	{
		// Validation will tell us if our Vulkan code is correct.
		// Sometimes, our code will execute the way we want it to,
		// but just becasue the code runs, does not mean the code
		// is correct. If you've ever used HTML, you may have used
		// and HTML validator, where your website might look correct,
		// but the validator will tell you that the code is wrong

		// If you run in Debug mode, no Validator text will appear,
		// text from Validator will only appear if you are in Release Mode

		// During development, this is a great tool. However, when it is
		// time to release a software or game, you don't want this running
		// in the background because it will continue constantly checking for errors
		// even if there are no errors. So, when you want to release a software or
		// game, simply set this to false.
		validate = true;

		// During development, it is good to have a console window.
		// You can read errors, and write printf statements.
		// However, if you want to release a software or game, you may
		// not want a console window. Simply comment out this line to 
		// disable the console window.
		prepare_console();

		// set the width and height of the window
		// literally set this to whatever you want
		width = 640;
		height = 360;

		// build the window with the Win32 API. This will look similar to how
		// a window is created in a DirectX 11/12 engine, and we will use the
		// WndProc from main.cpp to create the window
		prepare_window();

		// We create an instance of Vulkan, this allows us to use VUlkan
		// commands on the CPU, but we will not yet be able to talk to 
		// the graphics device, that comes later
		prepare_instance();

		// The physical device gives us all the properties of the GPU
		// that we want to use to render, such as the name of the GPU,
		// how much memory it has, what features it supports, etc.
		// We cannot send commands to the GPU through the PhysicalDevice,
		// but we can use it to determine what our GPU can do.
		prepare_physical_device();
	}
}


Demo::Demo()
{
	// Welcome to the Demo constructor
	// The Demo class will handle the majority
	// of our code in this tutorial

	// Please look at Demo.h, 
	// where you can see a list of every variable that
	// we will use in the Demo class. Every variable
	// in this class will be fully explained while
	// we move through the code

	// The first thing we do is initalize the scene
	prepare();
}

Demo::~Demo()
{
	// Destroy Vulkan Instance
	vkDestroyInstance(inst, NULL);
}
