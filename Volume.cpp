/*-
 * #%L
 * Fiji distribution of ImageJ for the life sciences.
 * %%
 * Copyright (C) 2010 - 2016 Fiji developers.
 * %%
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/gpl-3.0.html>.
 * #L%
 */

/**
 * This class encapsulates an image stack and provides various methods for
 * retrieving data. It is possible to control the loaded color channels of RGB
 * images, and to specify whether or not to average several channels (and merge
 * them in this way into one byte per pixel). Depending on these settings, and
 * on the type of image given at construction time, the returned data type is
 * one of INT_DATA or BYTE_DATA.
 *
 * @author Benjamin Schmid
 */

#include <math.h>

#include "ImagePlus.hpp"
#include "Loader.hpp"

#include "thirdparties/include/glm/glm.hpp"

using namespace glm;

class Volume {
private:
	int rLUT[256];
	int gLUT[256];
	int bLUT[256];
	int aLUT[256];

	/**
	 * Data is read as int data. If the input image is RGB, this is the case if
	 * isDefaultLUT() returns false or more than a single channel is used. If the
	 * input image is 8 bit, again this is the case if isDefaultLUT() returns
	 * false.
	 */
public:
	int INT_DATA = 0;

	/**
	 * Data is read as byte data. If the input image is RGB, this is the case if
	 * isDefaultLUT() returns true and only a single channel is used. If the input
	 * image is 8 bit, again this is the case if isDefaultLUT() returns true.
	 */
public:
	int BYTE_DATA = 1;

protected:
	/** The image holding the data */
	ImagePlus imp;

	/** Wraping the ImagePlus */
	InputImage image;

	/** The loader, initialized depending on the data type */
	//Loader loader;

	/**
	 * Indicates in which format the data is loaded. This depends on the image
	 * type and on the number of selected channels. May be one of INT_DATA or
	 * BYTE_DATA
	 */
	int dataType;

	/** Flag indicating that the channels should be averaged */
	bool average = false;

	/** Flag indicating that channels should be saturated */
	bool saturatedVolumeRendering = false;

	/** Channels in RGB images which should be loaded */
	bool channels[3] = { true, true, true };

	/** The dimensions of the data */
public:
	int xDim, yDim, zDim;

	/** The calibration of the data */
	double pw, ph, pd;

	/** The minimum coordinate of the data */
	vec3 minCoord = vec3();

	/** The maximum coordinate of the data */
	vec3 maxCoord = vec3();

	/** Create instance with a null imp. */
protected:
	Volume() {
		this->image = NULL;
	}

	/**
	 * Initializes this Volume with the specified image. All channels are used.
	 *
	 * @param imp
	 */
public:
	Volume(ImagePlus imp) {
		//this(imp, new bool[]{ true, true, true });
	}

