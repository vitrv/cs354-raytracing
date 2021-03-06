// The main ray tracer.

#pragma warning (disable: 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"

#include "parser/Tokenizer.h"
#include "parser/Parser.h"

#include "ui/TraceUI.h"
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>
#include <string.h> // for memset

#include <iostream>
#include <fstream>

using namespace std;
using namespace glm;
extern TraceUI* traceUI;

// Use this variable to decide if you want to print out
// debugging messages.  Gets set in the "trace single ray" mode
// in TraceGLWindow, for example.
bool debugMode = true;

// Trace a top-level ray through pixel(i,j), i.e. normalized window coordinates (x,y),
// through the projection plane, and out into the scene.  All we do is
// enter the main ray-tracing method, getting things started by plugging
// in an initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.

glm::dvec3 RayTracer::trace(double x, double y, unsigned char *pixel, unsigned int ctr)
{
    // Clear out the ray cache in the scene for debugging purposes,
  if (TraceUI::m_debug) scene->intersectCache.clear();

    ray r(glm::dvec3(0,0,0), glm::dvec3(0,0,0), pixel, ctr, glm::dvec3(1,1,1), ray::VISIBILITY);
    scene->getCamera().rayThrough(x,y,r);
    double dummy;
    glm::dvec3 ret = traceRay(r, glm::dvec3(1.0,1.0,1.0), traceUI->getDepth() , dummy);
    ret = glm::clamp(ret, 0.0, 1.0);
    return ret;
}

glm::dvec3 RayTracer::tracePixel(int i, int j, unsigned int ctr)
{
	glm::dvec3 col(0,0,0);

	if( ! sceneLoaded() ) return col;

	double x = double(i)/double(buffer_width);
	double y = double(j)/double(buffer_height);
	double x_offs = 0.25/buffer_width;
	double y_offs = 0.25/buffer_height;

	unsigned char *pixel = buffer + ( i + j * buffer_width ) * 3;
	

	//Anti-aliasing
	if(traceUI->aaSwitch()){
	    col = trace(x+x_offs, y+y_offs, pixel, ctr) * 0.25 + 
	          trace(x-x_offs, y-y_offs, pixel, ctr) * 0.25 +
	          trace(x+x_offs, y-y_offs, pixel, ctr) * 0.25 + 
	          trace(x-x_offs, y+y_offs, pixel, ctr) * 0.25; 

    }
    else col = trace(x, y, pixel, ctr);

    pixel[0] = (int)( 255.0 * col[0]);
	pixel[1] = (int)( 255.0 * col[1]);
	pixel[2] = (int)( 255.0 * col[2]);
	return col;
}


// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
glm::dvec3 RayTracer::traceRay(ray& r, const glm::dvec3& thresh, int depth, double& t )
{
	isect i;
	glm::dvec3 colorC;

	if(scene->intersect(r, i)) {
		const Material& m = i.getMaterial();	
		// YOUR CODE HERE

		// An intersection occurred!  We've got work to do.  For now,
		// this code gets the material for the surface that was intersected,
		// and asks that material to provide a color for the ray.  

		// This is a great place to insert code for recursive ray tracing.
		// Instead of just returning the result of shade(), add some
		// more steps: add in the contributions from reflected and refracted
		// rays.

		// https://www.cs.unc.edu/~rademach/xroads-RT/RTarticle.html
		// The reflection function = 2.0* (RayDirection * Normal)*Normal-RayDirection)
		// Get max Recursion depth 

		colorC = m.shade(scene, r, i);	

		dvec3 kt = m.kt(i);
		dvec3 kr = m.kr(i);

		glm::dvec3 rayDir = r.getDirection();


		if(depth >= 0){
			//Reflection
			double c1 = -dot(i.N,rayDir);
			glm::dvec3 reflectDirection = rayDir +(2.0 * i.N * c1);
			//Recursively get the reflection 
			ray refRay (r.at(i.t), normalize(reflectDirection), r.pixel, r.ctr, r.atten, ray::REFLECTION);
			glm::dvec3 reflectColor (0.0, 0.0, 0.0);
			reflectColor += traceRay(refRay, thresh, depth-1, t);

		    colorC += kr * reflectColor;
			//printf("I've reached recursion %d\n", depth); 


			//Refraction
			if(length(kt)!=0.0){
                double ni = 0.0;
                double nt = 0.0;
                glm::dvec3 normal(0.0,0.0,0.0); //normal for refraction 
                //Check if ray is entering or leaving
                if(dot(rayDir, i.N) > 0){ //exiting object
                    normal = -i.N;
                    ni = m.index(i);
                    nt = 1.0; 
                }
                else{ //entering object
                	normal = i.N;
                    ni = 1.0;
                    nt = m.index(i);
                }
				dvec3 refractD = refract(rayDir, normal, ni/nt);
				ray refractRay(r.at(i.t), refractD, r.pixel, r.ctr, r.atten, ray::REFRACTION);
				dvec3 refractColor (0.0, 0.0, 0.0);

                //TIR
				if(dot(refractD, rayDir) == 0){
					ray tirRay(r.at(i.t), reflectDirection, r.pixel, r.ctr, r.atten, ray::REFRACTION);
					refractColor += traceRay(tirRay, thresh, depth-1, t);
				}	
				else 	
				    refractColor += traceRay(refractRay, thresh, depth-1, t);

				colorC += kt * refractColor;
			}

		 }


	} else {
		// No intersection.  This ray travels to infinity, so we color
		// it according to the background color, which in this (simple) case
		// is just black.
		// 
		// FIXME: Add CubeMap support here.

		//cubemap.getColor();

		if(cubemap!=NULL) colorC = cubemap->getColor(r);
	}
	return colorC;
}

