#pragma once

class Loader {
	int load(int x, int y, int z);

	int loadWithLUT(int x, int y, int z);

	void set(int x, int y, int z, int v);

	void setNoCheck(int x, int y, int z, int v);
};