	/**
	 * Initializes this Volume with the specified image and channels.
	 *
	 * @param imp
	 * @param ch A boolean[] array of length three, which indicates whether the
	 *          red, blue and green channel should be read. This has only an
	 *          effect when reading color images.
	 */
	Volume(ImagePlus imp, bool* ch) {
		setImage(imp, ch);
	}

private:
	void setLUTsFromImage(ImagePlus imp) {
		switch (imp.getType()) {
			case ImagePlus.GRAY8:
			case ImagePlus.COLOR_256:
				IndexColorModel cm = (IndexColorModel)imp.getProcessor().getCurrentColorModel();
				for (int i = 0; i < 256; i++) {
					rLUT[i] = cm.getRed(i);
					gLUT[i] = cm.getGreen(i);
					bLUT[i] = cm.getBlue(i);
					aLUT[i] = min(254, (rLUT[i] + gLUT[i] + bLUT[i]) / 3);
				}
				break;
			case ImagePlus.COLOR_RGB:
				for (int i = 0; i < 256; i++) {
					rLUT[i] = gLUT[i] = bLUT[i] = i;
					aLUT[i] = min(254, i);
				}
				break;
			default:
				return;
		}
	}

public:
	void setImage(ImagePlus imp, bool ch[3]) {
		this->imp = imp;
		this->channels[0] = ch[0];
		this->channels[1] = ch[1];
		this->channels[2] = ch[2];
		switch (imp.getType()) {
			case ImagePlus.GRAY8:
			case ImagePlus.COLOR_256:
				image = new ByteImage(imp);
				break;
			case ImagePlus.COLOR_RGB:
				image = new IntImage(imp);
				break;
			default:
				return;
		}
		setLUTsFromImage(this->imp);

		xDim = imp.getWidth();
		yDim = imp.getHeight();
		zDim = imp.getStackSize();
		Calibration c = imp.getCalibration();
		pw = c.pixelWidth;
		ph = c.pixelHeight;
		pd = c.pixelDepth;

		const float xSpace = (float)pw;
		const float ySpace = (float)ph;
		const float zSpace = (float)pd;

		// real coords
		minCoord.x = c.xOrigin;
		minCoord.y = c.yOrigin;
		minCoord.z = c.zOrigin;

		maxCoord.x = minCoord.x + xDim * xSpace;
		maxCoord.y = minCoord.y + yDim * ySpace;
		maxCoord.z = minCoord.z + zDim * zSpace;

		initDataType();
		initLoader();
	}

	ImagePlus getImagePlus() {
		return imp;
	}

	void clear() {
		imp = NULL;
		image = NULL;
		loader = NULL;
	}

	void swap(std::string path) {
		IJ.save(imp, path + ".tif");
		imp = NULL;
		image = NULL;
		loader = NULL;
	}

	void restore(std::string path) {
		setImage(IJ.openImage(path + ".tif"), channels);
	}

	/**
	 * Checks if the LUTs of all the used color channels and of the alpha channel
	 * have a default LUT.
	 */
	bool isDefaultLUT() {
		for (int i = 0; i < 256; i++) {
			if ((channels[0] && rLUT[i] != i) || (channels[1] && gLUT[i] != i) ||
				(channels[2] && bLUT[i] != i) || aLUT[i] != i) return false;
		}
		return true;
	}

	/**
	 * Get the current set data type. This is one of BYTE_DATA or INT_DATA. The
	 * data type specifies in which format the data is read.
	 */
	int getDataType() {
		return dataType;
	}

	/**
	 * If true, build an average byte from the specified channels (for each
	 * pixel).
	 *
	 * @return true if the value for 'average' has changed.
	 */
	bool setAverage(const bool a) {
		if (average != a) {
			this->average = a;
			initDataType();
			initLoader();
			return true;
		}
		return false;
	}

	/**
	 * Returns true if specified channels are being averaged when reading the
	 * image data.
	 *
	 * @return
	 */
	bool isAverage() {
		return average;
	}

	/**
	 * If true, saturate the channels of RGB images; the RGB values of each pixels
	 * are scaled so that at least one of the values is 255, the alpha value is
	 * the average of the original RGB values.
	 *
	 * @return true if the value for 'saturatedVolumeRendering' has changed
	 */
	bool setSaturatedVolumeRendering(const bool b) {
		if (this->saturatedVolumeRendering != b) {
			this->saturatedVolumeRendering = b;
			initLoader();
			return true;
		}
		return false;
	}

	/**
	 * Returns whether if saturatedVolumeRendering is set to true.
	 */
	bool isSaturatedVolumeRendering() {
		return saturatedVolumeRendering;
	}

	/**
	 * Copies the current color table into the given array.
	 */
	void getRedLUT(const int* lut) {
		System.arraycopy(rLUT, 0, lut, 0, rLUT.length);
	}

	/**
	 * Copies the current color table into the given array.
	 */
	void getGreenLUT(const int* lut) {
		System.arraycopy(gLUT, 0, lut, 0, gLUT.length);
	}

