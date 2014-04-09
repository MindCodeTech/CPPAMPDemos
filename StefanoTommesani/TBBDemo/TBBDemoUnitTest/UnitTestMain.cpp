// Intel TBB Demo
// by Stefano Tommesani (www.tommesani.com) 2013
// this code is release under the Code Project Open License (CPOL) http://www.codeproject.com/info/cpol10.aspx
// The main points subject to the terms of the License are:
// -   Source Code and Executable Files can be used in commercial applications;
// -   Source Code and Executable Files can be redistributed; and
// -   Source Code can be modified to create derivative works.
// -   No claim of suitability, guarantee, or any warranty whatsoever is provided. The software is provided "as-is".
// -   The Article(s) accompanying the Work may not be distributed or republished without the Author's consent

#include "stdafx.h"
#include "CppUnitTest.h"

#include <TBBDemoRoutines.h>
#include <TBBDemoAMP.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TBBDemoUnitTest
{		
	const int DEFAULT_IMAGE_WIDTH = 1 * 1024;
	const int DEFAULT_IMAGE_HEIGHT = 1 * 1024;
	const int DEFAULT_IMAGE_SIZE = (DEFAULT_IMAGE_WIDTH * DEFAULT_IMAGE_HEIGHT);
	const int RGBA_PIXEL_SIZE = 4;

	TEST_CLASS(TBBUnitTest)
	{
	public:
		unsigned char *RGBAImage;
		unsigned char *SerialGrayImage;
		unsigned char *ParallelTBBGrayImage;

		TEST_METHOD_INITIALIZE(SetupBitmaps)
		{
			// create input image
			RGBAImage = new unsigned char[DEFAULT_IMAGE_SIZE * RGBA_PIXEL_SIZE];
			// fill RGBA image with random data
			srand(0x5555);
			for (int j = 0; j < (DEFAULT_IMAGE_SIZE * RGBA_PIXEL_SIZE); j++)
				RGBAImage[j] = rand() % 256;

			SerialGrayImage = new unsigned char[DEFAULT_IMAGE_SIZE];
			ParallelTBBGrayImage = new unsigned char[DEFAULT_IMAGE_SIZE];
			// build a reference gray image that will be compared to those built with multi-threaded code
			ProcessRGBSerial(RGBAImage, SerialGrayImage, DEFAULT_IMAGE_WIDTH, DEFAULT_IMAGE_HEIGHT, RGBA_PIXEL_SIZE);
			memset(ParallelTBBGrayImage, 0, DEFAULT_IMAGE_SIZE);  //< clear output image so that results from a previous run are zeroed
		}

		TEST_METHOD_CLEANUP(FreeBitmaps)
		{
			// free images
			delete[] SerialGrayImage;
			delete[] ParallelTBBGrayImage;
			delete[] RGBAImage;
		}
		
		TEST_METHOD(TestTBB1)
		{
			ProcessRGBTBB1(RGBAImage, ParallelTBBGrayImage, DEFAULT_IMAGE_WIDTH, DEFAULT_IMAGE_HEIGHT, RGBA_PIXEL_SIZE);
			Assert::IsTrue(memcmp(SerialGrayImage, ParallelTBBGrayImage, DEFAULT_IMAGE_SIZE) == 0);
		}

		TEST_METHOD(TestTBB2)
		{
			ProcessRGBTBB2(RGBAImage, ParallelTBBGrayImage, DEFAULT_IMAGE_WIDTH, DEFAULT_IMAGE_HEIGHT, RGBA_PIXEL_SIZE);
			Assert::IsTrue(memcmp(SerialGrayImage, ParallelTBBGrayImage, DEFAULT_IMAGE_SIZE) == 0);
		}

		TEST_METHOD(TestTBB3)
		{
			ProcessRGBTBB3(RGBAImage, ParallelTBBGrayImage, DEFAULT_IMAGE_WIDTH, DEFAULT_IMAGE_HEIGHT, RGBA_PIXEL_SIZE);
			Assert::IsTrue(memcmp(SerialGrayImage, ParallelTBBGrayImage, DEFAULT_IMAGE_SIZE) == 0);
		}

		TEST_METHOD(TestSIMD)
		{
			ProcessRGBSIMD(RGBAImage, ParallelTBBGrayImage, DEFAULT_IMAGE_WIDTH, DEFAULT_IMAGE_HEIGHT, RGBA_PIXEL_SIZE);
			Assert::IsTrue(memcmp(SerialGrayImage, ParallelTBBGrayImage, DEFAULT_IMAGE_SIZE) == 0);
		}

		TEST_METHOD(TestSIMD2)
		{
			ProcessRGBSIMD2(RGBAImage, ParallelTBBGrayImage, DEFAULT_IMAGE_WIDTH, DEFAULT_IMAGE_HEIGHT, RGBA_PIXEL_SIZE);
			Assert::IsTrue(memcmp(SerialGrayImage, ParallelTBBGrayImage, DEFAULT_IMAGE_SIZE) == 0);
		}

		TEST_METHOD(TestTBBSIMD)
		{
			ProcessRGBTBBSIMD(RGBAImage, ParallelTBBGrayImage, DEFAULT_IMAGE_WIDTH, DEFAULT_IMAGE_HEIGHT, RGBA_PIXEL_SIZE);
			Assert::IsTrue(memcmp(SerialGrayImage, ParallelTBBGrayImage, DEFAULT_IMAGE_SIZE) == 0);
		}		

		TEST_METHOD(TestTBBAMP)
		{
			ProcessRGBAMP(RGBAImage, ParallelTBBGrayImage, DEFAULT_IMAGE_WIDTH, DEFAULT_IMAGE_HEIGHT, RGBA_PIXEL_SIZE);
			Assert::IsTrue(memcmp(SerialGrayImage, ParallelTBBGrayImage, DEFAULT_IMAGE_SIZE) == 0);
		}
	};
}