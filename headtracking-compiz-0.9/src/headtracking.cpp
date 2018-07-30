/*
 * Compiz Fusion Head Tracking Plugin
 * 
 * Some of this code is based on Freewins
 * Portions were inspired and tested on a modified
 * Zoom plugin, but no code from Zoom has been taken.
 * 
 * Copyright 2010 Kevin Lange <kevin.lange@phpwnage.com>
 *
 * facedetect.c is from the OpenCV sample library, modified to run
 * threaded.
 *
 * Face detection is done through OpenCV.
 * Wiimote tracking is done through the `wiimote` plugin and probably
 * doesn't work anymore.
 *
 * Video demonstrations of both webcams and wiimotes are available
 * online. Check YouTube, as well as the C-F forums.
 *
 * More information is available in README.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
 
#include "headtracking.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "opencv/cv.h"
#include "opencv/highgui.h"

#define CASCADE "haarcascade_frontalface_alt2.xml"
#define UX 1024
#define UY 768
#define MAX_TRACK_PT 100
#define MIN_TRACK_PT 10
#define SMOOTH_STEP 5.0
#define MAX_REFRESH 100

COMPIZ_PLUGIN_20090315 (headtracking, HeadtrackingPluginVTable);

/**************** BEGIN OPENCV HEADTRACKING ENGINE ****************************/

static CvCapture* capture = 0;
static CvMemStorage* storage = 0;
static CvHaarClassifierCascade* cascade = 0;
static IplImage *frame_copy = 0;
static int prevx1, prevy1, prevx2, prevy2;
static int realprevx1, realprevy1, realprevx2, realprevy2;
static int barPx, barPy, barRx, barRy;
static int prevlar = 0, prevhau = 0;
static long int limitdist, mindist;
static CvPoint2D32f *trackingPoint[2] = {0,0};
static int nbTrackingPoint = 0;
static char *status = 0;
static IplImage *prevGray = 0, *gray = 0;
static IplImage *prevPyramid = 0, *pyramid = 0;
static int trackingFlags = 0;
static double beginx1 = 0.0, beginy1 = 0.0, beginx2 = 0.0, beginy2 = 0.0;
static int endx1 = 0, endy1 = 0, endx2 = 0, endy2 = 0;
static double pasx1 = 0.0, pasy1 = 0.0, pasx2 = 0.0, pasy2 = 0.0;
unsigned int refresh = 0;

// Thread part
static pthread_mutex_t mutex;
static pthread_attr_t attrThread;
static int thret = 0, thx1 = 0, thy1 = 0, thx2 = 0, thy2 = 0, threadAlive = 0;
static pthread_t tid = -1;

// Local protos
static void init(void);
static void end(void);
static int headtrackThread(int *x1, int *y1, int *x2, int *y2, int lissage, int scale);
void endThread(void);
static int detect(double scale, int uX, int uY, int *x1, int *y1, int *x2, int *y2, int lissage);
typedef struct {
	int lissage;
	int delay;
	int scale;
} threadarg_t;

// Thread part : background detection
static void *threadRoutine(void *p) {
	int ret, x1, y1, x2, y2, alive = 1;
	int lissage = ((threadarg_t *)p)->lissage;
	int delay = ((threadarg_t *)p)->delay;
	int scale = ((threadarg_t *)p)->scale;
	// free(p);

	init();
	while (alive) {
		ret = headtrackThread(&x1, &y1, &x2, &y2, lissage, scale);
		pthread_mutex_lock(&mutex);
		thx1 = x1;
		thy1 = y1;
		thx2 = x2;
		thy2 = y2;
		thret = ret;
		alive = threadAlive;
		pthread_mutex_unlock(&mutex);
		usleep(delay);
	}
	end();
	pthread_mutex_lock(&mutex);
	tid = -1;
	pthread_mutex_unlock(&mutex);
	return(NULL);
}

void endThread(void) {
	pthread_t t = tid;

	pthread_mutex_lock(&mutex);
	threadAlive = 0;
	pthread_mutex_unlock(&mutex);
	while (t != -1) {
		sleep(1);
		pthread_mutex_lock(&mutex);
		t = tid;
		pthread_mutex_unlock(&mutex);
	}
}