	/**
	 * Copies the current color table into the given array.
	 */
	void getBlueLUT(const int* lut) {
		System.arraycopy(bLUT, 0, lut, 0, bLUT.length);
	}

	/**
	 * Copies the current color table into the given array.
	 */
	void getAlphaLUT(const int* lut) {
		System.arraycopy(aLUT, 0, lut, 0, aLUT.length);
	}

	/**
	 * Specify the channels which should be read from the image. This only affects
	 * RGB images.
	 *
	 * @return true if the channels settings has changed.
	 */
	bool setChannels(const bool* ch) {
		if (ch[0] == channels[0] && ch[1] == channels[1] && ch[2] == channels[2]) return false;
		*channels = ch;
		if (initDataType()) initLoader();
		return true;
	}

	/**
	 * Set the lookup tables for this volume. Returns true if the data type of the
	 * textures have changed.
	 */
	bool setLUTs(const int* r, const int* g, const int* b,
		const int* a)
	{
		*this->rLUT = *r;
		*this->gLUT = *g;
		*this->bLUT = *b;
		*this->aLUT = *a;
		if (initDataType()) {
			initLoader();
			return true;
		}
		return false;
	}

	/**
	 * Set the alpha channel to fully opaque. Returns true if the data type of the
	 * textures have changed.
	 */
	bool setAlphaLUTFullyOpaque() {
		for (int i = 0; i < sizeof(this->aLUT); i++) {
			aLUT[i] = 254;
		}
		if (initDataType()) {
			initLoader();
			return true;
		}
		return false;
	}

	/**
	 * Init the loader, based on the currently set data type, which is either
	 * INT_DATA or BYTE_DATA.
	 */
protected:
	void initLoader() {
		if (image == NULL) {
			printf("No image. Maybe it is swapped?");
			return;
		}

		if (dataType == INT_DATA) {
			loader =
				saturatedVolumeRendering ? new SaturatedIntLoader(image)
				: new IntLoader(image);
			return;
		}

		// else: BYTE_DATA
		if (average) {
			loader = new AverageByteLoader(image);
			return;
		}
		int channel = 0;
		if (image instanceof IntImage) {
			for (int i = 0; i < 3; i++)
				if (channels[i]) channel = i;
		}
		loader = new ByteLoader(image, channel);
	}

	/**
	 * Init the data type. For 8 bit images, BYTE_DATA is used if isDefaultLUT()
	 * returns true. For RGB images, an additional condition is that only a single
	 * channel is used. For other cases, the data type is INT_DATA.
	 */
	bool initDataType() {
		if (image == NULL) {
			printf("No image. Maybe it is swapped?");
			return;
		}
		int noChannels = 0;
		if (image instanceof ByteImage) {
			noChannels = 1;
		}
		else {
			for (int i = 0; i < 3; i++)
				if (channels[i]) noChannels++;
		}
		const bool defaultLUT = isDefaultLUT();
		const int tmp = dataType;
		if (average || (defaultLUT && noChannels < 2)) dataType = BYTE_DATA;
		else dataType = INT_DATA;

		return tmp != dataType;
	}

	void setNoCheck(const int x, const int y, const int z, const int v) {
		try {
			loader.setNoCheck(x, y, z, v);
		}
		catch (const std::exception e) {
			printf("No image. Maybe it is swapped");
			return;
		}
	}

	void set(const int x, const int y, const int z, const int v) {
		try {
			loader.set(x, y, z, v);
		}
		catch (const std::exception e) {
			printf("No image. Maybe it is swapped");
			return;
		}
	}

	/**
	 * Load the value at the specified position
	 *
	 * @param x
	 * @param y
	 * @param z
	 * @return value. Casted to int if it was a byte value before.
	 */
	int load(const int x, const int y, const int z) {
		try {
			return loader.load(x, y, z);
		}
		catch (const std::exception e) {
			printf("No image. Maybe it is swapped");
			return;
		}
	}