RayTracer::RayTracer()
	: scene(0), buffer(0), thresh(0), buffer_width(256), buffer_height(256), m_bBufferReady(false), cubemap (0)
{
}

RayTracer::~RayTracer()
{
	delete scene;
	delete [] buffer;
}

void RayTracer::getBuffer( unsigned char *&buf, int &w, int &h )
{
	buf = buffer;
	w = buffer_width;
	h = buffer_height;
}

double RayTracer::aspectRatio()
{
	return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene( char* fn ) {
	ifstream ifs( fn );
	if( !ifs ) {
		string msg( "Error: couldn't read scene file " );
		msg.append( fn );
		traceUI->alert( msg );
		return false;
	}
	
	// Strip off filename, leaving only the path:
	string path( fn );
	if( path.find_last_of( "\\/" ) == string::npos ) path = ".";
	else path = path.substr(0, path.find_last_of( "\\/" ));

	// Call this with 'true' for debug output from the tokenizer
	Tokenizer tokenizer( ifs, false );
	Parser parser( tokenizer, path );
	try {
		delete scene;
		scene = 0;
		scene = parser.parseScene();
	} 
	catch( SyntaxErrorException& pe ) {
		traceUI->alert( pe.formattedMessage() );
		return false;
	}
	catch( ParserException& pe ) {
		string msg( "Parser: fatal exception " );
		msg.append( pe.message() );
		traceUI->alert( msg );
		return false;
	}
	catch( TextureMapException e ) {
		string msg( "Texture mapping exception: " );
		msg.append( e.message() );
		traceUI->alert( msg );
		return false;
	}

	if( !sceneLoaded() ) return false;

    if(traceUI->kdSwitch()){
        scene->buildTree();
    }
	return true;
}

void RayTracer::traceSetup(int w, int h)
{
	if (buffer_width != w || buffer_height != h)
	{
		buffer_width = w;
		buffer_height = h;
		bufferSize = buffer_width * buffer_height * 3;
		delete[] buffer;
		buffer = new unsigned char[bufferSize];
	}
	memset(buffer, 0, w*h*3);
	m_bBufferReady = true;
}

void RayTracer::traceImage(int w, int h, int bs, double thresh)
{
	// YOUR CODE HERE
	// FIXME: Start one or more threads for ray tracing


	//w = 512;
	//h = 512;


    traceSetup(w,h); 
    // go thru each pixel in image
    for (int i = 0; i < w; i++) 
        for (int j = 0; j < h; j++)
	        tracePixel(i,j,0);

}

int RayTracer::aaImage(int samples, double aaThresh)
{
	// YOUR CODE HERE
	// FIXME: Implement Anti-aliasing here
}

bool RayTracer::checkRender()
{
	// YOUR CODE HERE
	// FIXME: Return true if tracing is done.
	return true;
}

glm::dvec3 RayTracer::getPixel(int i, int j)
{
	unsigned char *pixel = buffer + ( i + j * buffer_width ) * 3;
	return glm::dvec3((double)pixel[0]/255.0, (double)pixel[1]/255.0, (double)pixel[2]/255.0);
}

void RayTracer::setPixel(int i, int j, glm::dvec3 color)
{
	unsigned char *pixel = buffer + ( i + j * buffer_width ) * 3;

	pixel[0] = (int)( 255.0 * color[0]);
	pixel[1] = (int)( 255.0 * color[1]);
	pixel[2] = (int)( 255.0 * color[2]);
}