void init(void)
{
    // FIXME: Use a type=file option in the CCS configuration
    //        to store the path to the cascade file.
	char *home = getenv("HOME"), *path = NULL;
	struct stat fileExists;

	// Cascade path
	if (home && strlen(home) > 0) {
		asprintf(&path, "%s/.compiz/data/%s", home, CASCADE);
		if (stat(path, &fileExists) != 0) {
			free(path);
			path = NULL;
		}
	}
	if (path == NULL) {
		asprintf(&path, "%s/%s", DATADIR, CASCADE);
		if (stat(path, &fileExists) != 0) {
			free(path);
			path = NULL;
		}
	}
	if (path != NULL) cascade = (CvHaarClassifierCascade*)cvLoad(path, 0, 0, 0);
	else cascade = 0;
	storage = cvCreateMemStorage(0);
	capture = cvCaptureFromCAM(0);
	prevx1 = prevy1 = prevx2 = prevy2 = 0;
	realprevx1 = realprevy1 = realprevx2 = realprevy2 = 0;
	barPx = barPy = barRx = barRy = 0;
	prevlar = prevhau = 0;
	limitdist = 0;
	mindist = 25;
	frame_copy = 0;
	trackingPoint[0] = (CvPoint2D32f*)cvAlloc(MAX_TRACK_PT * sizeof(trackingPoint[0][0]));
	trackingPoint[1] = (CvPoint2D32f*)cvAlloc(MAX_TRACK_PT * sizeof(trackingPoint[0][0]));
	nbTrackingPoint = 0;
	status = 0;
	prevGray = gray = 0;
	prevPyramid = pyramid = 0;
	trackingFlags = 0;
	refresh = 0;

	// Thread part
	thx1 = 0;
	thy1 = 0;
	thx2 = 0;
	thy2 = 0;
	thret = 0;
	pthread_mutex_init(&mutex, NULL);
	pthread_attr_init(&attrThread);
	pthread_attr_setdetachstate(&attrThread, PTHREAD_CREATE_DETACHED);
	pthread_attr_setschedpolicy(&attrThread, SCHED_OTHER);
	pthread_attr_setinheritsched(&attrThread, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setscope(&attrThread, PTHREAD_SCOPE_SYSTEM);
}

void end(void)
{
	cvReleaseImage(&frame_copy);
	cvReleaseCapture(&capture);
}

int headtrack(int *x1, int *y1, int *x2, int *y2, int lissage, int smooth, int delay, int opt_scale)
{
	int ret;
	threadarg_t *arg = NULL;

	if (tid == -1) {
		threadAlive = 1;
		arg = (threadarg_t *)malloc(sizeof(threadarg_t *));
		arg->lissage = lissage;
		arg->delay = delay;
		arg->scale = opt_scale;
		pthread_create(&tid, &attrThread, threadRoutine, (void *)arg);
		sleep(1);
		beginx1 = beginy1 = beginx2 = beginy2 = 0.0;
		endx1 = endy1 = endx2 = endy2 = 0;
		pasx1 = pasy1 = pasx2 = pasy2 = 0.0;
	}
	pthread_mutex_lock(&mutex);
	*x1 = thx1;
	*y1 = thy1;
	*x2 = thx2;
	*y2 = thy2;
	ret = thret;
	pthread_mutex_unlock(&mutex);

	// Smmoothing the movement
	if (smooth) {
		if (endx1 != *x1 || endy1 != *y1 || endx2 != *x2 || endy2 != *y2) {
			endx1 = *x1;
			endy1 = *y1;
			endx2 = *x2;
			endy2 = *y2;
			if (beginx1 != 0.0 && beginy1 != 0.0 && beginx2 != 0.0 && beginy2 != 0.0) {
				pasx1 = ((double)*x1 - beginx1) / SMOOTH_STEP;
				pasy1 = ((double)*y1 - beginy1) / SMOOTH_STEP;
				pasx2 = ((double)*x2 - beginx2) / SMOOTH_STEP;
				pasy2 = ((double)*y2 - beginy2) / SMOOTH_STEP;
				if (pasx1 != 0.0 && pasy1 != 0.0 && pasx2 != 0.0 && pasy2 != 0.0) {
					beginx1 += pasx1;
					beginy1 += pasy1;
					beginx2 += pasx2;
					beginy2 += pasy2;
					*x1 = cvRound(beginx1);
					*y1 = cvRound(beginy1);
					*x2 = cvRound(beginx2);
					*y2 = cvRound(beginy2);
				}
				else {
					pasx1 = pasy1 = pasx2 = pasy2 = 0.0;
				}
			}
			else {
				beginx1 = (double)*x1;
				beginy1 = (double)*y1;
				beginx2 = (double)*x2;
				beginy2 = (double)*y2;
			}
		}
		else
		if (pasx1 != 0.0 && pasy1 != 0.0 && pasx2 != 0.0 && pasy2 != 0.0) {
			if (((double)*x1 - beginx1) / pasx1 > 0.0 && ((double)*y1 - beginy1) / pasy1 > 0.0 && ((double)*x2 - beginx2) / pasx2 > 0.0 && ((double)*y2 - beginy2) / pasy2 > 0.0) {
				beginx1 += pasx1;
				beginy1 += pasy1;
				beginx2 += pasx2;
				beginy2 += pasy2;
				*x1 = cvRound(beginx1);
				*y1 = cvRound(beginy1);
				*x2 = cvRound(beginx2);
				*y2 = cvRound(beginy2);
			}
			else {
				pasx1 = pasy1 = pasx2 = pasy2 = 0.0;
			}
		}
		else {
			beginx1 = (double)*x1;
			beginy1 = (double)*y1;
			beginx2 = (double)*x2;
			beginy2 = (double)*y2;
			pasx1 = pasy1 = pasx2 = pasy2 = 0.0;
		}
	}
	return(ret);
}

int headtrackThread(int *x1, int *y1, int *x2, int *y2, int lissage, int scale)
{
	IplImage *frame = 0;
	int retour = 0;

	if (!capture) {
		init();
	}
	if (capture) {
		if (cvGrabFrame(capture)) {
			frame = cvRetrieveFrame( capture );
			if(frame) {
				if( !frame_copy )
					frame_copy = cvCreateImage( cvSize(frame->width,frame->height), IPL_DEPTH_8U, frame->nChannels );
				if( frame->origin == IPL_ORIGIN_TL )
					cvCopy(frame, frame_copy, 0);
				else
					cvFlip(frame, frame_copy, 0);
				retour = detect((double)scale / 10.0, UX, UY, x1, y1, x2, y2, lissage);
			}
		}
	}
	return(retour);
}

int detect(double scale, int uX, int uY, int *x1, int *y1, int *x2, int *y2, int lissage)
{
	// Haar
	IplImage *small_img;
	int i, resx1, resy1, resx2, resy2, barx, bary, minx, miny, maxx, maxy, resw, resh;
	int provrealprevx1, provrealprevy1, provrealprevx2, provrealprevy2, provbarRx, provbarRy;
	long int realdist, prevdist;
	int boucle, retour = 0;

	// Track points
	IplImage *subimg, *eig, *temp;
	CvPoint2D32f *swapPoint;
	double quality = 0.01;
	double min_distance = 5;
	CvPoint pt;
	int nbIn = 0;
	double barX, barY;
	int lar = 0, hau = 0;

	if (gray == 0) {
		gray = cvCreateImage(cvSize(frame_copy->width,frame_copy->height), 8, 1);
		status = (char*)cvAlloc(MAX_TRACK_PT);
		prevGray = cvCreateImage(cvGetSize(frame_copy), 8, 1);
		prevPyramid = cvCreateImage(cvGetSize(frame_copy), 8, 1);
		pyramid = cvCreateImage(cvGetSize(frame_copy), 8, 1);
	}
	if (limitdist == 0) {
		limitdist = frame_copy->width * frame_copy->width / 100;
	}

	if(cascade) {
		cvCvtColor(frame_copy, gray, CV_BGR2GRAY);

		// Recall last result
		resx1 = prevx1;
		resy1 = prevy1;
		resx2 = prevx2;
		resy2 = prevy2;
		lar = hau = 0;

		// Haar detection
		if (nbTrackingPoint < MIN_TRACK_PT || refresh > MAX_REFRESH) {
			small_img = cvCreateImage(cvSize(cvRound(frame_copy->width / scale), cvRound(frame_copy->height / scale)), 8, 1);
			cvResize(gray, small_img, CV_INTER_LINEAR);
			cvEqualizeHist(small_img, small_img);
			cvClearMemStorage(storage);
			CvSeq* faces = cvHaarDetectObjects( small_img, cascade, storage,
				1.1, 2, 0 | CV_HAAR_FIND_BIGGEST_OBJECT
				,
				cvSize(30, 30) );
			for( i = 0; i < (faces ? faces->total : 0); i++ ) {
				CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
				resx1 = r->x * scale;
				resy1 = r->y * scale;
				resx2 = (r->x + r->width) * scale;
				resy2 = (r->y + r->height) * scale;
				resw = r->width * scale;
				resh = r->height * scale;

				// Initialize the tracking points
				cvSetImageROI(gray, cvRect(r->x * scale, r->y * scale, r->width * scale, r->height * scale));
				subimg = cvCreateImage(cvSize(r->width * scale, r->height * scale), gray->depth, gray->nChannels);
				cvCopy(gray, subimg, 0);
				cvResetImageROI(gray);
				eig = cvCreateImage(cvGetSize(subimg), 32, 1);
				temp = cvCreateImage(cvGetSize(subimg), 32, 1);
				nbTrackingPoint = MAX_TRACK_PT;
				cvGoodFeaturesToTrack(subimg, eig, temp, trackingPoint[0], &nbTrackingPoint, quality, min_distance, 0, 3, 0, 0.04);
				cvReleaseImage(&eig);
				cvReleaseImage(&temp);
				barX = barY = 0.0;
				for(boucle = 0 ; boucle < nbTrackingPoint ; boucle++) {
					pt = cvPointFrom32f(trackingPoint[0][boucle]);
					trackingPoint[0][boucle] = cvPoint2D32f(pt.x + r->x * scale, pt.y + r->y * scale);
					barX += trackingPoint[0][boucle].x;
					barY += trackingPoint[0][boucle].y;
				}
				barX /= (double)nbTrackingPoint;
				barY /= (double)nbTrackingPoint;

				// Initialisations
				trackingFlags = 0;

				// Dimensions
				prevlar = lar = (resx2 - resx1) / 2;
				prevhau = hau = (resy2 - resy1) / 2;

				retour = 1;
				refresh = 0;
			}
			cvReleaseImage(&small_img);
		}

		// Update the tracking points (if yet initialized)
		if (nbTrackingPoint >= MIN_TRACK_PT) {
			refresh++;
			cvCalcOpticalFlowPyrLK(prevGray, gray, prevPyramid, pyramid, trackingPoint[0], trackingPoint[1], nbTrackingPoint, cvSize(10, 10), 3, status, 0, cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 20, 0.03), trackingFlags);
			trackingFlags |= CV_LKFLOW_PYR_A_READY;
			nbIn = 0;
			barX = barY = 0.0;
			maxx = maxy = 0;
			minx = frame_copy->width;
			miny = frame_copy->height;

			// Barycenter
			for (boucle = 0 ; boucle < nbTrackingPoint ; boucle++) {
				if (status[boucle]) {
					nbIn++;
					barX += trackingPoint[1][boucle].x;
					barY += trackingPoint[1][boucle].y;
				}
			}
			barX /= (double)nbIn;
			barY /= (double)nbIn;

			// Dispersion
			nbIn = 0;
			for (boucle = 0 ; boucle < nbTrackingPoint ; boucle++) {
				if (status[boucle]) {
					if (    trackingPoint[1][boucle].x <= barX + (double)prevlar &&
						trackingPoint[1][boucle].x >= barX - (double)prevlar &&
						trackingPoint[1][boucle].y <= barY + (double)prevhau &&
						trackingPoint[1][boucle].y >= barY - (double)prevhau) {
						nbIn++;
						if (minx > cvPointFrom32f(trackingPoint[1][boucle]).x) minx = cvPointFrom32f(trackingPoint[1][boucle]).x;
						if (miny > cvPointFrom32f(trackingPoint[1][boucle]).y) miny = cvPointFrom32f(trackingPoint[1][boucle]).y;
						if (maxx < cvPointFrom32f(trackingPoint[1][boucle]).x) maxx = cvPointFrom32f(trackingPoint[1][boucle]).x;
						if (maxy < cvPointFrom32f(trackingPoint[1][boucle]).y) maxy = cvPointFrom32f(trackingPoint[1][boucle]).y;
					}
				}
			}

			nbTrackingPoint = nbIn;
			if (lar == 0) lar = (maxx - minx) / 2;
			if (hau == 0) hau = (maxy - miny) / 2;

			// Swaping
			CV_SWAP(prevGray, gray, temp);
			CV_SWAP(prevPyramid, pyramid, temp);
			CV_SWAP(trackingPoint[0], trackingPoint[1], swapPoint);

			// Too narrow
			if (lar < prevlar / 2 || hau < prevhau / 2) refresh = MAX_REFRESH;
		}

		// Enought points
		if (nbTrackingPoint >= MIN_TRACK_PT) {

			// Coordinates
			if (lar > 0 && hau > 0) {
				barx = (int)cvRound(barX);
				bary = (int)cvRound(barY);
				resx1 = barx - lar;
				resy1 = bary - hau;
				resx2 = barx + lar;
				resy2 = bary + hau;

				retour = 1;
			}
		}
		else
		if (lissage) {
			barx = (resx1 + resx2) / 2;
			bary = (resy1 + resy2) / 2;
		}

		// Distances from candidates
		if (lissage) {
			realdist = (barx - barRx) * (barx - barRx) + (bary - barRy) * (bary - barRy);
			if (barRx != barPx || barRy != barPy) {
				prevdist = (barx - barPx) * (barx - barPx) + (bary - barPy) * (bary - barPy);
			} else {
				prevdist = realdist;
			}

			provrealprevx1 = resx1;
			provrealprevy1 = resy1;
			provrealprevx2 = resx2;
			provrealprevy2 = resy2;
			provbarRx = barx;
			provbarRy = bary;

			// Too litte distance (avoid shivering)
			if (prevdist < mindist) {
				resx1 = prevx1;
				resy1 = prevy1;
				resx2 = prevx2;
				resy2 = prevy2;
				barx = barPx;
				bary = barPy;
			}
			else {

				// The previous chosen is better in case of fallback
				if (prevdist < realdist) {

					// Fallback
					if (prevdist > limitdist) {
						resx1 = prevx1;
						resy1 = prevy1;
						resx2 = prevx2;
						resy2 = prevy2;
						barx = barPx;
						bary = barPy;
					}
				}

				// The previous real is better in case of fallback
				else {

					// Fallback
					if (realdist > limitdist) {
						resx1 = realprevx1;
						resy1 = realprevy1;
						resx2 = realprevx2;
						resy2 = realprevy2;
						barx = barRx;
						bary = barRy;
					}
				}
			}
			realprevx1 = provrealprevx1;
			realprevy1 = provrealprevy1;
			realprevx2 = provrealprevx2;
			realprevy2 = provrealprevy2;
			barRx = provbarRx;
			barRy = provbarRy;

			prevx1 = resx1;
			prevy1 = resy1;
			prevx2 = resx2;
			prevy2 = resy2;
			barPx = barx;
			barPy = bary;
		}

		*x1 = resx1 * uX / frame_copy->width;
		*y1 = (resy1 + (resy2 - resy1) / 3) * uY / frame_copy->height;
		*x2 = resx2 * uX / frame_copy->width;
		*y2 = *y1;
		lar = (*x1 + *x2) / 2;
	    //*x1 = lar - resw;
		//*x2 = lar + resw;
		*x1 = lar - 3;
		*x2 = lar + 3;
	}
	return(retour);
}

