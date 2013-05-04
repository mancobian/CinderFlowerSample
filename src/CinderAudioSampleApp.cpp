#ifdef __APPLE__
#include "CoreFoundation/CoreFoundation.h"
#endif
#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/audio/Input.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/PcmBuffer.h"
#include "cinder/qtime/QuickTime.h"
#include "cinder/ip/Resize.h"

using namespace ci;
using namespace ci::app;
using namespace cinder::audio;
using namespace cinder::qtime;
using namespace std;

//------------------------------------------------------------------------------
//! @brief Sample Cinder app demonstrating how to find, load and sample a
//  SoundFlower device to drive some neat visual effects.
//! @note I run pretty slow at higher resolution; be gentle. I need some shader love, for sure.
class CinderFlowerSample : public AppNative
{
public:
    static const char *SAMPLE_MOVIE;
    static const char *SOUNDFLOWER_DEVICE_NAME;
    static void setpwd();
	void setup();
	void update();
	void draw();
    
private:
    std::shared_ptr<Input> m_input;
    audio::PcmBuffer32fRef m_currentPcmBuffer;
    audio::Buffer32fRef m_leftBuffer;
	audio::Buffer32fRef m_rightBuffer;
    qtime::MovieSurface	m_movie;
	Surface	m_surface;
    
    void setupInputAudioDevice();
    void loadMovieFile( const fs::path &path );
    void drawWaveForm();
};

//------------------------------------------------------------------------------
//! @brief The movie we want to parametrically destroy using our audio input.
const char* CinderFlowerSample::SAMPLE_MOVIE = "./skydiverr.mov";

//------------------------------------------------------------------------------
//! @brief The audio device we expect to use for evil intent!
const char *CinderFlowerSample::SOUNDFLOWER_DEVICE_NAME = "Soundflower (2ch)";

//------------------------------------------------------------------------------
//! brief This makes relative paths work in C++ in Xcode by changing directory
//  to the Resources folder inside the .app bundle.
void CinderFlowerSample::setpwd()
{
#ifdef __APPLE__
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
    {
        // error!
    }
    CFRelease(resourcesURL);
    
    chdir(path);
    std::cout << "Current Path: " << path << std::endl;
#endif
}

//------------------------------------------------------------------------------
//! @brief Load up an audio device and a sample movie.
void CinderFlowerSample::setup()
{
    // I put my root down...
    setpwd();
    
    // ...I kick it root down!
    this->setupInputAudioDevice();
    this->loadMovieFile( CinderFlowerSample::SAMPLE_MOVIE );
}

//------------------------------------------------------------------------------
//! @brief Sample audio and video for the current frame.
void CinderFlowerSample::update()
{
    // Sample audio for the current frame
    if( this->m_input )
    {
        this->m_currentPcmBuffer = this->m_input->getPcmBuffer();
        this->m_leftBuffer = this->m_currentPcmBuffer->getChannelData( audio::CHANNEL_FRONT_LEFT );
        this->m_rightBuffer = this->m_currentPcmBuffer->getChannelData( audio::CHANNEL_FRONT_RIGHT );
    }
    
    // Sample video for the current frame
    if( this->m_movie )
    {
        this->m_movie.stepForward();
        if( !this->m_movie.checkNewFrame() )
        {
            this->m_movie.seekToStart();
        }
        
		this->m_surface = this->m_movie.getSurface();
        Surface resizedSurface( cinder::app::getWindowWidth(), cinder::app::getWindowHeight(), true );
        cinder::ip::resize( this->m_surface, &resizedSurface );
        this->m_surface = resizedSurface;
    }
}