	/**
	 * Load the color at the specified position
	 *
	 * @param x
	 * @param y
	 * @param z
	 * @return int-packed color
	 */
	int loadWithLUT(const int x, const int y, const int z) {
		try {
			return loader.loadWithLUT(x, y, z);
		}
		catch (const std::exception e) {
			printf("No image. Maybe it is swapped");
			return;
		}
	}

	/**
	 * Load the average value at the specified position
	 *
	 * @param x
	 * @param y
	 * @param z
	 * @return value.
	 */
	byte getAverage(const int x, const int y, const int z) {
		try {
			return image.getAverage(x, y, z);
		}
		catch (const std::exception e) {
			printf("No image. Maybe it is swapped");
			return;
		}
	}

	/**
	 * Abstract interface for the loader classes.
	 */
	interface Loader {

		int load(int x, int y, int z);

		int loadWithLUT(int x, int y, int z);

		void set(int x, int y, int z, int v);

		void setNoCheck(int x, int y, int z, int v);
	}

	/**
	 * Abstract interface for the input image.
	 */
	 interface InputImage {

		public int get(int x, int y, int z);

		public void get(int x, int y, int z, int[] c);

		public byte getAverage(int x, int y, int z);

		public void set(int x, int y, int z, int v);
	}

	 const class ByteImage implements InputImage {

		protected byte[][] fData;
		private final int w;

		protected ByteImage(final ImagePlus imp) {
			final ImageStack stack = imp.getStack();
			w = imp.getWidth();
			final int d = imp.getStackSize();
			fData = new byte[d][];
			for (int z = 0; z < d; z++)
				fData[z] = (byte[]) stack.getPixels(z + 1);
		}

		@Override
			public byte getAverage(final int x, final int y, final int z) {
			return fData[z][y * w + x];
		}

		@Override
			public int get(final int x, final int y, final int z) {
			return fData[z][y * w + x] & 0xff;
		}

		@Override
			public void get(final int x, final int y, final int z, final int[] c) {
			final int v = get(x, y, z);
			c[0] = c[1] = c[2] = v;
		}

		@Override
			public void set(final int x, final int y, final int z, final int v) {
			fData[z][y * w + x] = (byte)v;
		}
	}

	protected final class IntImage implements InputImage {

		protected int[][] fData;
		private final int w;

		protected IntImage(final ImagePlus imp) {
			final ImageStack stack = imp.getStack();
			w = imp.getWidth();
			final int d = imp.getStackSize();
			fData = new int[d][];
			for (int z = 0; z < d; z++)
				fData[z] = (int[]) stack.getPixels(z + 1);
		}

		@Override
			public byte getAverage(final int x, final int y, final int z) {
			final int v = fData[z][y * w + x];
			final int r = (v & 0xff0000) >> 16;
			final int g = (v & 0xff00) >> 8;
			final int b = (v & 0xff);
			return (byte)((r + g + b) / 3);
		}

		@Override
			public int get(final int x, final int y, final int z) {
			return fData[z][y * w + x];
		}

		@Override
			public void get(final int x, final int y, final int z, final int[] c) {
			final int v = get(x, y, z);
			c[0] = (v & 0xff0000) >> 16;
			c[1] = (v & 0xff00) >> 8;
			c[2] = (v & 0xff);
		}

		@Override
			public void set(final int x, final int y, final int z, final int v) {
			fData[z][y * w + x] = v;
		}
	}

	protected class IntLoader implements Loader {

		protected InputImage image;

		protected IntLoader(final InputImage imp) {
			this.image = imp;
		}

		@Override
			public final int load(final int x, final int y, final int z) {
			return image.get(x, y, z);
		}

		protected int[] color = new int[3];