/**************** END OPENCV HEADTRACKING ENGINE ******************************/

bool
WTWindow::is3D ()
{
    // Is this window one we need to consider
    // for automatic z depth and 3d positioning?
    
    if (window->overrideRedirect ())
	return FALSE;

    if (!(window->shaded () || !window->invisible ()))
	return FALSE;

    if (window->state () & (CompWindowStateSkipPagerMask |
		    	    CompWindowStateSkipTaskbarMask))
	return FALSE;
	
    if (window->state () & (CompWindowStateStickyMask))
    	return FALSE;

    if (window->type () & (NO_FOCUS_MASK))
    	return FALSE;
    return TRUE;
}

void
WTScreen::preparePaint (int msSinceLastPaint)
{
    int maxDepth = 0;
    
    // First establish our maximum depth
    foreach (CompWindow *w, screen->windows ())
    {
        HEADTRACKING_WINDOW (w);
        if (!(WIN_REAL_X(w) + WIN_REAL_W(w) <= 0.0 || WIN_REAL_X(w) >= screen->width ()))
        {
            if (!(wtw->mIsManualDepth)) {
                if (!wtw->is3D ())
		    continue;
                maxDepth++;
            }
        }
    }

    /* Kevin:
     * Doing:
     * if () {
     *     blah blah
     * }
     * else {
     * ...
     *
     * gives me a licence to stab you over the internet
     */

    // Then set our windows as such
    foreach (CompWindow *w, screen->windows ())
    {
        HEADTRACKING_WINDOW (w);
        if (!(wtw->mIsManualDepth) && wtw->mZDepth > 0.0)
        {
            wtw->mZDepth = 0.0;
        }
        if (!(WIN_REAL_X(w) + WIN_REAL_W(w) <= 0.0 || WIN_REAL_X(w) >= screen->width ()))
        {
            if (!(wtw->mIsManualDepth))
            {
                if (!wtw->is3D ())
	                continue;
                maxDepth--;
                float tempDepth = 0.0f - ((float) maxDepth * (float) optionGetWindowDepth () / 100.0);
                if (!wtw->mIsAnimating) {
                    wtw->mNewDepth = tempDepth;
                    if (wtw->mZDepth != wtw->mNewDepth)
                    {
                        wtw->mOldDepth = wtw->mZDepth;
                        wtw->mIsAnimating = TRUE;
                        wtw->mTimeRemaining = 0;
                    }
                }
                else
                {
                    if (wtw->mNewDepth != tempDepth) {
                        wtw->mNewDepth = tempDepth;
                        wtw->mOldDepth = wtw->mZDepth;
                        wtw->mIsAnimating = TRUE;
                        wtw->mTimeRemaining = 0;
                    }
                    else
                    {
			wtw->mTimeRemaining++;
			float dz = (float)(wtw->mOldDepth - wtw->mNewDepth) * (float)wtw->mTimeRemaining / optionGetFadeTime ();
			wtw->mZDepth = wtw->mOldDepth - dz;
			if (wtw->mTimeRemaining >= optionGetFadeTime ())
			{
			    wtw->mIsAnimating = FALSE;
			    wtw->mZDepth = wtw->mNewDepth;
			    wtw->mOldDepth = wtw->mNewDepth;
			}
		    }
                }
            }
            else 
            {
		wtw->mZDepth = wtw->mDepth;
            }
        }
        else
        {
            wtw->mZDepth = 0.0f;
        }
    }
    
    cScreen->preparePaint (msSinceLastPaint);
}

