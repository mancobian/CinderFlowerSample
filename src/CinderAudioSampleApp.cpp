#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CinderAudioSampleApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
};

void CinderAudioSampleApp::setup()
{
}

void CinderAudioSampleApp::mouseDown( MouseEvent event )
{
}

void CinderAudioSampleApp::update()
{
}

void CinderAudioSampleApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP_NATIVE( CinderAudioSampleApp, RendererGl )