		@Override
			public int loadWithLUT(final int x, final int y, final int z) {
			image.get(x, y, z, color);
			int sum = 0, av = 0, v = 0;

			if (channels[0]) {
				final int r = rLUT[color[0]];
				sum++;
				av += color[0];
				v += (r << 16);
			}
			if (channels[1]) {
				final int g = gLUT[color[1]];
				sum++;
				av += color[1];
				v += (g << 8);
			}
			if (channels[2]) {
				final int b = bLUT[color[2]];
				sum++;
				av += color[2];
				v += b;
			}
			av /= sum;
			final int a = aLUT[av];
			return (a << 24) + v;
		}

		@Override
			public void setNoCheck(final int x, final int y, final int z, final int v) {
			image.set(x, y, z, v);
		}

		@Override
			public void set(final int x, final int y, final int z, final int v) {
			if (x >= 0 && x < xDim && y >= 0 && y < yDim && z > 0 && z < zDim) {
				this.setNoCheck(x, y, z, v);
			}
		}
	}

	protected class SaturatedIntLoader extends IntLoader {

		protected SaturatedIntLoader(final InputImage imp) {
			super(imp);
		}

		@Override
			public final int loadWithLUT(final int x, final int y, final int z) {
			image.get(x, y, z, color);

			int sum = 0, av = 0, r = 0, g = 0, b = 0;
			if (channels[0]) {
				r = rLUT[color[0]];
				sum++;
				av += color[0];
			}
			if (channels[1]) {
				g = gLUT[color[1]];
				sum++;
				av += color[1];
			}
			if (channels[2]) {
				b = bLUT[color[2]];
				sum++;
				av += color[2];
			}

			av /= sum;

			final int maxC = Math.max(r, Math.max(g, b));
			final float scale = maxC == 0 ? 0 : 255.0f / maxC;

			r = Math.min(255, Math.round(scale * r));
			g = Math.min(255, Math.round(scale * g));
			b = Math.min(255, Math.round(scale * b));

			return (aLUT[av] << 24) | (r << 16) | (g << 8) | b;
		}
	}

	protected class ByteLoader implements Loader {

		protected InputImage image;
		protected int channel;

		protected ByteLoader(final InputImage imp, final int channel) {
			this.image = imp;
			this.channel = channel;
		}

		@Override
			public int load(final int x, final int y, final int z) {
			return image.get(x, y, z);
		}

		private final int[] color = new int[3];

		@Override
			public int loadWithLUT(final int x, final int y, final int z) {
			// ByteLoader only is in use with a default LUT
			image.get(x, y, z, color);
			return color[channel];
		}

		@Override
			public void setNoCheck(final int x, final int y, final int z, final int v) {
			image.set(x, y, z, v);
		}

		@Override
			public void set(final int x, final int y, final int z, final int v) {
			if (x >= 0 && x < xDim && y >= 0 && y < yDim && z > 0 && z < zDim) {
				this.setNoCheck(x, y, z, v);
			}
		}
	}

	protected class AverageByteLoader extends ByteLoader {

		protected AverageByteLoader(final InputImage imp) {
			super(imp, 0);
		}

		private final int[] color = new int[3];

		@Override
			public final int load(final int x, final int y, final int z) {
			image.get(x, y, z, color);
			return (color[0] + color[1] + color[2]) / 3;
		}

		@Override
			public final int loadWithLUT(final int x, final int y, final int z) {
			image.get(x, y, z, color);
			int sum = 0, av = 0;
			if (channels[0]) {
				av += rLUT[color[0]];
				sum++;
			}
			if (channels[1]) {
				av += gLUT[color[1]];
				sum++;
			}
			if (channels[2]) {
				av += bLUT[color[2]];
				sum++;
			}
			av /= sum;
			return av;
		}

		@Override
			public void setNoCheck(final int x, final int y, final int z, final int v) {
			image.set(x, y, z, v);
		}

		@Override
			public void set(final int x, final int y, final int z, final int v) {
			if (x >= 0 && x < xDim && y >= 0 && y < yDim && z > 0 && z < zDim) {
				this.setNoCheck(x, y, z, v);
			}
		}
	}
}