bool
WTWindow::shouldPaintStacked ()
{
    // Should we draw the windows or not?
    // TODO: Add more checks, ie, Expo, Scale
    // XXX: You'll get your checks in 0.9.0. Don't bug me now. NOOOOOOOOOOOEEEEEEEESSSSSSSSSSSSSS. This is WRONG. WRONG WRONG WRONG. I am giving you a big fat lecture over it tomorrow
    // TODO: actually, I'm sure cube does something in handleCompizEvent
    
    if (CompPlugin::find ("cube"))
    {
        // Cube is enabled, is it rotating?
        if (screen->otherGrabExist ("headtracking", "move", "resize", "cube",
        			    "rotate", "expo", "scale", "group-drag",
        			    "switcher", "shift", "ring", "staticswitcher",
        			    "stackswitch", 0))
    	    return FALSE;
    }
    return TRUE;
}

bool
WTWindow::glPaint (const GLWindowPaintAttrib &attrib,
		   const GLMatrix	     &transform,
		   const CompRegion	     &region,
		   unsigned int		     mask)
{
    // Draw the window with its correct z level
    GLMatrix wTransform  (transform);
    Bool status;
    Bool wasCulled = glIsEnabled(GL_CULL_FACE);
    
    HEADTRACKING_SCREEN (screen);
    
    if (shouldPaintStacked ())
    {
        if (!(window->type () == CompWindowTypeDesktopMask)) {
            if (!(WIN_REAL_X(window) + WIN_REAL_W(window) <= 0.0 || WIN_REAL_X(window) >= screen->width ()))
        	    wTransform.translate (0.0, 0.0,mZDepth + (float)wts->optionGetStackPadding () / 100.0f);
        	if (mZDepth + (float)wts->optionGetStackPadding() / 100.0f != 0.0)
        	    mask |= PAINT_WINDOW_TRANSFORMED_MASK;
        }
        else
        {
            wTransform.translate (0.0, 0.0, 0.0); // ??? WTF?
        }
    }
    
    wts->cScreen->damageScreen ();
    
    if (wasCulled)
        glDisable(GL_CULL_FACE);
        
    status = gWindow->glPaint (attrib, wTransform, region, mask);
    
    if (wasCulled)
	glEnable(GL_CULL_FACE);

    return status;
}

