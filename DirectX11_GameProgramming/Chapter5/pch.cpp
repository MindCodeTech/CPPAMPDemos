/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : pch.cpp
 * File Description : 
 */
#include "pch.h"

GraphicsDevice DX::GDevice;
FirstCamera^ DX::Camera;
concurrency::accelerator_view  DX::accViewObj = concurrency::accelerator().default_view;
bool DX::UseDispMap;
