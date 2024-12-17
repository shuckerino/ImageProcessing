#include "camDemo.h"
#include <algorithm>

#pragma region Mouse interaction

/* --------------------------------------------------------------
 * mouse_event()
 * openCV-Funktion um MouseEvents auszuwerten
 *----------------------------------------------------------------*/

/// <summary>
/// Mouse event
/// </summary>
/// <param name="evt"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name=""></param>
/// <param name="param"></param>
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
			sprintf_s(path, "%s/black_hole.wav", folder);
			PlaySoundA(path, NULL, SND_ASYNC);
			return true;
		}
	}
	return false;
}

/// <summary>
/// Check if left click was performed in rect
/// </summary>
/// <param name="mp"></param>
/// <param name="rect"></param>
/// <param name="folder"></param>
/// <returns></returns>
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
			sprintf_s(path, "%s/black_hole.wav", folder);
			PlaySoundA(path, NULL, SND_ASYNC);
			return true;
		}
	}
	return false;
}

/// <summary>
/// Checks if mouse was moved in area of rect
/// </summary>
/// <param name="mp"></param>
/// <param name="rect"></param>
/// <returns></returns>
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

#pragma region Main

int main(int, char**)
{
	// check for two different folders: correct folder depends on where executable is started either from IDE or bin folder
	const char* folder1 = "Resources";
	const char* folder2 = "../Resources";
	char folder[15], path[512];

	MouseParams mp;
	Scalar colour;
	Mat	cam_img;
	uint animation_counter_up = 0;
	int animation_counter_side = 0;
	Mat cam_img_grey;
	Mat strElement; //Strukturelement für Dilatations-Funktion
	Mat3b rgb_scale;	// leerer Bildkontainer für RGB-Werte
	char* windowGameOutput = "BlackHole Effect"; // Name of window
	unsigned int width, height, channels, stride;
	int
		key = 0,	// Tastatureingabe
		frames = 0, //frames zählen für FPS-Anzeige
		fps = 0;
	float scalingFactor = 1.0f;
	float radiusMult = 1.9;
	int camNum = 2;
	int counter = 0;
	int initialCounter;
	bool fullscreen_flag = false;
	bool median_flag = false;
	bool flip_flag = true;
	bool animation_flag = false;
	bool start_animation = false;
	bool start_fall = false;
	DemoState state;
#if defined _DEBUG || defined LOGGING
	FILE* log = NULL;
	log = fopen("log_debug.txt", "wt");
#endif

	clock_t start_time, finish_time;

	VideoCapture cap;

	// check connected cameras
	do
	{
		camNum--;
		cap.open(camNum);
	} while (camNum > 0 && !cap.isOpened());

	if (!cap.isOpened())
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
		// print manual on screen
		printf("==> Program Control <==\n");
		printf("==                   ==\n");
		printf("* Start Screen\n");
		printf(" - 'ESC' stop the program \n");
		printf(" - 'p'   open the camera-settings panel\n");
		printf(" - 't'   toggle window size\n");
		printf(" - 'f'   toggle fullscreen\n");
		printf(" - 'ESC' return to Start Screen \n");
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
	FreeConsole();
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

	/* find folder for resources	*/
	{
		FILE* in = NULL;
		strcpy_s(folder, folder1);/* try first folder */
		sprintf_s(path, "%s/black_hole.wav", folder);
		in = fopen(path, "r");
		if (in == NULL)
		{
			strcpy_s(folder, folder2); /* try other folder */
			sprintf_s(path, "%s/black_hole.wav", folder);
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

		// image smoothing
		if (median_flag) /* can be toggled with key 'm'*/
		{
			medianBlur(cam_img, cam_img, 15);
		}

		//for (unsigned int y = 0; y < height; y += 10) /* all 10th rows */
		//{
		//	for (unsigned int x = 0; x < width; x += 10)/* all 10th columns */
		//	{
		//		unsigned long pos = x * channels + y * stride; /* position of pixel (B component) */
		//		for (unsigned int c = 0; c < channels; c++) /* all components B, G, R */
		//		{
		//			cam_img.data[pos + c] = 0; /* set all components to black */
		//		}
		//	}
		//}

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
		else if (key == 'u')
		{
			scalingFactor++;
			//fprintf("Scaling factor increased to!");
		}
		else if (key == 'd')
		{
			scalingFactor--;
			//sprintf_s("Scaling factor increased to ")
		}
		else if (key == 'i')
		{
			radiusMult += 0.02;
			//sprintf_s("Scaling factor increased to ")
		}
		else if (key == 'k')
		{
			radiusMult -= 0.02;
			//sprintf_s("Scaling factor increased to ")
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
			initialCounter = width / 2 + 50;
			counter = initialCounter;
			scalingFactor = 1.0f;
		}

		if (start_animation)
		{
			float progress = (float)counter / initialCounter; // normalize (1.0 to 0.0)
			float decrement = 1 + (1 - progress) * 1.1; // adjust to control the speed of the effect
			counter -= (int)decrement;

			// make sure counter does not go below 0
			if (counter < 0) counter = 0;

			scalingFactor += 0.025f; // increase the distortion
			createBlackHoleEffect(cam_img, mp.mouse_pos.x, mp.mouse_pos.y, counter, radiusMult, scalingFactor, 10);

			// end animation
			if (counter == 0) 
			{
				start_animation = false;
				PlaySound(NULL, NULL, 0); // cancel sound
			}
		}

		if (!IsWindowVisible(cvHwnd))
		{
			break;
		}
		imshow(windowGameOutput, cam_img); //Ausgabefenster darstellen
	}	// end of while loop

	// release resources
	if (cap.isOpened()) cap.release();
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

void createBlackHoleEffect(cv::Mat& inputImage, int centreX, int centreY, int radius, int radiusMult, float scalingFactor, int marginWidth) {
	int rows = inputImage.rows;
	int cols = inputImage.cols;

	// Create an output image initialized to black
	cv::Mat outputImage = cv::Mat::zeros(inputImage.size(), inputImage.type());

	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			// Calculate the distance of the current pixel from the black hole center
			float distanceX = x - centreX;
			float distanceY = y - centreY;
			float distance = std::sqrt(distanceX * distanceX + distanceY * distanceY);

			if (distance < radius) {
				// Compute the angle of the current pixel relative to the center
				float angle = std::atan2(distanceY, distanceX);

				// Introduce rotational distortion based on the distance
				float rotationAmount = scalingFactor * (1.0f - distance / radius); // Decrease rotation with proximity
				angle += rotationAmount;

				// Compute scaled distance for the black hole effect
				float scale = std::pow(distance / radius, 5);
				float distortedDistance = distance + scale * radiusMult;

				// Convert polar coordinates back to Cartesian coordinates
				float sourceX = centreX + distortedDistance * std::cos(angle);
				float sourceY = centreY + distortedDistance * std::sin(angle);

				// Ensure the source position is within bounds
				if (sourceX >= 0 && sourceX < cols && sourceY >= 0 && sourceY < rows) {
					outputImage.at<cv::Vec3b>(y, x) = inputImage.at<cv::Vec3b>(static_cast<int>(sourceY), static_cast<int>(sourceX));
				}
			}
			else if (distance < radius + marginWidth) 
			{
				// Calculate gradient factor (0 at radius, 1 at radius + marginWidth)
				float gradientFactor = (distance - radius) / marginWidth;

				// Use a sigmoid-like easing function for smooth blending
				float easedFactor = 1.0f / (1.0f + std::exp(-10 * (gradientFactor - 0.5)));

				// Darken the outer edge towards black
				float brightness = easedFactor; // Adjust brightness with easedFactor

				// Interpolate color: dark edge to bright margin
				uchar blue = static_cast<uchar>(brightness * 3);
				uchar green = static_cast<uchar>(brightness * 107);
				uchar red = static_cast<uchar>(brightness * 252);

				outputImage.at<cv::Vec3b>(y, x) = cv::Vec3b(blue, green, red);
			}
			else if (distance > radius * radiusMult)
			{
				outputImage.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
			}
		}
	}

	// Copy the result back to the input image
	inputImage = outputImage.clone();
}
#pragma endregion

