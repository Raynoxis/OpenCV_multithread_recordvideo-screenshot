/* by Raynoxis 2018 */

#include "videorecordcapture.h"
#include <iostream>
#include <fstream>

using namespace std;
using namespace cv;



VideoRecordCapture::VideoRecordCapture()
{
}

VideoRecordCapture::~VideoRecordCapture()
{
}

// Paramètres vidéos des caméras utilisées. voir videorecordcapture.h
const VideoRecordCapture::CameraParameter VideoRecordCapture::listCameraParameter[] = {
	{"WEBCAM", 1280, 720, 30, ""},
	{"DAZZLE_Composite", 720, 576, 24, "-i 0 --set-fmt-video=width=720,height=576 -s PAL"},
	{"DAZZLE_SVideo", 720, 576, 24, "-i 1 --set-fmt-video=width=720,height=576 -s PAL"},
	{"QSonic_Composite", 720, 576, 24, "-i 0 -s PAL"},
	{"QSonic_SVideo", 720, 576, 30, "-i 1 -s 0"},
	{"" } // ne pas supprimer , necessaire pour bouclage de la lecture.
};

void VideoRecordCapture::SetParams(int inputDevice, string deviceName) // Récupération des paramètres de la caméra utilisée.
{
	cout << "Debut SetParams" << endl;
	int idTypeCamera;
	int i = 0;
	idTypeCamera = -1;
	while ( VideoRecordCapture::listCameraParameter[i].id[0] != '\0' ) // Récupération de l'id de la caméra.
	{
		if (VideoRecordCapture::listCameraParameter[i].id == deviceName) {
			idTypeCamera = i;
			break;
		}
		i++;
	}
	// Start init of devices
	if (idTypeCamera == -1) {
		returnError[inputDevice] = -8; // "SetParams() : Camera type not supported"
		//cout << "SetParams() : Camera type not supported" << endl;
	}

	opencv_width[inputDevice] = VideoRecordCapture::listCameraParameter[idTypeCamera].ocv_width;  // Largeur vidéo paramètre Opencv si utilisé
	opencv_height[inputDevice] = VideoRecordCapture::listCameraParameter[idTypeCamera].ocv_height;  // Hauteur vidéo paramètre Opencv si utilisé
	opencv_fps[inputDevice] = VideoRecordCapture::listCameraParameter[idTypeCamera].fps;   // Image par seconde du device utilisé.
	v4l2_params[inputDevice] = VideoRecordCapture::listCameraParameter[idTypeCamera].v4l2_params; // pramètres V4l2 si existants
	cout << "Fin SetParams" << endl;
}



int VideoRecordCapture::Start(string videoFolderSetup, int inputDevice, string deviceName)
{
	cout << "Debut Start" << endl;
	SetParams(inputDevice, deviceName); // remplissage des variables globales en fonction du périphérique vidéo d'entrée.
	videoFolderTab[inputDevice] = videoFolderSetup; // string : dossier video pour thread ecriture

	// INITILISATION PRIORITE THREAD ACQUISITION
	struct sched_param	param;
	pthread_t	acquisitionThread_id = 0;
	int			resPriority;
	std::stringstream	ss;
	ss.str(std::string()); // reset du stringstream utile !

	getImgMutex_bool[inputDevice] = 0;  //Initialisation de la variable d'activation/desactivation des mutex. (Optimisation lors d'un GetImg qui n'utilise pas les mutex)
	returnError[inputDevice] = 0; //Initialisation de la variable de retour.
	writterInitOk[inputDevice] = false; //Initialisation du writter pour enregistrement video.
	tabGetImg_bool[inputDevice].store(false); // Initialisation du GetImginVid
	tabAcquisition_bool[inputDevice].store(true); // Activation Thread Acquisition


	tabAcquisition_thread[inputDevice] = std::thread(&VideoRecordCapture::Acquisition, this, inputDevice); // Thread Acquisition vidéo
	tabEcriture_thread[inputDevice] = std::thread(&VideoRecordCapture::Ecriture, this, inputDevice); // Thread Ecriture vidéo


	// SET PRIORITE THREAD ACQUISITION
	memset ( &param, 0, sizeof (param) );
	param.sched_priority = 11 ;
	ss << tabAcquisition_thread[inputDevice].get_id();
	acquisitionThread_id = std::stoull (ss.str());
	resPriority = pthread_setschedparam ( acquisitionThread_id, SCHED_FIFO, &param);


	if ( resPriority != 0 ) {
		//cout << "Start() :  WARNING: Error while setting thread priority" << endl;
		returnError[inputDevice] = -5;
		return -1;
	}

	sleep (1);
	cout << "Fin Start" << endl;

	return (0);
}


