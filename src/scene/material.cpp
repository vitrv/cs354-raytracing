#include "material.h"
#include "ray.h"
#include "light.h"
#include "../ui/TraceUI.h"
extern TraceUI* traceUI;

#include "../fileio/images.h"
#include <glm/gtx/io.hpp>
#include <iostream>

using namespace std;
extern bool debugMode;

Material::~Material()
{
}

// Apply the phong model to this point on the surface of the object, returning
// the color of that point.
glm::dvec3 Material::shade(Scene *scene, const ray& r, const isect& i) const
{
	// YOUR CODE HERE

	// For now, this method just returns the diffuse color of the object.
	// This gives a single matte color for every distinct surface in the
	// scene, and that's it.  Simple, but enough to get you started.
	// (It's also inconsistent with the phong model...)

	// Your mission is to fill in this method with the rest of the phong
	// shading model, including the contributions of all the light sources.
	// You will need to call both distanceAttenuation() and shadowAttenuation()
	// somewhere in your code in order to compute shadows and light falloff.
	//	if( debugMode )
	//		std::cout << "Debugging Phong code..." << std::endl;

	// When you're iterating through the lights,
	// you'll want to use code that looks something
	// like this:
    // i->material.ks(i)

    glm::dvec3 intersect = r.at(i.t) ;

    glm::dvec3 phong = ke(i) + ka(i) * scene->ambient();

    glm::dvec3 atten(0.0, 0.0, 0.0);
    glm::dvec3 diffuseTerm(0.0, 0.0, 0.0);
    glm::dvec3 specTerm(0.0, 0.0, 0.0);
    glm::dvec3 R(0.0, 0.0, 0.0);
    glm::dvec3 ri(0.0, 0.0, 0.0);
    glm::dvec3 rinv = -1.0 * r.getDirection();


	for ( vector<Light*>::const_iterator litr = scene->beginLights(); 
	 		litr != scene->endLights(); 
	 		++litr )
	 {
	 		Light* pLight = *litr;
	 		atten = pLight->distanceAttenuation(intersect)
	 		* pLight->shadowAttenuation(r,intersect);

            double lightdir = dot(pLight->getDirection(intersect), i.N);
            ri = -1.0 * pLight->getDirection(intersect);
            R = ri - (2.0 * i.N * dot(ri, i.N));

            diffuseTerm = kd(i) * pLight->getColor()
                           * max(lightdir, 0.0);
            specTerm = max(ks(i) * pLight->getColor() * 
                        pow(dot(R , rinv), shininess(i)), 0.0);

            phong += atten * (diffuseTerm + specTerm);

	 }

	return phong;
}

TextureMap::TextureMap( string filename )
{
	data = readImage(filename.c_str(), width, height);
	if (data == NULL) {
		width = 0;
		height = 0;
		string error("Unable to load texture map '");
		error.append(filename);
		error.append("'.");
		throw TextureMapException(error);
	}
}

glm::dvec3 TextureMap::getMappedValue( const glm::dvec2& coord ) const
{
	// YOUR CODE HERE
	// 
	// In order to add texture mapping support to the 
	// raytracer, you need to implement this function.
	// What this function should do is convert from
	// parametric space which is the unit square
	// [0, 1] x [0, 1] in 2-space to bitmap coordinates,
	// and use these to perform bilinear interpolation
	// of the values.

	return glm::dvec3(1,1,1);
}


glm::dvec3 TextureMap::getPixelAt( int x, int y ) const
{
    // This keeps it from crashing if it can't load
    // the texture, but the person tries to render anyway.
    if (0 == data)
      return glm::dvec3(1.0, 1.0, 1.0);

    if( x >= width )
       x = width - 1;
    if( y >= height )
       y = height - 1;

    // Find the position in the big data array...
    int pos = (y * width + x) * 3;
    return glm::dvec3(double(data[pos]) / 255.0, 
       double(data[pos+1]) / 255.0,
       double(data[pos+2]) / 255.0);
}

glm::dvec3 MaterialParameter::value( const isect& is ) const
{
    if( 0 != _textureMap )
        return _textureMap->getMappedValue( is.uvCoordinates );
    else
        return _value;
}

double MaterialParameter::intensityValue( const isect& is ) const
{
    if( 0 != _textureMap )
    {
        glm::dvec3 value( _textureMap->getMappedValue( is.uvCoordinates ) );
        return (0.299 * value[0]) + (0.587 * value[1]) + (0.114 * value[2]);
    }
    else
        return (0.299 * _value[0]) + (0.587 * _value[1]) + (0.114 * _value[2]);
}