//------------------------------------------------------------------------------
//! @brief This is where we wield some audio to beat the shit out of our movie for awesome!
void CinderFlowerSample::draw()
{
	// Clear to black!
	gl::clear( Color( 0, 0, 0 ) );
    gl::enableAlphaBlending( false );
    
    // Vertically displace columns of pixels in the video as a function of the current frame's audio waveform!
    if( this->m_surface )
    {
        // We are using OpenGL to draw the frames here,
        // so we'll make a texture out of the surface!
        int col = 0;
        Surface cloneSurface = this->m_surface.clone();
        Surface::Iter cloneIter = cloneSurface.getIter();
        Surface::Iter iter = this->m_surface.getIter();
        
        // Foreach row...
		while( iter.line() && cloneIter.line() )
		{
            // Foreach column...
			while( iter.pixel() && cloneIter.pixel() )
			{
                float percent = (float)col / (float)iter.getWidth();
                const int MAX_DISPLACEMENT = 100; //! @note Magic numero!
                
                uint32_t bufferIndex = (uint32_t)std::floor( percent * (float)this->m_leftBuffer->mSampleCount );
                float leftMagnitude = (float)(this->m_leftBuffer->mData[bufferIndex ]);

                int displacement = (int)((leftMagnitude + 0.01f) * ((float)rand() / (float)RAND_MAX) * (float)MAX_DISPLACEMENT);
                
                float r = (float)iter.rClamped( 0, displacement ) / 255.0f;
                float g = (float)iter.gClamped( 0, displacement ) / 255.0f;
                float b = (float)iter.bClamped( 0, displacement ) / 255.0f;
                
                cloneIter.r() = (int)( r * 255.0f );
                cloneIter.g() = (int)( g * 255.0f );
                cloneIter.b() = (int)( b * 255.0f );
                cloneIter.a() = (int)((leftMagnitude) * 13.7f * 255.0f); //! @note Magic numero!
                ++col;
            }
            col = 0;
        }
        
        // Show our work!
        gl::Texture movieTexture( cloneSurface );
        gl::draw( movieTexture );
    }
    
    // Draw the audio waveform used for the video freakening we did above!
    this->drawWaveForm();
}

//------------------------------------------------------------------------------
//! @brief Find our target (SoundFlower) audio device.
void CinderFlowerSample::setupInputAudioDevice()
{
    const std::vector<InputDeviceRef> &devices = audio::Input::getDevices( true );
    for( size_t i = 0; i < devices.size(); ++i )
    {
        InputDeviceRef device = devices[i];
        std::cout << "Device: " << device->getName() << " (" << device->getDeviceId() << ")" << std::endl;
    }
    
    InputDeviceRef soundflowerDevice = cinder::audio::Input::findDeviceByName( CinderFlowerSample::SOUNDFLOWER_DEVICE_NAME );
    if( soundflowerDevice )
    {
        this->m_input.reset( new Input( soundflowerDevice ) );
        this->m_input->start();
    }
}

//------------------------------------------------------------------------------
//! @brief Load a sample movie to freak out.
void CinderFlowerSample::loadMovieFile( const fs::path &moviePath )
{
	try
    {
		this->m_movie = qtime::MovieSurface( moviePath );
        
		console() << "Dimensions:" << this->m_movie.getWidth() << " x " << this->m_movie.getHeight() << std::endl;
		console() << "Duration:  " << this->m_movie.getDuration() << " seconds" << std::endl;
		console() << "Frames:    " << this->m_movie.getNumFrames() << std::endl;
		console() << "Framerate: " << this->m_movie.getFramerate() << std::endl;
		console() << "Alpha channel: " << this->m_movie.hasAlpha() << std::endl;
		console() << "Has audio: " << this->m_movie.hasAudio() << " Has visuals: " << this->m_movie.hasVisuals() << std::endl;
        
		this->m_movie.setLoop( true, true );
		this->m_movie.seekToStart();
	}
	catch( ... )
    {
		console() << "Unable to load the movie." << std::endl;
	}	
}

//------------------------------------------------------------------------------
//! @brief Draw stereo waveform in the center of our screen.
void CinderFlowerSample::drawWaveForm()
{
	// If the buffer is null then don't do anything,
    // for example if this gets called before any PCM data has been buffered!
	if( ! this->m_currentPcmBuffer )
    {
		return;
	}
    
	uint32_t bufferLength = this->m_currentPcmBuffer->getSampleCount();
    
	int displaySize = getWindowWidth();
	float scale = displaySize / (float)bufferLength;
    
    const float VERTICAL_CENTER = cinder::app::getWindowHeight() / 2.0f;
    
	PolyLine<Vec2f>	leftBufferLine;
	PolyLine<Vec2f>	rightBufferLine;
    
	for( int i = 0; i < bufferLength; i++ )
    {
		float x = ( i * scale );
        
		//get the PCM value from the left channel buffer
		float y = ( ( this->m_leftBuffer->mData[i] - 1 ) * - VERTICAL_CENTER );
		leftBufferLine.push_back( Vec2f( x , y) );
        
		y = ( ( this->m_rightBuffer->mData[i] - 1 ) * - VERTICAL_CENTER );
		rightBufferLine.push_back( Vec2f( x , y) );
	}

	gl::draw( leftBufferLine );
	gl::draw( rightBufferLine );
    
}

//------------------------------------------------------------------------------
//! @brief App entry point.
CINDER_APP_NATIVE( CinderFlowerSample, RendererGl )
