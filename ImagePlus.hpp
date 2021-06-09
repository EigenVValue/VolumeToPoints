
#include <string>
#include <vector>
#include "Image.hpp"
#include "ImageJ.hpp"
#include "ImageProcessor.hpp"
#include "ImageWindow.hpp"
#include "Roi.hpp"
#include "Thread.hpp"
#include "FileInfo.hpp"
#include "ImageStack.hpp"
#include "Calibration.hpp"
#include "ImageWindow.hpp"
#include "Color.hpp"
#include "Component.hpp"
#include "Properties.hpp"
#include "Plot.hpp"
#include "ImageCanvas.hpp"
#include "Overlay.hpp"
#include "LUT.hpp"
#include "BufferedImage.hpp"
#include "ByteProcessor.hpp"
#include "Object.hpp"

class ImagePlus {

	/** 8-bit grayscale (unsigned)*/
public:
	int GRAY8 = 0;

	/** 16-bit grayscale (unsigned) */
	int GRAY16 = 1;

	/** 32-bit floating-point grayscale */
	int GRAY32 = 2;

	/** 8-bit indexed color */
	int COLOR_256 = 3;

	/** 32-bit RGB color */
	int COLOR_RGB = 4;

	/** Title of image used by Flatten command */
	std::string flattenTitle = "flatten~canvas";

	/** True if any changes have been made to this image. */
	bool changes;

protected:
	Image img;
	ImageProcessor ip;
	ImageWindow win;
	Roi roi;
	int currentSlice; // current stack index (one-based)
	int OPENED = 0, CLOSED = 1, UPDATED = 2, SAVED = 3;
	bool compositeImage;
	int width;
	int height;
	bool locked;
	int lockedCount;
	Thread lockingThread;
	int nChannels = 1;
	int nSlices = 1;
	int nFrames = 1;
	bool dimensionsSet;

private:
	//ImageJ ij = IJ.getInstance();
	std::string title;
	std::string url;
	FileInfo fileInfo;
	int imageType = GRAY8;
	bool typeSet;
	ImageStack stack;
	int currentID = -1;
	int ID;
	Component comp;
	bool imageLoaded;
	int imageUpdateY, imageUpdateW;
	Properties properties;
	long startTime;
	Calibration calibration;
	Calibration globalCalibration;
	bool activated;
	bool ignoreFlush;
	bool errorLoadingImage;
	ImagePlus clipboard;
	std::vector<int> listeners;
	bool openAsHyperStack;
	int position[3] = { 1,1,1 };
	bool noUpdateMode;
	ImageCanvas flatteningCanvas;
	Overlay overlay;
	bool compositeChanges;
	bool hideOverlay;
	int default16bitDisplayRange;
	bool antialiasRendering = true;
	bool ignoreGlobalCalibration;
	bool oneSliceStack;
	//bool setIJMenuBar = Prefs.setIJMenuBar;
	Plot plot;
	Properties imageProperties;
	Color borderColor;

public:
	ImagePlus();

	ImagePlus(std::string title, Image image);

	ImagePlus(std::string title, ImageProcessor ip);

	ImagePlus(std::string pathOrURL);

	ImagePlus(std::string title, ImageStack stack);

private:
	void setID();

public:
	bool lock();

	bool lockSilently();

private:
	bool lock(bool loud);

public:
	void unlock();

	bool isLocked();

	bool isLockedByAnotherThread();

private:
	void waitForImage(Image image);

	long waitStart;
private:

public:
	void draw();

	void draw(int x, int y, int width, int height);

	void updateAndDraw();

	void updateVirtualSlice();

	void setDisplayMode(int mode);

	int getDisplayMode();

	void setActiveChannels(std::string channels);

	void updateChannelAndDraw();

	ImageProcessor getChannelProcessor();

	LUT* getLuts();

	void repaintWindow();

	void updateAndRepaintWindow();

	void updateImage();

	void hide();

	void close();

	void show();

	void show(std::string statusMessage);

	void invertLookupTable();

	void setActivated();

	Image getImage();

	BufferedImage getBufferedImage();

	int getID();

	void setImage(Image image);

	void setImage(ImagePlus imp);

	void setProcessor(ImageProcessor ip);

	void setProcessor(std::string title, ImageProcessor ip);

	void setProcessor2(std::string title, ImageProcessor ip, ImageStack newStack);

	void setStack(ImageStack stack);

	void setStack(std::string title, ImageStack newStack);

	void setStack(ImageStack newStack, int channels, int slices, int frames);

private:
	void setStackNull();

public:
	void setFileInfo(FileInfo fi);

	ImageWindow getWindow();

	bool isVisible();

	void setWindow(ImageWindow win);

	ImageCanvas getCanvas();

	void setColor(Color c);

	void setupProcessor();

	bool isProcessor();

	ImageProcessor getProcessor();

	void trimProcessor();

	ImageProcessor getMask();

	ByteProcessor createRoiMask();

	ByteProcessor createThresholdMask();

	//ImageStatistics getStatistics();

	//ImageStatistics getAllStatistics();

	//ImageStatistics getRawStatistics();

	//ImageStatistics getStatistics(int mOptions);

	//ImageStatistics getStatistics(int mOptions, int nBins);

	//ImageStatistics getStatistics(int mOptions, int nBins, double histMin, double histMax);

	std::string getTitle();

	std::string getShortTitle();

	void setTitle(std::string title);

	int getWidth();

	int getHeight();

	double getSizeInBytes();

	int getStackSize();

	int getImageStackSize();

	void setDimensions(int nChannels, int nSlices, int nFrames);

	bool isHyperStack();

	int getNDimensions();

	bool isDisplayedHyperStack();

	int getNChannels();

	int getNSlices();

	int getNFrames();

	int* getDimensions();

	int* getDimensions(bool varify);

	void verifyDimensions();

	int getType();

	int getBitDepth();

	int getBytesPerPixel();

protected:
	void setType(int type);

public:
	void setTypeToColor256();

	std::string getStringProperty(std::string key);

private:
	bool isDicomTag(std::string key);

public: 
	double getNumericProperty(std::string key);

private: 
	std::string getStringProperty(std::string key, std::string info);

	int findKey(std::string s, std::string key);

public:
	void setProp(std::string key, std::string value);

	void setProp(std::string key, double value);

	std::string getProp(std::string key);

	double getNumericProp(std::string key);

	std::string getPropertiesAsArray();

	std::string getPropsInfo();

	void setProperties(std::string props);

	std::string getInfoProperty();

	Object getProperty(std::string key);

	void setProperty(std::string key, Object value);

	Properties getProperties();

	//LookUpTable createLut();

	bool isInvertedLut();

private:
	int pvalue[4];

public:
	int* getPixel(int x, int y);

	ImageStack createEmptyStack();

	ImageStack getStack();

	ImageStack getImageStack();

	int getCurrentSlice();

	void setCurrentSlice(int slice);

	int getChannel();

	int getSlice();

	int getFrame();

	void killStack();

	void setPosition(int channel, int slice, int frame);

	void setPositionWithoutUpdate(int channel, int slice, int frame);

	void setC(int channel);

	void setZ(int slice);

	void setT(int frame);

	int getC();

	int getZ();

	int getT();

	int getStackIndex(int channel, int slice, int frame);

	void resetStack();

	void setPosition(int n);

	int* convertIndexToPosition(int n);

	void setSlice(int n);

	void setSliceWithoutUpdate(int n);

	Roi getRoi();

	void setRoi(Roi newRoi);

	void setRoi(Roi newRoi, bool updateDisplay);

	void setRoi(int x, int y, int width, int height);

	//void setRoi(Rectangle r);

	void createNewRoi(int sx, int sy);

	void deleteRoi();

	bool okToDeleteRoi();