// Track head position with Johnny Chung Lee's trig stuff
// XXX: Note that positions should be float values from 0-1024
//      and 0-720 (width, height, respectively).
void
WTScreen::WTLeeTrackPosition (float x1, float y1, float x2, float y2)
{                            
    float radPerPix = (PI / 3.0f) / 1024.0f;
    // Where is the middle of the head?
    float dx = x1 - x2, dy = y1 - y2;
    float pointDist = (float)sqrt(dx * dx + dy * dy);
    float angle = radPerPix * pointDist / 2.0;
    // Set the head distance in units of screen size
    mHead.z = ((float) optionGetBarWidth () / 1000.0) / (float)tan(angle);
    float aX = (x1 + x2) / 2.0f, aY = (y1 + y2) / 2.0f;
    // Set the head position horizontally
    mHead.x = (float)sin(radPerPix * (aX - 512.0)) * mHead.z;
    float relAng = (aY - 384.0) * radPerPix;
    // Set the head height
    mHead.y = -0.5f + (float)sin((float)optionGetWiimoteVerticalAngle () / 100.0 + relAng) * mHead.z;
    // And adjust it to suit our needs
    mHead.y = mHead.y + (float)optionGetWiimoteAdjust () / 100.0;
    // And if our Wiimote is above our screen, adjust appropriately
    if (optionGetWiimoteAbove ())
        mHead.y = mHead.y + 0.5;
}

