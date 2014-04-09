// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <Windows.h>
#include <assert.h>
#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <random>

// d3d11 headers
#include <d3d11.h>
#include <dxgi.h>

// amp headers
#include <amp.h>
#include <amp_graphics.h>
#include <amp_math.h>

// my headers
#include "Timer.h"

// load libraries
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxgi")