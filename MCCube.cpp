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

#include "thirdparties/include/glm/vec3.hpp"
#include <vector>

#include "Carrier.hpp"

using namespace glm;

class MCCube {

	
private:
	// vertexes
	std::vector<vec3> v;

	// interpolated values
	std::vector<vec3> e;

	MCCube() {
		for (int i = 0; i < 8; i++)
			v.push_back(vec3());

		for (int i = 0; i < 12; i++)
			e.push_back(vec3());
	}

	/**
	 * initializes a MCCube object _________ 0______x /v0 v1/| /| /________/ | / |
	 * |v3 v2| /v5 y/ |z |________|/ v7 v6
	 */
public:
	void init(int x, int y, int z) {
		v.at(0) = vec3(x, y, z);
		v.at(1) = vec3(x + 1, y, z);
		v.at(2) = vec3(x + 1, y + 1, z);
		v.at(3) = vec3(x, y + 1, z);
		v.at(4) = vec3(x, y, z + 1);
		v.at(5) = vec3(x + 1, y, z + 1);
		v.at(6) = vec3(x + 1, y + 1, z + 1);
		v.at(7) = vec3(x, y + 1, z + 1);
	}

	/**
	 * computes the interpolated point along a specified whose intensity equals
	 * the reference value
	 *
	 * @param v1 first extremity of the edge
	 * @param v2 second extremity of the edge
	 * @param result stores the resulting edge return the point on the edge where
	 *          intensity equals the isovalue;
	 * @return false if the interpolated point is beyond edge boundaries
	 */
private: 
	bool computeEdge(vec3 v1, int i1, vec3 v2,
		int i2, vec3 result,  Carrier car)
	{
		// 30 --- 50 --- 70 : t=0.5
		// 70 --- 50 --- 30 : t=0.5
		// /int i1 = car.intensity(v1);
		// /int i2 = car.intensity(v2);
		if (i2 < i1) {
			return computeEdge(v2, i2, v1, i1, result, car);
		}

		float t = (car.threshold - i1) / (i2 - i1);
		if (t >= 0 && t <= 1) {
			// v1 + t*(v2-v1)
			result = v2;
			result -= v1;
			result /= t;
			result += v1;
			return true;
		}
		result = vec3(-1, -1, -1);
		return false;
	}

	/**
	 * computes interpolated values along each edge of the cube (null if
	 * interpolated value doesn't belong to the edge)
	 */
private: 
	void computeEdges(Carrier car) {
		int i0 = car.intensity(v[0]);
		int i1 = car.intensity(v[1]);
		int i2 = car.intensity(v[2]);
		int i3 = car.intensity(v[3]);
		int i4 = car.intensity(v[4]);
		int i5 = car.intensity(v[5]);
		int i6 = car.intensity(v[6]);
		int i7 = car.intensity(v[7]);
		
		this->computeEdge(v[0], i0, v[1], i1, e[0], car);
		this->computeEdge(v[1], i1, v[2], i2, e[1], car);
		this->computeEdge(v[2], i2, v[3], i3, e[2], car);
		this->computeEdge(v[3], i3, v[0], i0, e[3], car);

		this->computeEdge(v[4], i4, v[5], i5, e[4], car);
		this->computeEdge(v[5], i5, v[6], i6, e[5], car);
		this->computeEdge(v[6], i6, v[7], i7, e[6], car);
		this->computeEdge(v[7], i7, v[4], i4, e[7], car);

		this->computeEdge(v[0], i0, v[4], i4, e[8], car);
		this->computeEdge(v[1], i1, v[5], i5, e[9], car);
		this->computeEdge(v[3], i3, v[7], i7, e[10], car);
		this->computeEdge(v[2], i2, v[6], i6, e[11], car);
	}

	/**
	 * indicates if a number corresponds to an ambigous case
	 *
	 * @param n number of the case to test
	 * @return true if the case if ambigous
	 */
private:
	bool isAmbigous(int n) {
		bool result = false;
		for (int index = 0; index < sizeof(ambigous); index++) {
			result |= ambigous[index] == n;
		}
		return result;
	}

private:
	void getTriangles(std::vector<vec3> list, Carrier car) {
		int cn = caseNumber(car);
		bool directTable = !(isAmbigous(cn));
		directTable = true;

		// address in the table
		int offset = directTable ? cn * 15 : (255 - cn) * 15;
		for (int index = 0; index < 5; index++) {
			// if there's a triangle
			if (faces[offset] != -1) {
				// pick up vertexes of the current triangle
				list.push_back(vec3(this->e[faces[offset + 0]]));
				list.push_back(vec3(this->e[faces[offset + 1]]));
				list.push_back(vec3(this->e[faces[offset + 2]]));
			}
			offset += 3;
		}
	}

	/**
	 * computes the case number of the cube
	 *
	 * @return the number of the case corresponding to the cube
	 */
private:
	int caseNumber(Carrier car) {
		int caseNumber = 0;
		for (int index = -1; ++index < v.size(); 
			caseNumber += (car.intensity(v[index]) - car.threshold > 0) ? 1 << index : 0);
		return caseNumber;
	}

	/**
	 * Create a list of triangles from the specified image data and the given
	 * isovalue.
	 *
	 * @param volume
	 * @param thresh
	 * @return
	 */
public: 
	std::vector<vec3> getTriangles(
		//Volume volume,
		int volume,
		int thresh)
	{
		std::vector<vec3> tri;
		Carrier car = Carrier();
		//car.w = volume.xDim;
		//car.h = volume.yDim;
		//car.d = volume.zDim;
		car.w = volume;
		car.h = volume;
		car.d = volume;
		car.threshold = thresh + 0.5f;
		car.volume = volume;

		MCCube cube = MCCube();
		/*
		if (volume instanceof AreaListVolume) {
			return getTriangles(cube, (AreaListVolume)volume, car, tri);
		}
		*/
		for (int z = -1; z < car.d + 1; z += 1) {
			for (int x = -1; x < car.w + 1; x += 1) {
				for (int y = -1; y < car.h + 1; y += 1) {
					cube.init(x, y, z);
					cube.computeEdges(car);
					cube.getTriangles(tri, car);
				}
			}
			//IJ.showProgress(z, car.d - 2);
		}

		// convert pixel coordinates
		for (int i = 0; i < tri.size(); i++) {
			vec3 p = tri.at(i);
			//p.x = (float)(p.x * volume.pw + volume.minCoord.x);
			//p.y = (float)(p.y * volume.ph + volume.minCoord.y);
			//p.z = (float)(p.z * volume.pd + volume.minCoord.z);
			p.x = (float)(p.x * volume + volume);
			p.y = (float)(p.y * volume + volume);
			p.z = (float)(p.z * volume + volume);
		}
		return tri;
	}

	/**
	 * An efficient helper for {@link AreaListVolume}s.
	 *
	 * @param volume the volume
	 * @param tri the {@link List} to which to add the triangles
	 * @return the list of triangles
	 */

private:
	std::vector<vec3> getTriangles(MCCube cube,
		//AreaListVolume volume, 
		int volume,
		Carrier car,
		std::vector<vec3> tri)
	{
		std::vector<std::vector<Area>> list = volume.getAreas();
		final Area[] sectionAreas = new Area[list.size()];
		// Create one Area for each section, composed of the addition of all Shape
		// instances
		int next = -1;
		for (final List<Area> shapeList : list) {
			next++;
			if (shapeList.isEmpty()) continue;
			final Area a = shapeList.get(0);
			for (int i = 1; i < shapeList.size(); i++) {
				a.add(new Area(shapeList.get(i)));
			}
			sectionAreas[next] = a;
		}
		// Fuse Area instances for previous and next sections
		final Area[] scanAreas = new Area[sectionAreas.length];
		for (int i = 0; i < sectionAreas.length; i++) {
			if (null == sectionAreas[i]) continue;
			final Area a = new Area(sectionAreas[i]);
			if (i - 1 < 0 || null == sectionAreas[i - 1]) {}
			else a.add(sectionAreas[i - 1]);
			if (i + 1 > sectionAreas.length - 1 || null == sectionAreas[i + 1]) {}
			else a.add(sectionAreas[i + 1]);
			scanAreas[i] = a;
		}
		// Collect the bounds of all subareas in each scanArea:
		final Map<Integer, ArrayList<Rectangle>> sectionBounds =
			new HashMap<Integer, ArrayList<Rectangle>>();
		for (int i = 0; i < scanAreas.length; i++) {
			if (null == scanAreas[i]) continue;
			final ArrayList<Rectangle> bs = new ArrayList<Rectangle>();
			Polygon pol = new Polygon();
			final float[] coords = new float[6];
			for (final PathIterator pit = scanAreas[i].getPathIterator(null); !pit
				.isDone(); pit.next())
			{
				switch (pit.currentSegment(coords)) {
				case PathIterator.SEG_MOVETO:
				case PathIterator.SEG_LINETO:
					pol.addPoint((int)coords[0], (int)coords[1]);
					break;
				case PathIterator.SEG_CLOSE:
					bs.add(pol.getBounds());
					pol = new Polygon();
					break;
				default:
					System.out.println("WARNING: unhandled seg type.");
					break;
				}
			}
			sectionBounds.put(i, bs);
		}

		// Add Z paddings on top and bottom
		sectionBounds.put(-1, sectionBounds.get(0));
		sectionBounds.put(car.d, sectionBounds.get(car.d - 1));

		// Scan only relevant areas:
		for (int z = -1; z < car.d + 1; z += 1) {
			final ArrayList<Rectangle> bs = sectionBounds.get(z);
			if (null == bs || bs.isEmpty()) continue;
			for (final Rectangle bounds : bs) {
				for (int x = bounds.x - 1; x < bounds.x + bounds.width + 2; x += 1) {
					for (int y = bounds.y - 1; y < bounds.y + bounds.height + 2; y += 1) {
						cube.init(x, y, z);
						cube.computeEdges(car);
						cube.getTriangles(tri, car);
					}
				}
			}

			IJ.showProgress(z, car.d - 2);
		}

		// convert pixel coordinates
		for (int i = 0; i < tri.size(); i++) {
			final Point3f p = tri.get(i);
			p.x = (float)(p.x * volume.pw + volume.minCoord.x);
			p.y = (float)(p.y * volume.ph + volume.minCoord.y);
			p.z = (float)(p.z * volume.pd + volume.minCoord.z);
		}
		return tri;
	}

protected:
	const int ambigous[240] = { 250, 245, 237, 231, 222, 219, 189,
		183, 175, 126, 123, 95, 234, 233, 227, 214, 213, 211, 203, 199, 188, 186,
		182, 174, 171, 158, 151, 124, 121, 117, 109, 107, 93, 87, 62, 61, 229, 218,
		181, 173, 167, 122, 94, 91, 150, 170, 195, 135, 149, 154, 163, 166, 169,
		172, 180, 197, 202, 210, 225, 165 };

	// triangles to be drawn in each case
private:
	const int faces[15360] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 8, 3, 9, 8,
		1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 11, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, 0, 8, 3, 1, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, 9, 2, 11, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 8, 3, 2, 11,
		8, 11, 9, 8, -1, -1, -1, -1, -1, -1, 3, 10, 2, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, 0, 10, 2, 8, 10, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		1, 9, 0, 2, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 10, 2, 1, 9, 10,
		9, 8, 10, -1, -1, -1, -1, -1, -1, 3, 11, 1, 10, 11, 3, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, 0, 11, 1, 0, 8, 11, 8, 10, 11, -1, -1, -1, -1, -1, -1, 3,
		9, 0, 3, 10, 9, 10, 11, 9, -1, -1, -1, -1, -1, -1, 9, 8, 11, 11, 8, 10, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, 4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1,
		9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4, 1, 9, 4, 7, 1, 7, 3, 1,
		-1, -1, -1, -1, -1, -1, 1, 2, 11, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, 3, 4, 7, 3, 0, 4, 1, 2, 11, -1, -1, -1, -1, -1, -1, 9, 2, 11, 9, 0, 2,
		8, 4, 7, -1, -1, -1, -1, -1, -1, 2, 11, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1,
		-1, -1, 8, 4, 7, 3, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 4, 7,
		10, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, 9, 0, 1, 8, 4, 7, 2, 3, 10, -1,
		-1, -1, -1, -1, -1, 4, 7, 10, 9, 4, 10, 9, 10, 2, 9, 2, 1, -1, -1, -1, 3,
		11, 1, 3, 10, 11, 7, 8, 4, -1, -1, -1, -1, -1, -1, 1, 10, 11, 1, 4, 10, 1,
		0, 4, 7, 10, 4, -1, -1, -1, 4, 7, 8, 9, 0, 10, 9, 10, 11, 10, 0, 3, -1, -1,
		-1, 4, 7, 10, 4, 10, 9, 9, 10, 11, -1, -1, -1, -1, -1, -1, 9, 5, 4, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 9, 5, 4, 0, 8, 3, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, 0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, 1, 2, 11, 9, 5, 4, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, 3, 0, 8, 1, 2, 11, 4, 9, 5, -1, -1, -1, -1,
		-1, -1, 5, 2, 11, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, 2, 11, 5, 3, 2,
		5, 3, 5, 4, 3, 4, 8, -1, -1, -1, 9, 5, 4, 2, 3, 10, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, 0, 10, 2, 0, 8, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, 0, 5, 4,
		0, 1, 5, 2, 3, 10, -1, -1, -1, -1, -1, -1, 2, 1, 5, 2, 5, 8, 2, 8, 10, 4,
		8, 5, -1, -1, -1, 11, 3, 10, 11, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, 4,
		9, 5, 0, 8, 1, 8, 11, 1, 8, 10, 11, -1, -1, -1, 5, 4, 0, 5, 0, 10, 5, 10,
		11, 10, 0, 3, -1, -1, -1, 5, 4, 8, 5, 8, 11, 11, 8, 10, -1, -1, -1, -1, -1,
		-1, 9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, 9, 3, 0, 9, 5, 3,
		5, 7, 3, -1, -1, -1, -1, -1, -1, 0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1,
		-1, -1, 1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, 9, 7, 8, 9,
		5, 7, 11, 1, 2, -1, -1, -1, -1, -1, -1, 11, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7,
		3, -1, -1, -1, 8, 0, 2, 8, 2, 5, 8, 5, 7, 11, 5, 2, -1, -1, -1, 2, 11, 5,
		2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, 7, 9, 5, 7, 8, 9, 3, 10, 2, -1,
		-1, -1, -1, -1, -1, 9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 10, -1, -1, -1, 2, 3,
		10, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, 10, 2, 1, 10, 1, 7, 7, 1, 5, -1,
		-1, -1, -1, -1, -1, 9, 5, 8, 8, 5, 7, 11, 1, 3, 11, 3, 10, -1, -1, -1, 5,
		7, 0, 5, 0, 9, 7, 10, 0, 1, 0, 11, 10, 11, 0, 10, 11, 0, 10, 0, 3, 11, 5,
		0, 8, 0, 7, 5, 7, 0, 10, 11, 5, 7, 10, 5, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, 11, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 8, 3, 5,
		11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, 9, 0, 1, 5, 11, 6, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, 1, 8, 3, 1, 9, 8, 5, 11, 6, -1, -1, -1, -1, -1, -1,
		1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 6, 5, 1, 2, 6, 3,
		0, 8, -1, -1, -1, -1, -1, -1, 9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1,
		-1, -1, 5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, 2, 3, 10, 11, 6, 5,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 0, 8, 10, 2, 0, 11, 6, 5, -1, -1,
		-1, -1, -1, -1, 0, 1, 9, 2, 3, 10, 5, 11, 6, -1, -1, -1, -1, -1, -1, 5, 11,
		6, 1, 9, 2, 9, 10, 2, 9, 8, 10, -1, -1, -1, 6, 3, 10, 6, 5, 3, 5, 1, 3, -1,
		-1, -1, -1, -1, -1, 0, 8, 10, 0, 10, 5, 0, 5, 1, 5, 10, 6, -1, -1, -1, 3,
		10, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, 6, 5, 9, 6, 9, 10, 10, 9, 8,
		-1, -1, -1, -1, -1, -1, 5, 11, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, 4, 3, 0, 4, 7, 3, 6, 5, 11, -1, -1, -1, -1, -1, -1, 1, 9, 0, 5, 11, 6,
		8, 4, 7, -1, -1, -1, -1, -1, -1, 11, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1,
		-1, -1, 6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, 1, 2, 5, 5, 2,
		6, 3, 0, 4, 3, 4, 7, -1, -1, -1, 8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1,
		-1, -1, 7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, 3, 10, 2, 7, 8, 4, 11,
		6, 5, -1, -1, -1, -1, -1, -1, 5, 11, 6, 4, 7, 2, 4, 2, 0, 2, 7, 10, -1, -1,
		-1, 0, 1, 9, 4, 7, 8, 2, 3, 10, 5, 11, 6, -1, -1, -1, 9, 2, 1, 9, 10, 2, 9,
		4, 10, 7, 10, 4, 5, 11, 6, 8, 4, 7, 3, 10, 5, 3, 5, 1, 5, 10, 6, -1, -1,
		-1, 5, 1, 10, 5, 10, 6, 1, 0, 10, 7, 10, 4, 0, 4, 10, 0, 5, 9, 0, 6, 5, 0,
		3, 6, 10, 6, 3, 8, 4, 7, 6, 5, 9, 6, 9, 10, 4, 7, 9, 7, 10, 9, -1, -1, -1,
		11, 4, 9, 6, 4, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4, 11, 6, 4, 9, 11,
		0, 8, 3, -1, -1, -1, -1, -1, -1, 11, 0, 1, 11, 6, 0, 6, 4, 0, -1, -1, -1,
		-1, -1, -1, 8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 11, -1, -1, -1, 1, 4, 9, 1, 2,
		4, 2, 6, 4, -1, -1, -1, -1, -1, -1, 3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1,
		-1, -1, 0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, 8, 3, 2, 8,
		2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, 11, 4, 9, 11, 6, 4, 10, 2, 3, -1,
		-1, -1, -1, -1, -1, 0, 8, 2, 2, 8, 10, 4, 9, 11, 4, 11, 6, -1, -1, -1, 3,
		10, 2, 0, 1, 6, 0, 6, 4, 6, 1, 11, -1, -1, -1, 6, 4, 1, 6, 1, 11, 4, 8, 1,
		2, 1, 10, 8, 10, 1, 9, 6, 4, 9, 3, 6, 9, 1, 3, 10, 6, 3, -1, -1, -1, 8, 10,
		1, 8, 1, 0, 10, 6, 1, 9, 1, 4, 6, 4, 1, 3, 10, 6, 3, 6, 0, 0, 6, 4, -1, -1,
		-1, -1, -1, -1, 6, 4, 8, 10, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, 7,
		11, 6, 7, 8, 11, 8, 9, 11, -1, -1, -1, -1, -1, -1, 0, 7, 3, 0, 11, 7, 0, 9,
		11, 6, 7, 11, -1, -1, -1, 11, 6, 7, 1, 11, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1,
		11, 6, 7, 11, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, 1, 2, 6, 1, 6, 8, 1,
		8, 9, 8, 6, 7, -1, -1, -1, 2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, 7,
		8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, 7, 3, 2, 6, 7, 2, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, 2, 3, 10, 11, 6, 8, 11, 8, 9, 8, 6, 7, -1, -1,
		-1, 2, 0, 7, 2, 7, 10, 0, 9, 7, 6, 7, 11, 9, 11, 7, 1, 8, 0, 1, 7, 8, 1,
		11, 7, 6, 7, 11, 2, 3, 10, 10, 2, 1, 10, 1, 7, 11, 6, 1, 6, 7, 1, -1, -1,
		-1, 8, 9, 6, 8, 6, 7, 9, 1, 6, 10, 6, 3, 1, 3, 6, 0, 9, 1, 10, 6, 7, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, 7, 8, 0, 7, 0, 6, 3, 10, 0, 10, 6, 0, -1,
		-1, -1, 7, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 7, 6, 10,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 0, 8, 10, 7, 6, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, 0, 1, 9, 10, 7, 6, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, 8, 1, 9, 8, 3, 1, 10, 7, 6, -1, -1, -1, -1, -1, -1, 11, 1, 2, 6,
		10, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 11, 3, 0, 8, 6, 10, 7, -1,
		-1, -1, -1, -1, -1, 2, 9, 0, 2, 11, 9, 6, 10, 7, -1, -1, -1, -1, -1, -1, 6,
		10, 7, 2, 11, 3, 11, 8, 3, 11, 9, 8, -1, -1, -1, 7, 2, 3, 6, 2, 7, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, 7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1,
		-1, 2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, 1, 6, 2, 1, 8, 6, 1,
		9, 8, 8, 7, 6, -1, -1, -1, 11, 7, 6, 11, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1,
		-1, 11, 7, 6, 1, 7, 11, 1, 8, 7, 1, 0, 8, -1, -1, -1, 0, 3, 7, 0, 7, 11, 0,
		11, 9, 6, 11, 7, -1, -1, -1, 7, 6, 11, 7, 11, 8, 8, 11, 9, -1, -1, -1, -1,
		-1, -1, 6, 8, 4, 10, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 6, 10, 3,
		0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, 8, 6, 10, 8, 4, 6, 9, 0, 1, -1, -1,
		-1, -1, -1, -1, 9, 4, 6, 9, 6, 3, 9, 3, 1, 10, 3, 6, -1, -1, -1, 6, 8, 4,
		6, 10, 8, 2, 11, 1, -1, -1, -1, -1, -1, -1, 1, 2, 11, 3, 0, 10, 0, 6, 10,
		0, 4, 6, -1, -1, -1, 4, 10, 8, 4, 6, 10, 0, 2, 9, 2, 11, 9, -1, -1, -1, 11,
		9, 3, 11, 3, 2, 9, 4, 3, 10, 3, 6, 4, 6, 3, 8, 2, 3, 8, 4, 2, 4, 6, 2, -1,
		-1, -1, -1, -1, -1, 0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, 1, 9, 4, 1, 4, 2, 2, 4, 6,
		-1, -1, -1, -1, -1, -1, 8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 11, 1, -1, -1, -1,
		11, 1, 0, 11, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, 4, 6, 3, 4, 3, 8, 6,
		11, 3, 0, 3, 9, 11, 9, 3, 11, 9, 4, 6, 11, 4, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, 4, 9, 5, 7, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 8, 3, 4,
		9, 5, 10, 7, 6, -1, -1, -1, -1, -1, -1, 5, 0, 1, 5, 4, 0, 7, 6, 10, -1, -1,
		-1, -1, -1, -1, 10, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, 9, 5, 4,
		11, 1, 2, 7, 6, 10, -1, -1, -1, -1, -1, -1, 6, 10, 7, 1, 2, 11, 0, 8, 3, 4,
		9, 5, -1, -1, -1, 7, 6, 10, 5, 4, 11, 4, 2, 11, 4, 0, 2, -1, -1, -1, 3, 4,
		8, 3, 5, 4, 3, 2, 5, 11, 5, 2, 10, 7, 6, 7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1,
		-1, -1, -1, -1, 9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, 3, 6, 2, 3,
		7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, 6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1,
		5, 8, 9, 5, 4, 11, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, 1, 6, 11, 1, 7, 6,
		1, 0, 7, 8, 7, 0, 9, 5, 4, 4, 0, 11, 4, 11, 5, 0, 3, 11, 6, 11, 7, 3, 7,
		11, 7, 6, 11, 7, 11, 8, 5, 4, 11, 4, 8, 11, -1, -1, -1, 6, 9, 5, 6, 10, 9,
		10, 8, 9, -1, -1, -1, -1, -1, -1, 3, 6, 10, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1,
		-1, -1, 0, 10, 8, 0, 5, 10, 0, 1, 5, 5, 6, 10, -1, -1, -1, 6, 10, 3, 6, 3,
		5, 5, 3, 1, -1, -1, -1, -1, -1, -1, 1, 2, 11, 9, 5, 10, 9, 10, 8, 10, 5, 6,
		-1, -1, -1, 0, 10, 3, 0, 6, 10, 0, 9, 6, 5, 6, 9, 1, 2, 11, 10, 8, 5, 10,
		5, 6, 8, 0, 5, 11, 5, 2, 0, 2, 5, 6, 10, 3, 6, 3, 5, 2, 11, 3, 11, 5, 3,
		-1, -1, -1, 5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, 9, 5, 6, 9, 6,
		0, 0, 6, 2, -1, -1, -1, -1, -1, -1, 1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6,
		2, 8, 1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 3, 6, 1, 6,
		11, 3, 8, 6, 5, 6, 9, 8, 9, 6, 11, 1, 0, 11, 0, 6, 9, 5, 0, 5, 6, 0, -1,
		-1, -1, 0, 3, 8, 5, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, 11, 5, 6,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 5, 11, 7, 5, 10, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, 10, 5, 11, 10, 7, 5, 8, 3, 0, -1, -1, -1,
		-1, -1, -1, 5, 10, 7, 5, 11, 10, 1, 9, 0, -1, -1, -1, -1, -1, -1, 11, 7, 5,
		11, 10, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, 10, 1, 2, 10, 7, 1, 7, 5, 1, -1,
		-1, -1, -1, -1, -1, 0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 10, -1, -1, -1, 9, 7,
		5, 9, 2, 7, 9, 0, 2, 2, 10, 7, -1, -1, -1, 7, 5, 2, 7, 2, 10, 5, 9, 2, 3,
		2, 8, 9, 8, 2, 2, 5, 11, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, 8, 2, 0,
		8, 5, 2, 8, 7, 5, 11, 2, 5, -1, -1, -1, 9, 0, 1, 5, 11, 3, 5, 3, 7, 3, 11,
		2, -1, -1, -1, 9, 8, 2, 9, 2, 1, 8, 7, 2, 11, 2, 5, 7, 5, 2, 1, 3, 5, 3, 7,
		5, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1,
		-1, -1, -1, -1, 9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, 9, 8, 7,
		5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, 5, 8, 4, 5, 11, 8, 11, 10, 8,
		-1, -1, -1, -1, -1, -1, 5, 0, 4, 5, 10, 0, 5, 11, 10, 10, 3, 0, -1, -1, -1,
		0, 1, 9, 8, 4, 11, 8, 11, 10, 11, 4, 5, -1, -1, -1, 11, 10, 4, 11, 4, 5,
		10, 3, 4, 9, 4, 1, 3, 1, 4, 2, 5, 1, 2, 8, 5, 2, 10, 8, 4, 5, 8, -1, -1,
		-1, 0, 4, 10, 0, 10, 3, 4, 5, 10, 2, 10, 1, 5, 1, 10, 0, 2, 5, 0, 5, 9, 2,
		10, 5, 4, 5, 8, 10, 8, 5, 9, 4, 5, 2, 10, 3, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, 2, 5, 11, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, 5, 11, 2, 5, 2, 4,
		4, 2, 0, -1, -1, -1, -1, -1, -1, 3, 11, 2, 3, 5, 11, 3, 8, 5, 4, 5, 8, 0,
		1, 9, 5, 11, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, 8, 4, 5, 8, 5, 3, 3,
		5, 1, -1, -1, -1, -1, -1, -1, 0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, 8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, 9, 4, 5, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4, 10, 7, 4, 9, 10, 9, 11, 10, -1,
		-1, -1, -1, -1, -1, 0, 8, 3, 4, 9, 7, 9, 10, 7, 9, 11, 10, -1, -1, -1, 1,
		11, 10, 1, 10, 4, 1, 4, 0, 7, 4, 10, -1, -1, -1, 3, 1, 4, 3, 4, 8, 1, 11,
		4, 7, 4, 10, 11, 10, 4, 4, 10, 7, 9, 10, 4, 9, 2, 10, 9, 1, 2, -1, -1, -1,
		9, 7, 4, 9, 10, 7, 9, 1, 10, 2, 10, 1, 0, 8, 3, 10, 7, 4, 10, 4, 2, 2, 4,
		0, -1, -1, -1, -1, -1, -1, 10, 7, 4, 10, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1,
		-1, 2, 9, 11, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, 9, 11, 7, 9, 7, 4, 11,
		2, 7, 8, 7, 0, 2, 0, 7, 3, 7, 11, 3, 11, 2, 7, 4, 11, 1, 11, 0, 4, 0, 11,
		1, 11, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4, 9, 1, 4, 1, 7, 7,
		1, 3, -1, -1, -1, -1, -1, -1, 4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1,
		-1, 4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4, 8, 7, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 9, 11, 8, 11, 10, 8, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, 3, 0, 9, 3, 9, 10, 10, 9, 11, -1, -1, -1, -1, -1,
		-1, 0, 1, 11, 0, 11, 8, 8, 11, 10, -1, -1, -1, -1, -1, -1, 3, 1, 11, 10, 3,
		11, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 10, 1, 10, 9, 9, 10, 8, -1,
		-1, -1, -1, -1, -1, 3, 0, 9, 3, 9, 10, 1, 2, 9, 2, 10, 9, -1, -1, -1, 0, 2,
		10, 8, 0, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 2, 10, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, 2, 3, 8, 2, 8, 11, 11, 8, 9, -1, -1, -1,
		-1, -1, -1, 9, 11, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 3, 8,
		2, 8, 11, 0, 1, 8, 1, 11, 8, -1, -1, -1, 1, 11, 2, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, 1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, 0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 3, 8, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1 };
};