void
WTScreen::updatePosition (const CompPoint &p)
{
    mMouse = p;
    
    // The following lets us scale mouse movement to get beyond the screen
    
    float mult = 100.0 / ((float)optionGetScreenPercent ());
	    mHead.x = (-(float)p.x () / (float)screen->width () + 0.5) * mult;
	    mHead.y = ((float)p.y () / (float)screen->height () - 0.5) * mult;
	    mHead.z = (float)1.0;
}

bool
WTScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
			 const GLMatrix		   &transform,
			 const CompRegion	   &region,
			 CompOutput		   *output,
			 unsigned int		   mask)
{	
    Bool status;
    GLMatrix zTransform (transform);
    mask |= PAINT_SCREEN_CLEAR_MASK;

    if (optionGetEnableCamtracking ())
    {
	int x1, y1, x2, y2;
	

	if (headtrack(&x1, &y1, &x2, &y2, optionGetWebcamLissage (), optionGetWebcamSmooth (), 0, optionGetScale ()))
	{
            WTLeeTrackPosition((float)x1, (float)y1, (float)x2, (float)y2);
           
            mHead.z = (float)optionGetDepthAdjust () / 100.0;
        }
    }
    
    float nearPlane = 0.05; // Our near rendering plane
    float screenAspect = 1.0; // Leave this for future fixes

    glMatrixMode(GL_PROJECTION); // We're going to modify the projection matrix
    


    //glLoadMatrixf (gScreen->projectionMatrix ()); // Clear it first
    //glPushMatrix ();
    glLoadIdentity ();
    // Set our frustum view matrix.
    glFrustum  (nearPlane*(-0.5 * screenAspect + mHead.x)/mHead.z,
		nearPlane*(0.5 * screenAspect + mHead.x)/mHead.z,
		nearPlane*(-0.5 + mHead.y)/mHead.z,
		nearPlane*(0.5 + mHead.y)/mHead.z,
		nearPlane, 100.0);
		
    //glPopMatrix ();   
    glMatrixMode(GL_MODELVIEW); // Switch back to model matrix

	// Move our scene so that it appears to actually be the screen
    zTransform.translate (mHead.x, mHead.y, mHead.z - DEFAULT_Z_CAMERA * 1.31);
    
    // Update: Do not set the mask, it's not needed and blurs desktop parts that don't need to be.
	// And wrap us into the paintOutput set
	
    status = gScreen->glPaintOutput (attrib, zTransform, region, output, mask);

    return status;
}