int VideoRecordCapture::Stop(int inputDevice)
{
	cout << "Debut Stop" << endl;
	tabAcquisition_bool[inputDevice].store (false); // Changement de l'atomic Bool du Thread Acquisition pour l'arreter
	if (tabAcquisition_thread[inputDevice].joinable())
	{
		tabAcquisition_thread[inputDevice].join();
		cout << "Stop() :  tabAcquisition_thread join" << endl ;
		if (tabEcriture_thread[inputDevice].joinable())
		{
			cout << "Stop() :  tabEcriture_thread ready to join" << endl ;
			tabEcriture_thread[inputDevice].join();
			cout << "Stop() :  tabEcriture_thread join" << endl ;

			switch (returnError[inputDevice])
			{
			case -1 :
				return (-1); // Acquisition() : unable to open video
			case -2	:
				return (-2); //Acquisition() : bug frame
			case -3 :
				return (-3); //Ecriture() : unable to read a frame from video
			case -5 :
				return (-5); //Start() : error setting priority
			case -6 :
				return (-6); //Ecriture() : error writter
			case -7 :
				return (-7); //GetImgThread() : Timeout buffer.
			case -8 :
				return (-8); //SetParams() : Id Camera problem.
			default :
			{
				cout << "Fin Stop" << endl;
				return (0); // good
			}
			}
		}
		else
		{
			return (-4); // thread unreacheable
		}
	}
	else
	{
		return (-4); // thread unreacheable
	}

}



void VideoRecordCapture::Acquisition(int inputDevice)
{
	cout << "Debut Acquisition" << endl;
	Mat	src, frame;

	string conv = ("v4l2-ctl -d /dev/video" + to_string(inputDevice) + " " + v4l2_params[inputDevice]);
	system(conv.c_str());  // v4l2 params SVideo/Composite  | PAL format ==> Qsonic/Dazzle
	//cout << "Acquisition() : systemcommand :  " << conv << endl;


	VideoCapture cap(inputDevice); // Ouverture du device /dev/video'inputDevice'

	if (!cap.isOpened())
	{
		returnError[inputDevice] = -1;
		//cout << "Acquisition() :  error cap" << endl ;
	}
	else
	{
		if (opencv_width[inputDevice] != 0 && opencv_height[inputDevice] != 0) // Si on a un paramètre OpenCV mentionné (!=0).
		{
		   cout << "largeur" << cap.get(CV_CAP_PROP_FRAME_WIDTH) << endl;
			cout << "hauteur" << cap.get(CV_CAP_PROP_FRAME_HEIGHT) << endl;
			cap.set(CV_CAP_PROP_FRAME_WIDTH, opencv_width[inputDevice]);
			cap.set(CV_CAP_PROP_FRAME_HEIGHT, opencv_height[inputDevice]);
			cout << "largeur2" << cap.get(CV_CAP_PROP_FRAME_WIDTH) << endl;
			cout << "hauteur2" << cap.get(CV_CAP_PROP_FRAME_HEIGHT) << endl;
		}

		while (tabAcquisition_bool[inputDevice].load() == 1 || tabGetImg_bool[inputDevice].load() == 1)  // tant que pas de "end_video" OU "getimg pas terminé""
		{


			if (!cap.read( src )) // Lecture d'une frame
			{
				returnError[inputDevice] = -2;
				break;
			}

			if (getImgMutex_bool[inputDevice] == 0) // GETIMG hors enregistrement vidéo => pas besoin de mutex, l'écriture peut se faire après l'acquisition.
			{
				tabmtxBuffer[inputDevice].lock (); // MUTEX sur le buffer entre Acquisition vidéo et écriture vidéo.
				tabmatBuffer[inputDevice].push ( src.clone() ); // Une frame de plus dans le buffer de la vidéo régit par le mutex
			}

			if (tabGetImg_bool[inputDevice].load() == 1) // Si on doit faire un GETIMG à l'instant t
			{
				tabgetImgBuffer[inputDevice].push ( src.clone() ); // // Une frame de plus dans le buffer du getimg pas de mutex
				if (tabgetImgBuffer[inputDevice].size() == opencv_fps[inputDevice]) // nombre d'images suffisant pour moyenne => break.
				{
					//cout << "Acquisition() : Getimg buffer frames :  " << tabgetImgBuffer[inputDevice].size() << endl;
					tabGetImg_bool[inputDevice].store(false); // Le GetImgThread() prend le relais.
				}
			}

			if (getImgMutex_bool[inputDevice] == 0) // GETIMG hors enregistrement vidéo => pas besoin de mutex, l'écriture peut se faire après l'acquisition.
			{
				tabmtxBuffer[inputDevice].unlock (); // MUTEX
			}
		}
	}
	cout << "Acquisition() :  AcquisitionThread fin" << endl ;
}






