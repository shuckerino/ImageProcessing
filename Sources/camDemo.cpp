/*****************************************************************
*	File...:	camDemo.cpp
*	Purpose:	video processing
*	Date...:	30.09.2019
*	Changes:	16.10.2024 mouse events
*
*********************************************************************/

#include "camDemo.h"
#include <algorithm>

#pragma region mouse interaction

/* --------------------------------------------------------------
 * mouse_event()
 * openCV-Funktion um MouseEvents auszuwerten
 *----------------------------------------------------------------*/
void mouse_event(int evt, int x, int y, int, void* param)
{
	MouseParams* mp = (MouseParams*)param;
	mp->evt = evt; //Mouse-Event
	mp->mouse_pos.x = x; //Mouse x-Position
	mp->mouse_pos.y = y; //Mouse y-Position
}


/* --------------------------------------------------------------
 * click_left()
 *----------------------------------------------------------------*/
bool click_left(MouseParams mp, char* folder)
{
	if (mp.evt == EVENT_LBUTTONDOWN)
	{
		{
			char path[512];
			sprintf_s(path, "%s/click_on_button.wav", folder);
			PlaySoundA(path, NULL, SND_ASYNC);
			return true;
		}
	}
	return false;
}

/* --------------------------------------------------------------
 * click_in_rect()
 * Wurde in einen bestimmten Bereich mit links geklickt?
 *----------------------------------------------------------------*/
bool click_in_rect(MouseParams mp, Rect rect, char* folder)
{
	if (mp.evt == EVENT_LBUTTONDOWN)
	{
		if (mp.mouse_pos.x >= rect.x &&
			mp.mouse_pos.y >= rect.y &&
			mp.mouse_pos.x <= rect.x + rect.width &&
			mp.mouse_pos.y <= rect.y + rect.height)
		{
			char path[512];
			sprintf_s(path, "%s/click_on_button.wav", folder);
			PlaySoundA(path, NULL, SND_ASYNC);
			return true;
		}
	}
	return false;
}

/* --------------------------------------------------------------
 * mouse_in_rect()
 * Wurde Mauszeiger in einen bestimmten Bereich bewegt?
 *----------------------------------------------------------------*/
bool mouse_in_rect(MouseParams mp, Rect rect)
{
	if (mp.evt == EVENT_MOUSEMOVE)
	{
		if (mp.mouse_pos.x >= rect.x &&
			mp.mouse_pos.y >= rect.y &&
			mp.mouse_pos.x <= rect.x + rect.width &&
			mp.mouse_pos.y <= rect.y + rect.height)
		{
			return true;
		}
	}
	return false;
}

#pragma endregion

#pragma region main