// Fairly standard stuff, I don't even mess with anything here...

bool
WTWindow::damageRect (bool initial,
		      const CompRect &rect)
{
    bool status = true;
    
    HEADTRACKING_SCREEN (screen);
    
    if (!initial)
    {
        CompRegion dRegion (window->serverX (), window->serverY (),
        		    window->serverX () + window->serverWidth (),
        		    window->serverY () + window->serverHeight ());
        
        wts->cScreen->damageRegion (dRegion);
    }
    
    status |= cWindow->damageRect (initial, rect);

    wts->cScreen->damagePending ();
    
    if (initial)
    {
	wts->cScreen->damagePending ();
    }

    return status;
}

/* Changes width depth manually */

bool
WTScreen::WTManual (CompAction         *action,
		    CompAction::State  state,
		    CompOption::Vector &options,
		    bool depth,
		    bool reset)
{
    CompWindow *w = screen->findWindow (CompOption::getIntOptionNamed (options,
    								       "window",
    								       0));

    if (!w)
	return false;
						       
    HEADTRACKING_WINDOW (w);
    
    wtw->mIsManualDepth = true;
    
    if (depth)
	wtw->mDepth -= (float) optionGetWindowDepth () / 100.0f;
    else
	wtw->mDepth += (float) optionGetWindowDepth () / 100.0f;
	
    if (reset)
	wtw->mDepth = 0.0f;
	
    cScreen->damagePending ();
    
    return true;
}


/* Moves the camera manually */

bool
WTScreen::WTDebug (CompAction         *action,
		   CompAction::State  state,
		   CompOption::Vector options,
		   bool changeX, bool changeY, bool changeZ,
		   bool x, bool y, bool z,
		   bool reset)
{
    CompWindow *w;

    if (!optionGetDebugEnabled ())
	return false;
	
    w = screen->findWindow (CompOption::getIntOptionNamed (options, "window", 
    									    0));
    									    
    if (!w)
	return false;
	
    if (changeX)
    {
	if (x)
	    mHead.x += (float) optionGetCameraMove () / 100.0f;
	else
	    mHead.x -= (float) optionGetCameraMove () / 100.0f;
    }
    
    if (changeY)
    {
	if (y)
	    mHead.y += (float) optionGetCameraMove () / 100.0f;
	else
	    mHead.y -= (float) optionGetCameraMove () / 100.0f;
    }

    if (changeZ)
    {
	if (z)
	    mHead.z -= (float) optionGetCameraMove () / 100.0f;
	else
	    mHead.z += (float) optionGetCameraMove () / 100.0f;
    }
    
    if (reset)
    {
	mHead.x = 0.0f;
	mHead.y = 0.0f;
	mHead.z = 0.0f;
    }
    
    return true;
}