void VideoRecordCapture::Ecriture(int inputDevice)
{
	cout << "Debut Ecriture" << endl;
	size_t	bufferSize;
	Mat	frame;
	bool frameOk = 0;

	do
	{
		tabmtxBuffer[inputDevice].lock (); // on demande a bloquer le mutex pour lire la fifo

		bufferSize = tabmatBuffer[inputDevice].size();
		if (bufferSize > 0) // si la fifo contient au moins une frame
		{
			tabmatBuffer[inputDevice].front().copyTo (frame); // on lit la frame la plus ancienne
			tabmatBuffer[inputDevice].pop(); // suppression de la dernière frame ajoutée.
			frameOk = 1; // une frame a écrire
		}
		tabmtxBuffer[inputDevice].unlock (); // on libere le mutex


		if (frameOk == 1)  // si on a une frame disponible a écrire
		{
			frameOk = 0;
			if (frame.empty())  // si il y avait au moins une frame on fait le traitement en écriture
			{
				returnError[inputDevice] = -3; // vide
				break;
			}
			else if (!writterInitOk[inputDevice]) // Initialistaion du fichier en écriture sur la première frame
			{
				//--- INITIALIZE VIDEOWRITER
				// ici avec un codec jpeg a adapter pour le plugin
				int codec = CV_FOURCC('M', 'J', 'P', 'G');	// select desired codec (must be available at runtime)
				bool isColor = (frame.type() == CV_8UC3);	// is it a colored frame


				writer[inputDevice].open(videoFolderTab[inputDevice], codec, opencv_fps[inputDevice], frame.size(), isColor); // creation du writer, ouverture du fichier en ecriture

				// check if we succeeded
				if (!writer[inputDevice].isOpened())
				{
					returnError[inputDevice] = -6;
				}

				//cout << "Ecriture() :  Ecriture Initializing; frame size: " << frame.size() << " frame color: " << isColor << endl;
				writterInitOk[inputDevice] = true;
			}
			writer[inputDevice].write(frame); // ecriture
		}

	} while (bufferSize > 0 || tabAcquisition_bool[inputDevice].load() == 1 || tabGetImg_bool[inputDevice].load() == 1); // Tant que buffer non vide OU Acquisition en cours OU GetImg en cours
	writer[inputDevice].release();
	cout << "Ecriture() :  EcritureThread fin" << endl ;
}






