/* by Raynoxis 2018 */

#ifndef VIDEORECORDCAPTURE_H
#define VIDEORECORDCAPTURE_H


#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/types_c.h>
#include <opencv2/core/core_c.h>
#include "opencv2/opencv.hpp"


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <exception>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <unistd.h>




class VideoRecordCapture
{
public:

	VideoRecordCapture();
	~VideoRecordCapture();


	int Start(std::string videoFolderSetup, int inputDevice, std::string deviceName);
	int Stop(int inputDevice);
	int GetImg(std::string imageFolderSetup, int inputDevice, std::string deviceName);
	void GetImgInVid(std::string imageFolder, int inputDevice);
	void SetParams(int inputDevice, std::string deviceName); 



	struct CameraParameter {
		char id[50];                     // id of camera used in bench test
	    int ocv_width;                   // width of camera with opencv
	    int ocv_height;                  // height of camera with opencv
	    int fps; 		       			 // number image used to average traitement and record video              
		char v4l2_params[80]; 		     // param v4l2 if necessary. 
	};

	// Parameter of two cameras used in bench
	static const CameraParameter listCameraParameter[];

private:

	void Acquisition(int inputDeviceSetup);
	void Ecriture(int inputDeviceSetup);
	void GetImgThread(int inputDeviceSetup, std::string videoFolderSetup);


// Tableau de Fifos contenant les frames lues, l'indice du tableau sera l'input video ex : pour video0 case 0 du tableau
	std::queue<cv::Mat> tabmatBuffer[10];
	std::queue<cv::Mat> tabgetImgBuffer[10];
// mutex permettant de gerer les accès
// non concurrents à la Fifo, de la même manière que les fifos
	std::mutex tabmtxBuffer[10];

// indicateur de fin positionné par le programme
// principal pour indiquer au thread de lecture
// qu'on arrete
// C'est un atomic pour garantir que son affection
// a vrai ou faux se fera de facon atomique c'est a dire
// en une seule instruction ininterruptible.

	std::atomic<bool> tabAcquisition_bool[10];
	std::atomic<bool> tabGetImg_bool[10];

	std::thread tabEcriture_thread[10];
	std::thread tabAcquisition_thread[10];
	std::thread tabGetImg_thread[10];

	int returnError[10];
	int opencv_width[10];
	int opencv_height[10];
	int opencv_fps[10];
	std::string v4l2_params[10];
	bool writterInitOk[10];
	bool getImgMutex_bool[10];
	std::string videoFolderTab[10];

	cv::VideoWriter   writer[10];
};

#endif // VIDEORECORDCAPTURE_H