// Toggle Mouse-Tracking (overides normal tracking!)
bool
WTScreen::WTToggleMouse (CompAction          *action,
			 CompAction::State   state,
			 CompOption::Vector  options)
{
    if (!optionGetDebugEnabled ())
	return false;

    mTrackMouse = !mTrackMouse;
    
    if (!mMousepoller.active ())
    {
	mMousepoller.start ();
    }
    else
	mMousepoller.stop ();
    
    return true;
}

WTWindow::WTWindow (CompWindow *window) :
    PluginClassHandler <WTWindow, CompWindow> (window),
    window (window),
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window)),
    mDepth (0.0f),
    mManualDepth (0.0f),
    mZDepth (0.0f),
    mIsManualDepth (false),
    mIsGrabbed (false)
{
    GLWindowInterface::setHandler (gWindow);
    CompositeWindowInterface::setHandler (cWindow);
}

WTWindow::~WTWindow ()
{
    mDepth = 0.0f;
    
    HEADTRACKING_SCREEN (screen);
    
    if (wts->mGrabWindow == window)
	wts->mGrabWindow = NULL;
}

WTScreen::WTScreen (CompScreen *screen) :
    PluginClassHandler <WTScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    mGrabWindow (NULL),
    mFocusWindow (NULL),
    mGrabIndex (0),
    mTrackMouse (false)
{
    mHead.x = 0.0f;
    mHead.y = 0.0f;
    mHead.z = 1.0f;
    
    ScreenInterface::setHandler (screen);
    CompositeScreenInterface::setHandler (cScreen);
    GLScreenInterface::setHandler (gScreen);
    
    mMousepoller.setCallback (boost::bind (&WTScreen::updatePosition, this, _1));
    
    mMousepoller.stop ();
    
    optionSetManualOutInitiate (boost::bind (&WTScreen::WTManual, this, _1, _2,
    					     _3, true, false));
    optionSetManualInInitiate (boost::bind (&WTScreen::WTManual, this, _1, _2,
    					     _3, false, false));
    optionSetManualResetInitiate (boost::bind (&WTScreen::WTManual, this, _1, _2,
    					     _3, false, true));
/*
    optionSetCameraInInitiate (boost::bind (&WTScreen::WTDebug, this, _1, _2,
    					     _3, false, false, true, false,
    					     false, true, false));
    optionSetCameraOutInitiate (boost::bind (&WTScreen::WTDebug, this, _1, _2,
    					     _3, false, false, true, false,
    					     false, false, false));
    optionSetCameraLeftInitiate (boost::bind (&WTScreen::WTDebug, this, _1, _2,
    					     _3, true, false, false, false,
    					     false, false, false));
    optionSetCameraRightInitiate (boost::bind (&WTScreen::WTDebug, this, _1, _2,
    					     _3, true, false, false, true,
    					     false, false, false));
    optionSetCameraUpInitiate (boost::bind (&WTScreen::WTDebug, this, _1, _2,
    					     _3, false, true, false, false,
    					     false, false, false));
    optionSetCameraDownInitiate (boost::bind (&WTScreen::WTDebug, this, _1, _2,
    					     _3, false, true, false, false,
    					     true, false, false));

    optionSetCameraResetInitiate (boost::bind (&WTScreen::WTDebug, this, _1, _2,
    					     _3, false, false, false, false,
    					     false, false, true));
*/
    optionSetToggleMouseInitiate (boost::bind (&WTScreen::WTToggleMouse, this,
    					       _1, _2, _3));
    
    // NOTICE: Tracking events have been removed
    // A general "set head position" may be added
    // back in at a later time, depends on what
    // is needed. If a separate headtracking
    // program that uses other methods and tools
    // is written, I'll open this up for use anywhere
    // but may also rename the plugin to just
    // be "headtracking" or something.
}

WTScreen::~WTScreen ()
{
    endThread ();
}
    
/*}}}*/

bool
HeadtrackingPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI) ||
        !CompPlugin::checkPluginABI ("mousepoll", COMPIZ_MOUSEPOLL_ABI))
        return false;

    return true;
}