int VideoRecordCapture::GetImg(string imageFolderSetup, int inputDevice, string deviceName) //Get image no vid
{
	cout << "Debut GetImg" << endl;
	//INIT
	SetParams(inputDevice, deviceName);
	tabGetImg_bool[inputDevice].store(false); // getimg in vid init.
	getImgMutex_bool[inputDevice] = 1;
	returnError[inputDevice] = 0;


	//START
	tabAcquisition_bool[inputDevice].store(true); // Activation du thread d'acquisition
	tabAcquisition_thread[inputDevice] = std::thread(&VideoRecordCapture::Acquisition, this, inputDevice); // Lancement du thread d'acquisition

	tabGetImg_bool[inputDevice].store(true); // Activation du getImg
	tabGetImg_thread[inputDevice] = std::thread(&VideoRecordCapture::GetImgThread, this, inputDevice, imageFolderSetup);// Lancement du thread GetImgThread en attente du remplissage du buffer


	//END
	if (tabGetImg_thread[inputDevice].joinable())
	{
		tabGetImg_thread[inputDevice].join();
		cout << "GetImgNoVid() :  tabGetImg_thread join" << endl ;

		tabAcquisition_bool[inputDevice].store (false);
		if (tabAcquisition_thread[inputDevice].joinable())
		{
			tabAcquisition_thread[inputDevice].join();
			cout << "GetImgNoVid() :  tabAcquisition_thread join" << endl ;

			switch (returnError[inputDevice])
			{
			case -1 :
				return (-1); // Acquisition() : unable to open video
			case -2	:
				return (-2); //Acquisition() : bug frame
			case -7 :
				return (-7); //GetImgThread() : Timeout buffer.
			case -8 :
				return (-8); //SetParams() : Id Camera problem.
			default :
			{
				cout << "GetImgNoVid() : Fin" << endl;
				return (0); // good
			}
			}
		}
		else
		{
			return (-4); // thread unreacheable
		}
	}
}



void VideoRecordCapture::GetImgInVid(string imageFolder, int inputDevice) // Get_img durant un enregistrement vidéo sur la même entrée.
{
	cout << "Debut GetImgInVid" << endl;
	tabGetImg_bool[inputDevice].store(true);
	tabGetImg_thread[inputDevice] = std::thread(&VideoRecordCapture::GetImgThread, this, inputDevice, imageFolder);
	tabGetImg_thread[inputDevice].detach(); // il se termine tout seul dans son coin pour ne pas rajouter de temps à l'enregistrement vidéo (non bloquant) comme le ferait un DELAY.
	cout << "Fin GetImgInVid" << endl;
}





void VideoRecordCapture::GetImgThread(int inputDevice, string imageFolderSetup) // Le thread pour créer le .PPM final moyennant 1 seconde de vidéo.
{
	cout << "Debut GetImgThread" << endl;
	Mat imgFinal, verif, temp;
	int cpt = 0;

	while (tabGetImg_bool[inputDevice].load() == 1) // attente du remplissage du buffer.
	{
		//cout << "GetImgThread() : attente" << endl;
		sleep(1);
		cpt++;
		if (cpt > 5)
		{
			//cout << "GetImgThread() : error buffer" << endl;
			returnError[inputDevice] = -7;
			break;
		}
	}

	size_t	bufferSize = tabgetImgBuffer[inputDevice].size(); // taille buffer pour division moyenne
	//cout << "GetImgThread() :  taille buffer getImg : " << bufferSize << endl;


	tabgetImgBuffer[inputDevice].front().copyTo (verif); // 1 frame pour récupérer caractéristiques hauteur/largeur.
// Create a 0 initialized image to use as accumulator
	Mat m(verif.rows, verif.cols, CV_64FC3); // création d'un accumulateur de la taille d'une matrice du buffer.
	m.setTo(Scalar(0, 0, 0, 0));

// MOYENNE des IMAGES
	while (tabgetImgBuffer[inputDevice].size() != 0)
	{
		//cout << "GetImgThread() :  taille buffer getImg pop: " << tabgetImgBuffer[inputDevice].size() << endl;

		tabgetImgBuffer[inputDevice].front().convertTo(temp, CV_64FC3);
		tabgetImgBuffer[inputDevice].pop(); // suppression de la dernière frame ajoutée.

		m += temp; // ... so you can accumulate
	}


	m.convertTo(m, CV_8U, 1. / bufferSize); // Convert back to CV_8UC3 type, applying the division to get the actual mean

// CREATION DU .PPM
	vector<int> compression_params;
	compression_params.push_back(CV_IMWRITE_PXM_BINARY); // .PPM encode
	compression_params.push_back(1); //.PPM encode value 0 or 1
	imwrite(imageFolderSetup, m, compression_params);
	cout << "GetImgThread() : GetImgThread fin automatique" << endl;
}

