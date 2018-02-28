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
#include <unistd.h>
#include <chrono>


#include "videorecordcapture.h"


using namespace std;
using namespace cv;


VideoRecordCapture videorecordcapture_object;



int main(int argc, char *argv[])
{
	// PARAMS ENTREE
	//int cpt = 20;
	string deviceName = ""; // QSonic_Composite / QSonic_SVideo // WEBCAM
	int inputDev;
	int buff = 0;

	//while (cpt != 0)
	//{
	//	sleep(2);
	//	cpt --;
	//buff++;
	//if (buff == 3)
	//{
	//	buff = 0;
	//}


	cout << "Qu'elle entrée ? /dev/vidéo?" << endl;
	cin >> inputDev;
	cout << "OK,  /dev/video" << inputDev << " selectionné" << endl;
	cout << "Nom ? 1 => QSonic_SVideo, 2 => QSonic_Composite, 3 => DAZZLE_SVideo, 4 => DAZZLE_Composite, 5 => WEBCAM" << endl;
	cin >> buff;
	if (buff == 1)
	{
		deviceName = "QSonic_SVideo";
	}
	else if (buff == 2)
	{
		deviceName = "QSonic_Composite";
	}
	else if (buff == 3)
	{
		deviceName = "DAZZLE_SVideo";
	}
	else if (buff == 4)
	{
		deviceName = "DAZZLE_Composite";
	}
	else if (buff == 5)
	{
		deviceName = "WEBCAM";
	}
	cout << "OK, " << deviceName << " selectionné" << endl;


/*
	cout << "Main() :  GETIMG AVANT VIDEO" << endl;
	videorecordcapture_object.GetImg("ppmprevid.ppm", inputDev, deviceName);  // GET IMG hors video

*/
	cout << "Main() :  START_VIDEO" << endl;
	videorecordcapture_object.Start("testvideo02.avi", inputDev, deviceName); // S_VIDEO // START VIDEO


	std::cout << "Main() :  Wait 2s\n"; // DELAY
	std::this_thread::sleep_for(chrono::seconds(2));
	std::cout << "Main() :  Waited 2s\n"; // DELAY




	cout << "Main() :  GETIMG IN VIDEO 1" << endl;
	videorecordcapture_object.GetImgInVid("ppminvid01.ppm", inputDev); //getimginvid           // GET IMG in video
	//videorecordcapture_object.GetImgInVid("ppminvid11.ppm", 1); //getimginvid


	std::cout << "Main() :  Wait 3s\n";	// DELAY
	std::this_thread::sleep_for(chrono::seconds(3));
	std::cout << "Main() :  Waited 3s\n";	// DELAY


/*
	cout << "Main() :  GETIMG IN VIDEO 2" << endl;
	videorecordcapture_object.GetImgInVid("ppminvid02.ppm", inputDev); //getimginvid           // GET IMG in video
	//videorecordcapture_object.GetImgInVid("ppminvid12.ppm", 1); //getimginvid

	std::cout << "Main() :  Wait 5s\n"; // DELAY
	std::this_thread::sleep_for(chrono::seconds(5));
	std::cout << "Main() :  Waited 5s\n"; // DELAY


*/
	cout << "Main() :  STOP_VIDEO" << endl;
	int returnValue  = videorecordcapture_object.Stop(inputDev); //video0         // END VIDEO
	cout << "returnValue Stop ? " << returnValue << endl;

/*
	cout << "Main() :  GETIMG APRES VIDEO" << endl;
	videorecordcapture_object.GetImg("ppmpostvid.ppm", inputDev, deviceName); //    GET IMG hors video
//}
*/
	return (0);
}