int main(int, char**)
{
	// check for two different folders: correct folder depends on where executable is started either from IDE or bin folder
	const char* folder1 = "Resources";
	const char* folder2 = "../Resources";
	char folder[15], path[512];

	MouseParams mp; // Le-Wi: Zur Auswertung von Mouse-Events
	Scalar colour;
	Mat	cam_img; //eingelesenes Kamerabild
	Mat	last_modified_img; // img which was last modified
	uint animation_counter_up = 0;
	int animation_counter_side = 0;
	Mat cam_img_grey; //Graustufenkamerabild für autom. Startgesichtfestlegung
	Mat strElement; //Strukturelement für Dilatations-Funktion
	Mat3b rgb_scale;	// leerer Bildkontainer für RGB-Werte
	char* windowGameOutput = "camDemo"; // Name of window
	//double	scale = 1.0;				// Skalierung der Berechnungsmatrizen 
	unsigned int width, height, channels, stride;	// Werte des angezeigten Bildes
	int
		key = 0,	// Tastatureingabe
		frames = 0, //frames zählen für FPS-Anzeige
		fps = 0;	//frames pro Sekunde
	int camNum = 2;
	int counter = 0;
	bool fullscreen_flag = false; //Ist fullscreen aktivert oder nicht?
	bool median_flag = false;
	bool flip_flag = true;
	bool animation_flag = false;
	bool start_animation = false;
	bool start_fall = false;
	DemoState state; //Aktueller Zustand des Spiels
#if defined _DEBUG || defined LOGGING
	FILE* log = NULL;
	log = fopen("log_debug.txt", "wt");
#endif

	clock_t start_time, finish_time;

	VideoCapture cap;
	do
	{
		camNum--; /* try next camera */
		cap.open(camNum);
	} while (camNum > 0 && !cap.isOpened()); /* solange noch andere Kameras verfügbar sein könnten */

	if (!cap.isOpened())	// ist die Kamera nicht aktiv?
	{
		AllocConsole();
		printf("Keine Kamera gefunden!\n");
		printf("Zum Beenden 'Enter' druecken\n");
#if defined _DEBUG || defined LOGGING
		fprintf(log, "Keine Kamera gefunden!\n");
		fclose(log);
#endif
		while (getchar() == NULL);
		return -1;
	}
	else
	{
		/* some infos on console	*/
		//printf( "==> Program Control <==\n");
		//printf( "==                   ==\n");
		//printf( "* Start Screen\n");
		//printf( " - 'ESC' stop the program \n");
		//printf( " - 'p'   open the camera-settings panel\n");
		//printf( " - 't'   toggle window size\n");
		//printf( " - 'f'   toggle fullscreen\n");
		//printf( " - 'ESC' return to Start Screen \n");
	}
	{
		HWND console = GetConsoleWindow();
		RECT r;
		GetWindowRect(console, &r); //stores the console's current dimensions

		//MoveWindow(window_handle, x, y, width, height, redraw_window);
		//MoveWindow( console, r.left, r.top, 800, 600, TRUE);
		MoveWindow(console, 10, 0, 850, 800, TRUE);
	}

#ifndef _DEBUG
	FreeConsole(); //Konsole ausschalten
#endif

	// capture the image
	cap >> cam_img;

	// get format of camera image
	width = cam_img.cols;
	height = cam_img.rows;
	channels = cam_img.channels();
	stride = width * channels;

	// prepare handle for window
	namedWindow(windowGameOutput, WINDOW_NORMAL | CV_GUI_EXPANDED); //Erlauben des Maximierens
	resizeWindow(windowGameOutput, width, height); //Start Auflösung der Kamera
	HWND cvHwnd = (HWND)cvGetWindowHandle(windowGameOutput); //window-handle to detect window-states

	srand((unsigned)time(NULL)); //seeds the random number generator

	/* find folder for ressources	*/
	{
		FILE* in = NULL;
		strcpy_s(folder, folder1);/* try first folder */
		sprintf_s(path, "%s/click_on_button.wav", folder);
		in = fopen(path, "r");
		if (in == NULL)
		{
			strcpy_s(folder, folder2); /* try other folder */
			sprintf_s(path, "%s/click_on_button.wav", folder);
			in = fopen(path, "r");
			if (in == NULL)
			{
				printf("Resources cannot be found\n");
				exit(99);
			}
		}
		fclose(in);
	}

	start_time = clock();


	/* structure element for dilation of binary image */
	//{
	//	int strElRadius = 3;
	//	int size = strElRadius * 2 + 1;
	//	strElement = Mat( size, size, CV_8UC1, Scalar::all( 0)); 
	//	//Einzeichnen des eigentlichen Strukturelements (weißer Kreis)
	//	/* ( , Mittelpunkt, radius, weiß, thickness=gefüllt*/
	//	circle( strElement, Point( strElRadius, strElRadius), strElRadius, Scalar( 255), -1);
	//}

	state = START_SCREEN;

	// Setup zum Auswerten von Mausevents
	setMouseCallback(windowGameOutput, mouse_event, (void*)&mp);


	/*-------------------- main loop ---------------*/
	while (state != DEMO_STOP)
	{
		// ein Bild aus dem Video betrachten und in cam_img speichern
		if (cap.read(cam_img) == false)
		{ //falls nicht möglich: Fehlermeldung
			destroyWindow("camDemo"); //Ausgabefenster ausblenden

			AllocConsole(); //Konsole wieder einschalten
			printf("Verbindung zur Kamera verloren!\n");
			printf("Zum Beenden 'Enter' druecken\n");
			cap.release(); //Freigabe der Kamera
#if defined _DEBUG || defined LOGGING
			fprintf(log, "Verbindung zur Kamera verloren!\n");
			fclose(log);
#endif
			while (getchar() == NULL); //Warten auf Eingabe
			break; //Beende die Endlosschleife
		}

		if (flip_flag) flip(cam_img, cam_img, 1); // cam_img vertikal spiegeln
		// Strutz cvtColor( cam_img, rgb, CV_BGR2RGB); // Konvertierung BGR zu RGB

		//Runterskalierung des Bildes für weniger Rechaufwand (Faktor 1/2)
		//resize(cam_img, rgb_scale, Size(), scale, scale);

		/* smoothing of images */
		if (median_flag) /* can be toggled with key 'm'*/
		{
			medianBlur(cam_img, cam_img, 15);
		}

		/* example of accessing the image data */
		/* order is:
			BGRBGRBGR...
			BGRBGRBGR...
			:
			*/
		for (unsigned int y = 0; y < height; y += 10) /* all 10th rows */
		{
			for (unsigned int x = 0; x < width; x += 10)/* all 10th columns */
			{
				unsigned long pos = x * channels + y * stride; /* position of pixel (B component) */
				for (unsigned int c = 0; c < channels; c++) /* all components B, G, R */
				{
					cam_img.data[pos + c] = 0; /* set all components to black */
				}
			}
		}

		/* determination of frames per second*/
		frames++;
		finish_time = clock();
		if ((double)(finish_time - start_time) / CLOCKS_PER_SEC >= 1.0)
		{
			fps = frames;
			frames = 0;
			start_time = clock();
		}

#define FPS_OUTPUT
#if defined FPS_OUTPUT || defined _DEBUG
		//FPS-Ausgabe oben rechts
		char fps_char[3];
		sprintf(fps_char, "%d", fps);
		const string& fps_string = (string)fps_char;
		putText(cam_img, fps_string, Point(width - 40, 25), FONT_HERSHEY_SIMPLEX,
			0.5 /*fontScale*/, Scalar(0, 255, 255), 2);
#endif

		/* input from keyboard */
		key = tolower(waitKey(1)); /* Strutz  convert to lower case */
		// Vollbildschirm ein- bzw. ausschalten
		if (key == 'f')
		{
			if (!fullscreen_flag)
			{
				//skaliere fenster auf vollbild
				cvSetWindowProperty(windowGameOutput, WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
				fullscreen_flag = true;
			}
			else
			{
				//setzte fenster auf original-größe
				cvSetWindowProperty(windowGameOutput, WINDOW_NORMAL, WINDOW_NORMAL);
				fullscreen_flag = false;
			}
		}
		if (key == 'm') /* toggle flag */
		{
			if (median_flag) median_flag = false;
			else median_flag = true;
		}
		else if (key == 'l')
		{
			flip_flag = 1 - flip_flag; /* toggle left-right flipping of image	*/
		}
		else if (key == 'r')
		{
			start_animation = true;
			animation_flag = !animation_flag;
		}

		if (state == START_SCREEN)
		{
			if (key == 27) // Abbruch mit ESC
			{
				state = DEMO_STOP; /* leave loop	*/
				continue;
			}
			else if (key == 'p')
			{
				/* show properties of camera	*/
				if (cap.set(CAP_PROP_SETTINGS, 0) != 1)
				{
#if defined _DEBUG || defined LOGGING
					fprintf(log, "\nlocal webcam > Webcam Settings cannot be opened!\n");
#endif
				}
			}
		}

		if (click_left(mp, folder))
		{
			start_animation = true;
			counter = width / 2;
		}

		if (start_animation) 
		{
			counter--;
			//drawBlackHole(cam_img, mp.mouse_pos.x, mp.mouse_pos.y, 50);
			createBlackHoleEffect(cam_img, mp.mouse_pos.x, mp.mouse_pos.y, counter);

			// end animation
			if (counter == 0)
				start_animation = false;
		}

		/********************************************************************************************/
		/* show window with live video	*/		//Le-Wi: Funktionalitäten zum Schließen (x-Button)
		if (!IsWindowVisible(cvHwnd))
		{
			break;
		}
		imshow(windowGameOutput, cam_img); //Ausgabefenster darstellen
	}	// Ende der Endlos-Schleife

	//Freigabe aller Matrizen
	if (cap.isOpened()) cap.release(); //Freigabe der Kamera
	if (cam_img.data) cam_img.release();
	if (cam_img_grey.data) cam_img_grey.release();
	if (rgb_scale.data) rgb_scale.release();
	if (strElement.data) strElement.release();


#if defined _DEBUG || defined LOGGING
	fclose(log);
#endif
	//FreeConsole(); //Konsole ausschalten
	cvDestroyAllWindows();
	//_CrtDumpMemoryLeaks();
	exit(0);
}

#pragma endregion

#pragma region Project

void drawBlackHole(Mat& inputImage, unsigned centreX, unsigned centreY, unsigned int radius)
{
	for (unsigned int y = centreY - radius; y <= centreY + radius; y++)
	{
		for (unsigned int x = centreX - radius; x <= centreX + radius; x++)
		{
			unsigned int distance_x = centreX - x;
			unsigned int distance_y = centreY - y;
			unsigned int squaredDistance = distance_x * distance_x + distance_y * distance_y;
			if (squaredDistance <= radius * radius)
			{
				inputImage.at<Vec3b>(y, x) = Vec3b(0, 0, 0);
			}
		}
	}
}

//void createBlackHoleEffect(cv::Mat& inputImage, int centreX, int centreY, int radius) {
//
//	int rows = inputImage.rows;
//	int cols = inputImage.cols;
//
//	// Parameters to control the intensity of the distortion effect
//	float distortionStrength = 1.0;  // Larger values = stronger distortion
//
//	for (int y = 0; y < rows; y++) {
//		for (int x = 0; x < cols; x++) {
//			// Calculate the distance of the current pixel from the black hole center
//			float distanceX = x - centreX;
//			float distanceY = y - centreY;
//			float distance = sqrt(distanceX * distanceX + distanceY * distanceY);
//
//			// Only apply distortion within the specified radius
//			//if (distance < radius) {
//			//	// Calculate a "pull" factor that gets stronger as we approach the center
//			//	float pullFactor = 1.0 - (distortionStrength * (radius - distance) / radius);
//
//			//	// Compute new (source) coordinates for the current pixel
//			//	int srcX = centreX + static_cast<int>(distanceX * pullFactor);
//			//	int srcY = centreY + static_cast<int>(distanceY * pullFactor);
//
//			//	//// Ensure coordinates are within image bounds
//			//	//srcX = std::min(std::max(srcX, 0), cols - 1);
//			//	//srcY = std::min(std::max(srcY, 0), rows - 1);
//
//			//	// Set the output pixel to the color of the source pixel
//			//	outputImage.at<cv::Vec3b>(y, x) = inputImage.at<cv::Vec3b>(srcY, srcX);
//			//}
//			if (distance <= 50) 
//			{
//				// Copy pixels outside the radius without distortion
//				inputImage.at<cv::Vec3b>(y, x) = Vec3b(0, 0, 0);
//			}
//			if (distance >= radius) 
//			{
//				inputImage.at<cv::Vec3b>(y, x) = Vec3b(0, 0, 0);
//			}
//		}
//	}
//}

void createBlackHoleEffect(cv::Mat& inputImage, int centreX, int centreY, int radius) {
	int rows = inputImage.rows;
	int cols = inputImage.cols;

	// Parameters to control the intensity of the distortion effect
	float distortionStrength = 4.0f;  // Larger values = stronger distortion

	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			// Calculate the distance of the current pixel from the black hole center
			float distanceX = x - centreX;
			float distanceY = y - centreY;
			float distance = sqrt(distanceX * distanceX + distanceY * distanceY);

			// If the pixel is inside the radius, apply distortion (pull effect)
			if (distance < radius) {
				// Calculate a "pull" factor that gets stronger as we approach the center
				float pullFactor = (1.0f - (distance / radius)) * distortionStrength;

				// Compute new (distorted) coordinates for the current pixel (move towards the center)
				int srcX = centreX + static_cast<int>(distanceX * pullFactor);
				int srcY = centreY + static_cast<int>(distanceY * pullFactor);

				//// Ensure the coordinates are within image bounds
				//srcX = std::min(std::max(srcX, 0), cols - 1);
				//srcY = std::min(std::max(srcY, 0), rows - 1);

				// Move the pixel to the new distorted position
				inputImage.at<cv::Vec3b>(y, x) = inputImage.at<cv::Vec3b>(srcY, srcX);
			}
			// Set pixels outside the radius to black
			else {
				inputImage.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
			}
		}
	}
}

#pragma endregion