	void killRoi();

	void resetRoi();

	void saveRoi();

	void restoreRoi();
	
	bool isSmaller(Roi r);

	void revert();

	void revertStack(FileInfo fi);

	FileInfo getFileInfo();

private: 
	//void addLut(LookUpTable lut, FileInfo fi);

public:
	FileInfo getOriginalFileInfo();

	bool imageUpdate(Image img, int flags, int x, int y, int w, int h);

	void flush();

	void setIgnoreFlush(bool ignoreFlush);

	ImagePlus duplicate();

	ImagePlus resize(int dstWidth, int dstHeight, std::string options);

	ImagePlus resize(int dstWidth, int dstHeight, int dstDepth, std::string options);

	ImagePlus crop();

	ImagePlus crop(std::string options);

	ImagePlus* crop(Roi* rois, std::string options);

	ImagePlus* crop(Roi* rois);

	void cropAndSave(Roi* rois, std::string directory, std::string format);

	ImagePlus createImagePlus();

	ImagePlus createHyperStack(std::string title, int channels, int slices, int frames, int bitDepth);

	void copyScale(ImagePlus imp);

	void copyAttributes(ImagePlus imp);

	void startTiming();

	long getStartTime();

	Calibration getCalibration();

	void setCalibration(Calibration cal);

	void setGlobalCalibration(Calibration global);

	Calibration getGlobalCalibration();

	Calibration getStaticGlobalCalibration();

	Calibration getLocalCalibration();

	void setIgnoreGlobalCalibration(bool ignoreGlobalCalibration);

	void mouseMoved(int x, int y);

	void updateStatusbarValue();

	std::string getFFTLocation(int x, int y, Calibration cal);

	std::string getLocationAsString(int x, int y);

	std::string d2s(double n);

	std::string getValueAsString(int x, int y);

	void cut();

	void copy();

	void copy(bool cut);
	
	void paste();

	void paste(int x, int y);

	void paste(int x, int y, std::string mode);

	ImagePlus getClipboard();

	void resetClipboard();

	void copyToSystem();

	void notifyListeners(int id);

	//void addImageListener(ImageListener listener);

	//void removeImageListener(ImageListener listener);

	//std::vector<> getListeners();

	void logImageListeners();

	void setOpenAsHyperStack(bool openAsHyperStack);

	bool getOpenAsHyperStack();

	bool isComposite();

	int getCompositeMode();

	void setDisplayRange(double min, double max);

	double getDisplayRangeMin();

	double getDisplayRangeMax();

	void setDisplayRange(double min, double max, int channels);

	void resetDisplayRange();

	bool isThreshold();
	
	void setDefault16bitRange(int bitDepth);

	int getDefault16bitRange();

	void updatePosition(int c, int z, int t);

	ImagePlus flatten();

	void flattenStack();

	void flattenImage(ImageStack stack, int slice, Overlay overlay, bool showAll);

	void flattenImage(ImageStack stack, int slice, Overlay overlay, bool showAll, int z, int t);

	bool tempOverlay();

	void setPointScale(Roi roi2, Overlay overlay2);

	void setLut(LUT lut);

	void setOverlay(Overlay overlay);

	//void setOverlay(Shape shape, Color color, BasicStroke stroke);

	void setOverlay(Roi roi, Color strokeColor, int strokeWidth, Color fillColor);

	Overlay getOverlay();

	void setHideOverlay(bool hide);

	bool getHideOverlay();

	void setAntialiasRendering(bool antialiasRendering);

	Object clone();

	//PlotWindow plotHistogram();
	
	//PlotWindow plotHistogram(int bins);

	std::string toString();

	void setIJMenuBar(bool b);

	bool setIJMenuBar();

	bool isStack();

	void setPlot(Plot plot);

	Plot getPlot();

	Properties getImageProperties();

	bool isRGB();

	void setBorderColor(Color borderColor);

	bool windowActivated();

};
